/**
 * @file FakeCardReader.hpp FELIX FULLMODE Fake Links
 * Generates user payloads at a given rate, from WIB to FELIX binary captures
 * This implementation is purely software based, no FELIX card and tools
 * are needed to use this module.
 *
 * TODO: Make it generic, to consumer other types of user payloads.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#ifndef READOUT_PLUGINS_FAKECARDREADER_HPP_
#define READOUT_PLUGINS_FAKECARDREADER_HPP_

// package
#include "readout/fakecardreader/Structs.hpp"
#include "readout/ReusableThread.hpp"
#include "readout/ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "RateLimiter.hpp"
#include "FileSourceBuffer.hpp"

// appfwk
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"

// std
#include <memory>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>

namespace dunedaq {
namespace readout {

class FakeCardReader : public dunedaq::appfwk::DAQModule
{
public:
  /**
  * @brief FakeCardReader Constructor
  * @param name Instance name for this FakeCardReader instance
  */
  explicit FakeCardReader(const std::string& name);

  FakeCardReader(const FakeCardReader&) =
    delete; ///< FakeCardReader is not copy-constructible
  FakeCardReader& operator=(const FakeCardReader&) =
    delete; ///< FakeCardReader is not copy-assignable
  FakeCardReader(FakeCardReader&&) =
    delete; ///< FakeCardReader is not move-constructible
  FakeCardReader& operator=(FakeCardReader&&) =
    delete; ///< FakeCardReader is not move-assignable

  void init(const data_t&) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  using sink_t = appfwk::DAQSink<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>;
  using tp_sink_t = appfwk::DAQSink<std::unique_ptr<types::RAW_WIB_TP_STRUCT>>;
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_scrap(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

  void generate_data(sink_t* queue, int link_id);
  void generate_tp_data(tp_sink_t* queue, int link_id);

  // Configuration
  bool m_configured;
  using module_conf_t = fakecardreader::Conf;
  module_conf_t m_cfg;

  // appfwk Queues
  std::chrono::milliseconds m_queue_timeout_ms;
  //std::vector<std::unique_ptr<sink_t>> m_output_queues;
  std::vector<sink_t*> m_output_queues;
  std::vector<tp_sink_t*> m_tp_output_queues;

  // Internals
  std::unique_ptr<FileSourceBuffer> m_source_buffer;
  std::unique_ptr<FileSourceBuffer> m_tp_source_buffer;

  // Processor
  std::vector<std::thread> m_worker_threads;

  // Threading
  std::atomic<bool> m_run_marker;

  // Opmon
  stats::counter_t m_packet_count{0};
  stats::counter_t m_packet_count_tot{0};
  // Stats
  stats::counter_t m_stat_packet_count{0};
  stats::counter_t m_stat_packet_count_tot{0};
  ReusableThread m_stats_thread;	   
  void run_stats();

  // raw WIB TP parsing
  bool m_found_tp_header{false};

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_PLUGINS_FAKECARDREADER_HPP_
