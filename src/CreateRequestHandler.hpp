/**
* @file CreateRequestHandler.hpp Implementation of latency buffer creator function
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_CREATEREQUESTHANDLER_HPP_
#define UDAQ_READOUT_SRC_CREATEREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "RequestHandlerConcept.hpp"
#include "DefaultRequestHandlerModel.hpp"

namespace dunedaq {
namespace readout {

template <class RawType>
std::unique_ptr<RequestHandlerConcept> 
createRequestHandler(const std::string& rawtype,
                     std::atomic<bool>& run_marker,
                     std::function<size_t()>& occupancy_callback,
                     std::function<bool(RawType&)>& read_callback,
                     std::function<void(unsigned)>& pop_callback,
                     std::function<RawType*()>& front_callback)
{
  if (rawtype == "wib") {
    return std::make_unique<DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT>>(rawtype, run_marker,
      occupancy_callback, read_callback, pop_callback, front_callback);
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

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_CREATEREQUESTHANDLER_HPP_
