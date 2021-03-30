/**
 * @file SNBWriter.hpp Module to write snb data
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/

#ifndef READOUT_PLUGINS_BUFFERED_FILE_STREAMER_HPP_
#define READOUT_PLUGINS_BUFFERED_FILE_STREAMER_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "readout/bufferedfilestreamer/Structs.hpp"
#include "readout/ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "BufferedWriter.hpp"

#include <iostream>
#include <fstream>
#include <atomic>

namespace dunedaq {
  namespace readout {

    class BufferedFileStreamer : public dunedaq::appfwk::DAQModule {
    public:
      explicit BufferedFileStreamer(const std::string& name);

      BufferedFileStreamer(const BufferedFileStreamer&) = delete;
      BufferedFileStreamer& operator=(const BufferedFileStreamer&) = delete;
      BufferedFileStreamer(BufferedFileStreamer&&) = delete;
      BufferedFileStreamer& operator=(BufferedFileStreamer&&) = delete;

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
      bufferedfilestreamer::Conf m_conf;
      BufferedWriter<types::WIB_SUPERCHUNK_STRUCT> m_buffered_writer;

      // Stats
      stats::counter_t m_packets_processed_total{0};
      stats::counter_t m_packets_processed_since_last_info{0};
      std::chrono::steady_clock::time_point m_time_point_last_info;
    };
  }
}

#endif //READOUT_PLUGINS_BUFFERED_FILE_STREAMER_HPP_
