/**
* @file WIBRequestHandler.hpp Trigger matching mechanism for WIB frames. 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_WIBREQUESTHANDLER_HPP_
#define READOUT_SRC_WIBREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "DefaultRequestHandlerModel.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>
#include <utility>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace readout {

class WIBRequestHandler : public DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT> {
public:
  explicit WIBRequestHandler(const std::string& rawtype,
                             std::atomic<bool>& marker,
                             std::function<size_t()>& occupancy_callback,
                             std::function<bool(types::WIB_SUPERCHUNK_STRUCT&)>& read_callback,
                             std::function<void(unsigned)>& pop_callback,
                             std::function<types::WIB_SUPERCHUNK_STRUCT*(unsigned)>& front_callback,
                             std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink)
  : DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT>(rawtype, marker,
      occupancy_callback, read_callback, pop_callback, front_callback, fragment_sink)
  {
    TLOG() << "WIBRequestHandler created...";
    data_request_callback_ = std::bind(&WIBRequestHandler::tpc_data_request, 
      this, std::placeholders::_1, std::placeholders::_2);
  } 

  void conf(const nlohmann::json& args) override
  {
    // Call up to the base class, whose conf function does useful things
    DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT>::conf(args);
    auto config = args.get<datalinkhandler::Conf>();
    m_apa_number = config.apa_number;
    m_link_number = config.link_number;
  }
  
protected:

  inline dataformats::FragmentHeader 
  create_fragment_header(const dfmessages::DataRequest& dr) 
  {
    dataformats::FragmentHeader fh;
    fh.m_size = sizeof(fh);
    fh.m_trigger_number = dr.m_trigger_number;    
    fh.m_trigger_timestamp = dr.m_trigger_timestamp;
    fh.m_window_offset = dr.m_window_offset;
    fh.m_window_width = dr.m_window_width;
    fh.m_run_number = dr.m_run_number;
    fh.m_link_id = { m_apa_number, m_link_number };
    return std::move(fh);
  } 

  RequestResult
  tpc_data_request(dfmessages::DataRequest dr, unsigned delay_us) {
    if (delay_us > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
    }

    // Prepare response
    RequestResult rres(ResultCode::kUnknown, dr);

    // Data availability is calculated here
    size_t occupancy_guess = occupancy_callback_(); 
    dataformats::WIBHeader front_wh = *(reinterpret_cast<const dataformats::WIBHeader*>( front_callback_(0) )); // NOLINT
    uint64_t start_win_ts = dr.m_trigger_timestamp - dr.m_window_offset;   // NOLINT
    uint64_t last_ts = front_wh.get_timestamp();                           // NOLINT
    uint64_t time_tick_diff = (start_win_ts - last_ts) / m_tick_dist;      // NOLINT
    uint32_t num_element_offset = time_tick_diff / m_frames_per_element;   // NOLINT
    uint32_t num_elements_in_window = dr.m_window_width / (m_tick_dist * m_frames_per_element) + 1; // NOLINT
    uint32_t min_num_elements = (time_tick_diff + dr.m_window_width/m_tick_dist)                    // NOLINT
                                   / m_frames_per_element + m_safe_num_elements_margin;
    TLOG_DEBUG(2) << "TPC (WIB frame) data request for " 
      << "Trigger TS=" << dr.m_trigger_timestamp << " "
      << "Last TS=" << last_ts << " Tickdiff=" << time_tick_diff << " "
      << "ElementOffset=" << num_element_offset << " "
      << "ElementsInWindow=" << num_elements_in_window << " "
      << "MinNumElements=" << min_num_elements << " "
      << "Occupancy=" << occupancy_guess;

    // Prepare FragmentHeader and empty Fragment pieces list
    auto frag_header = create_fragment_header(dr);
    std::vector<std::pair<void*, size_t>> frag_pieces;

    // List of safe-extraction conditions
    if ( num_elements_in_window > max_requested_elements_ ) {
      frag_header.m_error_bits |= 0x2; // error bit too big window
      rres.result_code = ResultCode::kPass;
      ++m_bad_requested_count;
    } else if ( last_ts > start_win_ts ) { // data is gone.
      frag_header.m_error_bits |= 0x1; // error bit for not-found data
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    } else if ( min_num_elements > occupancy_guess ) {
      rres.result_code = ResultCode::kNotYet; // give it another chance
      //rres.request_delay_us = 1000;
      if(run_marker_.load()) {
         rres.request_delay_us = (min_num_elements - occupancy_guess) * m_frames_per_element * m_tick_dist / 1000.;
         if (rres.request_delay_us < m_min_delay_us) { // minimum delay protection
           rres.request_delay_us = m_min_delay_us; 
         }
      } else {
          frag_header.m_error_bits |= 0x1; // error bit for not-found data
          rres.result_code = ResultCode::kNotFound;
          ++m_bad_requested_count;
      }
    } else {
      rres.result_code = ResultCode::kFound;
      ++m_found_requested_count;
    }

    // Find data in Latency Buffer
    if ( rres.result_code != ResultCode::kFound ) {
      std::ostringstream oss;
      oss << "***ERROR: timestamp match result: " << resultCodeAsString(rres.result_code) << ' ' 
        << "Triggered window first ts: " << start_win_ts << " "
        << "Trigger TS=" << dr.m_trigger_timestamp << " " 
        << "Last TS=" << last_ts << " Tickdiff=" << time_tick_diff << " "
        << "ElementOffset=" << num_element_offset << ".th "
        << "ElementsInWindow=" << num_elements_in_window << " "
        << "MinNumElements=" << min_num_elements << " "
        << "Occupancy=" << occupancy_guess;
      TLOG() << oss.str();
    } else {
      //auto fromheader = *(reinterpret_cast<const dataformats::WIBHeader*>(front_callback_(num_element_offset)));
      for (uint32_t idxoffset=0; idxoffset<num_elements_in_window; ++idxoffset) { // NOLINT
        auto* element = front_callback_(num_element_offset+idxoffset);
        if (element != nullptr) {
          frag_pieces.emplace_back( 
            std::make_pair<void*, size_t>(
              static_cast<void*>(front_callback_(num_element_offset + idxoffset)), 
              std::size_t(m_element_size))
          );
        }
      }
    }

    // Finish Request handling with Fragment creation
    if (rres.result_code != ResultCode::kNotYet) { // Don't send out fragment for requeued request
      try {
        // Create fragment from pieces
        auto frag = std::make_unique<dataformats::Fragment>(frag_pieces);
        // Set header
        frag->set_header_fields(frag_header);
        // Push to Fragment queue
        fragment_sink_->push( std::move(frag) );
      } 
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ers::error(QueueTimeoutError(ERS_HERE, " fragment sink "));
      }
    }
    return rres;
  }

private:
  // Constants
  static const constexpr uint64_t m_tick_dist = 25; // 2 MHz@50MHz clock // NOLINT
  static const constexpr size_t m_wib_frame_size = 464;
  static const constexpr uint8_t m_frames_per_element = 12; // NOLINT
  static const constexpr size_t m_element_size = m_wib_frame_size * m_frames_per_element;
  static const constexpr uint64_t m_safe_num_elements_margin = 10; // NOLINT

  static const constexpr uint32_t m_min_delay_us = 30000; // NOLINT

  // Stats
  stats::counter_t m_found_requested_count{0};
  stats::counter_t m_bad_requested_count{0};

  uint32_t m_apa_number; // NOLINT
  uint32_t m_link_number; // NOLINT

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIBREQUESTHANDLER_HPP_
