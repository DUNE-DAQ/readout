/**
 * @file SSPFrameProcessor.hpp SSP specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_SSP_SSPFRAMEPROCESSOR_HPP_
#define READOUT_SRC_SSP_SSPFRAMEPROCESSOR_HPP_

#include "appfwk/DAQModuleHelper.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/models/IterableQueueModel.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"
#include "readout/utils/ReusableThread.hpp"

#include "dataformats/ssp/SSPTypes.hpp"
#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class SSPFrameProcessor : public TaskRawDataProcessorModel<types::SSP_FRAME_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::SSP_FRAME_STRUCT>;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  // Channel map funciton type
  typedef int (*chan_map_fn_t)(int);

  explicit SSPFrameProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::SSP_FRAME_STRUCT>(error_registry)
  {
    // Setup pre-processing pipeline
  }

  ~SSPFrameProcessor()
  {

  }

  void start(const nlohmann::json& args) override
  {
    inherited::start(args);
  }

  void stop(const nlohmann::json& args) override
  {
    inherited::stop(args);
  }

  void init(const nlohmann::json& args) override
  {

  }

  void conf(const nlohmann::json& cfg) override
  {
    inherited::conf(cfg);
  }

  void get_info(opmonlib::InfoCollector& ci, int level)
  {

  }

protected:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SSP_SSPFRAMEPROCESSOR_HPP_
