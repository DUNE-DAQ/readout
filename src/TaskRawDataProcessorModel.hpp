/**
* @file TaskRawDataProcessorModel.hpp Raw processor parallel task and pipeline
* combination using TBB Flow Graph
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_ASYNCRAWPROCESSOR_HPP_
#define UDAQ_READOUT_SRC_ASYNCRAWPROCESSOR_HPP_

#include "RawDataProcessorConcept.hpp"

#include <functional>
#include <future>

#include "tbb/flow_graph.h"

namespace dunedaq {
namespace readout {

template<class RawType>
class TaskRawDataProcessorModel : public RawDataProcessorConcept {
public:

  explicit TaskRawDataProcessorModel(const std::string& rawtype, std::function<void(RawType*)>& process_override)
  : RawDataProcessorConcept(rawtype)
  , raw_type_name_(rawtype)
  , process_override_(process_override)
  {
    // Bind custom raw process of data
    process_override_ = std::bind(&TaskRawDataProcessorModel<RawType>::process_item, this, std::placeholders::_1);
  }

  ~TaskRawDataProcessorModel() {

  }

  void conf(const nlohmann::json& cfg) {
    ERS_INFO("Setting up async task tree.");
  }

  void process_item(RawType* item) {
    invoke_all(item);
  }

  template<typename Task>
  void add_task(Task&& task) {
    tasklist_.push_back(std::forward<Task>(task));
  }
  
  void invoke_all(RawType* item) {
    for(auto&& task : tasklist_) {
      task(item);
    }   
  }
    
  void launch_all(RawType* item) {
    for(auto&& task : tasklist_) {
      auto fut = std::async(std::launch::async, task, item);
    }
  }

protected:
  // Async tasks and
  std::vector<std::function<void(RawType*)>> tasklist_;
  //std::map<std::string, std::function<void(RawType*)>> tasklist_; // futures

private:
  std::string raw_type_name_;
  std::function<void(RawType*)>& process_override_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_GRAPHRAWPROCESSOR_HPP_
