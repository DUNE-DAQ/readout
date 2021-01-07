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
                             std::function<types::WIB_SUPERCHUNK_STRUCT*()>& front_callback)
  : DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT>(rawtype, marker, 
      occupancy_callback, read_callback, pop_callback, front_callback)
  {
    ERS_INFO("WIBRequestHandler created...");
    data_request_callback_ = std::bind(&WIBRequestHandler::data_request_handle, this);
  } 

protected:

  void data_request_handle() {
  
  }

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_WIBREQUESTHANDLER_HPP_
