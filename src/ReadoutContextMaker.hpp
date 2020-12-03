/**
* @file LatencyBufferMaker.hpp Specific latency buffer and request
* handler maker functions.
* Thanks for Brett and Phil for the idea
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTCONTEXTBASEMAKER_HPP_
#define UDAQ_READOUT_SRC_READOUTCONTEXTBASEMAKER_HPP_

#include "ReadoutTypes.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutContextBase.hpp"
#include "ReadoutContext.hpp"

namespace dunedaq {
namespace readout {

std::unique_ptr<ReadoutContextBase> 
ReadoutContextMaker(const std::string& rawtype, std::atomic<bool>& run_marker)
{
  if (rawtype == "wib") {
    return std::make_unique<ReadoutContext<types::WIB_SUPERCHUNK_STRUCT>>(rawtype, run_marker);
  }

  return nullptr;
}

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_READOUTCONTEXTBASEMAKER_HPP_
