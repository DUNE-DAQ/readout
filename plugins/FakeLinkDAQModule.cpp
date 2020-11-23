/**
 * @file FakeLinkDAQModule.cpp FakeLinkDAQModule class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "FakeLinkDAQModule.hpp"
#include "ReadoutIssues.hpp"

#include <sstream>
#include <memory>
#include <string>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "FakeLinkDAQModule" // NOLINT

namespace dunedaq {
namespace readout { 

FakeLinkDAQModule::FakeLinkDAQModule(const std::string& name)
  : DAQModule(name)
{
  register_command("start", &FakeLinkDAQModule::do_start);
  register_command("stop", &FakeLinkDAQModule::do_stop);
}

void
FakeLinkDAQModule::init(const data_t& /*args*/)
{

}

void 
FakeLinkDAQModule::do_start(const data_t& /*args*/)
{

}

void 
FakeLinkDAQModule::do_stop(const data_t& /*args*/)
{
 
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::FakeLinkDAQModule)
