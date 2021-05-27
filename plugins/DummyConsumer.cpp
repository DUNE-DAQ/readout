/**
 * @file DummyConsumer.cpp DummyConsumer implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "readout/ReadoutLogging.hpp"

#include "DummyConsumer.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "readout/dummyconsumerinfo/InfoNljs.hpp"

#include "ReadoutIssues.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"
#include <string>

namespace dunedaq {
namespace readout {

using namespace logging;

template<class T>
DummyConsumer<T>::DummyConsumer(const std::string& name)
  : DAQModule(name)
  , m_work_thread(0)
{
  register_command("start", &DummyConsumer::do_start);
  register_command("stop", &DummyConsumer::do_stop);
}

template<class T>
void
DummyConsumer<T>::init(const data_t& args)
{
  try {
    auto qi = appfwk::queue_index(args, { "input_queue" });
    m_input_queue.reset(new source_t(qi["input_queue"].inst));
  } catch (const ers::Issue& excpt) {
    throw ResourceQueueError(ERS_HERE, "Could not initialize queue", "input_queue", "");
  }
}

template<class T>
void
DummyConsumer<T>::get_info(opmonlib::InfoCollector& ci, int /* level */)
{
  dummyconsumerinfo::Info info;
  info.packets_processed = m_packets_processed;

  ci.add(info);
}

template<class T>
void
DummyConsumer<T>::do_start(const data_t& /* args */)
{
  m_run_marker.store(true);
  m_work_thread.set_work(&DummyConsumer::do_work, this);
}

template<class T>
void
DummyConsumer<T>::do_stop(const data_t& /* args */)
{
  m_run_marker.store(false);
  while (!m_work_thread.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

template<class T>
void
DummyConsumer<T>::do_work()
{
  T element;
  while (m_run_marker) {
    try {
      m_input_queue->pop(element, std::chrono::milliseconds(100));
      packet_callback(element);
      m_packets_processed++;
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      continue;
    }
  }
}

} // namespace readout
} // namespace dunedaq
