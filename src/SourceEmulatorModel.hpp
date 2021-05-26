/**
 * @file SourceEmulatorModel.hpp Emulates a source with given raw type
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_SOURCEEMULATORMODEL_HPP_
#define READOUT_SRC_SOURCEEMULATORMODEL_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "logging/Logging.hpp"

#include "opmonlib/InfoCollector.hpp"

#include "readout/fakecardreader/Structs.hpp"

#include "FileSourceBuffer.hpp"
#include "RandomEngine.hpp"
#include "RateLimiter.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutStatistics.hpp"
#include "SourceEmulatorConcept.hpp"

#include "readout/ReusableThread.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

using dunedaq::readout::logging::TLVL_QUEUE_POP;
using dunedaq::readout::logging::TLVL_TAKE_NOTE;
using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class RawType>
class SourceEmulatorModel : public SourceEmulatorConcept
{
public:
  using sink_t = appfwk::DAQSink<RawType>;

  explicit SourceEmulatorModel(std::string name,
                               std::atomic<bool>& run_marker,
                               uint64_t time_tick_diff, // NOLINT(build/unsigned)
                               double dropout_rate,
                               double rate_khz)
    : m_run_marker(run_marker)
    , m_time_tick_diff(time_tick_diff)
    , m_dropout_rate(dropout_rate)
    , m_packet_count{ 0 }
    , m_sink_queue_timeout_ms(0)
    , m_raw_data_sink(nullptr)
    , m_producer_thread(0)
    , m_name(name)
    , m_rate_khz(rate_khz)
  {}

  void init(const nlohmann::json& /*args*/) {}

  void set_sink(const std::string& sink_name)
  {
    if (!m_sink_is_set) {
      m_raw_data_sink = std::make_unique<sink_t>(sink_name);
      m_sink_is_set = true;
    } else {
      // ers::error();
    }
  }

  void conf(const nlohmann::json& args, const nlohmann::json& link_conf)
  {
    if (m_is_configured) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "This emulator is already configured!";
    } else {
      m_conf = args.get<module_conf_t>();
      m_link_conf = link_conf.get<link_conf_t>();
      m_sink_queue_timeout_ms = std::chrono::milliseconds(m_conf.queue_timeout_ms);

      m_file_source = std::make_unique<FileSourceBuffer>(m_link_conf.input_limit, sizeof(RawType));
      try {
        m_file_source->read(m_link_conf.data_filename);
      } catch (const ers::Issue& ex) {
        ers::fatal(ex);
        throw ConfigurationError(ERS_HERE, "", ex);
      }

      m_is_configured = true;
    }
    // Configure thread:
    m_producer_thread.set_name("fakeprod", m_link_conf.geoid.element);
  }

  bool is_configured() override { return m_is_configured; }

  void scrap(const nlohmann::json& /*args*/) { m_is_configured = false; }

  void start(const nlohmann::json& /*args*/)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Starting threads...";
    m_rate_limiter = std::make_unique<RateLimiter>(m_rate_khz / m_link_conf.slowdown);
    // m_stats_thread.set_work(&SourceEmulatorModel<RawType>::run_stats, this);
    m_producer_thread.set_work(&SourceEmulatorModel<RawType>::run_produce, this);
  }

  void stop(const nlohmann::json& /*args*/)
  {
    while (!m_producer_thread.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void get_info(fakecardreaderinfo::Info& fcr)
  {
    fcr.packets += m_packet_count_tot.load();
    fcr.new_packets += m_packet_count.exchange(0);
  }

protected:
  void run_produce()
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Data generation thread " << m_this_link_number << " started";

    // pthread_setname_np(pthread_self(), get_name().c_str());

    int offset = 0;
    auto& source = m_file_source->get();

    int num_elem = m_file_source->num_elements();
    if (num_elem == 0) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "No elements to read from buffer! Sleeping...";
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      num_elem = m_file_source->num_elements();
    }

    auto rptr = reinterpret_cast<RawType*>(source.data()); // NOLINT

    // set the initial timestamp to a configured value, otherwise just use the timestamp from the header
    uint64_t ts_0 = (m_conf.set_t0_to >= 0) ? m_conf.set_t0_to : rptr->get_timestamp(); // NOLINT(build/unsigned)
    TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp in the source file: " << ts_0;
    uint64_t timestamp = ts_0; // NOLINT(build/unsigned)

    while (m_run_marker.load()) {
      // Which element to push to the buffer
      if (offset == num_elem || (offset + 1) * sizeof(RawType) > source.size()) {
        offset = 0;
      }

      bool create_frame =
        static_cast<double>(rand()) / static_cast<double>(RAND_MAX) >= m_dropout_rate; // NOLINT(runtime/threadsafe_fn)
      if (create_frame) {
        RawType payload;
        // Memcpy from file buffer to flat char array
        ::memcpy(
          static_cast<void*>(&payload), static_cast<void*>(source.data() + offset * sizeof(RawType)), sizeof(RawType));

        // Fake timestamp
        payload.fake_timestamp(timestamp, m_time_tick_diff);

        // queue in to actual DAQSink
        try {
          m_raw_data_sink->push(std::move(payload), m_sink_queue_timeout_ms);
        } catch (ers::Issue& excpt) {
          ers::warning(CannotWriteToQueue(ERS_HERE, "raw data input queue", excpt));
          // std::runtime_error("Queue timed out...");
        }

        // Count packet and limit rate if needed.
        ++offset;
        ++m_packet_count;
        ++m_packet_count_tot;
      }

      timestamp += m_time_tick_diff * 12;

      m_rate_limiter->limit();
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Data generation thread " << m_this_link_number << " finished";
  }

private:
  // Constuctor params
  std::atomic<bool>& m_run_marker;

  // CONFIGURATION
  uint32_t m_this_apa_number;  // NOLINT(build/unsigned)
  uint32_t m_this_link_number; // NOLINT(build/unsigned)

  uint64_t m_time_tick_diff; // NOLINT(build/unsigned)
  double m_dropout_rate;

  // STATS
  stats::counter_t m_packet_count{ 0 };
  stats::counter_t m_packet_count_tot{ 0 };

  fakecardreader::Conf m_cfg;

  // RAW SINK
  std::chrono::milliseconds m_sink_queue_timeout_ms;
  using raw_sink_qt = appfwk::DAQSink<RawType>;
  std::unique_ptr<raw_sink_qt> m_raw_data_sink;

  bool m_sink_is_set = false;
  using module_conf_t = dunedaq::readout::fakecardreader::Conf;
  module_conf_t m_conf;
  using link_conf_t = dunedaq::readout::fakecardreader::LinkConfiguration;
  link_conf_t m_link_conf;

  std::unique_ptr<RateLimiter> m_rate_limiter;
  std::unique_ptr<FileSourceBuffer> m_file_source;

  ReusableThread m_producer_thread;

  std::string m_name;
  bool m_is_configured = false;
  double m_rate_khz;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SOURCEEMULATORMODEL_HPP_
