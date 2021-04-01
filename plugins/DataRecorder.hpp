/**
 * @file DataRecorder.hpp Module to record data
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/

#ifndef READOUT_PLUGINS_DATA_RECORDER_HPP_
#define READOUT_PLUGINS_DATA_RECORDER_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "readout/datarecorder/Structs.hpp"
#include "readout/ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "BufferedFileWriter.hpp"

#include <iostream>
#include <fstream>
#include <atomic>

namespace dunedaq {
  namespace readout {

    class DataRecorder : public dunedaq::appfwk::DAQModule {
    public:
      explicit DataRecorder(const std::string& name);

      DataRecorder(const DataRecorder&) = delete;
      DataRecorder& operator=(const DataRecorder&) = delete;
      DataRecorder(DataRecorder&&) = delete;
      DataRecorder& operator=(DataRecorder&&) = delete;

      void init(const nlohmann::json& obj) override;
      void get_info(opmonlib::InfoCollector& ci, int level) override;

    private:
      void do_conf(const nlohmann::json& obj);
      void do_start(const nlohmann::json& obj);
      void do_stop(const nlohmann::json& obj);

      dunedaq::appfwk::ThreadHelper m_thread;
      void do_work(std::atomic<bool>&);

      using source_t = dunedaq::appfwk::DAQSource<types::WIB_SUPERCHUNK_STRUCT>;
      std::unique_ptr<source_t> m_input_queue;

      int fd;
      std::string m_output_file;
      std::mutex m_start_lock;
      datarecorder::Conf m_conf;
      BufferedFileWriter<types::WIB_SUPERCHUNK_STRUCT> m_buffered_writer;

      // Stats
      stats::counter_t m_packets_processed_total{0};
      stats::counter_t m_packets_processed_since_last_info{0};
      std::chrono::steady_clock::time_point m_time_point_last_info;
    };
  }
}

#endif //READOUT_PLUGINS_DATA_RECORDER_HPP_
