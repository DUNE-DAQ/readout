/**
 * @file SNBWriter.cpp SNBWriter implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/bufferedfilestreamer/Nljs.hpp"
#include "readout/bufferedfilestreamer/Structs.hpp"
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

    void BufferedFileStreamer::get_info(opmonlib::InfoCollector& /* ci */, int /* level */) {

    }

    void BufferedFileStreamer::do_conf(const data_t& args) {
      m_conf = args;
      auto conf = args.get<bufferedfilestreamer::Conf>();
      std::string output_file = conf.output_file;
      if (remove(output_file.c_str()) == 0) {
        TLOG() << "Removed existing output file from previous run" << std::endl;
      }
      fd = open(output_file.c_str(), O_CREAT | O_RDWR | O_DIRECT);
      if (fd == -1) {
        throw ConfigurationError(ERS_HERE, "Could not open output file.");
      }

      io_sink_t io_sink(fd, boost::iostreams::file_descriptor_flags::close_handle);
      m_output_stream.open(io_sink, conf.stream_buffer_size);

      if (!m_output_stream.is_open()) {
        TLOG() << "Could not open stream" << std::endl;
        throw ConfigurationError(ERS_HERE, "Could not open output stream");
      }
    }

    void BufferedFileStreamer::do_start(const data_t& /* args */) {
      m_start_lock.unlock();
    }

    void BufferedFileStreamer::do_stop(const data_t& /* args */) {
      m_thread.stop_working_thread();
    }

    void BufferedFileStreamer::do_work(std::atomic<bool>& running_flag) {
      std::lock_guard<std::mutex> lock_guard(m_start_lock);

      types::WIB_SUPERCHUNK_STRUCT element;
      while (running_flag.load()) {
        try {
          m_input_queue->pop(element, std::chrono::milliseconds(100));
          if (!m_output_stream.write((char*)&element, sizeof(element))) {
            TLOG() << "Write was not successful" << std::endl;
          }
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          continue;
        }
      }
    }

  } // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::BufferedFileStreamer)