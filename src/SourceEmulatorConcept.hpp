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
  : m_sink_is_set(false) 
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
  virtual void get_info(opmonlib::InfoCollector & /*ci*/, int /*level*/) = 0;

protected:
  virtual void run_adjust() = 0;
  virtual void run_produce() = 0;

  bool m_sink_is_set;
  using module_conf_t = dunedaq::readout::fakecardreader::Conf;
  module_conf_t m_conf;

  std::unique_ptr<RateLimiter> m_rate_limiter;
  std::unique_ptr<RandomEngine> m_random_engine;
  std::unique_ptr<FileSourceBuffer> m_file_source;

  std::vector<double> m_random_rate_population;
  std::vector<int> m_random_size_population; // NOLINT

  std::thread m_producer_thread;
  std::thread m_adjuster_thread;

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SOURCEEMULATORCONCEPT_HPP_
