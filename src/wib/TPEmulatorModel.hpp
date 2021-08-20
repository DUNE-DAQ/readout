/**
 * @file TPEmulatorModel.hpp Emulates a tp source
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_TPEMULATORMODEL_HPP_
#define READOUT_SRC_WIB_TPEMULATORMODEL_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "logging/Logging.hpp"

#include "opmonlib/InfoCollector.hpp"

#include "readout/fakecardreader/Structs.hpp"

#include "readout/RawWIBTp.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/concepts/SourceEmulatorConcept.hpp"
#include "readout/utils/FileSourceBuffer.hpp"
#include "readout/utils/RateLimiter.hpp"

#include "readout/utils/ReusableThread.hpp"

#include <functional>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_TAKE_NOTE;
using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

class TPEmulatorModel : public SourceEmulatorConcept
{
public:
  using sink_t = appfwk::DAQSink<dataformats::RawWIBTp>;

  // Very bad, use these from readout types, when RAW_WIB_TP is introduced
  static const constexpr std::size_t WIB_FRAME_SIZE = 464;
  static const constexpr std::size_t FLX_SUPERCHUNK_FACTOR = 12;
  static const constexpr std::size_t WIB_SUPERCHUNK_SIZE = 5568; // for 12: 5568

  // Raw WIB TP
  static const constexpr std::size_t RAW_WIB_TP_SUBFRAME_SIZE = 12; 
  // same size for header, tp data, pedinfo: 3 words * 4 bytes/word

  explicit TPEmulatorModel(std::atomic<bool>& run_marker, double rate_khz)
    : m_run_marker(run_marker)
    , m_packet_count{ 0 }
    , m_sink_queue_timeout_ms(0)
    , m_raw_data_sink(nullptr)
    , m_producer_thread(0)
    , m_rate_khz(rate_khz)
  {}

  void init(const nlohmann::json& /*args*/) {}

  void set_sink(const std::string& sink_name)
  {
    if (!m_sink_is_set) {
      m_raw_data_sink = std::make_unique<raw_sink_qt>(sink_name);
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

      m_geoid.element_id = m_link_conf.geoid.element;
      m_geoid.region_id = m_link_conf.geoid.region;
      m_geoid.system_type = dataformats::GeoID::SystemType::kTPC;;

      m_file_source = std::make_unique<FileSourceBuffer>(m_link_conf.input_limit, RAW_WIB_TP_SUBFRAME_SIZE);
      
      try {
        m_file_source->read(m_link_conf.data_filename);
      } catch (const ers::Issue& ex) {
        ers::fatal(ex);
        throw ConfigurationError(ERS_HERE, m_geoid, "", ex);
      }

      m_is_configured = true;
    }
    // Configure thread:
    m_producer_thread.set_name("fakeprod-tp", m_link_conf.geoid.element);
  }

  bool is_configured() override { return m_is_configured; }

  void scrap(const nlohmann::json& /*args*/) { m_is_configured = false; }

  void start(const nlohmann::json& /*args*/)
  {
    m_packet_count_tot = 0;
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Starting threads...";
    m_rate_limiter = std::make_unique<RateLimiter>(m_rate_khz / m_link_conf.slowdown);
    m_producer_thread.set_work(&TPEmulatorModel::run_produce, this);
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
      TLOG_DEBUG(TLVL_WORK_STEPS) << "No raw WIB TP elements to read from buffer! Sleeping...";
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      num_elem = m_file_source->num_elements();
    }
    auto rwtpptr = reinterpret_cast<dunedaq::dataformats::RawWIBTp*>(source.data()); // NOLINT
    TLOG_DEBUG(TLVL_BOOKKEEPING) << "Number of raw WIB TP elements to read from buffer: " << num_elem
                                 << "; rwtpptr is: " << rwtpptr;

    uint64_t ts_0 = (m_cfg.set_t0_to >= 0) ? m_cfg.set_t0_to : rwtpptr->get_header()->get_timestamp(); // NOLINT
    TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp in the raw WIB TP source file: " << ts_0;
    uint64_t ts_next = ts_0; // NOLINT

    while (m_run_marker.load()) {
      // Which element to push to the buffer
      if (offset == num_elem * static_cast<int>(RAW_WIB_TP_SUBFRAME_SIZE) ||
          static_cast<uint>((offset + 1) * RAW_WIB_TP_SUBFRAME_SIZE) >
            source.size()) { // NOLINT(build/unsigned)
        offset = 0;
      }

      // Count number of subframes in a TP frame
      int n = 1;
      while (offset + (n - 1) * RAW_WIB_TP_SUBFRAME_SIZE < source.size() &&
             reinterpret_cast<types::TpSubframe*>(((uint8_t*)source.data()) // NOLINT
                                                  + offset + (n - 1) * RAW_WIB_TP_SUBFRAME_SIZE)
                 ->word3 != 0xDEADBEEF) {
        n++;
      }
      // Create next TP frame
      std::unique_ptr<types::RAW_WIB_TP_STRUCT> payload_ptr = std::make_unique<types::RAW_WIB_TP_STRUCT>();
      for (int i = 0; i < n; ++i) {
        auto* sp = reinterpret_cast<types::TpSubframe*>(                                // NOLINT
          ((uint8_t*)source.data()) + offset + i * RAW_WIB_TP_SUBFRAME_SIZE); // NOLINT
        if (!m_found_tp_header) {
          dunedaq::dataformats::TpHeader* tfh = reinterpret_cast<dunedaq::dataformats::TpHeader*>(sp); // NOLINT
          tfh->set_timestamp(ts_next);
          ts_next += 1600;
          m_found_tp_header = true;
          payload_ptr->head = *tfh;
          continue;
        }
        if (sp->word3 == 0xDEADBEEF) {
          const dunedaq::dataformats::TpPedinfo* tpi = reinterpret_cast<dunedaq::dataformats::TpPedinfo*>(sp); // NOLINT
          m_found_tp_header = false;
          payload_ptr->ped = *tpi;
          continue;
        }
        dunedaq::dataformats::TpData* td = reinterpret_cast<dunedaq::dataformats::TpData*>(sp); // NOLINT
        payload_ptr->block.set_tp(*td);
      }

      // queue in to actual DAQSink
      try {
        m_raw_data_sink->push(std::move(payload_ptr), m_sink_queue_timeout_ms);
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        // std::runtime_error("Queue timed out...");
      }

      // Count packet and limit rate if needed.
      offset += n * RAW_WIB_TP_SUBFRAME_SIZE;
      // if (m_alloc_) { free(m_data_); }
      ++m_packet_count;
      ++m_packet_count_tot;
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

  bool m_found_tp_header{ false };

  // STATS
  std::atomic<int> m_packet_count{ 0 };
  std::atomic<int> m_packet_count_tot{ 0 };

  fakecardreader::Conf m_cfg;

  // RAW SINK
  std::chrono::milliseconds m_sink_queue_timeout_ms;
  using raw_sink_qt = appfwk::DAQSink<std::unique_ptr<types::RAW_WIB_TP_STRUCT>>;
  std::unique_ptr<raw_sink_qt> m_raw_data_sink;

  bool m_sink_is_set = false;
  using module_conf_t = dunedaq::readout::fakecardreader::Conf;
  module_conf_t m_conf;
  using link_conf_t = dunedaq::readout::fakecardreader::LinkConfiguration;
  link_conf_t m_link_conf;

  std::unique_ptr<RateLimiter> m_rate_limiter;
  std::unique_ptr<FileSourceBuffer> m_file_source;

  ReusableThread m_producer_thread;

  bool m_is_configured = false;
  double m_rate_khz;
  dataformats::GeoID m_geoid;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB_TPEMULATORMODEL_HPP_
