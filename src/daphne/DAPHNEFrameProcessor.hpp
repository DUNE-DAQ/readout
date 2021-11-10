/**
 * @file DAPHNEFrameProcessor.hpp DAPHNE specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_DAPHNE_DAPHNEFRAMEPROCESSOR_HPP_
#define READOUT_SRC_DAPHNE_DAPHNEFRAMEPROCESSOR_HPP_

#include "readout/ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "detdataformats/daphne/DAPHNEFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class DAPHNEFrameProcessor : public TaskRawDataProcessorModel<types::DAPHNE_SUPERCHUNK_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::DAPHNE_SUPERCHUNK_STRUCT>;
  using frameptr = types::DAPHNE_SUPERCHUNK_STRUCT*;
  using daphneframeptr = dunedaq::detdataformats::daphne::DAPHNEFrame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  explicit DAPHNEFrameProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::DAPHNE_SUPERCHUNK_STRUCT>(error_registry)
  {
    TaskRawDataProcessorModel<types::DAPHNE_SUPERCHUNK_STRUCT>::add_preprocess_task(
      std::bind(&DAPHNEFrameProcessor::timestamp_check, this, std::placeholders::_1));
    // m_tasklist.push_back( std::bind(&DAPHNEFrameProcessor::frame_error_check, this, std::placeholders::_1) );
  }

protected:
  // Internals
  timestamp_t m_previous_ts = 0;
  timestamp_t m_current_ts = 0;
  bool m_first_ts_fake = true;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  std::atomic<int> m_ts_error_ctr{ 0 };

  /**
   * Pipeline Stage 1.: Check proper timestamp increments in DAPHNE frame
   * */
  void timestamp_check(frameptr fp)
  {
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) { // emulate perfectly incrementing timestamp
      // RS warning : not fixed rate!
      if (m_first_ts_fake) {
        fp->fake_timestamp(m_previous_ts, 16);
        m_first_ts_fake = false;
      } else {
        fp->fake_timestamp(m_previous_ts + 192, 16);
      }
    }

    // Acquire timestamp
    m_current_ts = fp->get_timestamp();

    // Check timestamp
    // RS warning : not fixed rate!
    // if (m_current_ts - m_previous_ts != ???) {
    //  ++m_ts_error_ctr;
    //}

    if (m_ts_error_ctr > 1000) {
      if (!m_problem_reported) {
        TLOG() << "*** Data Integrity ERROR *** Timestamp continuity is completely broken! "
               << "Something is wrong with the FE source or with the configuration!";
        m_problem_reported = true;
      }
    }

    m_previous_ts = m_current_ts;
    m_last_processed_daq_ts = m_current_ts;
  }

  /**
   * Pipeline Stage 2.: Check DAPHNE headers for error flags
   * */
  void frame_error_check(frameptr /*fp*/)
  {
    // check error fields
  }

private:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_DAPHNE_DAPHNEFRAMEPROCESSOR_HPP_
