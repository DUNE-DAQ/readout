/**
 * @file WIBFrameProcessor.hpp WIB specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
#define READOUT_SRC_WIBFRAMEPROCESSOR_HPP_

#include "../../include/readout/ReadoutIssues.hpp"
#include "../../include/readout/ReadoutStatistics.hpp"
#include "../../include/readout/TaskRawDataProcessorModel.hpp"
#include "../../include/readout/Time.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include <atomic>
#include <functional>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class WIBFrameProcessor : public TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>;
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;

  WIBFrameProcessor()
    : TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>()
  {
    m_tasklist.push_back(std::bind(&WIBFrameProcessor::timestamp_check, this, std::placeholders::_1));
    // m_tasklist.push_back( std::bind(&WIBFrameProcessor::frame_error_check, this, std::placeholders::_1) );
  }

protected:
  // Internals
  time::timestamp_t m_previous_ts = 0;
  time::timestamp_t m_current_ts = 0;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  stats::counter_t m_ts_error_ctr{ 0 };

  /**
   * Pipeline Stage 1.: Check proper timestamp increments in WIB frame
   * */
  void timestamp_check(frameptr fp)
  {
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) {         // emulate perfectly incrementing timestamp
      uint64_t ts_next = m_previous_ts + 300; // NOLINT(build/unsigned)
      for (unsigned int i = 0; i < 12; ++i) { // NOLINT(build/unsigned)
        auto wf = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(((uint8_t*)fp) + i * 464); // NOLINT
        auto wfh = const_cast<dunedaq::dataformats::WIBHeader*>(wf->get_wib_header());
        wfh->set_timestamp(ts_next);
        ts_next += 25;
      }
    }

    // Acquire timestamp
    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(fp); // NOLINT
    m_current_ts = wfptr->get_wib_header()->get_timestamp();

    // Check timestamp
    if (m_current_ts - m_previous_ts != 300) {
      ++m_ts_error_ctr;
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

#endif // READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
