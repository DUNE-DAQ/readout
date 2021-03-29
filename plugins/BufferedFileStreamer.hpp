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

#include <boost/align/aligned_allocator.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zstd.hpp>
#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <iostream>
#include <fstream>
#include <atomic>

namespace dunedaq {
  namespace readout {
    using io_sink_t = boost::iostreams::file_descriptor_sink;
    using aligned_allocator_t = boost::alignment::aligned_allocator<io_sink_t::char_type, 4096>;
    using filtering_stream_t = boost::iostreams::filtering_stream<boost::iostreams::output, char, std::char_traits<char>, aligned_allocator_t>;
    //using stream_t = boost::iostreams::stream<io_sink_t, std::char_traits<io_sink_t::char_type>, aligned_allocator_t>;

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
      data_t m_conf;
      filtering_stream_t m_output_stream;
    };
  }
}

#endif //READOUT_PLUGINS_BUFFERED_FILE_STREAMER_HPP_
