/**
* @file SourceEmulatorConcept.hpp SourceEmulatorConcept for constructors and
* forwarding command args.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_SOURCEEMULATORCONCEPT_HPP_
#define READOUT_SRC_SOURCEEMULATORCONCEPT_HPP_

#include "opmonlib/InfoCollector.hpp"
#include "readout/fakecardreaderinfo/Nljs.hpp"

#include "RateLimiter.hpp"
#include "RandomEngine.hpp"
#include "FileSourceBuffer.hpp"

#include <thread>
#include <map>

namespace dunedaq {
namespace readout {

class SourceEmulatorConcept {
public:
  SourceEmulatorConcept() 
  { }

  ~SourceEmulatorConcept() {}
  SourceEmulatorConcept(const SourceEmulatorConcept&)
    = delete; ///< SourceEmulatorConcept is not copy-constructible
  SourceEmulatorConcept& operator=(const SourceEmulatorConcept&)
    = delete; ///< SourceEmulatorConcept is not copy-assginable
  SourceEmulatorConcept(SourceEmulatorConcept&&)
    = delete; ///< SourceEmulatorConcept is not move-constructible
  SourceEmulatorConcept& operator=(SourceEmulatorConcept&&)
    = delete; ///< SourceEmulatorConcept is not move-assignable

  virtual void init(const nlohmann::json& /*args*/) = 0;
  virtual void set_sink(const std::string& /*sink_name*/) = 0;
  virtual void conf(const nlohmann::json& /*args*/) = 0;
  virtual void start(const nlohmann::json& /*args*/) = 0;
  virtual void stop(const nlohmann::json& /*args*/) = 0;
  virtual void get_info(fakecardreaderinfo::Info fcr) = 0;

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SOURCEEMULATORCONCEPT_HPP_
