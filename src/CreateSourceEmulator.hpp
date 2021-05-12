/**
* @file CreateSourceEmulator.hpp Specific source emulator creator.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_CREATESOURCEEMULATOR_HPP_
#define READOUT_SRC_CREATESOURCEEMULATOR_HPP_

#include "appfwk/cmd/Structs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/app/Nljs.hpp"

#include "readout/ReadoutTypes.hpp"
#include "readout/ReadoutLogging.hpp"

#include "SourceEmulatorModel.hpp"

#include <utility>
#include <string>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

std::unique_ptr<SourceEmulatorConcept> 
createSourceEmulator(const nlohmann::json& args, std::atomic<bool>& run_marker)
{
  std::string raw_type_name("");
  auto queues = args.get<appfwk::app::ModInit>().qinfos;
  for (const auto& qi : queues) {
    if (qi.name == "raw_output") {
      auto& inst = qi.inst;

      // IF WIB
      if (inst.find("wib") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a wib" ;
        raw_type_name = "wib";
        auto source_emu_model = std::make_unique<SourceEmulatorModel<types::WIB_SUPERCHUNK_STRUCT>>(run_marker);
        source_emu_model->init(args);
        return std::move(source_emu_model);
      }

      // IF WIB2
      if (inst.find("wib2") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a wib2" ;
        raw_type_name = "wib2";
        auto source_emu_model = std::make_unique<SourceEmulatorModel<types::WIB2_SUPERCHUNK_STRUCT>>(run_marker);
        source_emu_model->init(args);
        return std::move(source_emu_model);
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

#endif // READOUT_SRC_CREATESOURCEEMULATOR_HPP_
