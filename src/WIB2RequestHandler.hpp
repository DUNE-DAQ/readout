/**
* @file WIB2RequestHandler.hpp Trigger matching mechanism for WIB frames. 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_WIB2REQUESTHANDLER_HPP_
#define READOUT_SRC_WIB2REQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "DefaultRequestHandlerModel.hpp"

#include "dataformats/wib2/WIB2Frame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "ContinousLatencyBufferModel.hpp"

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>
#include <utility>
#include <memory>
#include <string>
#include <vector>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

class WIB2RequestHandler : public DefaultRequestHandlerModel<types::WIB2_SUPERCHUNK_STRUCT, ContinousLatencyBufferModel<types::WIB2_SUPERCHUNK_STRUCT>> {
public:
  explicit WIB2RequestHandler(std::unique_ptr<ContinousLatencyBufferModel<types::WIB2_SUPERCHUNK_STRUCT>>& latency_buffer,
                              std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                              std::unique_ptr<appfwk::DAQSink<types::WIB2_SUPERCHUNK_STRUCT>>& snb_sink)
  : DefaultRequestHandlerModel<types::WIB2_SUPERCHUNK_STRUCT, ContinousLatencyBufferModel<types::WIB2_SUPERCHUNK_STRUCT>>(latency_buffer, fragment_sink, snb_sink)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "WIB2RequestHandler created...";
  } 

  void conf(const nlohmann::json& args) override
  {
    // Call up to the base class, whose conf function does useful things
    DefaultRequestHandlerModel<types::WIB2_SUPERCHUNK_STRUCT, ContinousLatencyBufferModel<types::WIB2_SUPERCHUNK_STRUCT>>::conf(args);
    auto config = args.get<datalinkhandler::Conf>();
    m_apa_number = config.apa_number;
    m_link_number = config.link_number;
  }
  
protected:

  RequestResult
  data_request(dfmessages::DataRequest dr, unsigned delay_us = 0) // NOLINT
  {
    return tpc_data_request(dr, delay_us);
  }

  inline dataformats::FragmentHeader 
  create_fragment_header(const dfmessages::DataRequest& dr) 
  {
    dataformats::FragmentHeader fh;
    fh.size = sizeof(fh);
    fh.trigger_number = dr.trigger_number;    
    fh.trigger_timestamp = dr.trigger_timestamp;
    fh.window_begin = dr.window_begin;
    fh.window_end = dr.window_end;
    fh.run_number = dr.run_number;
    fh.link_id = { m_apa_number, m_link_number };
    fh.fragment_type = static_cast<dataformats::fragment_type_t>(dataformats::FragmentType::kTPCData);
    return std::move(fh);
  } 

  inline void
  dump_to_buffer(const void* data, std::size_t size,
                 void* buffer, uint32_t buffer_pos, // NOLINT
                 const std::size_t& buffer_size)
  {
    auto bytes_to_copy = size; // NOLINT
    while(bytes_to_copy > 0) {
      auto n = std::min(bytes_to_copy, buffer_size-buffer_pos); // NOLINT
      std::memcpy(static_cast<char*>(buffer)+buffer_pos, static_cast<const char*>(data), n);
      buffer_pos += n;
      bytes_to_copy -= n;
      if(buffer_pos == buffer_size) {
        buffer_pos = 0;
      }
    }
  }

  RequestResult
  tpc_data_request(dfmessages::DataRequest dr, unsigned delay_us) {
    if (delay_us > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
    }

    // Prepare response
    RequestResult rres(ResultCode::kUnknown, dr);

    // Data availability is calculated here
    size_t occupancy_guess = m_latency_buffer->occupancy();
    auto front_frame = *(reinterpret_cast<const dataformats::WIB2Frame*>( m_latency_buffer->front(0) )); // NOLINT
    uint64_t last_ts = front_frame.get_timestamp();  // NOLINT
    uint64_t start_win_ts = dr.window_begin;  // NOLINT
    uint64_t end_win_ts = dr.window_end;  // NOLINT
    uint64_t newest_ts = last_ts + (occupancy_guess-m_safe_num_elements_margin) * m_tick_dist * m_frames_per_element; // NOLINT
    int64_t time_tick_diff = (start_win_ts - last_ts) / m_tick_dist;      // NOLINT
    int32_t num_element_offset = time_tick_diff / m_frames_per_element;   // NOLINT
    uint32_t num_elements_in_window = (end_win_ts - start_win_ts) / (m_tick_dist * m_frames_per_element) + 1; // NOLINT
    int32_t min_num_elements = num_element_offset + num_elements_in_window + m_safe_num_elements_margin; //NOLINT
     

    TLOG_DEBUG(TLVL_WORK_STEPS) << "TPC (WIB frame) data request for " 
      << "Trigger TS=" << dr.trigger_timestamp << " "
      << "Oldest stored TS=" << last_ts << " "
      << "Start of window TS=" << start_win_ts << " "
      << "End of window TS=" << end_win_ts << " "
      << "Estimated newest stored TS=" << newest_ts;

     // << "ElementOffset=" << num_element_offset << " "
     // << "ElementsInWindow=" << num_elements_in_window << " "
     // << "MinNumElements=" << min_num_elements << " "
     // << "Occupancy=" << occupancy_guess;

    // Prepare FragmentHeader and empty Fragment pieces list
    auto frag_header = create_fragment_header(dr);

    // List of safe-extraction conditions
    //
    if ( num_elements_in_window > m_max_requested_elements ) { // too big window, cannot handle it yet
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kInvalidWindow));
      rres.result_code = ResultCode::kPass;
      ++m_bad_requested_count;
    }
    else if (last_ts <= start_win_ts && end_win_ts <= newest_ts) { // data is there
      rres.result_code = ResultCode::kFound;
      ++m_found_requested_count; 
    }
    else if ( last_ts > start_win_ts ) { // data is gone.
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    }
    else if ( newest_ts < end_win_ts ) {
      rres.result_code = ResultCode::kNotYet; // give it another chance
    }
    else {
      TLOG() << "Don't know how to categorise this request";
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    }

    // Requeue if needed
    if ( rres.result_code == ResultCode::kNotYet ) {
     if (m_run_marker.load()) {
        rres.request_delay_us = (min_num_elements - occupancy_guess) * m_frames_per_element * m_tick_dist / 1000.;
        if (rres.request_delay_us < m_min_delay_us) { // minimum delay protection
          rres.request_delay_us = m_min_delay_us; 
        }
        return rres; // If kNotYet, return immediately, don't check for fragment pieces.
      } else {
        frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
        rres.result_code = ResultCode::kNotFound;
        ++m_bad_requested_count;
      }
    } 

    // Build fragment
    std::vector<std::pair<void*, size_t> > frag_pieces;
    std::ostringstream oss;
    oss << "TS match result on link " << m_link_number << ": " << resultCodeAsString(rres.result_code) << ' ' 
      << "Trigger number=" << dr.trigger_number << " "
      << "Oldest stored TS=" << last_ts << " "
      << "Start of window TS=" << start_win_ts << " "
      << "End of window TS=" << end_win_ts << " "
      << "Estimated newest stored TS=" << newest_ts;
    TLOG_DEBUG(TLVL_WORK_STEPS) << oss.str();

    if ( rres.result_code != ResultCode::kFound ) {
      ers::warning(dunedaq::readout::TrmWithEmptyFragment(ERS_HERE, oss.str()));
    } else {

      auto elements_handled = 0;
      
      for (uint32_t idxoffset=0; idxoffset<num_elements_in_window; ++idxoffset) { // NOLINT
        auto* element = static_cast<void*>(m_latency_buffer->front(num_element_offset+idxoffset));

        if (element != nullptr) {
          frag_pieces.emplace_back( 
            std::make_pair<void*, size_t>(
              static_cast<void*>(m_latency_buffer->front(num_element_offset + idxoffset)),
              std::size_t(m_element_size))
          );
          elements_handled++;
          //TLOG() << "Elements handled: " << elements_handled;
        } else {
          TLOG() << "NULLPTR NOHANDLE";
          break;
        }
      }
   }
   // Create fragment from pieces

   auto frag = std::make_unique<dataformats::Fragment>(frag_pieces);


   // Set header
   frag->set_header_fields(frag_header);
   // Push to Fragment queue
   try {
     m_fragment_sink->push( std::move(frag) );
   }
   catch (const ers::Issue& excpt) {
     std::ostringstream oss;
     oss << "fragments output queueu for link " << m_link_number ;
     ers::warning(CannotWriteToQueue(ERS_HERE, oss.str(), excpt));
   }
  return rres;
  }

private:
  // Constants
  static const constexpr uint64_t m_tick_dist = 25; // 2 MHz@50MHz clock // NOLINT
  static const constexpr size_t m_wib_frame_size = 468;
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
