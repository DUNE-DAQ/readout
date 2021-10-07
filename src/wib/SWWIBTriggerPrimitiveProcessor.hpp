/**
 * @file WIBTriggerPrimitiveProcessor.hpp WIB TP specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_SWWIBTRIGGERPRIMITIVEPROCESSOR_HPP_
#define READOUT_SRC_WIB_WIBTRIGGERPRIMITIVEPROCESSOR_HPP_

#include "appfwk/DAQModuleHelper.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "trigger/TPSet.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class SWWIBTriggerPrimitiveProcessor : public TaskRawDataProcessorModel<types::SW_WIB_TRIGGERPRIMITIVE_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::SW_WIB_TRIGGERPRIMITIVE_STRUCT>;
  using frameptr = types::SW_WIB_TRIGGERPRIMITIVE_STRUCT*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  explicit SWWIBTriggerPrimitiveProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::SW_WIB_TRIGGERPRIMITIVE_STRUCT>(error_registry)
  {}

private:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB_SWWIBTRIGGERPRIMITIVEPROCESSOR_HPP_
