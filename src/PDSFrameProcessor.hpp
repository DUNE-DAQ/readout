/**
* @file PDSFrameProcessor.hpp PDS specific Task based raw processor
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_PDSFRAMEPROCESSOR_HPP_
#define READOUT_SRC_PDSFRAMEPROCESSOR_HPP_

#include "TaskRawDataProcessorModel.hpp"
#include "ReadoutStatistics.hpp"
#include "ReadoutIssues.hpp"
#include "Time.hpp"

#include "dataformats/pds/PDSFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"

#include <string>
#include <atomic>
#include <functional>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class PDSFrameProcessor : public TaskRawDataProcessorModel<types::PDS_SUPERCHUNK_STRUCT> {

public:
  using inherited = TaskRawDataProcessorModel<types::PDS_SUPERCHUNK_STRUCT>;
  using frameptr = types::PDS_SUPERCHUNK_STRUCT*;
  using pdsframeptr = dunedaq::dataformats::PDSFrame*;


  PDSFrameProcessor()
  : TaskRawDataProcessorModel<types::PDS_SUPERCHUNK_STRUCT>()
  {
    m_tasklist.push_back( std::bind(&PDSFrameProcessor::timestamp_check, this, std::placeholders::_1) );
    //m_tasklist.push_back( std::bind(&PDSFrameProcessor::frame_error_check, this, std::placeholders::_1) );
  } 

protected:
  // Internals  
  time::timestamp_t m_previous_ts = 0;
  time::timestamp_t m_current_ts = 0;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  stats::counter_t m_ts_error_ctr{0};

  /**
   * Pipeline Stage 1.: Check proper timestamp increments in PDS frame
   * */
  void timestamp_check(frameptr fp) {
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) { // emulate perfectly incrementing timestamp
      // RS warning : not fixed rate!
    }

    // Acquire timestamp
    auto wfptr = reinterpret_cast<dunedaq::dataformats::PDSFrame*>(fp); // NOLINT
    m_current_ts = wfptr->get_timestamp();

    // Check timestamp
    // RS warning : not fixed rate!
    //if (m_current_ts - m_previous_ts != ???) {
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
   * Pipeline Stage 2.: Check PDS headers for error flags
   * */
  void frame_error_check(frameptr /*fp*/) {
    // check error fields
  }

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_PDSFRAMEPROCESSOR_HPP_
