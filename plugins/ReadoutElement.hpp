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

#include "ReadoutTypes.hpp"
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

  void init(const data_t&) override;

private:
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

  // Configuration
  bool configured_;
  using module_conf_t = readoutelement::Conf;
  module_conf_t cfg_;

  // appfwk queues
  types::UniqueWIBFramePtrSource input_queue_;
  std::chrono::milliseconds queue_timeout_ms_;

  // Internal
  std::unique_ptr<ReadoutContextBase> readout_context_impl_;

  // Threading
  std::atomic<bool> run_marker_;

};

} // namespace dunedaq::readout
}

#endif // UDAQ_READOUT_SRC_READOUTELEMENT_HPP_
