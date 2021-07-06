/**
 * @file WIB2FrameProcessor.hpp WIB2 specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB2_WIB2FRAMEPROCESSOR_HPP_
#define READOUT_SRC_WIB2_WIB2FRAMEPROCESSOR_HPP_

#include "ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "dataformats/wib2/WIB2Frame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "readout/FrameErrorRegistry.hpp"

#include <atomic>
#include <functional>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class WIB2FrameProcessor : public TaskRawDataProcessorModel<types::WIB2_SUPERCHUNK_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::WIB2_SUPERCHUNK_STRUCT>;
  using frameptr = types::WIB2_SUPERCHUNK_STRUCT*;
  using wib2frameptr = dunedaq::dataformats::WIB2Frame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  WIB2FrameProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::WIB2_SUPERCHUNK_STRUCT>(error_registry)
  {
    m_tasklist.push_back(std::bind(&WIB2FrameProcessor::timestamp_check, this, std::placeholders::_1));
    // m_tasklist.push_back( std::bind(&WIB2FrameProcessor::frame_error_check, this, std::placeholders::_1) );
  }

protected:
  // Internals
  timestamp_t m_previous_ts = 0;
  timestamp_t m_current_ts = 0;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  std::atomic<int> m_ts_error_ctr{ 0 };

  /**
   * Pipeline Stage 1.: Check proper timestamp increments in WIB frame
   * */
  void timestamp_check(frameptr fp)
  {
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) {         // emulate perfectly incrementing timestamp
      uint64_t ts_next = m_previous_ts + 384; // NOLINT(build/unsigned)
      for (unsigned int i = 0; i < 12; ++i) { // NOLINT(build/unsigned)
        auto wf = reinterpret_cast<dunedaq::dataformats::WIB2Frame*>(((uint8_t*)fp) + i * 468); // NOLINT
        auto& wfh = wf->header; // const_cast<dunedaq::dataformats::WIB2Frame::Header*>(wf->get_wib_header());
        // wfh->set_timestamp(ts_next);
        wfh.timestamp_1 = ts_next;
        wfh.timestamp_2 = ts_next >> 32;
        ts_next += 32;
      }
    }

    // Acquire timestamp
    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIB2Frame*>(fp); // NOLINT
    m_current_ts = wfptr->get_timestamp();

    // Check timestamp
    if (m_current_ts - m_previous_ts != 384) {
      ++m_ts_error_ctr;
      m_error_registry->add_error(FrameErrorRegistry::FrameError(m_previous_ts + 384, m_current_ts));
      if (m_first_ts_missmatch) { // log once
        TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp MISSMATCH! -> | previous: " << std::to_string(m_previous_ts)
                                     << " current: " + std::to_string(m_current_ts);
        m_first_ts_missmatch = false;
      }
    }

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
   * Pipeline Stage 2.: Check WIB headers for error flags
   * */
  void frame_error_check(frameptr /*fp*/)
  {
    // check error fields
  }

private:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB2_WIB2FRAMEPROCESSOR_HPP_
