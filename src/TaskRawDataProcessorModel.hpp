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
#include <string>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class RawType>
class TaskRawDataProcessorModel : public RawDataProcessorConcept<RawType>
{
public:
  TaskRawDataProcessorModel()
    : RawDataProcessorConcept<RawType>()
  {}

  ~TaskRawDataProcessorModel() {}

  // RS FIXME -> DON'T OVERRIDE IF YOU NEED EMULATOR MODE
  // void conf(const nlohmann::json& /*cfg*/) override {
  //}

  void process_item(RawType* item) { invoke_all(item); }

  template<typename Task>
  void add_task(Task&& task)
  {
    m_tasklist.push_back(std::forward<Task>(task));
  }

  void invoke_all(RawType* item)
  {
    for (auto&& task : m_tasklist) {
      task(item);
    }
  }

  void launch_all(RawType* item)
  {
    for (auto&& task : m_tasklist) {
      auto fut = std::async(std::launch::async, task, item);
    }
  }

protected:
  // Async tasks and
  std::vector<std::function<void(RawType*)>> m_tasklist;
  // std::map<std::string, std::function<void(RawType*)>> m_tasklist; // futures
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_TASKRAWDATAPROCESSORMODEL_HPP_
