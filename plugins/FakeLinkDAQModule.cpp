/**
 * @file FakeLinkDAQModule.cpp FakeLinkDAQModule class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "FakeLinkDAQModule.hpp"
#include "ReadoutIssues.hpp"

#include <fstream>
#include <limits>
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
  , configured_(false)
  , frame_size_(0)
  , superchunk_factor_(0)
  , qtype_("")
  , qsize_(0)
  , data_filename_("")
  , rthread_(0)
  , latency_buffer_(nullptr)
{
  register_command("conf", &FakeLinkDAQModule::do_conf);
  register_command("start", &FakeLinkDAQModule::do_start);
  register_command("stop", &FakeLinkDAQModule::do_stop);
}

void
FakeLinkDAQModule::init(const data_t& /*args*/)
{

}

void 
FakeLinkDAQModule::do_conf(const data_t& /*args*/)
{
  if (configured_) {
    ers::error(ConfigurationError(ERS_HERE, "This module is already configured!"));
  } else {
    frame_size_ = 464;
    superchunk_factor_ = 12;
    qtype_ = std::string("WIBFrame");
    qsize_ = 1000000;
  
    data_filename_ = std::string("/tmp/frames.bin");
    rawdata_ifs_.open(data_filename_, std::ios::in | std::ios::binary);
    if (!rawdata_ifs_) {
      ers::error(ConfigurationError(ERS_HERE, "Couldn't open binary file."));
    }
    rawdata_ifs_.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize filesize = rawdata_ifs_.gcount();
    ERS_INFO("Available bytes in the input binary file: " << filesize);
  
    input_buffer_ = std::make_unique<WIBLatencyBuffer>(qsize_);
    latency_buffer_ = std::make_unique<WIBLatencyBuffer>(qsize_);
  
    configured_ = true;
  }
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
