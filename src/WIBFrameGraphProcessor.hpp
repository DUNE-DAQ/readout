/**
* @file WIBFrameGraphProcessor.hpp Glue between input receiver, payload processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_WIBFRAMEGRAPHPROCESSOR_HPP_
#define READOUT_SRC_WIBFRAMEGRAPHPROCESSOR_HPP_

#include "GraphRawProcessor.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include "tbb/flow_graph.h"

#include <functional>
#include <atomic>
#include <string>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

class WIBFrameGraphProcessor : public GraphRawProcessor<types::WIB_SUPERCHUNK_STRUCT> {

public:
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;
  using funcnode_t = tbb::flow::function_node<frameptr, frameptr>;

  explicit WIBFrameGraphProcessor(const std::string& rawtype, std::function<void(frameptr)>& process_override)
  : GraphRawProcessor<types::WIB_SUPERCHUNK_STRUCT>(rawtype, process_override)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << get_name() << "Creating WIB specific GraphRawProcessor workflow.";

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
    uint64_t ts_prev = 0; // NOLINT
    uint64_t ts_next = 0; // NOLINT
    uint64_t counter = 0; // NOLINT
    bool first = true;
    frameptr operator()(frameptr fp) {
      auto* wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(fp); // NOLINT
      ts_next = wfptr->timestamp();
      if (ts_next - ts_prev != 300) {
        ++counter;
        if (first) {
          wfptr->wib_header()->print();
          TLOG_DEBUG(TLVL_BOOKKEEPING) << get_name() << "First TS mismatch is fine | previous: " << ts_prev << " next: " << ts_next;
          first = false;
        }
        TLOG_DEBUG(TLVL_BOOKKEEPING) << get_name() << "SCREAM | previous: " << ts_prev << " next: " << ts_next;
      }
      ts_prev = ts_next;
      return fp;
    }
  };

  struct error_check {
    uint8_t s1err = 0; // NOLINT
    uint8_t s2err = 0; // NOLINT
    frameptr operator()(frameptr fp) {
      auto* wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(fp); // NOLINT
      ;
      return fp;
    }
  };

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIBFRAMEGRAPHPROCESSOR_HPP_
