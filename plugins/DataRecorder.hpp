/**
 * @file DataRecorder.hpp Module to record data
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_PLUGINS_DATARECORDER_HPP_
#define READOUT_PLUGINS_DATARECORDER_HPP_

#include "BufferedFileWriter.hpp"
#include "ReadoutStatistics.hpp"
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "readout/ReadoutTypes.hpp"
#include "readout/ReusableThread.hpp"
#include "readout/datarecorder/Structs.hpp"

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

class DataRecorder : public dunedaq::appfwk::DAQModule
{
public:
  explicit DataRecorder(const std::string& name);

  DataRecorder(const DataRecorder&) = delete;
  DataRecorder& operator=(const DataRecorder&) = delete;
  DataRecorder(DataRecorder&&) = delete;
  DataRecorder& operator=(DataRecorder&&) = delete;

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_conf(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_work();

  // Queue
  using source_t = dunedaq::appfwk::DAQSource<types::WIB_SUPERCHUNK_STRUCT>;
  std::unique_ptr<source_t> m_input_queue;

  // Internal
  datarecorder::Conf m_conf;
  BufferedFileWriter<types::WIB_SUPERCHUNK_STRUCT> m_buffered_writer;

  // Threading
  ReusableThread m_work_thread;
  std::atomic<bool> m_run_marker;

  // Stats
  stats::counter_t m_packets_processed_total{ 0 };
  stats::counter_t m_packets_processed_since_last_info{ 0 };
  std::chrono::steady_clock::time_point m_time_point_last_info;
};
} // namespace readout
} // namespace dunedaq

#endif // READOUT_PLUGINS_DATARECORDER_HPP_
