/**
* @file TaskRawDataProcessorModel.hpp Raw processor parallel task and pipeline
* combination using a vector of std::functions
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_TASKRAWDATAPROCESSORMODEL_HPP_
#define READOUT_SRC_TASKRAWDATAPROCESSORMODEL_HPP_

#include "RawDataProcessorConcept.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include "tbb/flow_graph.h"

#include <functional>
#include <future>
#include <utility>
#include <string>
#include <vector>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

template<class RawType>
class TaskRawDataProcessorModel : public RawDataProcessorConcept {
public:

  explicit TaskRawDataProcessorModel(const std::string& rawtype, std::function<void(RawType*)>& process_override)
  : RawDataProcessorConcept(rawtype)
  , m_raw_type_name(rawtype)
  , m_process_override(process_override)
  {
    // Bind custom raw process of data
    m_process_override = std::bind(&TaskRawDataProcessorModel<RawType>::process_item, this, std::placeholders::_1);
  }

  ~TaskRawDataProcessorModel() {

  }

  void conf(const nlohmann::json& /*cfg*/) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Setting up async task tree.";
  }

  void process_item(RawType* item) {
    invoke_all(item);
  }

  template<typename Task>
  void add_task(Task&& task) {
    m_tasklist.push_back(std::forward<Task>(task));
  }
  
  void invoke_all(RawType* item) {
    for(auto&& task : m_tasklist) {
      task(item);
    }   
  }
    
  void launch_all(RawType* item) {
    for(auto&& task : m_tasklist) {
      auto fut = std::async(std::launch::async, task, item);
    }
  }

protected:
  // Async tasks and
  std::vector<std::function<void(RawType*)>> m_tasklist;
  //std::map<std::string, std::function<void(RawType*)>> m_tasklist; // futures

private:
  std::string m_raw_type_name;
  std::function<void(RawType*)>& m_process_override;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_TASKRAWDATAPROCESSORMODEL_HPP_
