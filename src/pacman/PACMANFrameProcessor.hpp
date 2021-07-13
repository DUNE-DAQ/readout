/**
 * @file PACMANFrameProcessor.hpp PACMAN specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_PACMAN_PACMANFRAMEPROCESSOR_HPP_
#define READOUT_SRC_PACMAN_PACMANFRAMEPROCESSOR_HPP_

#include "ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "dataformats/pacman/PACMANFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/NDReadoutTypes.hpp"

#include <atomic>
#include <functional>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class PACMANFrameProcessor : public TaskRawDataProcessorModel<types::PACMAN_MESSAGE_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::PACMAN_MESSAGE_STRUCT>;
  using frameptr = types::PACMAN_MESSAGE_STRUCT*;
  using pacmanframeptr = dunedaq::dataformats::PACMANFrame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  PACMANFrameProcessor()
    : TaskRawDataProcessorModel<types::PACMAN_MESSAGE_STRUCT>()
  {
    m_tasklist.push_back(std::bind(&PACMANFrameProcessor::timestamp_check, this, std::placeholders::_1));
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
      // FIX ME - add fake timestamp to PACMAN message struct
    }

    // Acquire timestamp
    m_current_ts = fp->get_timestamp();

    // Check timestamp
    // RS warning : not fixed rate!
    /*if (m_current_ts - m_previous_ts <= 0) {
      ++m_ts_error_ctr;
        TLOG_DEBUG(TLVL_BOOKKEEPING) << "Timestamp continuity MISSMATCH! -> | previous: " << std::to_string(m_previous_ts)
                                     << " current: " + std::to_string(m_current_ts);
      }
*/
    if (m_ts_error_ctr > 1000) {
      if (!m_problem_reported) {
        TLOG() << "*** Data Integrity ERROR *** Timestamp continuity is completely broken! "
               << "Something is wrong with the FE source or with the configuration!";
        m_problem_reported = true;
      }
    }

    m_previous_ts = m_current_ts;
    m_last_processed_daq_ts = m_current_ts;
    //std::cout << "Last Processed DAQ TS: " << m_last_processed_daq_ts << std::endl;
  }

  /**
   * Pipeline Stage 2.: Check headers for error flags
   * */
  void frame_error_check(frameptr /*fp*/)
  {
    // check error fields
    // FIX ME - to be implemented

    //fp->inspect_message();
  }

private:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_PACMAN_PACMANFRAMEPROCESSOR_HPP_
