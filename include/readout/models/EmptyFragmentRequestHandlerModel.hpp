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

      explicit EmptyFragmentRequestHandlerModel(
          std::unique_ptr<LatencyBufferType>& latency_buffer,
          std::unique_ptr<FrameErrorRegistry>& error_registry)
          : DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>(latency_buffer, error_registry)
      {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "EmptyFragmentRequestHandlerModel created...";
      }

      using RequestResult = typename dunedaq::readout::RequestHandlerConcept<ReadoutType, LatencyBufferType>::RequestResult;
      using ResultCode = typename dunedaq::readout::RequestHandlerConcept<ReadoutType, LatencyBufferType>::ResultCode;

      void issue_request(dfmessages::DataRequest datarequest, appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>& fragment_queue) override
      {
        auto frag_header = inherited::create_fragment_header(datarequest);
        frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
        auto fragment = std::make_unique<dataformats::Fragment>(std::vector<std::pair<void*, size_t>>());
        fragment->set_header_fields(frag_header);

        //ers::warning(dunedaq::readout::TrmWithEmptyFragment(ERS_HERE, "DLH is configured to send empty fragment"));
        TLOG_DEBUG(TLVL_WORK_STEPS) << "DLH is configured to send empty fragment";
        
        try { // Push to Fragment queue
          TLOG_DEBUG(TLVL_QUEUE_PUSH) << "Sending fragment with trigger_number "
                                      << fragment->get_trigger_number() << ", run number "
                                      << fragment->get_run_number() << ", and GeoID "
                                      << fragment->get_element_id();
          fragment_queue.push(std::move(fragment));
        } catch (const ers::Issue& excpt) {
          std::ostringstream oss;
          oss << "fragments output queue for link " << inherited::m_geoid.element_id;
          ers::warning(CannotWriteToQueue(ERS_HERE, oss.str(), excpt));
        }
      }
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_EMPTYFRAGMENTREQUESTHANDLERMODEL_HPP_
