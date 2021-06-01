/**
 * @file DummyConsumer.hpp Module that consumes data from a queue and does nothing with it
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_PLUGINS_DUMMYCONSUMER_HPP_
#define READOUT_PLUGINS_DUMMYCONSUMER_HPP_

#include "readout/ReadoutStatistics.hpp"
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "dfmessages/TimeSync.hpp"
#include "readout/types/ReadoutTypes.hpp"
#include "readout/utils/ReusableThread.hpp"

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace dunedaq {
namespace readout {
template<class T>
class DummyConsumer : public dunedaq::appfwk::DAQModule
{
public:
  explicit DummyConsumer(const std::string& name);

  DummyConsumer(const DummyConsumer&) = delete;
  DummyConsumer& operator=(const DummyConsumer&) = delete;
  DummyConsumer(DummyConsumer&&) = delete;
  DummyConsumer& operator=(DummyConsumer&&) = delete;

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

protected:
  virtual void packet_callback(T& /*packet*/) {}

private:
  // Commands
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_work();

  // Queue
  using source_t = dunedaq::appfwk::DAQSource<T>;
  std::unique_ptr<source_t> m_input_queue;

  // Threading
  ReusableThread m_work_thread;
  std::atomic<bool> m_run_marker;

  // Stats
  stats::counter_t m_packets_processed{ 0 };
};
} // namespace readout
} // namespace dunedaq

#endif // READOUT_PLUGINS_DUMMYCONSUMER_HPP_
