/**
* @file CreateLatencyBuffer.hpp Implementation of latency buffer creator function
* Thanks for Brett and Phil for the idea
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_CREATELATENCYBUFFER_HPP_
#define UDAQ_READOUT_SRC_CREATELATENCYBUFFER_HPP_

#include "ReadoutIssues.hpp"
#include "LatencyBufferConcept.hpp"
#include "ContinousLatencyBufferModel.hpp"

namespace dunedaq {
namespace readout {

template <class RawType>
std::unique_ptr<LatencyBufferConcept> 
createLatencyBuffer(const std::string& rawtype, int qsize, std::function<void(std::unique_ptr<RawType>)>& write_override) 
{
  if (rawtype == "wib") {
    return std::make_unique<ContinousLatencyBufferModel<RawType>>(qsize, write_override);
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

#endif // UDAQ_READOUT_SRC_CREATELATENCYBUFFER_HPP_
