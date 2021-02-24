/**
* @file CreateLatencyBuffer.hpp Implementation of latency buffer creator function
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_CREATELATENCYBUFFER_HPP_
#define READOUT_SRC_CREATELATENCYBUFFER_HPP_

#include "ReadoutIssues.hpp"
#include "LatencyBufferConcept.hpp"
#include "ContinousLatencyBufferModel.hpp"

#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

template <class RawType>
std::unique_ptr<LatencyBufferConcept> 
createLatencyBuffer(const std::string& rawtype, int qsize,
                    std::function<size_t()>& occupancy_override,
                    std::function<void(std::unique_ptr<RawType>)>& write_override,
                    std::function<bool(RawType&)>& read_override,
                    std::function<void(unsigned)>& pop_override,       // NOLINT
                    std::function<RawType*(unsigned)>& front_override) // NOLINT
{
  if (rawtype == "wib") {
    return std::make_unique<ContinousLatencyBufferModel<RawType>>(qsize, 
        occupancy_override, write_override, read_override, pop_override, front_override);
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

#endif // READOUT_SRC_CREATELATENCYBUFFER_HPP_
