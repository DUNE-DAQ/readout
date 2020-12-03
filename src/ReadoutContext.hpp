/**
* @file ReadoutContext.hpp Glue between data source, payload raw processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTCONTEXT_HPP_
#define UDAQ_READOUT_SRC_READOUTCONTEXT_HPP_

#include "ReadoutIssues.hpp"
#include "RawProcessorMaker.hpp"
#include "LatencyBufferMaker.hpp"
#include "ReusableThread.hpp"
//#include "RequestHandler.hpp"

#include <functional>

namespace dunedaq {
namespace readout {

template<class RawType>
class ReadoutContext : public ReadoutContextBase {
public:

  explicit ReadoutContext(const std::string& rawtype, std::atomic<bool>& run_marker)
  : raw_type_name_(rawtype)
  , run_marker_(run_marker)
  , process_callback_(nullptr)
  , write_callback_(nullptr)
  {
    ERS_INFO("ReadoutContext created for raw type: " << rawtype);
  }

  void conf(const nlohmann::json& cfg) {
    raw_processor_impl_ = RawProcessorMaker<RawType>(raw_type_name_, process_callback_);
    raw_processor_impl_->conf(cfg);
    latency_buffer_impl_ = LatencyBufferMaker<RawType>(raw_type_name_, 100000, write_callback_);
    if(latency_buffer_impl_.get() == nullptr) {
      throw std::runtime_error("No Implementation available for raw type");
    }
  }

  void start(const nlohmann::json& args) {
    while(run_marker_.load()) {
      types::WIB_SUPERCHUNK_STRUCT payload;
      ERS_INFO("Try processing...");
      process_callback_( &payload );
      ERS_INFO("Try writing...");
      write_callback_( std::move(payload) );
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void stop(const nlohmann::json& args) {

  }


private:
  std::string raw_type_name_;
  std::atomic<bool>& run_marker_;

  // PRODUCER:
  //ReusableThread producer_;
  //std::function<void()> producer_;

  // RAW PROCESSING:
  std::unique_ptr<RawProcessorBase> raw_processor_impl_;
  std::function<void(RawType*)> process_callback_;

  // LATENCY BUFFER:
  std::unique_ptr<LatencyBufferBase> latency_buffer_impl_;
  std::function<void(RawType&&)> write_callback_;

  // Consumers: Request handlers
  //std::unique_ptr<RequestHandlerBase> request_handler_impl_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTCONTEXT_HPP_
