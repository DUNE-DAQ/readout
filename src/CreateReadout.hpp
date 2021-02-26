/**
* @file CreateRedout.hpp Specific readout creator
* Thanks for Brett and Phil for the idea
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_CREATEREADOUT_HPP_
#define READOUT_SRC_CREATEREADOUT_HPP_

#include "appfwk/cmd/Structs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/app/Nljs.hpp"

#include "readout/ReadoutTypes.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutConcept.hpp"
#include "ReadoutModel.hpp"

#include <utility>
#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

std::unique_ptr<ReadoutConcept> 
createReadout(const nlohmann::json& args, std::atomic<bool>& run_marker)
{
  std::string raw_type_name("");
  auto queues = args.get<appfwk::app::ModInit>().qinfos;
  for (const auto& qi : queues) {
    if (qi.name == "raw_input") {
      auto& inst = qi.inst;

      // IF WIB
      if (inst.find("wib") != std::string::npos) {
        TLOG() << "Creating readout for a wib" ;
        raw_type_name = "wib";
        auto readout_model = std::make_unique<ReadoutModel<types::WIB_SUPERCHUNK_STRUCT>>(run_marker);
        readout_model->init(args, raw_type_name);
        return std::move(readout_model);
      }

      // IF PDS
      if (inst.find("pds") != std::string::npos) {

      }

    }
  }

  return nullptr;
}

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CREATEREADOUT_HPP_
