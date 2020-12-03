/**
 * @file ReadoutElement.cpp ReadoutElement class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "ReadoutElement.hpp"

#include <sstream>
#include <memory>
#include <string>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "ReadoutElement" // NOLINT

namespace dunedaq {
namespace readout { 

ReadoutElement::ReadoutElement(const std::string& name)
  : DAQModule(name)
//  , queue_timeout_ms_(0)
  , input_queue_(nullptr)
  , configured_(false)
  , run_marker_{false}
{
  register_command("conf", &ReadoutElement::do_conf);
  register_command("start", &ReadoutElement::do_start);
  register_command("stop", &ReadoutElement::do_stop);
}

void
ReadoutElement::init(const data_t& /*args*/)
{

}

void
ReadoutElement::do_conf(const data_t& args)
{
  queue_timeout_ms_ = std::chrono::milliseconds(1000);
  std::string rawtype("wib");
  readout_context_impl_ = ReadoutContextMaker(rawtype, run_marker_);
  readout_context_impl_->conf(args);
}

void 
ReadoutElement::do_start(const data_t& args)
{
  run_marker_.store(true);
  readout_context_impl_->start(args);
}

void 
ReadoutElement::do_stop(const data_t& args)
{
  run_marker_.store(false);
  readout_context_impl_->stop(args);
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::ReadoutElement)
