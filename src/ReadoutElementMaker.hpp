/**
* @file LatencyBufferMaker.hpp Glue between raw processor, latency buffer 
* and request handler.
* Thanks for Brett and Phil for the idea
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_LATENCYBUFFERMAKER_HPP_
#define UDAQ_READOUT_SRC_LATENCYBUFFERMAKER_HPP_

#include "ReadoutTypes.hpp"
#include "ReadoutIssues.hpp"
#include "LatencyBufferBase.hpp"
#include "ContinousLatencyBuffer.hpp"
#include "DefaultParserImpl.hpp"

namespace dunedaq {
namespace readout {

std::unique_ptr<LatencyBufferBase> 
LatencyBufferBaseMaker(const std::string& rawtype, int qsize)
{
  if (rawtype == "wib") {
    return std::make_unique<ContinousLatencyBuffer<types::WIB_SUPERCHUNK_STRUCT>>(qsize);
  }

  if (rawtype == "pd") {
    //return std::make_unique<LookupTableLatencyBuffer<types::PD_STRUCT??>>(qsize);
  }

  if (rawtype == "byte") {
    return std::make_unique<ContinousLatencyBuffer<std::vector<uint8_t>>>(qsize);
  }

  //ers::error()
  return nullptr;      
}

/*
std::unique_ptr<RequestHandlerBase>
RequestHandlerBaseMaker(const std::string& rawtype)
{
  //if 
}
*/

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_LATENCYBUFFERMAKER_HPP_
