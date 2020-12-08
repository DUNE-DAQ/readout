/**
 * @file FakeCardReader.hpp FELIX FULLMODE Fake Links
 * Generates user payloads at a given rate, from WIB to FELIX binary captures
 * This implementation is purely software based, no FELIX card and tools
 * are needed to use this module.
 *
 * TODO: Make it generic, to consumer other types of user payloads.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#ifndef UDAQ_READOUT_SRC_FAKECARDREADER_HPP_
#define UDAQ_READOUT_SRC_FAKECARDREADER_HPP_

#include "readout/fakecardreader/Structs.hpp"

// appfwk
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

// package
#include "ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "ReusableThread.hpp"
#include "RateLimiter.hpp"
#include "FileSourceBuffer.hpp"

// std
#include <memory>
#include <fstream>
#include <cstdint>

namespace dunedaq {
namespace readout {

class FakeCardReader : public dunedaq::appfwk::DAQModule
{
public:
  /**
  * @brief FakeCardReader Constructor
  * @param name Instance name for this FakeCardReader instance
  */
  explicit FakeCardReader(const std::string& name);

  FakeCardReader(const FakeCardReader&) =
    delete; ///< FakeCardReader is not copy-constructible
  FakeCardReader& operator=(const FakeCardReader&) =
    delete; ///< FakeCardReader is not copy-assignable
  FakeCardReader(FakeCardReader&&) =
    delete; ///< FakeCardReader is not move-constructible
  FakeCardReader& operator=(FakeCardReader&&) =
    delete; ///< FakeCardReader is not move-assignable

  void init(const data_t&) override;

private:
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

  // Configuration
  bool configured_;
  using module_conf_t = fakecardreader::Conf;
  module_conf_t cfg_;

  // appfwk Queues
  std::chrono::milliseconds queue_timeout_ms_;
  using sink_t = appfwk::DAQSink<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>;
  std::unique_ptr<sink_t> output_queue_;

  // Internals
  std::unique_ptr<FileSourceBuffer> source_buffer_;
  std::unique_ptr<RateLimiter> rate_limiter_;

  // Processor
  dunedaq::appfwk::ThreadHelper worker_thread_;
  void do_work(std::atomic<bool>& running_flag);

  // Threading
  std::atomic<bool> run_marker_;

  // Stats
  stats::counter_t packet_count_;
  ReusableThread stats_thread_;
  void run_stats();

};

} // namespace dunedaq::readout
}

#endif // UDAQ_READOUT_SRC_FAKECARDREADER_HPP_
