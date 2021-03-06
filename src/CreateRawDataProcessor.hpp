/**
* @file RawProcessorMaker.hpp Specific RawProcessor implementation maker
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_CREATERAWDATAPROCESSOR_HPP_
#define READOUT_SRC_CREATERAWDATAPROCESSOR_HPP_

#include "readout/ReadoutTypes.hpp"
#include "RawDataProcessorConcept.hpp"
#include "FlowGraphRawDataProcessorModel.hpp"
#include "WIBFrameProcessor.hpp"

#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

template <class RawType>
std::unique_ptr<RawDataProcessorConcept> 
createRawDataProcessor(const std::string& rawtype, std::function<void(RawType*)>& process_function)
{
  if (rawtype == "wib") {
    return std::make_unique<WIBFrameProcessor>(rawtype, process_function);
  }

  return nullptr;
}

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CREATERAWDATAPROCESSOR_HPP_
