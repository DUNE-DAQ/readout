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

#include "detdataformats/ssp/SSPTypes.hpp"
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
  using frameptr = types::SSP_FRAME_STRUCT*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  // Channel map funciton type
  typedef int (*chan_map_fn_t)(int);

  explicit SSPFrameProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::SSP_FRAME_STRUCT>(error_registry)
  {
    // Setup pre-processing pipeline
    TaskRawDataProcessorModel<types::SSP_FRAME_STRUCT>::add_preprocess_task(
      std::bind(&SSPFrameProcessor::timestamp_check, this, std::placeholders::_1));
  }

  ~SSPFrameProcessor() {}

  void start(const nlohmann::json& args) override { inherited::start(args); }

  void stop(const nlohmann::json& args) override { inherited::stop(args); }

  void init(const nlohmann::json& args) override { inherited::init(args); }

  void conf(const nlohmann::json& cfg) override { inherited::conf(cfg); }

  void get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/) {}

  void timestamp_check(frameptr fp)
  {
    /*
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) {         // emulate perfectly incrementing timestamp
      uint64_t ts_next = m_previous_ts + 300; // NOLINT(build/unsigned)
      for (unsigned int i = 0; i < 12; ++i) { // NOLINT(build/unsigned)
<<<<<<< HEAD
        auto wf = reinterpret_cast<dunedaq::detdataformats::wib::WIBFrame*>(((uint8_t*)fp) + i * 464); // NOLINT
        auto wfh = const_cast<dunedaq::detdataformats::wib::WIBHeader*>(wf->get_wib_header());
=======
        auto wf = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(((uint8_t*)fp) + i * 464); // NOLINT
        auto wfh = const_cast<dunedaq::dataformats::WIBHeader*>(wf->get_wib_header());
>>>>>>> fgrotsch/zero_copy_recording
        wfh->set_timestamp(ts_next);
        ts_next += 25;
      }
    }
    */
    // TLOG() << "Got frame with timestamp: " << fp->get_timestamp();
    inherited::m_last_processed_daq_ts = fp->get_first_timestamp();
  }

protected:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SSP_SSPFRAMEPROCESSOR_HPP_
