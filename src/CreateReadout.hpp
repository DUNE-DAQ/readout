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
#include "readout/ReadoutLogging.hpp"

#include "ReadoutIssues.hpp"
#include "ReadoutConcept.hpp"
#include "ReadoutModel.hpp"

#include "ContinousLatencyBufferModel.hpp"
#include "SkipListLatencyBufferModel.hpp"
#include "WIBFrameProcessor.hpp"
#include "WIBRequestHandler.hpp"
#include "WIB2FrameProcessor.hpp"
#include "WIB2RequestHandler.hpp"
#include "PDSFrameProcessor.hpp"
#include "PDSQueueRequestHandler.hpp"
#include "PDSListRequestHandler.hpp"

#include <utility>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

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
      if (inst.find("wib") != std::string::npos && inst.find("wib2") == std::string::npos ) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a wib";
        raw_type_name = "wib";
        auto readout_model = std::make_unique<ReadoutModel<types::WIB_SUPERCHUNK_STRUCT, WIBRequestHandler,
            ContinousLatencyBufferModel<types::WIB_SUPERCHUNK_STRUCT>, WIBFrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF WIB2
      if (inst.find("wib2") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a wib2";
        raw_type_name = "wib2";
        auto readout_model = std::make_unique<ReadoutModel<types::WIB2_SUPERCHUNK_STRUCT, WIB2RequestHandler,
            ContinousLatencyBufferModel<types::WIB2_SUPERCHUNK_STRUCT>, WIB2FrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF PDS queue
      if (inst.find("pds-queue") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a pds using Searchable Queue";
        raw_type_name = "pds";
        auto readout_model = std::make_unique<ReadoutModel<types::PDS_SUPERCHUNK_STRUCT, PDSQueueRequestHandler,
            SearchableLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>, PDSFrameProcessor>>(run_marker);
        readout_model->init(args);
        return std::move(readout_model);
      }

      // IF PDS skiplist
      if (inst.find("pds-list") != std::string::npos) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating readout for a pds using SkipList LB";
        raw_type_name = "pds";
        auto readout_model = std::make_unique<ReadoutModel<types::PDS_SUPERCHUNK_STRUCT, PDSListRequestHandler,
            SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT, uint64_t, types::PDSTimestampGetter>, PDSFrameProcessor>>(run_marker);
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
