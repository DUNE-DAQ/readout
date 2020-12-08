/**
* @file ReadoutModel.hpp Glue between data source, payload raw processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTMODEL_HPP_
#define UDAQ_READOUT_SRC_READOUTMODEL_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "ReadoutIssues.hpp"
#include "CreateRawDataProcessor.hpp"
#include "CreateLatencyBuffer.hpp"
#include "ReusableThread.hpp"
//#include "RequestHandler.hpp"

#include <functional>

namespace dunedaq {
namespace readout {

template<class RawType>
class ReadoutModel : public ReadoutConcept {
public:
  //using RawPtrSource = appfwk::DAQSource<std::unique_ptr<RawType>>;
  //using UniqueRawPtrSource = std::unique_ptr<RawPtrSource>;

  explicit ReadoutModel(const std::string& rawtype, std::atomic<bool>& run_marker) //UniqueRawPtrSource& source)
  : raw_type_name_(rawtype)
  , run_marker_(run_marker)
  //, raw_source(source)
  , process_callback_(nullptr)
  , write_callback_(nullptr)
  {
    ERS_INFO("ReadoutModel created for raw type: " << rawtype);
  }

  void conf(const nlohmann::json& cfg) {
    raw_processor_impl_ = createRawDataProcessor<RawType>(raw_type_name_, process_callback_);
    raw_processor_impl_->conf(cfg);
    latency_buffer_impl_ = createLatencyBuffer<RawType>(raw_type_name_, 100000, write_callback_);
    if(latency_buffer_impl_.get() == nullptr) {
      throw std::runtime_error("No Implementation available for raw type");
    }
  }

  void start(const nlohmann::json& args) {
    /*while(run_marker_.load()) {
      RawType payload;
      ERS_INFO("Try processing...");
      process_callback_( &payload );
      ERS_INFO("Try writing...");
      write_callback_( std::move(payload) );
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }*/
  }

  void stop(const nlohmann::json& args) {

  }

  using RawPtr = std::unique_ptr<RawType>;
  void handle(RawPtr rawptr) {
    process_callback_(rawptr.get());
    write_callback_(std::move(rawptr));
  }

private:
  std::string raw_type_name_;
  std::atomic<bool>& run_marker_;

  // SOURCE
  //UniqueRawPtrSource& raw_source_;

  // PRODUCER:
  //ReusableThread producer_;
  //std::function<void()> producer_;

  // RAW PROCESSING:
  std::unique_ptr<RawDataProcessorConcept> raw_processor_impl_;
  std::function<void(RawType*)> process_callback_;

  // LATENCY BUFFER:
  std::unique_ptr<LatencyBufferConcept> latency_buffer_impl_;
  std::function<void(RawPtr)> write_callback_;

  // Consumers: Request handlers
  //std::unique_ptr<RequestHandlerBase> request_handler_impl_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTMODEL_HPP_
