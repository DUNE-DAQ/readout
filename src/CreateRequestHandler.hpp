/**
* @file CreateRequestHandler.hpp Implementation of latency buffer creator function
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_CREATEREQUESTHANDLER_HPP_
#define READOUT_SRC_CREATEREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "RequestHandlerConcept.hpp"
#include "WIBRequestHandler.hpp"

#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

template <class RawType>
std::unique_ptr<RequestHandlerConcept> 
createRequestHandler(const std::string& rawtype,
                     std::atomic<bool>& run_marker,
                     std::function<size_t()>& occupancy_callback,
                     std::function<bool(RawType&)>& read_callback,
                     std::function<void(unsigned)>& pop_callback,       // NOLINT
                     std::function<RawType*(unsigned)>& front_callback, // NOLINT
                     std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                     std::unique_ptr<appfwk::DAQSink<RawType>>& snb_sink)
{
  if (rawtype == "wib") {
    return std::make_unique<WIBRequestHandler>(rawtype, run_marker,
      occupancy_callback, read_callback, pop_callback, front_callback, fragment_sink, snb_sink);
  }

  if (rawtype == "pd") {
    //return std::make_unique<LookupTableLatencyBuffer<types::PD_STRUCT>>(qsize); // example
  }

  if (rawtype == "byte") {
    //return std::make_unique<VectorLatencyBuffer<std::vector<uint8_t>>>(qsize, write_override); // example
  }
  //ers::error()
  return nullptr;      
}

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CREATEREQUESTHANDLER_HPP_
