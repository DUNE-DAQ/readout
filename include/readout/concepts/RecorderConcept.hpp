/**
 * @file RecorderConcept.hpp Recorder concept
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_RECORDERCONCEPT_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_RECORDERCONCEPT_HPP_

#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "opmonlib/InfoCollector.hpp"
#include "readout/ReadoutTypes.hpp"
#include "readout/datarecorder/Structs.hpp"
#include "readout/utils/BufferedFileWriter.hpp"
#include "readout/utils/ReusableThread.hpp"

#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace dunedaq {
namespace readout {
class RecorderConcept
{
public:
  RecorderConcept() {}
  ~RecorderConcept() {}
  RecorderConcept(const RecorderConcept&) = delete;
  RecorderConcept& operator=(const RecorderConcept&) = delete;
  RecorderConcept(RecorderConcept&&) = delete;
  RecorderConcept& operator=(RecorderConcept&&) = delete;

  virtual void init(const nlohmann::json& obj) = 0;
  virtual void get_info(opmonlib::InfoCollector& ci, int level) = 0;

  // Commands
  virtual void do_conf(const nlohmann::json& obj) = 0;
  virtual void do_start(const nlohmann::json& obj) = 0;
  virtual void do_stop(const nlohmann::json& obj) = 0;
};
} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_RECORDERCONCEPT_HPP_