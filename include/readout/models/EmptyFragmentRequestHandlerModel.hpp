/**
 * @file EmptyFragmentRequestHandlerModel.hpp Request handler that always returns empty fragments
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_EMPTYFRAGMENTREQUESTHANDLERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_EMPTYFRAGMENTREQUESTHANDLERMODEL_HPP_

#include "readout/models/DefaultRequestHandlerModel.hpp"

#include <memory>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_HOUSEKEEPING;
using dunedaq::readout::logging::TLVL_QUEUE_PUSH;
using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class ReadoutType, class LatencyBufferType>
class EmptyFragmentRequestHandlerModel : public DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>
{
public:
  using inherited = DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>;

  explicit EmptyFragmentRequestHandlerModel(std::unique_ptr<LatencyBufferType>& latency_buffer,
                                            std::unique_ptr<FrameErrorRegistry>& error_registry)
    : DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>(latency_buffer, error_registry)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "EmptyFragmentRequestHandlerModel created...";
  }

  using RequestResult = typename dunedaq::readout::RequestHandlerConcept<ReadoutType, LatencyBufferType>::RequestResult;
  using ResultCode = typename dunedaq::readout::RequestHandlerConcept<ReadoutType, LatencyBufferType>::ResultCode;

  void issue_request(dfmessages::DataRequest datarequest) override
  {
    auto frag_header = inherited::create_fragment_header(datarequest);
    frag_header.error_bits |= (0x1 << static_cast<size_t>(daqdataformats::FragmentErrorBits::kDataNotFound));
    auto fragment = std::make_unique<daqdataformats::Fragment>(std::vector<std::pair<void*, size_t>>());
    fragment->set_header_fields(frag_header);

    // ers::warning(dunedaq::readout::TrmWithEmptyFragment(ERS_HERE, "DLH is configured to send empty fragment"));
    TLOG_DEBUG(TLVL_WORK_STEPS) << "DLH is configured to send empty fragment";

    try {
      auto serialised_frag = dunedaq::serialization::serialize(std::move(fragment), dunedaq::serialization::kMsgPack);
      networkmanager::NetworkManager::get().send_to(datarequest.data_destination,
                                                    static_cast<const void*>(serialised_frag.data()),
                                                    serialised_frag.size(),
                                                    std::chrono::milliseconds(1000));
    } catch (ers::Issue& e) {
      ers::warning(
        FragmentTransmissionFailed(ERS_HERE, datarequest.request_information.component, datarequest.trigger_number, e));
    }

  }
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_EMPTYFRAGMENTREQUESTHANDLERMODEL_HPP_
