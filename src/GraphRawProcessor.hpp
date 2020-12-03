/**
* @file GraphRawProcessor.hpp Raw processor parallel task and pipeline
* combination using TBB Flow Graph
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_GRAPHRAWPROCESSOR_HPP_
#define UDAQ_READOUT_SRC_GRAPHRAWPROCESSOR_HPP_

#include "RawProcessorBase.hpp"

#include <functional>

#include "tbb/flow_graph.h"

namespace dunedaq {
namespace readout {

template<class RawType>
class GraphRawProcessor : public RawProcessorBase {
public:

  explicit GraphRawProcessor(const std::string& rawtype, std::function<void(RawType*)>& process_override)
  : RawProcessorBase(rawtype)
  , graph_input_(task_graph_)
  , join_node_(task_graph_)
  , raw_type_name_(rawtype)
  , process_override_(process_override)
  {
    // Bind custom raw process of data
    process_override_ = std::bind(&GraphRawProcessor<RawType>::process_item, this, std::placeholders::_1);
  }

  ~GraphRawProcessor() {
    task_graph_.wait_for_all();
  }

  void conf(const nlohmann::json& cfg) {
    ERS_INFO("Setting up flow grap.");
    if (!parallel_task_nodes_.empty()){
      for (int i=0; i<parallel_task_nodes_.size(); ++i) {
        tbb::flow::make_edge(graph_input_, parallel_task_nodes_[i]);
        // we need constexpr for the node ports, so this becomes the responsibility of the derived
        //tbb::flow::make_edge(parallel_task_nodes_[i], tbb::flow::get<0>( join_node_.input_ports() ));
      }
    } else {
      if (!pipeline_task_nodes_.empty()) {
        tbb::flow::make_edge(graph_input_, pipeline_task_nodes_[0]);
      }
    }
    if (!pipeline_task_nodes_.empty()) {
      tbb::flow::make_edge(graph_input_, pipeline_task_nodes_[0]);
    }
    if (pipeline_task_nodes_.size() > 1) {
      for (int i=1; i<pipeline_task_nodes_.size(); ++i) {
        tbb::flow::make_edge(pipeline_task_nodes_[i-1], pipeline_task_nodes_[i]);
      }
    }
  }

  void process_item(RawType* item) {
    graph_input_.try_put(item);
  } 

protected:
  // Task Flow Graph
  tbb::flow::graph task_graph_;
  tbb::flow::broadcast_node<RawType*> graph_input_;
  tbb::flow::join_node< tbb::flow::tuple<RawType*, RawType*>, tbb::flow::queueing> join_node_;
  std::vector<tbb::flow::function_node<RawType*, RawType*>> parallel_task_nodes_;
  std::vector<tbb::flow::function_node<RawType*, RawType*>> pipeline_task_nodes_; 

private:
  std::string raw_type_name_;
  std::function<void(RawType*)>& process_override_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_GRAPHRAWPROCESSOR_HPP_
