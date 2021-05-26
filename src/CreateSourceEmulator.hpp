/**
 * @file CreateSourceEmulator.hpp Specific source emulator creator.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_CREATESOURCEEMULATOR_HPP_
#define READOUT_SRC_CREATESOURCEEMULATOR_HPP_

#include "appfwk/app/Nljs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/cmd/Structs.hpp"

#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"

#include "SourceEmulatorModel.hpp"

#include <memory>
#include <string>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

std::unique_ptr<SourceEmulatorConcept>
createSourceEmulator(const appfwk::app::QueueInfo qi, std::atomic<bool>& run_marker)
{
  auto& inst = qi.inst;

  // IF WIB2
  if (inst.find("wib2") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake wib2 link";
    auto source_emu_model =
      std::make_unique<SourceEmulatorModel<types::WIB2_SUPERCHUNK_STRUCT>>(qi.name, run_marker, 32, 0.0, 166.0);
    return std::move(source_emu_model);
  }

  // IF WIB
  if (inst.find("wib") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake wib link";
    auto source_emu_model =
      std::make_unique<SourceEmulatorModel<types::WIB_SUPERCHUNK_STRUCT>>(qi.name, run_marker, 25, 0.0, 166.0);
    return std::move(source_emu_model);
  }

  // IF PDS
  if (inst.find("pds") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake pds link";
    auto source_emu_model =
      std::make_unique<SourceEmulatorModel<types::PDS_SUPERCHUNK_STRUCT>>(qi.name, run_marker, 16, 0.9, 200.0);
    return std::move(source_emu_model);
  }

  return nullptr;
}

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CREATESOURCEEMULATOR_HPP_
