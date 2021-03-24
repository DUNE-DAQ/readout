/**
 * @file SNBWriter.cpp SNBWriter implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/snbwriter/Nljs.hpp"
#include "readout/ReadoutLogging.hpp"

#include "SNBWriter.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"

#include <boost/align/aligned_allocator.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

using namespace dunedaq::readout::logging;

namespace dunedaq {
  namespace readout {

    SNBWriter::SNBWriter(const std::string& name)
        : DAQModule(name)
        , m_thread(std::bind(&SNBWriter::do_work, this, std::placeholders::_1))
        , m_start_lock{} {
      register_command("conf", &SNBWriter::do_conf);
      register_command("start", &SNBWriter::do_start);
      register_command("stop", &SNBWriter::do_stop);
    }

    void SNBWriter::init(const data_t& args) {
      try {
        auto qi = appfwk::queue_index(args, {"snb"});
        m_input_queue.reset(new source_t(qi["snb"].inst));
      } catch (const ers::Issue& excpt) {
        //throw QueueFatalError(ERS_HERE, get_name(), "snb", excpt);
      }
      m_start_lock.lock();
      m_thread.start_working_thread(get_name());
    }

    void SNBWriter::get_info(opmonlib::InfoCollector& ci, int level) {

    }

    void SNBWriter::do_conf(const data_t& args) {

    }

    void SNBWriter::do_start(const data_t& args) {
      m_start_lock.unlock();
    }

    void SNBWriter::do_stop(const data_t& args) {
      m_thread.stop_working_thread();
    }

    void SNBWriter::do_work(std::atomic<bool>& running_flag) {
      std::lock_guard<std::mutex> lock_guard(m_start_lock);
      std::string output_file = "output_" + get_name();
      if (remove(output_file.c_str()) == 0) {
        TLOG() << "Removed existing output file from previous run" << std::endl;
      }
      fd = open(output_file.c_str(), O_CREAT | O_RDWR | O_DIRECT);
      if (fd == -1) {
        //throw FileError(ERS_HERE, get_name(), output_file);
      }

      boost::iostreams::file_descriptor_sink io_sink(fd, boost::iostreams::file_descriptor_flags::close_handle);

      boost::iostreams::stream<boost::iostreams::file_descriptor_sink, std::char_traits<boost::iostreams::file_descriptor_sink::char_type>, boost::alignment::aligned_allocator<boost::iostreams::file_descriptor_sink::char_type, 4096>> output_stream(io_sink, 8388608);
      if (!output_stream.is_open()) {
        TLOG() << "Could not open output stream" << std::endl;
      } else {
        TLOG() << "Successfully opened stream" << std::endl;
      }

      types::WIB_SUPERCHUNK_STRUCT element;

      while (running_flag.load()) {
        try {
          m_input_queue->pop(element, std::chrono::milliseconds(100));
          if (!output_stream.write((char*)&element, sizeof(element))) {
            TLOG() << "Write was not successful" << std::endl;
          }
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          continue;
        }
      }
    }

  } // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::SNBWriter)
