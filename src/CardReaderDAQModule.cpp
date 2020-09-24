/**
 * @file CardReaderDAQModule.cc CardReaderDAQModule class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CardReaderDAQModule.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "FakeDataProducer" // NOLINT

namespace dunedaq {
namespace appfwk {

CardReaderDAQModule::CardReaderDAQModule(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&CardReaderDAQModule::do_work, this, std::placeholders::_1))
  , outputQueue_(nullptr)
  , queueTimeout_(100)
{

  register_command("configure", &CardReaderDAQModule::do_configure);
  register_command("start", &CardReaderDAQModule::do_start);
  register_command("stop", &CardReaderDAQModule::do_stop);
}

void
CardReaderDAQModule::init()
{
  outputQueue_.reset(new DAQSink<std::vector<int>>(get_config()["output"].get<std::string>()));
}

void
CardReaderDAQModule::do_configure(const std::vector<std::string>& /*args*/)
{
  nIntsPerVector_ = get_config().value<int>("nIntsPerVector", 10);
  starting_int_ = get_config().value<int>("starting_int", -4);
  ending_int_ = get_config().value<int>("ending_int", 14);
  wait_between_sends_ms_ = get_config().value<int>("wait_between_sends_ms", 1000);
  queueTimeout_ = std::chrono::milliseconds(get_config().value<int>("queue_timeout_ms", 100));
}

void
CardReaderDAQModule::do_start(const std::vector<std::string>& /*args*/)
{
  thread_.start_working_thread();
}

void
CardReaderDAQModule::do_stop(const std::vector<std::string>& /*args*/)
{
  thread_.stop_working_thread();
}

/**
 * @brief Format a std::vector<int> to a stream
 * @param t ostream Instance
 * @param ints Vector to format
 * @return ostream Instance
 */
std::ostream&
operator<<(std::ostream& t, std::vector<int> ints)
{
  t << "{";
  bool first = true;
  for (auto& i : ints) {
    if (!first)
      t << ", ";
    first = false;
    t << i;
  }
  return t << "}";
}

void
CardReaderDAQModule::do_work(std::atomic<bool>& running_flag)
{
  int current_int = starting_int_;
  size_t counter = 0;
  std::ostringstream oss;

  while (running_flag.load()) {
    TLOG(TLVL_TRACE) << get_name() << ": Start of sleep between sends";
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_between_sends_ms_));
    TLOG(TLVL_TRACE) << get_name() << ": End of do_work loop";
    counter++;
  }
}

} // namespace appfwk
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::appfwk::CardReaderDAQModule)
