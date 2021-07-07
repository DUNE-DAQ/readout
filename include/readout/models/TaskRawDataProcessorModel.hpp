/**
 * @file TaskRawDataProcessorModel.hpp Raw processor parallel task and pipeline
 * combination using a vector of std::functions
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_TASKRAWDATAPROCESSORMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_TASKRAWDATAPROCESSORMODEL_HPP_

#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/concepts/RawDataProcessorConcept.hpp"

#include <functional>
#include <future>
#include <string>
#include <utility>
#include <vector>
#include <memory>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class ReadoutType>
class TaskRawDataProcessorModel : public RawDataProcessorConcept<ReadoutType>
{
public:
  explicit TaskRawDataProcessorModel(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : RawDataProcessorConcept<ReadoutType>()
    , m_error_registry(error_registry)
  {}

  ~TaskRawDataProcessorModel() {}

  // RS FIXME -> DON'T OVERRIDE IF YOU NEED EMULATOR MODE
  // void conf(const nlohmann::json& /*cfg*/) override {
  //}

  void process_item(ReadoutType* item) { invoke_all(item); }

  template<typename Task>
  void add_task(Task&& task)
  {
    m_tasklist.push_back(std::forward<Task>(task));
  }

  void invoke_all(ReadoutType* item)
  {
    for (auto&& task : m_tasklist) {
      task(item);
    }
  }

  void launch_all(ReadoutType* item)
  {
    for (auto&& task : m_tasklist) {
      auto fut = std::async(std::launch::async, task, item);
    }
  }

protected:
  // Async tasks and
  std::vector<std::function<void(ReadoutType*)>> m_tasklist;
  std::unique_ptr<FrameErrorRegistry>& m_error_registry;
  // std::map<std::string, std::function<void(ReadoutType*)>> m_tasklist; // futures
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_TASKRAWDATAPROCESSORMODEL_HPP_
