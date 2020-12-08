/**
 * @file ReadoutElement.hpp Generic readout element module
 * Owns appfwk queue references and a ReadoutContext implementation.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTELEMENT_HPP_
#define UDAQ_READOUT_SRC_READOUTELEMENT_HPP_

#include "readout/readoutelement/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "ReadoutContextMaker.hpp"

#include <string>
#include <chrono>
#include <vector>

namespace dunedaq {
namespace readout {

class ReadoutElement : public dunedaq::appfwk::DAQModule
{
public:
  /**
  * @brief ReadoutElement Constructor
  * @param name Instance name for this ReadoutElement instance
  */
  explicit ReadoutElement(const std::string& name);

  ReadoutElement(const ReadoutElement&) =
    delete; ///< ReadoutElement is not copy-constructible
  ReadoutElement& operator=(const ReadoutElement&) =
    delete; ///< ReadoutElement is not copy-assignable
  ReadoutElement(ReadoutElement&&) =
    delete; ///< ReadoutElement is not move-constructible
  ReadoutElement& operator=(ReadoutElement&&) =
    delete; ///< ReadoutElement is not move-assignable

  void init(const data_t& args) override;

private:
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

  // Threading
  dunedaq::appfwk::ThreadHelper worker_thread_;
  void do_work(std::atomic<bool>& run_marker);

  // Configuration
  bool configured_;
  using module_conf_t = readoutelement::Conf;
  module_conf_t cfg_;

  // appfwk Queues
  std::chrono::milliseconds queue_timeout_ms_;
  using source_t = appfwk::DAQSource<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>;
  std::unique_ptr<source_t> input_queue_;

  // Internal
  std::unique_ptr<ReadoutContext<types::WIB_SUPERCHUNK_STRUCT>> readout_context_impl_;

  // Threading
  std::atomic<bool> run_marker_;

  // Stats
  stats::counter_t packet_count_;
  ReusableThread stats_thread_;
  void run_stats();

};

} // namespace dunedaq::readout
}

#endif // UDAQ_READOUT_SRC_READOUTELEMENT_HPP_
