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

#include "readout/models/SourceEmulatorModel.hpp"
#include "readout/models/TPEmulatorModel.hpp"

#include <memory>
#include <string>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

//! Values suitable to emulation

static constexpr int daphne_time_tick_diff = 16;
static constexpr double daphne_dropout_rate = 0.9;
static constexpr double daphne_rate_khz = 200.0;

static constexpr int wib_time_tick_diff = 25;
static constexpr double wib_dropout_rate = 0.0;
static constexpr double wib_rate_khz = 166.0;

static constexpr int wib2_time_tick_diff = 32;
static constexpr double wib2_dropout_rate = 0.0;
static constexpr double wib2_rate_khz = 166.0;

std::unique_ptr<SourceEmulatorConcept>
createSourceEmulator(const appfwk::app::QueueInfo qi, std::atomic<bool>& run_marker)
{
  auto& inst = qi.inst;

  // IF WIB2
  if (inst.find("wib2") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake wib2 link";

    auto source_emu_model = std::make_unique<SourceEmulatorModel<types::WIB2_SUPERCHUNK_STRUCT>>(
      qi.name, run_marker, wib2_time_tick_diff, wib2_dropout_rate, wib2_rate_khz);
    return std::move(source_emu_model);
  }

  // IF WIB
  if (inst.find("wib") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake wib link";
    auto source_emu_model = std::make_unique<SourceEmulatorModel<types::WIB_SUPERCHUNK_STRUCT>>(
      qi.name, run_marker, wib_time_tick_diff, wib_dropout_rate, wib_rate_khz);
    return std::move(source_emu_model);
  }

  // IF PDS
  if (inst.find("pds") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake pds link";
    auto source_emu_model = std::make_unique<SourceEmulatorModel<types::DAPHNE_SUPERCHUNK_STRUCT>>(
      qi.name, run_marker, daphne_time_tick_diff, daphne_dropout_rate, daphne_rate_khz);
    return std::move(source_emu_model);
  }

  // TP link
  if (inst.find("tp") != std::string::npos) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating fake tp link";
    auto source_emu_model = std::make_unique<TPEmulatorModel>(run_marker, 200.0);
    return std::move(source_emu_model);
  }

  return nullptr;
}

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CREATESOURCEEMULATOR_HPP_
