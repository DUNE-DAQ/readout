/**
* @file PDSListRequestHandler.hpp Trigger matching mechanism for WIB frames. 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_PDSLISTREQUESTHANDLER_HPP_
#define READOUT_SRC_PDSLISTREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "DefaultRequestHandlerModel.hpp"
#include "SkipListLatencyBufferModel.hpp"

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
#include <deque>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

class PDSListRequestHandler : public DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT, 
                                     SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>> {
public:
  using inherited = DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT,
    SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>;
  using SkipListAcc = typename folly::ConcurrentSkipList<types::PDS_SUPERCHUNK_STRUCT>::Accessor;
  using SkipListSkip = typename folly::ConcurrentSkipList<types::PDS_SUPERCHUNK_STRUCT>::Skipper;

  PDSListRequestHandler(std::unique_ptr<SkipListLatencyBufferModel<
    types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>& latency_buffer,
                    std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                    std::unique_ptr<appfwk::DAQSink<types::PDS_SUPERCHUNK_STRUCT>>& snb_sink)
  : DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT, SkipListLatencyBufferModel<
      types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>(latency_buffer, fragment_sink, snb_sink)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "PDSListRequestHandler created...";
  } 

  void conf(const nlohmann::json& args) override
  {
    // Call up to the base class, whose conf function does useful things
    DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT, 
      SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>>::conf(args);
    auto config = args.get<datalinkhandler::Conf>();
    m_apa_number = config.apa_number;
    m_link_number = config.link_number;
  }

protected:
  RequestResult
  cleanup_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0) override // NOLINT
  {
    return pds_cleanup_request(dr);
  }

  RequestResult
  pds_cleanup_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0) {
    //size_t occupancy_guess = m_latency_buffer->occupancy();
    size_t removed_ctr = 0;
    uint64_t tailts = 0; // oldest
    uint64_t headts = 0; // newest
    {
      SkipListAcc acc(m_latency_buffer->get_skip_list());
      auto tail = acc.last();
      auto head = acc.first();
      if (tail && head) {
        //auto tailptr = reinterpret_cast<const dataformats::PDSFrame*>(tail); // NOLINT
        //auto headptr = reinterpret_cast<const dataformats::PDSFrame*>(head); // NOLINT
        tailts = (*tail).get_timestamp(); //tailptr->get_timestamp();  // NOLINT
        headts = (*head).get_timestamp(); //headptr->get_timestamp(); 
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Cleanup REQUEST with " 
          << "Oldest stored TS=" << tailts << " "
          << "Newest stored TS=" << headts;
        if (headts - tailts > m_max_ts_diff) { // ts differnce exceeds maximum
          ++inherited::m_pop_reqs;
          uint64_t timediff = m_max_ts_diff;
          while (timediff >= m_max_ts_diff) {
            bool removed = acc.remove(*tail);
            if (!removed) {
              TLOG_DEBUG(TLVL_WORK_STEPS) << "Unsuccesfull remove from SKL during cleanup: " << removed; 
            } else {
              ++removed_ctr;
            }
            tail = acc.last();
            //headptr = reinterpret_cast<const dataformats::PDSFrame*>(head);
            tailts = (*tail).get_timestamp(); // headptr->get_timestamp();
            timediff = headts - tailts;
          }
          inherited::m_pops_count += removed_ctr;
        }
      } else {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Didn't manage to get SKL head and tail!";
      }
    }
    inherited::m_cleanup_requested = false;
    return RequestResult(ResultCode::kCleanup, dr);
  } 

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
    fh.element_id = { dataformats::GeoID::SystemType::kPDS, m_apa_number, m_link_number };
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
    // Prepare FragmentHeader
    auto frag_header = create_fragment_header(dr);
    // Prepare empty fragment piece container
    std::deque<std::pair<void*, size_t>> frag_deq; // for push front (reverse ordered list)
    std::vector<std::pair<void*, size_t> > frag_pieces;

    // TS calculation
    uint64_t start_win_ts = dr.window_begin;  // NOLINT
    uint64_t end_win_ts = dr.window_end;  // NOLINT
    uint64_t tailts = 0; // oldest ts
    uint64_t headts = 0; // newest ts

    //size_t occupancy_guess = m_latency_buffer->occupancy();

    // Accessing CSKL
    {
      SkipListAcc acc(m_latency_buffer->get_skip_list());
      auto tail = acc.last();
      auto head = acc.first();
      if (tail && head) {
        //auto tailptr = reinterpret_cast<const dataformats::PDSFrame*>(tail); // NOLINT
        //auto headptr = reinterpret_cast<const dataformats::PDSFrame*>(head); // NOLINT
        tailts = (*tail).get_timestamp(); //tailptr->get_timestamp();  // NOLINT
        headts = (*head).get_timestamp(); //headptr->get_timestamp();

        if (tailts <= start_win_ts && end_win_ts <= headts) { // data is there
          rres.result_code = ResultCode::kFound;
          ++m_found_requested_count;
        }
        else if ( headts < end_win_ts ) {
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
            rres.request_delay_us = m_min_delay_us; 
            return rres; // If kNotYet, return immediately, don't check for fragment pieces.
          } else {
            frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
            rres.result_code = ResultCode::kNotFound;
            ++m_bad_requested_count;
          }
        }
      
        std::ostringstream oss;
        oss << "TS match result on link " << m_link_number << ": " << resultCodeAsString(rres.result_code) << ' '
            << "Trigger number=" << dr.trigger_number << " "
            << "Oldest stored TS=" << tailts << " "
            << "Newest stored TS=" << headts << " "
            << "Start of window TS=" << start_win_ts << " "
            << "End of window TS=" << end_win_ts;
        TLOG_DEBUG(TLVL_WORK_STEPS) << oss.str();
      
        // Build fragment if found
        if ( rres.result_code != ResultCode::kFound ) {
          ers::warning(dunedaq::readout::TrmWithEmptyFragment(ERS_HERE, oss.str()));
        } else {
    
          // Prepare key struct
          types::PDS_SUPERCHUNK_STRUCT trig_key_start;
          trig_key_start.set_timestamp(start_win_ts);

          types::PDS_SUPERCHUNK_STRUCT trig_key_end;
          trig_key_end.set_timestamp(end_win_ts); //! sort order is TS decremented
    
          // Find iterator to lower bound of key
          auto close = acc.lower_bound(trig_key_end);

          // ... or with Skipper
          //SkipListSkip skip(acc);
          //skip.to(trig_key);

          // Frag piece counter
          auto elements_handled = 0;     
 
          if (close.good()) {
            //frag_pieces.emplace_back( // emplace first pointer
            frag_deq.push_front(
              std::make_pair<void*, size_t>(
                static_cast<void*>(&(*close)), sizeof(types::PDS_SUPERCHUNK_STRUCT))
            ); 
            ++elements_handled;
            auto it_ts = (*close).get_timestamp();
            while (it_ts > start_win_ts) { // while window not closed
              close++; // iterate
              if (!close.good()) { // reached invalid it state
                break;
              }
              //TLOG_DEBUG(TLVL_WORK_STEPS) << "    -> TS: " << (*close).get_timestamp();
              //frag_pieces.emplace_back( // emplace first pointer
              frag_deq.push_front(
                std::make_pair<void*, size_t>(
                  static_cast<void*>(&(*close)), sizeof(types::PDS_SUPERCHUNK_STRUCT))
              );
              it_ts = (*close).get_timestamp();
              ++elements_handled;             
            }
          }

        } // found request

      } // found both head and tail
    } // SKL access end

    // Create fragment from pieces
    while (!frag_deq.empty()){
      TLOG() << "BOOM BOOM";
      frag_pieces.emplace_back(std::move(frag_deq.front()));
      frag_deq.pop_front();
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fragment with " << frag_pieces.size() << " pieces. "
                                << "(Deque consumed? " << frag_deq.size() << ")";
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
  static const constexpr uint64_t m_max_ts_diff = 10000000;

  static const constexpr uint64_t m_tick_dist = 16; // NOLINT
  static const constexpr size_t m_wib_frame_size = 468;
  static const constexpr uint8_t m_frames_per_element = 12; // NOLINT
  static const constexpr size_t m_element_size = m_wib_frame_size * m_frames_per_element;
  static const constexpr uint64_t m_safe_num_elements_margin = 10; // NOLINT

  static const constexpr uint32_t m_min_delay_us = 30000; // NOLINT

  // Stats
  stats::counter_t m_found_requested_count{0};
  stats::counter_t m_bad_requested_count{0};

  uint16_t m_apa_number; // NOLINT
  uint32_t m_link_number; // NOLINT

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_PDSLISTREQUESTHANDLER_HPP_
