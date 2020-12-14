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

#include "appfwk/cmd/Structs.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "readout/datalinkhandler/Structs.hpp"

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
  explicit ReadoutModel(const nlohmann::json& args, std::atomic<bool>& run_marker)
  : raw_type_name_("")
  , stats_thread_(0)
  , consumer_thread_(0)
  , source_queue_timeout_ms_(0)
  , run_marker_(run_marker)
  , raw_data_source_(nullptr)
  , raw_processor_impl_(nullptr)
  , process_callback_(nullptr)
  , latency_buffer_size_(0)
  , latency_buffer_impl_(nullptr)
  , write_callback_(nullptr)
  {
    // Queue specs are in ModInit structs.
    ERS_INFO("Generic ReaoutModel initialized with queue config, but no resets yet.");
    queue_config_ = args.get<appfwk::cmd::ModInit>();
  }

  void conf(const nlohmann::json& args) {
    auto conf = args.get<datalinkhandler::Conf>();
    raw_type_name_ = conf.raw_type;
    latency_buffer_size_ = conf.latency_buffer_size;
    source_queue_timeout_ms_ = std::chrono::milliseconds(conf.source_queue_timeout_ms);
    ERS_INFO("ReadoutModel creation for raw type: " << raw_type_name_); 

    // Reset queues
    for (const auto& qi : queue_config_.qinfos) { 
#warning RS -> also set up sinks for requesthandler
      if (qi.dir != "input") { // Wrong logic, need to setup also OUT queues.
        continue;
      }
      ERS_INFO("Resetting queue: " << qi.inst);
      try {
        raw_data_source_.reset(new source_t(qi.inst));
      }
      catch (const ers::Issue& excpt) {
        ers::error(ResourceQueueError(ERS_HERE, "ReadoutModel", qi.name, excpt));
      }
    }

    // Instantiate functionalities
    raw_processor_impl_ = createRawDataProcessor<RawType>(raw_type_name_, process_callback_);
    latency_buffer_impl_ = createLatencyBuffer<RawType>(raw_type_name_, latency_buffer_size_, write_callback_);
    if( latency_buffer_impl_.get() == nullptr) {
      ers::error(NoImplementationAvailableError(ERS_HERE, "Latency Buffer", raw_type_name_));
    }
    if(raw_processor_impl_.get() == nullptr) {
      ers::error(NoImplementationAvailableError(ERS_HERE, "Raw Processor", raw_type_name_));
    }

    // Configure implementations:
    raw_processor_impl_->conf(args);

    // Configure threads:
    stats_thread_.set_name("stats", 0);
    consumer_thread_.set_name("consumer", 0);
  }

  void start(const nlohmann::json& args) {
    ERS_INFO("Starting threads...");
    stats_thread_.set_work(&ReadoutModel<RawType>::run_stats, this);
    consumer_thread_.set_work(&ReadoutModel<RawType>::consume, this);
  }

  void stop(const nlohmann::json& args) {    
    ERS_INFO("Stoppping threads...");
    while (!consumer_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));        
    }
    while (!stats_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));         
    }
#warning RS FIXME -> Handle leftover items in queue?
    /* When and where to flush? Producer/Consumer should have the "logic"? */
  }

private:

  void consume() {
    ERS_INFO("Consumer thread started...");
    while (run_marker_.load()) {
      std::unique_ptr<RawType> payload_ptr;
      bool cp = raw_data_source_->can_pop();
      try {
        raw_data_source_->pop(payload_ptr, source_queue_timeout_ms_);
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        std::runtime_error("Queue Source timed out...");
      }
      process_callback_(payload_ptr.get());
      write_callback_(std::move(payload_ptr));;
      ++packet_count_;                   
    }
    ERS_INFO("Consumer thread stopped...");
  }   

  void run_stats() {
    // Temporarily, for debugging, a rate checker thread...
    ERS_INFO("Statistics thread started...");
    int new_packets = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (run_marker_.load()) {
      auto now = std::chrono::high_resolution_clock::now();
      new_packets = packet_count_.exchange(0);
      double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
      ERS_INFO("Packet rate: " << new_packets/seconds/1000. << " [kHz]");
      std::this_thread::sleep_for(std::chrono::seconds(2));
      t0 = now;
    }
    ERS_INFO("Statistics thread stopped...");
  } 

  // Constuctor params
  std::string raw_type_name_;
  std::atomic<bool>& run_marker_;

  // CONFIGURATION
  appfwk::cmd::ModInit queue_config_;

  // STATS
  stats::counter_t packet_count_;
  ReusableThread stats_thread_;

  // CONSUMER
  ReusableThread consumer_thread_;

  // SOURCE
  std::chrono::milliseconds source_queue_timeout_ms_;
  using source_t = appfwk::DAQSource<std::unique_ptr<RawType>>;
  std::unique_ptr<source_t> raw_data_source_;

  // RAW PROCESSING:
  std::unique_ptr<RawDataProcessorConcept> raw_processor_impl_;
  std::function<void(RawType*)> process_callback_;

  // LATENCY BUFFER:
  size_t latency_buffer_size_;
  std::unique_ptr<LatencyBufferConcept> latency_buffer_impl_;
  std::function<void(std::unique_ptr<RawType>)> write_callback_;

  // Consumers: Request handlers
  //std::unique_ptr<RequestHandlerBase> request_handler_impl_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTMODEL_HPP_
