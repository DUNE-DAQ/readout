/**
 * @file FakeLinkDAQModule.hpp FELIX FULLMODE Fake Link
 * Generates user payloads at a given rate, from FELIX binary captures
 * This implementation is purely software based, no FELIX card and tools
 * are needed to use this module.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#ifndef UDAQ_READOUT_SRC_FAKELINKDAQMODULE_HPP_
#define UDAQ_READOUT_SRC_FAKELINKDAQMODULE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/ThreadHelper.hpp"

// 3rd party felix
#include "packetformat/block_parser.hpp"

// module
#include "ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "ReusableThread.hpp"
#include "RateLimiter.hpp"
#include "DefaultParserImpl.hpp"
#include "LatencyBufferInterface.hpp"
#include "WIBLatencyBuffer.hpp"

#include <memory>
#include <fstream>
#include <cstdint>

namespace dunedaq {
namespace readout {

class FakeLinkDAQModule : public dunedaq::appfwk::DAQModule
{
public:
  /**
  * @brief FakeLinkDAQModule Constructor
  * @param name Instance name for this FakeLinkDAQModule instance
  */
  explicit FakeLinkDAQModule(const std::string& name);

  FakeLinkDAQModule(const FakeLinkDAQModule&) =
    delete; ///< FakeLinkDAQModule is not copy-constructible
  FakeLinkDAQModule& operator=(const FakeLinkDAQModule&) =
    delete; ///< FakeLinkDAQModule is not copy-assignable
  FakeLinkDAQModule(FakeLinkDAQModule&&) =
    delete; ///< FakeLinkDAQModule is not move-constructible
  FakeLinkDAQModule& operator=(FakeLinkDAQModule&&) =
    delete; ///< FakeLinkDAQModule is not move-assignable

  void init(const data_t&) override;

private:
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

  // Configuration
  bool configured_;
  int input_limit_;
  int frame_size_;
  int superchunk_factor_;
  int superchunk_size_;
  int element_count_;
  std::string qtype_;
  size_t qsize_;
  int rate_;
  stats::packet_counter_t packet_count_;
  std::string data_filename_;

  // Internals
  std::ifstream rawdata_ifs_;
  std::vector<std::uint8_t> input_buffer_;
  std::unique_ptr<WIBLatencyBuffer> latency_buffer_;
  std::unique_ptr<RateLimiter> rate_limiter_;

  // Processor
  dunedaq::appfwk::ThreadHelper worker_thread_;
  void do_work(std::atomic<bool>& running_flag);

  // Threading
  std::atomic<bool> run_marker_;
  ReusableThread stats_thread_;
  void run_stats();

};

} // namespace dunedaq::readout
}

#endif // UDAQ_READOUT_SRC_LINKREADERDAQMODULE_HPP_
