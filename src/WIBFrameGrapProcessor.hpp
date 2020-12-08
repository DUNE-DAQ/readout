/**
* @file WIBFrameGraphProcessor.hpp Glue between input receiver, payload processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_WIBFRAMEGRAPHPROCESSOR_HPP_
#define UDAQ_READOUT_SRC_WIBFRAMEGRAPHPROCESSOR_HPP_

#include "GraphRawProcessor.hpp"

#include "daq-rawdata/wib/WIBFrame.hpp"

#include <functional>
#include <atomic>

#include "tbb/flow_graph.h"

namespace dunedaq {
namespace readout {

class WIBFrameGraphProcessor : public GraphRawProcessor<types::WIB_SUPERCHUNK_STRUCT> {

public:
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using wibframeptr = dunedaq::rawdata::WIBFrame*;
  using funcnode_t = tbb::flow::function_node<frameptr, frameptr>;

  explicit WIBFrameGraphProcessor(const std::string& rawtype, std::function<void(frameptr)>& process_override)
  : GraphRawProcessor<types::WIB_SUPERCHUNK_STRUCT>(rawtype, process_override)
  {
    ERS_INFO("Creating WIB specific GraphRawProcessor workflow.");

    // We need to alter parallel tasks
    parallel_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, timestamp_check()));
    tbb::flow::make_edge(parallel_task_nodes_.back(), tbb::flow::get<0>( join_node_.input_ports() ));
    parallel_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, error_check()));
    tbb::flow::make_edge(parallel_task_nodes_.back(), tbb::flow::get<1>( join_node_.input_ports() ));

    // And edit the pipeline
    //pipeline_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, pipeline1()));
    ////pipeline_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, pipeline2()));
  }

  // task1
  struct timestamp_check {
    uint64_t ts_prev = 0;
    uint64_t ts_next = 0;
    uint64_t counter = 0;
    bool first = true;
    frameptr operator()(frameptr fp) {
      auto* wfptr = reinterpret_cast<dunedaq::rawdata::WIBFrame*>(fp);
      ts_next = wfptr->timestamp();
      //std::cout << " TS: " << wfptr->timestamp() << " stored: " << ts_next << '\n'; 
      if (ts_next - ts_prev != 300) {
        ++counter;
        if (first) {
          wfptr->wib_header()->print();
          std::cout << "First TS mismatch is fine | previous: " << ts_prev << " next: " << ts_next << '\n';
          first = false;
        }
        std::cout << "SCREAM | previous: " << ts_prev << " next: " << ts_next << '\n';
      }
      ts_prev = ts_next;
      return fp;
    }
  };

  struct error_check {
    uint8_t s1err = 0;
    uint8_t s2err = 0;
    frameptr operator()(frameptr fp) {
      auto* wfptr = reinterpret_cast<dunedaq::rawdata::WIBFrame*>(fp);
      ;
      return fp;
    }
  };

  /*
  struct pipeline1 {
    bool first = false;
    frameptr operator()(frameptr fp) { 
      const wibframeptr wfptr = reinterpret_cast<dunedaq::rawdata::WIBFrame*>(fp);
      if (!first) {
        wfptr->wib_header()->print();
        first = true;
      }
      //std::cout << "Pipeline1 run on: " << std::hex << static_cast<void*>(fp) << std::dec << '\n';
      return fp;
    }
  };

  struct pipeline2 {
    frameptr operator()(frameptr fp) { 
      //std::cout << "Pipeline2 run on: " << std::hex << static_cast<void*>(fp) << std::dec << '\n';
      return fp;
    }
  };
  */

private:

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
