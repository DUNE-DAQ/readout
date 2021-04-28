/**
* @file WIB2FrameProcessor.hpp WIB2 specific Task based raw processor
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_WIB2FRAMEPROCESSOR_HPP_
#define READOUT_SRC_WIB2FRAMEPROCESSOR_HPP_

#include "TaskRawDataProcessorModel.hpp"
#include "ReadoutStatistics.hpp"
#include "ReadoutIssues.hpp"
#include "Time.hpp"

#include "dataformats/wib2/WIB2Frame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include <string>
#include <atomic>
#include <functional>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

class WIB2FrameProcessor : public TaskRawDataProcessorModel<types::WIB2_SUPERCHUNK_STRUCT> {

public:
  using inherited = TaskRawDataProcessorModel<types::WIB2_SUPERCHUNK_STRUCT>;
  using frameptr = types::WIB2_SUPERCHUNK_STRUCT*;
  using wib2frameptr = dunedaq::dataformats::WIB2Frame*;


  WIB2FrameProcessor(const std::string& rawtype)
  : TaskRawDataProcessorModel<types::WIB2_SUPERCHUNK_STRUCT>(rawtype)
  {
    m_tasklist.push_back( std::bind(&WIB2FrameProcessor::timestamp_check, this, std::placeholders::_1) );
    //m_tasklist.push_back( std::bind(&WIB2FrameProcessor::frame_error_check, this, std::placeholders::_1) );
  } 

protected:
  // Internals  
  time::timestamp_t m_previous_ts = 0;
  time::timestamp_t m_current_ts = 0;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  stats::counter_t m_ts_error_ctr{0};

  /**
   * Pipeline Stage 1.: Check proper timestamp increments in WIB frame
   * */
  void timestamp_check(frameptr fp) {
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) { // emulate perfectly incrementing timestamp
      uint64_t ts_next = m_previous_ts + 300;
      for (unsigned int i=0; i<12; ++i) { // NOLINT
        auto wf = reinterpret_cast<dunedaq::dataformats::WIB2Frame*>(((uint8_t*)fp)+i*468); // NOLINT
        auto wfh = wf->header; //const_cast<dunedaq::dataformats::WIB2Frame::Header*>(wf->get_wib_header());
        
        //wfh->set_timestamp(ts_next);

        ts_next += 25;
      }
    }

    // Acquire timestamp
    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIB2Frame*>(fp); // NOLINT
    m_current_ts = wfptr->get_timestamp();

    // Check timestamp
    if (m_current_ts - m_previous_ts != 300) {
      ++m_ts_error_ctr;
      if (m_first_ts_missmatch) { // log once
        TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp MISSMATCH! -> | previous: " << std::to_string(m_previous_ts) 
          << " current: "+std::to_string(m_current_ts);
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
  void frame_error_check(frameptr /*fp*/) {
    // check error fields
  }

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB2FRAMEPROCESSOR_HPP_
