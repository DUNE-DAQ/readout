/**
 * @file ReadoutModel.hpp Glue between data source, payload raw processor,
 * latency buffer and request handler.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_READOUTMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_READOUTMODEL_HPP_

#include "appfwk/app/Nljs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/cmd/Structs.hpp"

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "logging/Logging.hpp"

#include "opmonlib/InfoCollector.hpp"

#include "dataformats/ComponentRequest.hpp"
#include "dataformats/Fragment.hpp"

#include "dfmessages/DataRequest.hpp"
#include "dfmessages/TimeSync.hpp"

#include "readout/ReadoutLogging.hpp"
#include "readout/datalinkhandler/Structs.hpp"
#include "readout/datalinkhandlerinfo/InfoNljs.hpp"

#include "readout/FrameErrorRegistry.hpp"

#include "readout/concepts/LatencyBufferConcept.hpp"
#include "readout/concepts/RawDataProcessorConcept.hpp"
#include "readout/concepts/RequestHandlerConcept.hpp"

#include "readout/ReadoutIssues.hpp"
#include "readout/utils/ReusableThread.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

using dunedaq::readout::logging::TLVL_QUEUE_POP;
using dunedaq::readout::logging::TLVL_TAKE_NOTE;
using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class ReadoutType, class RequestHandlerType, class LatencyBufferType, class RawDataProcessorType>
class ReadoutModel : public ReadoutConcept
{
public:
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  static inline constexpr timestamp_t ns = 1;
  static inline constexpr timestamp_t us = 1000 * ns;
  static inline constexpr timestamp_t ms = 1000 * us;
  static inline constexpr timestamp_t s = 1000 * ms;

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
  {}

  void init(const nlohmann::json& args)
  {
    setup_request_response_queues(args);

    try {
      auto queue_index = appfwk::queue_index(args, {"raw_input", "timesync"});
      m_raw_data_source.reset(new raw_source_qt(queue_index["raw_input"].inst));
      m_timesync_sink.reset(new timesync_sink_qt(queue_index["timesync"].inst));
    } catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, "ReadoutModel", "", excpt);
    }

    // Instantiate functionalities
    m_error_registry.reset(new FrameErrorRegistry());
    m_latency_buffer_impl.reset(new LatencyBufferType());
    m_raw_processor_impl.reset(new RawDataProcessorType(m_error_registry));
    m_request_handler_impl.reset(
      new RequestHandlerType(m_latency_buffer_impl, m_error_registry));

    m_request_handler_impl->init(args);
  }

  void conf(const nlohmann::json& args)
  {
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
    // m_raw_processor_impl->set_emulator_mode(conf.emulator_mode);
    m_request_handler_impl->conf(args);
    try {
      m_latency_buffer_impl->resize(m_latency_buffer_size);
    } catch (const std::bad_alloc& be) {
      ers::error(InitializationError(ERS_HERE, "Latency Buffer can't be allocated with size!"));
    }

    // Configure threads:
    m_stats_thread.set_name("stats", conf.link_number);
    m_consumer_thread.set_name("consumer", conf.link_number);
    m_timesync_thread.set_name("timesync", conf.link_number);
    m_requester_thread.set_name("requests", conf.link_number);

    m_this_apa_number = conf.apa_number;
    m_this_link_number = conf.link_number;
  }

  void start(const nlohmann::json& args)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Starting threads...";
    m_raw_processor_impl->start(args);
    m_request_handler_impl->start(args);
    m_stats_thread.set_work(
      &ReadoutModel<ReadoutType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_stats, this);
    m_consumer_thread.set_work(
      &ReadoutModel<ReadoutType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_consume, this);
    m_requester_thread.set_work(
      &ReadoutModel<ReadoutType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_requests, this);
    m_timesync_thread.set_work(
      &ReadoutModel<ReadoutType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_timesync, this);
  }

  void stop(const nlohmann::json& args)
  {
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
    m_raw_processor_impl->stop(args);
    m_raw_processor_impl->reset_last_daq_time();
  }

  void record(const nlohmann::json& args) override { m_request_handler_impl->record(args); }

  void get_info(opmonlib::InfoCollector& ci, int /*level*/)
  {
    datalinkhandlerinfo::Info dli;
    dli.packets = m_packet_count_tot.load();
    dli.new_packets = m_packet_count.exchange(0);
    dli.requests = m_request_count_tot.load();
    dli.new_requests = m_request_count.exchange(0);
    dli.overwritten_packet_count = m_overwritten_packet_count.load();

    m_request_handler_impl->get_info(dli);
    m_raw_processor_impl->get_info(dli);

    ci.add(dli);
  }

private:
  void setup_request_response_queues(const nlohmann::json& args) {
    auto queue_index = appfwk::queue_index(args, {});
    size_t index = 0;
    while (queue_index.find("data_requests_" + std::to_string(index)) != queue_index.end()) {
      m_data_request_queues.push_back(std::make_unique<request_source_qt>(queue_index["data_requests_" + std::to_string(index)].inst));
      if (queue_index.find("fragment_response_" + std::to_string(index)) == queue_index.end()) {
        throw ResourceQueueError(ERS_HERE, "Queue not found: ", "fragment_response_" + std::to_string(index), "");
      } else {
        m_fragment_response_queues.push_back(std::make_unique<fragment_sink_qt>(queue_index["fragment_response_" + std::to_string(index)].inst));
      }
      index++;
    }
    if (index == 0) {
      throw ResourceQueueError(ERS_HERE, "Queue not found (need at least one request queue): ", "data_requests_0", "");
    }
  }

  void run_consume()
  {
    m_rawq_timeout_count = 0;
    m_packet_count = 0;
    m_packet_count_tot = 0;
    m_stats_packet_count = 0;

    TLOG_DEBUG(TLVL_WORK_STEPS) << "Consumer thread started...";
    while (m_run_marker.load() || m_raw_data_source->can_pop()) {
      ReadoutType payload;
      // Try to acquire data
      try {
        // m_raw_data_source->pop(payload_ptr, m_source_queue_timeout_ms);
        m_raw_data_source->pop(payload, m_source_queue_timeout_ms);
        m_raw_processor_impl->preprocess_item(&payload);
        if (!m_latency_buffer_impl->write(std::move(payload))) {
          TLOG_DEBUG(TLVL_TAKE_NOTE) << "***ERROR: Latency buffer is full and data was overwritten!";
          m_overwritten_packet_count++;
        }
        m_raw_processor_impl->postprocess_item(m_latency_buffer_impl->back());
        ++m_packet_count;
        ++m_packet_count_tot;
        ++m_stats_packet_count;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ++m_rawq_timeout_count;
        // ers::error(QueueTimeoutError(ERS_HERE, " raw source "));
      }
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Consumer thread joins... ";
  }

  void run_timesync()
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "TimeSync thread started...";
    m_request_count = 0;
    m_request_count_tot = 0;
    auto once_per_run = true;
    while (m_run_marker.load()) {
      try {
        auto timesyncmsg = dfmessages::TimeSync(m_raw_processor_impl->get_last_daq_time());
        // TLOG() << "New timesync: daq=" << timesyncmsg.daq_time << " wall=" << timesyncmsg.system_time;
        if (timesyncmsg.daq_time != 0) {
          try {
            m_timesync_sink->push(std::move(timesyncmsg));
          } catch (const ers::Issue& excpt) {
            ers::warning(CannotWriteToQueue(ERS_HERE, "timesync message queue", excpt));
          }

          if (m_fake_trigger) {
            dfmessages::DataRequest dr;
            dr.trigger_timestamp = timesyncmsg.daq_time > 500 * us ? timesyncmsg.daq_time - 500 * us : 0;
            auto width = 10000;
            uint offset = 100;
            dr.window_begin = dr.trigger_timestamp > offset ? dr.trigger_timestamp - offset : 0;
            dr.window_end = dr.window_begin + width;
            TLOG_DEBUG(TLVL_WORK_STEPS) << "Issuing fake trigger based on timesync. "
                                        << " ts=" << dr.trigger_timestamp << " window_begin=" << dr.window_begin
                                        << " window_end=" << dr.window_end;
            m_request_handler_impl->issue_request(dr, *m_fragment_response_queues[0]);
            ++m_request_count;
            ++m_request_count_tot;
          }
        } else {
          if (once_per_run) {
            TLOG() << "Timesync with DAQ time 0 won't be sent out as it's an invalid sync.";
            once_per_run = false;
          }
        }
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // ++m_timesyncqueue_timeout;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    once_per_run = true;
    TLOG_DEBUG(TLVL_WORK_STEPS) << "TimeSync thread joins...";
  }

  void run_requests()
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Requester thread started...";
    m_request_count = 0;
    m_request_count_tot = 0;
    dfmessages::DataRequest data_request;

    while (m_run_marker.load()) {
      for (size_t i = 0; i < m_data_request_queues.size(); ++i) {
        auto& request_source = *m_data_request_queues[i];
        auto& response_sink = *m_fragment_response_queues[i];
        try {
          request_source.pop(data_request, m_source_queue_timeout_ms);
          m_request_handler_impl->issue_request(data_request, response_sink);
          ++m_request_count;
          ++m_request_count_tot;
          TLOG_DEBUG(TLVL_QUEUE_POP) << "Received DataRequest for trigger_number " << data_request.trigger_number
                                     << ", run number " << data_request.run_number << " (APA number " << m_this_apa_number
                                     << ", link number " << m_this_link_number << ")";
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          // not an error, safe to continue
        }
      }
    }

    // Clear queues
    for (auto& queue : m_data_request_queues) {
      while (queue->can_pop()) {
        queue->pop(data_request, m_source_queue_timeout_ms);
      }
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Requester thread joins... ";
  }

  void run_stats()
  {
    // Temporarily, for debugging, a rate checker thread...
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread started...";
    int new_packets = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (m_run_marker.load()) {
      auto now = std::chrono::high_resolution_clock::now();
      new_packets = m_stats_packet_count.exchange(0);
      double seconds = std::chrono::duration_cast<std::chrono::microseconds>(now - t0).count() / 1000000.;
      TLOG_DEBUG(TLVL_TAKE_NOTE) << "Consumed Packet rate: " << std::to_string(new_packets / seconds / 1000.)
                                 << " [kHz]";
      auto rawq_timeouts = m_rawq_timeout_count.exchange(0);
      if (rawq_timeouts > 0) {
        TLOG_DEBUG(TLVL_TAKE_NOTE) << "***ERROR: Raw input queue timed out " << std::to_string(rawq_timeouts)
                                   << " times!";
      }
      for (int i = 0; i < 100 && m_run_marker.load(); ++i) { // 100 x 100ms = 10s sleeps
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
  uint32_t m_this_apa_number;  // NOLINT(build/unsigned)
  uint32_t m_this_link_number; // NOLINT(build/unsigned)

  // STATS
  std::atomic<int> m_packet_count{ 0 };
  std::atomic<int> m_packet_count_tot{ 0 };
  std::atomic<int> m_request_count{ 0 };
  std::atomic<int> m_request_count_tot{ 0 };
  std::atomic<int> m_rawq_timeout_count{ 0 };
  std::atomic<int> m_stats_packet_count{ 0 };
  std::atomic<int> m_overwritten_packet_count{ 0 };
  ReusableThread m_stats_thread;

  // CONSUMER
  ReusableThread m_consumer_thread;

  // RAW SOURCE
  std::chrono::milliseconds m_source_queue_timeout_ms;
  using raw_source_qt = appfwk::DAQSource<ReadoutType>;
  std::unique_ptr<raw_source_qt> m_raw_data_source;

  // REQUEST SOURCE
  std::chrono::milliseconds m_request_queue_timeout_ms;
  using request_source_qt = appfwk::DAQSource<dfmessages::DataRequest>;
  std::vector<std::unique_ptr<request_source_qt>> m_data_request_queues;

  // FRAGMENT SINK
  std::chrono::milliseconds m_fragment_queue_timeout_ms;
  using fragment_sink_qt = appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>;
  std::vector<std::unique_ptr<fragment_sink_qt>> m_fragment_response_queues;


  // LATENCY BUFFER:
  size_t m_latency_buffer_size;
  std::unique_ptr<LatencyBufferType> m_latency_buffer_impl;

  // RAW PROCESSING:
  std::unique_ptr<RawDataProcessorType> m_raw_processor_impl;

  // REQUEST HANDLER:
  std::unique_ptr<RequestHandlerType> m_request_handler_impl;
  ReusableThread m_requester_thread;

  std::unique_ptr<FrameErrorRegistry> m_error_registry;

  // TIME-SYNC
  std::chrono::milliseconds m_timesync_queue_timeout_ms;
  using timesync_sink_qt = appfwk::DAQSink<dfmessages::TimeSync>;
  std::unique_ptr<timesync_sink_qt> m_timesync_sink;
  ReusableThread m_timesync_thread;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_READOUTMODEL_HPP_
