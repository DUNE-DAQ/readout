/**
* @file ReadoutModel.hpp Glue between data source, payload raw processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_READOUTMODEL_HPP_
#define READOUT_SRC_READOUTMODEL_HPP_

#include "appfwk/cmd/Structs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/app/Nljs.hpp"

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "logging/Logging.hpp"

#include "opmonlib/InfoCollector.hpp"

#include "dataformats/ComponentRequest.hpp"
#include "dataformats/Fragment.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/DataRequest.hpp"

#include "readout/datalinkhandler/Structs.hpp"
#include "readout/datalinkhandlerinfo/Nljs.hpp"
#include "readout/ReadoutLogging.hpp"

#include "LatencyBufferConcept.hpp"
#include "RequestHandlerConcept.hpp"
#include "RawDataProcessorConcept.hpp"

#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "readout/ReusableThread.hpp"

#include <functional>
#include <utility>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_WORK_STEPS;
using dunedaq::readout::logging::TLVL_TAKE_NOTE;

namespace dunedaq {
namespace readout {

template<class RawType, class RequestHandlerType, class LatencyBufferType, class RawDataProcessorType>
class ReadoutModel : public ReadoutConcept {
public:
  explicit ReadoutModel(std::atomic<bool>& run_marker)
  : m_run_marker(run_marker)
  , m_fake_trigger(false)
  , m_stats_thread(0)
  , m_consumer_thread(0)
  , m_source_queue_timeout_ms(0)
  , m_raw_data_source(nullptr)
  , m_latency_buffer_size(0)
  , m_latency_buffer_impl(nullptr)
  , m_raw_processor_impl(nullptr)
  , m_requester_thread(0)
  , m_timesync_queue_timeout_ms(0)
  , m_timesync_thread(0)
  { }

  void init(const nlohmann::json& args) {
    m_queue_config = args.get<appfwk::app::ModInit>();
    // Reset queues
    for (const auto& qi : m_queue_config.qinfos) { 
      try {
        if (qi.name == "raw_input") {
          m_raw_data_source.reset(new raw_source_qt(qi.inst));
        } else if (qi.name == "requests") {
          m_request_source.reset(new request_source_qt(qi.inst));
        } else if (qi.name == "timesync") {
          m_timesync_sink.reset(new timesync_sink_qt(qi.inst));
        } else if (qi.name == "fragments") {
          m_fragment_sink.reset(new fragment_sink_qt(qi.inst));
        } else if (qi.name == "snb") {
          m_snb_sink.reset(new snb_sink_qt(qi.inst));
        } else {
          // throw error
          ers::error(ResourceQueueError(ERS_HERE, "Unknown queue requested!", qi.name, ""));
        }
      }
      catch (const ers::Issue& excpt) {
        ers::error(ResourceQueueError(ERS_HERE, "ReadoutModel", qi.name, excpt));
      }
    }

    // Instantiate functionalities
    m_latency_buffer_impl.reset(new LatencyBufferType());
    m_raw_processor_impl.reset(new RawDataProcessorType());

    m_request_handler_impl.reset(new RequestHandlerType());
    m_request_handler_impl->initialize(m_latency_buffer_impl, m_fragment_sink, m_snb_sink);
  }

  void conf(const nlohmann::json& args) {
    auto conf = args.get<datalinkhandler::Conf>();
    if (conf.fake_trigger_flag == 0) {
      m_fake_trigger = false;
    } else {
      m_fake_trigger = true;
   }
    m_latency_buffer_size = conf.latency_buffer_size;
    m_source_queue_timeout_ms = std::chrono::milliseconds(conf.source_queue_timeout_ms);
    TLOG_DEBUG(TLVL_WORK_STEPS) << "ReadoutModel creation";

    // Configure implementations:
    m_raw_processor_impl->conf(args);
    //m_raw_processor_impl->set_emulator_mode(conf.emulator_mode);
    m_request_handler_impl->conf(args);
    try {
      m_latency_buffer_impl->conf(args);
    } catch (const std::bad_alloc& be) {
      ers::error(InitializationError(ERS_HERE, "Latency Buffer can't be allocated with size!"));
    }


    // Configure threads:
    m_stats_thread.set_name("stats", conf.link_number);
    m_consumer_thread.set_name("consumer", conf.link_number);
    m_timesync_thread.set_name("timesync", conf.link_number);
    m_requester_thread.set_name("requests", conf.link_number);
  }

  void start(const nlohmann::json& args) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Starting threads...";
    m_request_handler_impl->start(args);
    m_stats_thread.set_work(&ReadoutModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_stats, this);
    m_consumer_thread.set_work(&ReadoutModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_consume, this);
    m_requester_thread.set_work(&ReadoutModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_requests, this);
    m_timesync_thread.set_work(&ReadoutModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_timesync, this);
  }

  void stop(const nlohmann::json& args) {    
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Stoppping threads...";
    m_request_handler_impl->stop(args);
    while (!m_timesync_thread.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));         
    } 
    while (!m_consumer_thread.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));        
    }
    while (!m_requester_thread.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));        
    }
    while (!m_stats_thread.get_readiness()) {	
      std::this_thread::sleep_for(std::chrono::milliseconds(100));         	
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Flushing latency buffer with occupancy: " << m_latency_buffer_impl->occupancy();
    m_latency_buffer_impl->pop(m_latency_buffer_impl->occupancy());
    m_raw_processor_impl->reset_last_daq_time();
  }

  void record(const nlohmann::json& args) override {
    m_request_handler_impl->record(args);
  }

  void get_info(opmonlib::InfoCollector & ci, int /*level*/) {
    datalinkhandlerinfo::Info dli;
    dli.packets = m_packet_count_tot.load();
    dli.new_packets = m_packet_count.exchange(0);
    dli.requests = m_request_count_tot.load();
    dli.new_requests = m_request_count.exchange(0);

    ci.add(dli);
  }


private:

  void run_consume() {
    m_rawq_timeout_count = 0;
    m_packet_count = 0;
    m_packet_count_tot = 0;
    m_stats_packet_count = 0;

    TLOG_DEBUG(TLVL_WORK_STEPS) << "Consumer thread started...";
    while (m_run_marker.load() || m_raw_data_source->can_pop()) {
      //std::unique_ptr<RawType> payload_ptr(nullptr);
      RawType payload;
      // Try to acquire data
      try {
        //m_raw_data_source->pop(payload_ptr, m_source_queue_timeout_ms);
        m_raw_data_source->pop(payload, m_source_queue_timeout_ms);
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ++m_rawq_timeout_count;
        //ers::error(QueueTimeoutError(ERS_HERE, " raw source "));
      }
      // Only process if data was acquired
      //if (payload != nullptr) { // payload_ptr
        m_raw_processor_impl->process_item(&payload);
        if (!m_latency_buffer_impl->write(std::move(payload))) {
          TLOG_DEBUG(TLVL_TAKE_NOTE) << "***ERROR: Latency buffer is full and data was overwritten!";
        }
        m_request_handler_impl->auto_cleanup_check();
        ++m_packet_count;
        ++m_packet_count_tot;
        ++m_stats_packet_count;
      //}
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Consumer thread joins... ";
  }   

  void run_timesync() {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "TimeSync thread started...";
    m_request_count = 0;
    m_request_count_tot = 0;
    auto once_per_run = true;
    while (m_run_marker.load()) {
      try {
        auto timesyncmsg = dfmessages::TimeSync(m_raw_processor_impl->get_last_daq_time());
        //TLOG_DEBUG(0) << "New timesync: daq=" << timesyncmsg.DAQ_time << " wall=" << timesyncmsg.system_time;
        if (timesyncmsg.daq_time != 0) {
          try {
            m_timesync_sink->push(std::move(timesyncmsg));
          } catch (const ers::Issue &excpt) {
            ers::warning(CannotWriteToQueue(ERS_HERE, "timesync message queue", excpt));
          }

          if (m_fake_trigger) {
            dfmessages::DataRequest dr;
            dr.trigger_timestamp = timesyncmsg.daq_time > 500*time::us ? timesyncmsg.daq_time - 500*time::us : 0;
            auto width = 1000;
            uint offset = 100;
            dr.window_begin = dr.trigger_timestamp > offset ? dr.trigger_timestamp - offset : 0;
            dr.window_end = dr.window_begin + width;
            TLOG_DEBUG(TLVL_TAKE_NOTE) << "Issuing fake trigger based on timesync. "
              << " ts=" << dr.trigger_timestamp << " window_begin=" << dr.window_begin
                << " window_end=" << dr.window_end;
            m_request_handler_impl->issue_request(dr);
            ++m_request_count;
            ++m_request_count_tot;
          }
        } else {
          if (once_per_run) {
            TLOG() << "Timesync with DAQ time 0 won't be sent out as it's an invalid sync.";
            once_per_run = false;
          }
        }
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // ++m_timesyncqueue_timeout;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    once_per_run = true;
    TLOG_DEBUG(TLVL_WORK_STEPS) << "TimeSync thread joins...";
  } 

  void run_requests() {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Requester thread started...";
    m_request_count = 0;
    m_request_count_tot = 0;
    while (m_run_marker.load() || m_request_source->can_pop()) {
      dfmessages::DataRequest data_request;
      try {
        m_request_source->pop(data_request, m_source_queue_timeout_ms);
        m_request_handler_impl->issue_request(data_request);
        ++m_request_count;
        ++m_request_count_tot;
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // not an error, safe to continue
      }
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Requester thread joins... ";
  }

  void run_stats() {
    // Temporarily, for debugging, a rate checker thread...
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread started...";
    int new_packets = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (m_run_marker.load()) {
      auto now = std::chrono::high_resolution_clock::now();
      new_packets = m_stats_packet_count.exchange(0);
      double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
      TLOG_DEBUG(TLVL_TAKE_NOTE) << "Consumed Packet rate: " << std::to_string(new_packets/seconds/1000.) << " [kHz]";	
      auto rawq_timeouts = m_rawq_timeout_count.exchange(0);
      if (rawq_timeouts > 0) {
        TLOG_DEBUG(TLVL_TAKE_NOTE) << "***ERROR: Raw input queue timed out " << std::to_string(rawq_timeouts) << " times!";
      }
      for (int i=0; i<100 && m_run_marker.load(); ++i) { // 100 x 100ms = 10s sleeps
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      t0 = now;
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread joins...";
  }

  // Constuctor params
  std::atomic<bool>& m_run_marker;

  // CONFIGURATION
  appfwk::app::ModInit m_queue_config;
  bool m_fake_trigger;

  // STATS
  stats::counter_t m_packet_count{0};
  stats::counter_t m_packet_count_tot{0};
  stats::counter_t m_request_count{0};
  stats::counter_t m_request_count_tot{0};
  stats::counter_t m_rawq_timeout_count{0};
  stats::counter_t m_stats_packet_count{0};
  ReusableThread m_stats_thread;

  // CONSUMER
  ReusableThread m_consumer_thread;

  // RAW SOURCE
  std::chrono::milliseconds m_source_queue_timeout_ms;
  using raw_source_qt = appfwk::DAQSource<RawType>;
  std::unique_ptr<raw_source_qt> m_raw_data_source;

  // REQUEST SOURCE
  std::chrono::milliseconds m_request_queue_timeout_ms;
  using request_source_qt = appfwk::DAQSource<dfmessages::DataRequest>;
  std::unique_ptr<request_source_qt> m_request_source; 

  // FRAGMENT SINK
  std::chrono::milliseconds m_fragment_queue_timeout_ms;
  using fragment_sink_qt = appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>;
  std::unique_ptr<fragment_sink_qt> m_fragment_sink;

  // SNB SINK
  std::chrono::milliseconds m_snb_queue_timeout_ms;
  using snb_sink_qt = appfwk::DAQSink<RawType>;
  std::unique_ptr<snb_sink_qt> m_snb_sink;

  // LATENCY BUFFER:
  size_t m_latency_buffer_size;
  std::unique_ptr<LatencyBufferType> m_latency_buffer_impl;

  // RAW PROCESSING:
  std::unique_ptr<RawDataProcessorType> m_raw_processor_impl;

  // REQUEST HANDLER:
  std::unique_ptr<RequestHandlerType> m_request_handler_impl;
  ReusableThread m_requester_thread;

  // TIME-SYNC
  std::chrono::milliseconds m_timesync_queue_timeout_ms;
  using timesync_sink_qt = appfwk::DAQSink<dfmessages::TimeSync>;
  std::unique_ptr<timesync_sink_qt> m_timesync_sink;
  ReusableThread m_timesync_thread;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_READOUTMODEL_HPP_
