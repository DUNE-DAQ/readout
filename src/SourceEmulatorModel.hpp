/**
* @file SourceEmulatorModel.hpp Emulates a source with given raw type
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_SOURCEEMULATOR_HPP_
#define READOUT_SRC_SOURCEEMULATOR_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "logging/Logging.hpp"

#include "opmonlib/InfoCollector.hpp"

#include "readout/fakecardreader/Structs.hpp"

#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "RandomEngine.hpp"
#include "RateLimiter.hpp"

#include <functional>
#include <utility>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_WORK_STEPS;
using dunedaq::readout::logging::TLVL_TAKE_NOTE;
using dunedaq::readout::logging::TLVL_QUEUE_POP;

namespace dunedaq {
namespace readout {

template<class RawType>
class SourceEmulatorModel : public SourceEmulatorConcept {
public:
  using sink_t = appfwk::DAQSink<RawType>;
  using inherited = SourceEmulatorConcept;

  explicit SourceEmulatorModel(std::atomic<bool>& run_marker)
  : m_run_marker(run_marker)
  , m_packet_count{0}
  , m_byte_count{0}
  , m_sink_queue_timeout_ms(0)
  , m_raw_data_sink(nullptr)
  { }

  void init(const nlohmann::json& /*args*/) {


  }

  void set_sink(const std::string& sink_name) {
    if (!inherited::m_sink_is_set) {
      m_raw_data_sink = std::make_unique<sink_t>(sink_name);
      inherited::m_sink_is_set = true;
    } else {
      //ers::error();
    }
  }

  void conf(const nlohmann::json& args) {
    inherited::m_conf = args.get<inherited::module_conf_t>();

    // Create a simple randomengine
    inherited::m_random_engine = std::make_unique<RandomEngine>();

    // Configure possible rates
    if (inherited::m_conf.variable_rate) {
      inherited::m_rate_limiter = std::make_unique<RateLimiter>(0);
      inherited::m_random_rate_population = inherited::m_random_engine->get_random_population(
        1000, static_cast<double>(m_conf.rate_min_khz), static_cast<double>(m_conf.rate_max_khz));
    } else {
      inherited::m_random_rate_population = inherited::m_random_engine->get_random_population(
        1, static_cast<double>(m_conf.rate_min_khz), static_cast<double>(m_conf.rate_max_khz));
    }

    // Configure possible sizes
    if (inherited::m_conf.variable_size) {
      inherited::m_random_size_population = inherited::m_random_engine->get_random_population(
        1000, static_cast<int>(m_conf.size_min_bytes), static_cast<int>(m_conf.size_max_bytes));
    } else {
      inherited::m_random_size_population = inherited::m_random_engine->get_random_population(
        1, static_cast<int>(m_conf.size_min_bytes), static_cast<int>(m_conf.size_max_bytes));
    }

    inherited::m_file_source = std::make_unique<FileSourceBuffer>(m_conf.input_limit, m_conf.size_max_bytes);
    try {
      inherited::m_file_source->read(inherited::m_conf.data_filename);
    } 
    catch (const ers::Issue& ex) {
      ers::fatal(ex);
      throw ConfigurationError(ERS_HERE, "", ex);
    }

    // Configure threads:
    //m_producer_thread.set_name("fakeprod", conf.link_number);
  }

  void start(const nlohmann::json& /*args*/) {
    /*
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Starting threads...";
    m_request_handler_impl->start(args);
    m_stats_thread.set_work(&SourceEmulatorModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_stats, this);
    m_consumer_thread.set_work(&SourceEmulatorModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_consume, this);
    m_requester_thread.set_work(&SourceEmulatorModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_requests, this);
    m_timesync_thread.set_work(&SourceEmulatorModel<RawType, RequestHandlerType, LatencyBufferType, RawDataProcessorType>::run_timesync, this);
  */
  }


  void stop(const nlohmann::json& /*args*/) {

  }

  void get_info(opmonlib::InfoCollector & /*ci*/, int /*level*/) {
/*
    datalinkhandlerinfo::Info dli;
    dli.packets = m_packet_count_tot.load();
    dli.new_packets = m_packet_count.exchange(0);
    dli.requests = m_request_count_tot.load();
    dli.new_requests = m_request_count.exchange(0);

    ci.add(dli);
*/
  }


protected:

  void run_adjust() {

  }

  void run_produce() {
/*
    m_rawq_timeout_count = 0;
    m_packet_count = 0;
    m_packet_count_tot = 0;
    m_stats_packet_count = 0;

    TLOG_DEBUG(TLVL_WORK_STEPS) << "Consumer thread started...";
    while (m_run_marker.load() || m_raw_data_source->can_pop()) {
      RawType payload;
      // Try to acquire data
      try {
        //m_raw_data_source->pop(payload_ptr, m_source_queue_timeout_ms);
        m_raw_data_source->pop(payload, m_source_queue_timeout_ms);
        m_raw_processor_impl->process_item(&payload);
        if (!m_latency_buffer_impl->write(std::move(payload))) {
          TLOG_DEBUG(TLVL_TAKE_NOTE) << "***ERROR: Latency buffer is full and data was overwritten!";
        }
        m_request_handler_impl->auto_cleanup_check();
        ++m_packet_count;
        ++m_packet_count_tot;
        ++m_stats_packet_count;
      }
      catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ++m_rawq_timeout_count;
        //ers::error(QueueTimeoutError(ERS_HERE, " raw source "));
      }
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Consumer thread joins... ";
*/
  }

private:

  // Constuctor params
  std::atomic<bool>& m_run_marker;

  // CONFIGURATION
  appfwk::app::ModInit m_queue_config;
  uint32_t m_this_apa_number; // NOLINT
  uint32_t m_this_link_number; // NOLINT

  // INTERNALS
  //RateLimiter m_rate_limiter;
  //RandomEngine m_random_engine;
  //FileSourceBuffer m_file_source;

  // STATS
  stats::counter_t m_packet_count{0};
  stats::counter_t m_byte_count{0};
  //ReusableThread m_stats_thread;

  // PRODUCER
  //ReusableThread m_producer_thread;

  // ADJUSTER
  //ReusableThread m_adjuster_thread;

  // RAW SINK
  std::chrono::milliseconds m_sink_queue_timeout_ms;
  using raw_sink_qt = appfwk::DAQSink<RawType>;
  std::unique_ptr<raw_sink_qt> m_raw_data_sink;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SOURCEEMULATOR_HPP_
