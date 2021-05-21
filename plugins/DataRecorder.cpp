/**
 * @file DataRecorder.cpp DataRecorder implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "readout/ReadoutLogging.hpp"
#include "readout/datarecorder/Nljs.hpp"
#include "readout/datarecorder/Structs.hpp"
#include "readout/datarecorderinfo/Nljs.hpp"

#include "DataRecorder.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "ReadoutIssues.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"
#include <string>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

DataRecorder::DataRecorder(const std::string& name)
  : DAQModule(name)
  , m_work_thread(0)
{
  register_command("conf", &DataRecorder::do_conf);
  register_command("start", &DataRecorder::do_start);
  register_command("stop", &DataRecorder::do_stop);
}

void
DataRecorder::init(const data_t& args)
{
  try {
    auto qi = appfwk::queue_index(args, { "raw_recording" });
    m_input_queue.reset(new source_t(qi["raw_recording"].inst));
  } catch (const ers::Issue& excpt) {
    throw ResourceQueueError(ERS_HERE, "Could not initialize queue", "raw_recording", "");
  }
}

void
DataRecorder::get_info(opmonlib::InfoCollector& ci, int /* level */)
{
  datarecorderinfo::Info info;
  info.packets_processed = m_packets_processed_total;
  double time_diff =
    std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - m_time_point_last_info)
      .count();
  info.throughput_processed_packets = m_packets_processed_since_last_info / time_diff;

  ci.add(info);

  m_packets_processed_since_last_info = 0;
  m_time_point_last_info = std::chrono::steady_clock::now();
}

void
DataRecorder::do_conf(const data_t& args)
{
  m_conf = args.get<datarecorder::Conf>();
  std::string output_file = m_conf.output_file;
  if (remove(output_file.c_str()) == 0) {
    TLOG(TLVL_WORK_STEPS) << "Removed existing output file from previous run" << std::endl;
  }

  m_buffered_writer.open(m_conf.output_file, m_conf.stream_buffer_size, m_conf.compression_algorithm);
  m_work_thread.set_name(get_name(), 0);
}

void
DataRecorder::do_start(const data_t& /* args */)
{
  m_run_marker.store(true);
  m_work_thread.set_work(&DataRecorder::do_work, this);
}

void
DataRecorder::do_stop(const data_t& /* args */)
{
  m_run_marker.store(false);
  while (!m_work_thread.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void
DataRecorder::do_work()
{
  m_time_point_last_info = std::chrono::steady_clock::now();

  types::WIB_SUPERCHUNK_STRUCT element;
  while (m_run_marker) {
    try {
      m_input_queue->pop(element, std::chrono::milliseconds(100));
      m_packets_processed_total++;
      m_packets_processed_since_last_info++;
      if (!m_buffered_writer.write(element)) {
        throw CannotWriteToFile(ERS_HERE, m_conf.output_file);
      }
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      continue;
    }
  }
  m_buffered_writer.flush();
}

} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DataRecorder)
