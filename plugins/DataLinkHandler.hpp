/**
 * @file DataLinkHandler.hpp Generic readout module
 * Owns appfwk queue references and a DataLinkHandlerConcept implementation.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#ifndef UDAQ_READOUT_SRC_DATALINKHANDLER_HPP_
#define UDAQ_READOUT_SRC_DATALINKHANDLER_HPP_

#include "readout/datalinkhandler/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "CreateReadout.hpp"

#include <string>
#include <chrono>
#include <vector>

namespace dunedaq {
namespace readout {

class DataLinkHandler : public dunedaq::appfwk::DAQModule
{
public:
  /**
  * @brief DataLinkHandler Constructor
  * @param name Instance name for this DataLinkHandler instance
  */
  explicit DataLinkHandler(const std::string& name);

  DataLinkHandler(const DataLinkHandler&) =
    delete; ///< DataLinkHandler is not copy-constructible
  DataLinkHandler& operator=(const DataLinkHandler&) =
    delete; ///< DataLinkHandler is not copy-assignable
  DataLinkHandler(DataLinkHandler&&) =
    delete; ///< DataLinkHandler is not move-constructible
  DataLinkHandler& operator=(DataLinkHandler&&) =
    delete; ///< DataLinkHandler is not move-assignable

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
  using module_conf_t = datalinkhandler::Conf;
  module_conf_t cfg_;

  // appfwk Queues
  //std::chrono::milliseconds queue_timeout_ms_;
  //using source_t = appfwk::DAQSource<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>;
  //std::unique_ptr<source_t> input_queue_;

  // Internal
  std::unique_ptr<ReadoutConcept> readout_impl_;

  // Threading
  std::atomic<bool> run_marker_;

  // Stats
  //stats::counter_t packet_count_;
  //ReusableThread stats_thread_;
  //void run_stats();

};

} // namespace dunedaq::readout
}

#endif // UDAQ_READOUT_SRC_DATALINKHANDLER_HPP_
