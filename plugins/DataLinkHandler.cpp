/**
 * @file DataLinkHandler.cpp DataLinkHandler class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/datalinkhandler/Nljs.hpp"

#include "DataLinkHandler.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include <sstream>
#include <memory>
#include <string>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "DataLinkHandler" // NOLINT

namespace dunedaq {
namespace readout { 

DataLinkHandler::DataLinkHandler(const std::string& name)
  : DAQModule(name)
  , configured_(false)
  , readout_impl_(nullptr)
  , run_marker_{false}
{
  register_command("conf", &DataLinkHandler::do_conf);
  register_command("start", &DataLinkHandler::do_start);
  register_command("stop", &DataLinkHandler::do_stop);
}

void
DataLinkHandler::init(const data_t& args)
{
  ERS_INFO("Initialiyze readout implementation...");
  readout_impl_ = createReadout(args, run_marker_);
  if (readout_impl_ == nullptr) {
    throw std::runtime_error("Readout implementation creation failed...");
  }
}

void
DataLinkHandler::do_conf(const data_t& args)
{
  ERS_INFO("Configure readout implementation...");
  readout_impl_->conf(args);
}

void 
DataLinkHandler::do_start(const data_t& args)
{
  ERS_INFO("Start readout implementeation...");
  run_marker_.store(true);
  readout_impl_->start(args);
}

void 
DataLinkHandler::do_stop(const data_t& args)
{
  ERS_INFO("Stop readout implementation...");
  run_marker_.store(false);
  readout_impl_->stop(args);
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DataLinkHandler)
