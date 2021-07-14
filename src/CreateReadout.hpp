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

#include "appfwk/app/Nljs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/cmd/Structs.hpp"

#include "readout/NDReadoutTypes.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"

#include "ReadoutIssues.hpp"
#include "readout/concepts/ReadoutConcept.hpp"
#include "readout/models/ReadoutModel.hpp"

#include "daphne/DAPHNEFrameProcessor.hpp"
#include "daphne/DAPHNEListRequestHandler.hpp"
#include "pacman/PACMANFrameProcessor.hpp"
#include "pacman/PACMANListRequestHandler.hpp"
#include "wib/WIBFrameProcessor.hpp"
#include "wib2/WIB2FrameProcessor.hpp"

#include "readout/models/BinarySearchQueueModel.hpp"
#include "readout/models/DefaultRequestHandlerModel.hpp"
#include "readout/models/FixedRateQueueModel.hpp"

#include <memory>
#include <string>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

std::unique_ptr<ReadoutConcept>
createReadout(const nlohmann::json& args, std::atomic<bool>& run_marker)
{
  auto queues = args.get<appfwk::app::ModInit>().qinfos;
  for (const auto& qi : queues) {
    if (qi.name == "raw_input") {
      auto& inst = qi.inst;

      // IF WIB
      if (inst.find("wib") != std::string::npos && inst.find("wib2") == std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a wib";
        auto readout_model = std::make_unique<ReadoutModel<
          types::WIB_SUPERCHUNK_STRUCT,
          DefaultRequestHandlerModel<types::WIB_SUPERCHUNK_STRUCT, FixedRateQueueModel<types::WIB_SUPERCHUNK_STRUCT>>,
          FixedRateQueueModel<types::WIB_SUPERCHUNK_STRUCT>,
          WIBFrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF WIB2
      if (inst.find("wib2") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a wib2";
        auto readout_model = std::make_unique<ReadoutModel<
          types::WIB2_SUPERCHUNK_STRUCT,
          DefaultRequestHandlerModel<types::WIB2_SUPERCHUNK_STRUCT, FixedRateQueueModel<types::WIB2_SUPERCHUNK_STRUCT>>,
          FixedRateQueueModel<types::WIB2_SUPERCHUNK_STRUCT>,
          WIB2FrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF DAPHNE queue
      if (inst.find("pds_queue") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a pds using Searchable Queue";
        auto readout_model = std::make_unique<
          ReadoutModel<types::DAPHNE_SUPERCHUNK_STRUCT,
                       DefaultRequestHandlerModel<types::DAPHNE_SUPERCHUNK_STRUCT,
                                                  BinarySearchQueueModel<types::DAPHNE_SUPERCHUNK_STRUCT>>,
                       BinarySearchQueueModel<types::DAPHNE_SUPERCHUNK_STRUCT>,
                       DAPHNEFrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF PDS skiplist
      if (inst.find("pds_list") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a pds using SkipList LB";
        auto readout_model = std::make_unique<ReadoutModel<types::DAPHNE_SUPERCHUNK_STRUCT,
                                                           DAPHNEListRequestHandler,
                                                           SkipListLatencyBufferModel<types::DAPHNE_SUPERCHUNK_STRUCT>,
                                                           DAPHNEFrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF ND LAr PACMAN
      if (inst.find("pacman") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a pacman";
        auto readout_model = std::make_unique<ReadoutModel<types::PACMAN_MESSAGE_STRUCT,
                                                           PACMANListRequestHandler,
                                                           SkipListLatencyBufferModel<types::PACMAN_MESSAGE_STRUCT>,
                                                           PACMANFrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF variadic
      if (inst.find("varsize") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a variable size FE";
      }
    }
  }

  return nullptr;
}

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CREATEREADOUT_HPP_
