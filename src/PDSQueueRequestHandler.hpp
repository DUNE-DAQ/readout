/**
* @file PDSQueueRequestHandler.hpp Trigger matching mechanism for WIB frames. 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_PDSQUEUEREQUESTHANDLER_HPP_
#define READOUT_SRC_PDSQUEUEREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "DefaultRequestHandlerModel.hpp"
#include "SearchableLatencyBufferModel.hpp"

#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "dataformats/pds/PDSFrame.hpp"
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

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

class PDSQueueRequestHandler : public DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT, 
                                   SearchableLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>> { //NOLINT
public:
  PDSQueueRequestHandler(std::unique_ptr<SearchableLatencyBufferModel<
    types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>& latency_buffer, //NOLINT
                    std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                    std::unique_ptr<appfwk::DAQSink<types::PDS_SUPERCHUNK_STRUCT>>& snb_sink)
  : DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT, SearchableLatencyBufferModel<
      types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>(latency_buffer, fragment_sink, snb_sink) //NOLINT
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "PDSQueueRequestHandler created...";
  } 

  void conf(const nlohmann::json& args) override
  {
    // Call up to the base class, whose conf function does useful things
    DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT, 
      SearchableLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>::conf(args); //NOLINT
    auto config = args.get<datalinkhandler::Conf>();
    m_apa_number = config.apa_number;
    m_link_number = config.link_number;
  }
  
protected:

  /**
  RequestResult
  cleanup_request(dfmessages::DataRequest dr, unsigned delay_us  = 0) override // NOLINT
  {
    return pds_cleanup_request(dr);

  }
  */

  /**
  RequestResult
  pds_cleanup_request(dfmessages::DataRequest dr, unsigned delay_us = 0) {
    return RequestResult(ResultCode::kCleanup, dr);
  }
  */

  RequestResult
  data_request(dfmessages::DataRequest dr, unsigned delay_us = 0) // NOLINT
  {
    return pds_data_request(dr, delay_us);
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
    fh.element_id = { dataformats::GeoID::SystemType::kPDS, static_cast<uint16_t>(m_apa_number), m_link_number };
    fh.fragment_type = static_cast<dataformats::fragment_type_t>(dataformats::FragmentType::kPDSData);
    return std::move(fh);
  } 

  RequestResult
  pds_data_request(dfmessages::DataRequest dr, unsigned delay_us) {
    if (delay_us > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
    }

    // Prepare response
    RequestResult rres(ResultCode::kUnknown, dr);

    // Data availability is calculated here
    size_t occupancy_guess = m_latency_buffer->occupancy();
    auto front_frame = *(reinterpret_cast<const dataformats::PDSFrame*>( m_latency_buffer->get_ptr(0) )); // NOLINT
    auto last_frame = *(reinterpret_cast<const dataformats::PDSFrame*>( m_latency_buffer->get_ptr(occupancy_guess) )); // NOLINT
    uint64_t last_ts = front_frame.get_timestamp();  // NOLINT
    uint64_t newest_ts = last_frame.get_timestamp(); //NOLINT

    uint64_t start_win_ts = dr.window_begin;  // NOLINT
    uint64_t end_win_ts = dr.window_end;  // NOLINT
    auto start_idx = m_latency_buffer->find_index(start_win_ts);
    auto end_idx = m_latency_buffer->find_index(end_win_ts);
    //std::cout << start_idx << ", " << end_idx << std::endl;
    //std::cout << "Timestamps: " << start_win_ts << ", " << end_win_ts << std::endl;
    //std::cout << "Found timestamps: " << m_latency_buffer->at(start_idx)->get_timestamp() << ", " << m_latency_buffer->at(end_idx)->get_timestamp() << std::endl;

    TLOG_DEBUG(TLVL_WORK_STEPS) << "PDS (DAPHNE frame) data request for "
      << "Trigger TS=" << dr.trigger_timestamp << " "
      << "Oldest stored TS=" << last_ts << " "
      << "Newest stored TS=" << newest_ts << " "
      << "Start of window TS=" << start_win_ts << " "
      << "End of window TS=" << end_win_ts;

    // Prepare FragmentHeader and empty Fragment pieces list
    auto frag_header = create_fragment_header(dr);

    // List of safe-extraction conditions

    if ( start_idx >= 0 && end_idx < 0 ) { // data is not fully in buffer yet
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kInvalidWindow));
      rres.result_code = ResultCode::kPass;
      ++m_bad_requested_count;
    } else if (last_ts <= start_win_ts && end_win_ts <= newest_ts) { // data is there
      if (start_idx >= 0 && end_idx >= 0) { // data is there (double check)
        rres.result_code = ResultCode::kFound;
        ++m_found_requested_count;
      } else {
        
      }
    } else if (start_idx >= 0 && end_idx >= 0) { // data is there
      rres.result_code = ResultCode::kFound;
      ++m_found_requested_count; 
    } else if ( start_idx < 0 && end_idx >= 0 ) { // data is partially gone.
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    } else if ( newest_ts < end_win_ts ) {
      rres.result_code = ResultCode::kNotYet; // give it another chance
    } else {
      TLOG() << "Don't know how to categorise this request";
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    }

    // Requeue if needed
    if ( rres.result_code == ResultCode::kNotYet ) {
     if (m_run_marker.load()) {
        //rres.request_delay_us = (min_num_elements - occupancy_guess) * m_frames_per_element * m_tick_dist / 1000.;
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

      int last_chunk = m_latency_buffer->at(end_idx)->get_timestamp() == end_win_ts ? m_latency_buffer->next_index(end_idx) : end_idx;
      for (int index = start_idx; index != m_latency_buffer->next_index(last_chunk); index = m_latency_buffer->next_index(index)) { // NOLINT
        auto* element = static_cast<void*>(m_latency_buffer->at(index));

        if (element != nullptr) {
          frag_pieces.emplace_back( 
            std::make_pair<void*, size_t>(
              static_cast<void*>(m_latency_buffer->at(index)),
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
  static const constexpr uint64_t m_tick_dist = 16; // NOLINT
  static const constexpr size_t m_pds_frame_size = 584;
  static const constexpr uint8_t m_frames_per_element = 12; // NOLINT
  static const constexpr size_t m_element_size = m_pds_frame_size * m_frames_per_element;
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

#endif // READOUT_SRC_PDSQUEUEREQUESTHANDLER_HPP_
