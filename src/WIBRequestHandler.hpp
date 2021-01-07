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
                             std::function<types::WIB_SUPERCHUNK_STRUCT*()>& front_callback,
                             std::unique_ptr<appfwk::DAQSource<dfmessages::DataRequest>>& request_source,
                             std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink)
  : DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT>(rawtype, marker, 
      occupancy_callback, read_callback, pop_callback, front_callback, 
      request_source, fragment_sink)
  {
    ERS_INFO("WIBRequestHandler created...");
    data_request_callback_ = std::bind(&WIBRequestHandler::tpc_data_request, this, std::placeholders::_1);
  } 

protected:
  void tpc_data_request(dfmessages::DataRequest dr) {
    std::vector<std::pair<void*, size_t>> frag_pieces;
    size_t occupancy_guess = occupancy_callback_(); 
    uint_fast64_t start_win_ts = dr.trigger_timestamp - (uint_fast64_t)(dr.window_offset * tick_dist_);
    dataformats::WIBHeader front_wh = *(reinterpret_cast<const dataformats::WIBHeader*>( front_callback_() ));
    uint_fast64_t last_ts = front_wh.timestamp();
    uint_fast64_t time_tick_diff = (start_win_ts - last_ts) / tick_dist_;
    uint_fast64_t num_element_offset = time_tick_diff / frames_per_element_;
    uint_fast64_t num_elements_in_window = dr.window_width / frames_per_element_ + 1;
    uint_fast64_t min_num_elements = (time_tick_diff + dr.window_width) / frames_per_element_ + safe_num_elements_margin_;
    ERS_INFO("Last TS=" << last_ts << " Tickdiff=" << time_tick_diff << " "
          << "ElementOffset=" << num_element_offset << ".th "
          << "ElementsInWindow=" << num_elements_in_window << " "
          << "MinNumElements=" << min_num_elements << " "
          << "Occupancy=" << occupancy_guess
    );
#warning RS FIXME -> replying with empty fragment.
    dataformats::FragmentHeader fraghead;
    fraghead.size = 0;
    fraghead.trigger_number = dr.trigger_number;
    fraghead.trigger_timestamp = dr.trigger_timestamp;
    fraghead.window_offset = dr.window_offset;
    fraghead.window_width = dr.window_width;
    fraghead.run_number = dr.run_number;

    auto frag = std::make_unique<dataformats::Fragment>(frag_pieces);
    frag->set_header(fraghead);
    try {
        fragment_sink_->push( std::move(std::make_unique<dataformats::Fragment>(frag_pieces)) );
    } 
    catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      ers::error(QueueTimeoutError(ERS_HERE, " fragment sink "));
    }    
    return;

/*
    if ( last_ts > start_win_ts || min_num_elements > occupancy_guess ) { // RS FIXME -> timestamp boundary
      ERS_INFO("Out of bound reqested timestamp based on latency buffer occupancy!");

      //auto frag = std::make_unique<dataformats::Fragmentfrag(frag_pieces);
      try {
        fragment_sink_->push( std::move(std::make_unique<dataformats::Fragment>(frag_pieces)) );
      } 
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ers::error(QueueTimeoutError(ERS_HERE, " fragment sink "));
      }
      return;
    } else {
      //auto neededheader = *(reinterpret_cast<const dataformats::WIBHeader*>( (char*)(front_callback_())-(num_element_offset*element_size_) ));
      size_t frag_size = wib_frame_size_ * num_elements_in_window;
      for (unsigned i=0; i<num_elements_in_window; ++i) {
        char* start_ptr = (char*)front_callback_() + (i*element_size)
        frag_pieces.emplace_back( 
          std::make_pair<void*, size_t>( front_callback_()
      }
    }
*/
  }

private:
  const uint_fast64_t tick_dist_ = 25;
  const size_t wib_frame_size_ = 464;
  const uint_fast8_t frames_per_element_ = 12;
  const size_t element_size_ = wib_frame_size_ * frames_per_element_;
  const uint_fast64_t safe_num_elements_margin_ = 10;
};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_WIBREQUESTHANDLER_HPP_
