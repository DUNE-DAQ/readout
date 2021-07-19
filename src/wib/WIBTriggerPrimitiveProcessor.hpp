/**
 * @file WIBTriggerPrimitiveProcessor.hpp WIB TP specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_WIBTRIGGERPRIMITIVEPROCESSOR_HPP_
#define READOUT_SRC_WIB_WIBTRIGGERPRIMITIVEPROCESSOR_HPP_

#include "appfwk/DAQModuleHelper.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"
#include "trigger/TPSet.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class WIBTriggerPrimitiveProcessor : public TaskRawDataProcessorModel<types::TP_READOUT_TYPE>
{

public:
  using inherited = TaskRawDataProcessorModel<types::TP_READOUT_TYPE>;
  using frameptr = types::TP_READOUT_TYPE*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  explicit WIBTriggerPrimitiveProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::TP_READOUT_TYPE>(error_registry)
  {}

  void init(const nlohmann::json& args) override
  {
    try {
      auto queue_index = appfwk::queue_index(args, {});
      if (queue_index.find("tpset_out") != queue_index.end()) {
        m_tpset_sink.reset(new appfwk::DAQSink<trigger::TPSet>(queue_index["tpset_out"].inst));
      }
    } catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, "DefaultRequestHandlerModel", "tpset_out", excpt);
    }
  }

private:
  std::unique_ptr<appfwk::DAQSink<trigger::TPSet>> m_tpset_sink;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB_WIBTRIGGERPRIMITIVEPROCESSOR_HPP_
