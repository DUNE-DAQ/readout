/**
 * @file ReadoutConcept.hpp ReadoutConcept for constructors and
 * forwarding command args.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_READOUTCONCEPT_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_READOUTCONCEPT_HPP_

#include "opmonlib/InfoCollector.hpp"

namespace dunedaq {
namespace readout {

class ReadoutConcept
{
public:
  ReadoutConcept() {}
  ~ReadoutConcept() {}
  ReadoutConcept(const ReadoutConcept&) = delete;            ///< ReadoutConcept is not copy-constructible
  ReadoutConcept& operator=(const ReadoutConcept&) = delete; ///< ReadoutConcept is not copy-assginable
  ReadoutConcept(ReadoutConcept&&) = delete;                 ///< ReadoutConcept is not move-constructible
  ReadoutConcept& operator=(ReadoutConcept&&) = delete;      ///< ReadoutConcept is not move-assignable

  //! Forward calls from the appfwk
  virtual void init(const nlohmann::json& args) = 0;
  virtual void conf(const nlohmann::json& args) = 0;
  virtual void start(const nlohmann::json& args) = 0;
  virtual void stop(const nlohmann::json& args) = 0;
  virtual void get_info(opmonlib::InfoCollector& ci, int level) = 0;
  virtual void record(const nlohmann::json& args) = 0;

  //! Function that will be run in its own thread to read the raw packets from the queue and add them to the LB
  virtual void run_consume() = 0;
  //! Function that will be run in its own thread and sends periodic timesync messages by pushing them to the queue
  virtual void run_timesync() = 0;
  //! Function that will be run in its own thread and consumes new incoming requests and handles them
  virtual void run_requests() = 0;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_READOUTCONCEPT_HPP_
