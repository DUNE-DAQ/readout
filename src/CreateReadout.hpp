/**
* @file CreateRedout.hpp Specific readout creator
* Thanks for Brett and Phil for the idea
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_CREATEREADOUT_HPP_
#define UDAQ_READOUT_SRC_CREATEREADOUT_HPP_

#include "ReadoutTypes.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutConcept.hpp"
#include "ReadoutModel.hpp"

namespace dunedaq {
namespace readout {

std::unique_ptr<ReadoutConcept> 
createReadout(const std::string& rawtype, std::atomic<bool>& run_marker)
{
  if (rawtype == "wib") {
    return std::make_unique<ReadoutModel<types::WIB_SUPERCHUNK_STRUCT>>(rawtype, run_marker);
  }

  return nullptr;
}

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_CREATEREADOUT_HPP_
