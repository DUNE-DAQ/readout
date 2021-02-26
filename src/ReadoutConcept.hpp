/**
* @file ReadoutConcept.hpp ReadoutConcept for constructors and
* forwarding command args.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_READOUTCONCEPT_HPP_
#define READOUT_SRC_READOUTCONCEPT_HPP_

#include "opmonlib/InfoCollector.hpp"
#include <string>

namespace dunedaq {
namespace readout {

class ReadoutConcept {
public:
  ReadoutConcept() {}
  ~ReadoutConcept() {}
  ReadoutConcept(const ReadoutConcept&)
    = delete; ///< ReadoutConcept is not copy-constructible
  ReadoutConcept& operator=(const ReadoutConcept&)
    = delete; ///< ReadoutConcept is not copy-assginable
  ReadoutConcept(ReadoutConcept&&)
    = delete; ///< ReadoutConcept is not move-constructible
  ReadoutConcept& operator=(ReadoutConcept&&)
    = delete; ///< ReadoutConcept is not move-assignable

  virtual void init(const nlohmann::json& args, const std::string& raw_type_name) = 0;
  virtual void conf(const nlohmann::json& args) = 0;
  virtual void start(const nlohmann::json& args) = 0;
  virtual void stop(const nlohmann::json& args) = 0;
  virtual void get_info(opmonlib::InfoCollector & ci, int level) = 0;

  virtual void run_consume() = 0;
  virtual void run_timesync() = 0;
  virtual void run_requests() = 0;

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_READOUTCONCEPT_HPP_
