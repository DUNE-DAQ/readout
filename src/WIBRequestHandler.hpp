/*
* @file WIBRequestHandler.hpp Buffers objects for some time
* Software defined latency buffer to temporarily store objects from the
* frontend apparatus. It wraps a bounded SPSC queue from Folly for
* aligned memory access, and convenient frontPtr loads.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_WIBREQUESTHANDLER_HPP_
#define UDAQ_READOUT_SRC_WIBREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "DefaultRequestHandlerModel.hpp"

#include "dataformats/wib/WIBFrame.hpp"

#include "ReadoutStatistics.hpp"

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>

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
    ERS_INFO("WIBRequestHandler created...");
    data_request_callback_ = std::bind(&WIBRequestHandler::tpc_data_request, this, std::placeholders::_1);
  } 

  void conf(const nlohmann::json& args) override
  {
    // Call up to the base class, whose conf function does useful things
    DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT>::conf(args);
    auto config = args.get<datalinkhandler::Conf>();
    apa_number_ = config.apa_number;
    link_number_ = config.link_number;
  }
  
protected:

  inline dataformats::FragmentHeader 
  create_fragment_header(const dfmessages::DataRequest& dr) 
  {
    dataformats::FragmentHeader fh;
    fh.size = 0;
    fh.trigger_number = dr.trigger_number;
    fh.trigger_timestamp = dr.trigger_timestamp;
    fh.window_offset = dr.window_offset;
    fh.window_width = dr.window_width;
    fh.run_number = dr.run_number;
    fh.link_id = { apa_number_, link_number_ };
    return std::move(fh);
  }

  void tpc_data_request(dfmessages::DataRequest dr) {
    // Data availability is calculated here
    size_t occupancy_guess = occupancy_callback_(); 
    uint_fast64_t start_win_ts = dr.trigger_timestamp - (uint_fast64_t)(dr.window_offset * tick_dist_);
    dataformats::WIBHeader front_wh = *(reinterpret_cast<const dataformats::WIBHeader*>( front_callback_(0) ));
    uint_fast64_t last_ts = front_wh.timestamp();
    uint_fast64_t time_tick_diff = (start_win_ts - last_ts) / tick_dist_;
    uint_fast32_t num_element_offset = time_tick_diff / frames_per_element_;
    uint_fast32_t num_elements_in_window = dr.window_width / frames_per_element_ + 1;
    uint_fast32_t min_num_elements = (time_tick_diff + dr.window_width) / frames_per_element_ + safe_num_elements_margin_;
    ERS_INFO("TPC (WIB frame) data request for " 
      << "Last TS=" << last_ts << " Tickdiff=" << time_tick_diff << " "
      << "ElementOffset=" << num_element_offset << ".th "
      << "ElementsInWindow=" << num_elements_in_window << " "
      << "MinNumElements=" << min_num_elements << " "
      << "Occupancy=" << occupancy_guess
    );

    // Prepare FragmentHeader and empty Fragment pieces list
    auto frag_header = create_fragment_header(dr);
    std::vector<std::pair<void*, size_t>> frag_pieces;

#warning RS FIXME -> fix/enforce every possible timestamp boundary & occupancy checks
    // Find data in Latency Buffer
    if ( last_ts > start_win_ts || min_num_elements > occupancy_guess ) {
      ERS_INFO("Out of bound reqested timestamp based on latency buffer occupancy!");
      // PAR 2021-01-12: At some point, the error bits will have
      // well-defined meanings, but at time of writing they don't, so
      // we just set an arbitrary error bit
      frag_header.error_bits |= 0x1;
      ++bad_requested_count_;
    } else {
      auto fromheader = *(reinterpret_cast<const dataformats::WIBHeader*>(front_callback_(num_element_offset)));
      //fromheader.print();
      size_t piecesize = element_size_;
      for (uint_fast32_t idxoffset=0; idxoffset<num_elements_in_window; ++idxoffset) {
        frag_pieces.emplace_back( 
          std::make_pair<void*, size_t>( (void*)(front_callback_(num_element_offset+idxoffset)), std::size_t(element_size_) ) 
        ); 
      }
      ++found_requested_count_;
    }

    // Finish Request handling with Fragment creation
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

private:
  // Constants
  const uint_fast64_t tick_dist_ = 25;
  const size_t wib_frame_size_ = 464;
  const uint_fast8_t frames_per_element_ = 12;
  const size_t element_size_ = wib_frame_size_ * frames_per_element_;
  const uint_fast64_t safe_num_elements_margin_ = 10;

  // Stats
  stats::counter_t found_requested_count_{0};
  stats::counter_t bad_requested_count_{0};

  uint32_t apa_number_;
  uint32_t link_number_;

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_WIBREQUESTHANDLER_HPP_
