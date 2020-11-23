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

// 3rd party felix
#include "packetformat/block_parser.hpp"

// module
#include "ReadoutTypes.hpp"
#include "ReusableThread.hpp"
#include "DefaultParserImpl.hpp"
#include "LatencyBuffer.hpp"

#include <string>
#include <chrono>
#include <vector>

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
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

};

} // namespace dunedaq::readout
}

#endif // UDAQ_READOUT_SRC_LINKREADERDAQMODULE_HPP_
