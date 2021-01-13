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

#include "dataformats/ComponentRequest.hpp"
#include "dataformats/Fragment.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/DataRequest.hpp"

#include "readout/datalinkhandler/Structs.hpp"

#include "ReadoutIssues.hpp"
#include "CreateRawDataProcessor.hpp"
#include "CreateLatencyBuffer.hpp"
#include "CreateRequestHandler.hpp"
#include "ReusableThread.hpp"

#include <functional>

namespace dunedaq {
namespace readout {

template<class RawType>
class ReadoutModel : public ReadoutConcept {
public:
  explicit ReadoutModel(const nlohmann::json& args, std::atomic<bool>& run_marker)
  : raw_type_name_("")
  , run_marker_(run_marker)
  , fake_trigger_(false)
  , stats_thread_(0)
  , consumer_thread_(0)
  , source_queue_timeout_ms_(0)
  , raw_data_source_(nullptr)
  , latency_buffer_size_(0)
  , latency_buffer_impl_(nullptr)
  , write_callback_(nullptr)
  , read_callback_(nullptr)
  , pop_callback_(nullptr)
  , front_callback_(nullptr)
  , raw_processor_impl_(nullptr)
  , process_callback_(nullptr)
  , requester_thread_(0)
  , timesync_queue_timeout_ms_(0)
  , timesync_thread_(0)
  {
    // Queue specs are in ModInit structs.
    ERS_INFO("Generic ReaoutModel initialized with queue config, but no resets yet.");
    queue_config_ = args.get<appfwk::cmd::ModInit>();
  }

  void conf(const nlohmann::json& args) {
    auto conf = args.get<datalinkhandler::Conf>();
    raw_type_name_ = conf.raw_type;
    fake_trigger_ = false; //conf.fake_trigger;
    latency_buffer_size_ = conf.latency_buffer_size;
    source_queue_timeout_ms_ = std::chrono::milliseconds(conf.source_queue_timeout_ms);
    ERS_INFO("ReadoutModel creation for raw type: " << raw_type_name_); 

    // Reset queues
    ERS_INFO("Resetting queues...");
    for (const auto& qi : queue_config_.qinfos) { 
      try {
        if (qi.name == "raw-input") {
          raw_data_source_.reset(new raw_source_qt(qi.inst));
        } else if (qi.name == "requests") {
          request_source_.reset(new request_source_qt(qi.inst));
        } else if (qi.name == "timesync") {
          timesync_sink_.reset(new timesync_sink_qt(qi.inst));
        } else if (qi.name == "fragments") {
          fragment_sink_.reset(new fragment_sink_qt(qi.inst));
        } else {
          // throw error
        }
      }
      catch (const ers::Issue& excpt) {
        ers::error(ResourceQueueError(ERS_HERE, "ReadoutModel", qi.name, excpt));
      }
    }

    // Instantiate functionalities
    try {
      latency_buffer_impl_ = createLatencyBuffer<RawType>(raw_type_name_, latency_buffer_size_, 
          occupancy_callback_, write_callback_, read_callback_, pop_callback_, front_callback_);
    }
    catch (const std::bad_alloc& be) {
      ers::error(InitializationError(ERS_HERE, "Latency Buffer can't be allocated with size!"));
    }
    if(latency_buffer_impl_.get() == nullptr) {
      ers::error(NoImplementationAvailableError(ERS_HERE, "Latency Buffer", raw_type_name_));
    }

    raw_processor_impl_ = createRawDataProcessor<RawType>(raw_type_name_, process_callback_);
    if(raw_processor_impl_.get() == nullptr) {
      ers::error(NoImplementationAvailableError(ERS_HERE, "Raw Processor", raw_type_name_));
    }

    request_handler_impl_ = createRequestHandler<RawType>(raw_type_name_, run_marker_, 
        occupancy_callback_,  read_callback_, pop_callback_, front_callback_, fragment_sink_);
    if(request_handler_impl_.get() == nullptr) {
      ers::error(NoImplementationAvailableError(ERS_HERE, "Request Handler", raw_type_name_));
    }

    // Configure implementations:
    raw_processor_impl_->conf(args);
    request_handler_impl_->conf(args);

    // Configure threads:
    stats_thread_.set_name("stats", conf.link_number);
    consumer_thread_.set_name("consumer", conf.link_number);
    timesync_thread_.set_name("timesync", conf.link_number);
    requester_thread_.set_name("requests", conf.link_number);
  }

  void start(const nlohmann::json& args) {
    ERS_INFO("Starting threads...");
    request_handler_impl_->start(args);
    stats_thread_.set_work(&ReadoutModel<RawType>::run_stats, this);
    consumer_thread_.set_work(&ReadoutModel<RawType>::run_consume, this);
    requester_thread_.set_work(&ReadoutModel<RawType>::run_requests, this);
    timesync_thread_.set_work(&ReadoutModel<RawType>::run_timesync, this);
  }

  void stop(const nlohmann::json& args) {    
    ERS_INFO("Stoppping threads...");
    request_handler_impl_->stop(args);
    while (!timesync_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));         
    } 
    while (!consumer_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));        
    }
    while (!requester_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));        
    }
    while (!stats_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));         
    }
    ERS_INFO("Flushing latency buffer with occupancy: " << occupancy_callback_());
    pop_callback_(occupancy_callback_()); 
  }

private:

  void run_consume() {
    ERS_INFO("Consumer thread started...");
    while (run_marker_.load()) {
      std::unique_ptr<RawType> payload_ptr;
      try {
        raw_data_source_->pop(payload_ptr, source_queue_timeout_ms_);
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ers::error(QueueTimeoutError(ERS_HERE, " raw source "));
      }
      process_callback_(payload_ptr.get());
      write_callback_(std::move(payload_ptr));
      request_handler_impl_->auto_cleanup_check();
      ++packet_count_;           
    }
    ERS_INFO("Consumer thread joins...");
  }   

  void run_timesync() {
    ERS_INFO("TimeSync thread started...");
    while (run_marker_.load()) {
      try {
        auto timesyncmsg = dfmessages::TimeSync(raw_processor_impl_->get_last_daq_time());
        //ERS_DEBUG(0,"New timesync: daq=" << timesyncmsg.DAQ_time << " wall=" << timesyncmsg.system_time);
        if (timesyncmsg.DAQ_time != 0) {
          timesync_sink_->push(std::move(timesyncmsg));
          if (fake_trigger_) {
            dfmessages::DataRequest dr;
            dr.trigger_timestamp = timesyncmsg.DAQ_time - 500*time::us;
            dr.window_width = 1000;
            dr.window_offset = 100;
            ERS_INFO("Issuing fake trigger based on timesync. "
              << " ts=" << dr.trigger_timestamp
              << " window_width=" << dr.window_width
              << " window_offset=" << dr.window_offset);
            request_handler_impl_->issue_request(dr);
            ++request_count_;
          }
        } else {
          ERS_INFO("Timesync with DAQ time 0 won't be sent out as it's an invalid sync.");
        }
      }
      catch (...) { // RS FIXME
        std::runtime_error("TimeSync queue timed out...");
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ERS_INFO("TimeSync thread joins...");
  } 

  void run_requests() {
    ERS_INFO("Requester thread started...");
    while (run_marker_.load()) {
      dfmessages::DataRequest data_request;
      try {
        request_source_->pop(data_request, source_queue_timeout_ms_);
        request_handler_impl_->issue_request(data_request);
        ++request_count_;
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // not an error, safe to continue
      }
    }
    ERS_INFO("Requester thread joins...");
  }

  void run_stats() {
    // Temporarily, for debugging, a rate checker thread...
    ERS_INFO("Statistics thread started...");
    int new_packets = 0;
    int new_requests = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (run_marker_.load()) {
      auto now = std::chrono::high_resolution_clock::now();
      new_packets = packet_count_.exchange(0);
      new_requests = request_count_.exchange(0);
      double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
      ERS_INFO("Consumed Packet rate: " << new_packets/seconds/1000. << " [kHz] "
        << "Consumed DataRequests: " << new_requests);
      for (int i=0; i<100 && run_marker_.load(); ++i) { // 100 x 100ms = 10s sleeps
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      t0 = now;
    }
    ERS_INFO("Statistics thread joins...");
  }

  // Constuctor params
  std::string raw_type_name_;
  std::atomic<bool>& run_marker_;

  // CONFIGURATION
  appfwk::cmd::ModInit queue_config_;
  bool fake_trigger_;

  // STATS
  stats::counter_t packet_count_{0};
  stats::counter_t request_count_{0};
  ReusableThread stats_thread_;

  // CONSUMER
  ReusableThread consumer_thread_;

  // RAW SOURCE
  std::chrono::milliseconds source_queue_timeout_ms_;
  using raw_source_qt = appfwk::DAQSource<std::unique_ptr<RawType>>;
  std::unique_ptr<raw_source_qt> raw_data_source_;

  // REQUEST SOURCE
  std::chrono::milliseconds request_queue_timeout_ms_;
  using request_source_qt = appfwk::DAQSource<dfmessages::DataRequest>;
  std::unique_ptr<request_source_qt> request_source_; 

  // FRAGMENT SINK
  std::chrono::milliseconds fragment_queue_timeout_ms_;
  using fragment_sink_qt = appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>;
  std::unique_ptr<fragment_sink_qt> fragment_sink_;

  // LATENCY BUFFER:
  size_t latency_buffer_size_;
  std::unique_ptr<LatencyBufferConcept> latency_buffer_impl_;
  std::function<size_t()> occupancy_callback_;
  std::function<void(std::unique_ptr<RawType>)> write_callback_;
  std::function<bool(RawType&)> read_callback_;
  std::function<void(unsigned)> pop_callback_;
  std::function<RawType*(unsigned)> front_callback_;

  // RAW PROCESSING:
  std::unique_ptr<RawDataProcessorConcept> raw_processor_impl_;
  std::function<void(RawType*)> process_callback_;

  // REQUEST HANDLER:
  std::unique_ptr<RequestHandlerConcept> request_handler_impl_;
  ReusableThread requester_thread_;

  // TIME-SYNC
  std::chrono::milliseconds timesync_queue_timeout_ms_;
  using timesync_sink_qt = appfwk::DAQSink<dfmessages::TimeSync>;
  std::unique_ptr<timesync_sink_qt> timesync_sink_;
  ReusableThread timesync_thread_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTMODEL_HPP_
