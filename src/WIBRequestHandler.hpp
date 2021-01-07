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
    uint_fast64_t start_win_ts = dr.trigger_timestamp - (uint_fast64_t)(dr.window_offset * tick_dist_);
    dataformats::WIBHeader front_wh = *(reinterpret_cast<const dataformats::WIBHeader*>( front_callback_() ));
    uint_fast64_t last_ts = front_wh.timestamp();
    if ( last_ts > start_win_ts ) { // RS FIXME -> Add proper check for timestamp boundary
      ERS_INFO("Reqested data is not present in latency buffer.");
    } 
  }

private:
  const uint_fast64_t tick_dist_=25;

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_WIBREQUESTHANDLER_HPP_
