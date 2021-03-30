/**
 * @file SNBWriter.cpp SNBWriter implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/bufferedfilestreamer/Nljs.hpp"
#include "readout/bufferedfilestreamer/Structs.hpp"
#include "readout/bufferedfilestreamerinfo/Nljs.hpp"
#include "readout/ReadoutLogging.hpp"

#include "BufferedFileStreamer.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"
#include "ReadoutIssues.hpp"

#include <boost/align/aligned_allocator.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace dunedaq::readout::logging;

namespace dunedaq {
  namespace readout {

    BufferedFileStreamer::BufferedFileStreamer(const std::string& name)
        : DAQModule(name)
        , m_thread(std::bind(&BufferedFileStreamer::do_work, this, std::placeholders::_1))
        , m_start_lock{} {
      register_command("conf", &BufferedFileStreamer::do_conf);
      register_command("start", &BufferedFileStreamer::do_start);
      register_command("stop", &BufferedFileStreamer::do_stop);
    }

    void BufferedFileStreamer::init(const data_t& args) {
      try {
        auto qi = appfwk::queue_index(args, {"snb"});
        m_input_queue.reset(new source_t(qi["snb"].inst));
      } catch (const ers::Issue& excpt) {
        ers::error(ResourceQueueError(ERS_HERE, "Could not initialize queue", "snb", ""));
      }
      m_start_lock.lock();
      m_thread.start_working_thread(get_name());
    }

    void BufferedFileStreamer::get_info(opmonlib::InfoCollector& ci, int /* level */) {
      bufferedfilestreamerinfo::Info info;
      info.packets_processed = m_packets_processed_total;
      info.bytes_written = m_bytes_written_total;
      double time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()
                                                                                    - m_time_point_last_info).count();
      info.throughput_processed_packets = m_packets_processed_since_last_info / time_diff;
      info.throughput_to_file = m_bytes_written_since_last_info / time_diff;

      ci.add(info);

      m_packets_processed_since_last_info = 0;
      m_bytes_written_since_last_info = 0;
      m_time_point_last_info = std::chrono::steady_clock::now();
    }

    void BufferedFileStreamer::do_conf(const data_t& args) {
      m_conf = args;
      auto conf = args.get<bufferedfilestreamer::Conf>();
      std::string output_file = conf.output_file;
      if (remove(output_file.c_str()) == 0) {
        TLOG(TLVL_WORK_STEPS) << "Removed existing output file from previous run" << std::endl;
      }
      fd = open(output_file.c_str(), O_CREAT | O_RDWR | O_DIRECT);
      if (fd == -1) {
        throw ConfigurationError(ERS_HERE, "Could not open output file.");
      }

      io_sink_t io_sink(fd, boost::iostreams::file_descriptor_flags::close_handle);
      if (conf.compression_algorithm == "zstd") {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Using zstd compression";
        m_output_stream.push(boost::iostreams::zstd_compressor(boost::iostreams::zstd::best_speed));
      } else if (conf.compression_algorithm == "lzma") {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Using lzma compression";
        m_output_stream.push(boost::iostreams::lzma_compressor(boost::iostreams::lzma::best_speed));
      } else if (conf.compression_algorithm == "zlib") {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Using zlib compression";
        m_output_stream.push(boost::iostreams::zlib_compressor(boost::iostreams::zlib::best_speed));
      } else if (conf.compression_algorithm == "None") {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Running without compression";
      } else {
        ers::error(ConfigurationError(ERS_HERE, "Non-recognized compression algorithm: " + conf.compression_algorithm));
      }

      m_output_stream.push(io_sink, conf.stream_buffer_size);
    }

    void BufferedFileStreamer::do_start(const data_t& /* args */) {
      m_start_lock.unlock();
    }

    void BufferedFileStreamer::do_stop(const data_t& /* args */) {
      m_thread.stop_working_thread();
    }

    void BufferedFileStreamer::do_work(std::atomic<bool>& running_flag) {
      std::lock_guard<std::mutex> lock_guard(m_start_lock);
      m_time_point_last_info = std::chrono::steady_clock::now();

      types::WIB_SUPERCHUNK_STRUCT element;
      while (running_flag.load()) {
        try {
          m_input_queue->pop(element, std::chrono::milliseconds(100));
          m_packets_processed_total++;
          m_packets_processed_since_last_info++;
          if (!m_output_stream.write((char*)&element, sizeof(element))) {
            TLOG() << "Write was not successful" << std::endl;
          } else {
            m_bytes_written_total += sizeof(element);
            m_bytes_written_since_last_info += sizeof(element);
          }
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          continue;
        }
      }
      m_output_stream.flush();
    }

  } // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::BufferedFileStreamer)
