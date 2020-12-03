/**
* @file WIBFrameProcessor.hpp Glue between input receiver, payload processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
#define UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_

#include "GraphRawProcessor.hpp"

#include <functional>

#include "tbb/flow_graph.h"

namespace dunedaq {
namespace readout {

class WIBFrameProcessor : public GraphRawProcessor<types::WIB_SUPERCHUNK_STRUCT> {

public:
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using funcnode_t = tbb::flow::function_node<frameptr, frameptr>;

  explicit WIBFrameProcessor(const std::string& rawtype, std::function<void(frameptr)>& process_override)
  : GraphRawProcessor<types::WIB_SUPERCHUNK_STRUCT>(rawtype, process_override)
  {
    ERS_INFO("Creating WIB specific GraphRawProcessor workflow.");

    // We need to alter parallel tasks
    parallel_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, ptask1()));
    tbb::flow::make_edge(parallel_task_nodes_.back(), tbb::flow::get<0>( join_node_.input_ports() ));
    parallel_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, ptask2()));
    tbb::flow::make_edge(parallel_task_nodes_.back(), tbb::flow::get<1>( join_node_.input_ports() ));

    // And edit the pipeline
    pipeline_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, pipeline1()));
    pipeline_task_nodes_.emplace_back(funcnode_t(task_graph_, tbb::flow::unlimited, pipeline2()));
  }

  // task1
  struct ptask1 {
    frameptr operator()(frameptr fp) { 
      std::cout << "Task1 run on: " << std::hex << static_cast<void*>(fp) << std::dec << '\n';
      return fp;
    }
  };

  struct ptask2 {
    frameptr operator()(frameptr fp) { 
      std::cout << "Task2 run on: " << std::hex << static_cast<void*>(fp) << std::dec << '\n';
      return fp;
    }
  };

  struct pipeline1 {
    frameptr operator()(frameptr fp) { 
      std::cout << "Pipeline1 run on: " << std::hex << static_cast<void*>(fp) << std::dec << '\n';
      return fp;
    }
  };

  struct pipeline2 {
    frameptr operator()(frameptr fp) { 
      std::cout << "Pipeline2 run on: " << std::hex << static_cast<void*>(fp) << std::dec << '\n';
      return fp;
    }
  };

private:

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
