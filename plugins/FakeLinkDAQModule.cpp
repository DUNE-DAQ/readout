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
#include <iomanip>
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
  , rate_(0)
  , packet_count_{0}
  , data_filename_("")
  , latency_buffer_(nullptr)
  , rate_limiter_(nullptr)
  , worker_thread_(std::bind(&FakeLinkDAQModule::do_work, this, std::placeholders::_1))
  , run_marker_{false}
  , stats_thread_(0)
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

    // These should come from args
    input_limit_ = 10*1024*1024;
    frame_size_ = 464;
    superchunk_factor_ = 12;
    superchunk_size_ = frame_size_*superchunk_factor_;
    qtype_ = std::string("WIBFrame");
    qsize_ = 100000;
    rate_ = 166; // 166 khz
    data_filename_ = std::string("/tmp/frames.bin");

    // Open file
    rawdata_ifs_.open(data_filename_, std::ios::in | std::ios::binary);
    if (!rawdata_ifs_) {
      ers::error(ConfigurationError(ERS_HERE, "Couldn't open binary file."));
    }

    // Check file size
    rawdata_ifs_.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize filesize = rawdata_ifs_.gcount();
    if (filesize > input_limit_) { // bigger than configured limit
      ers::error(ConfigurationError(ERS_HERE, "File size limit exceeded."));
    }
    
    // Check for exact match
    int remainder = filesize % superchunk_size_;
    if (remainder > 0) {
      ers::error(ConfigurationError(ERS_HERE, "Binary file contains more data than expected."));
    }

    // Set usable element count
    element_count_ = filesize / superchunk_size_;

    // Read file into input buffer
    rawdata_ifs_.seekg(0, std::ios::beg);
    input_buffer_.reserve(filesize);
    input_buffer_.insert(input_buffer_.begin(), std::istreambuf_iterator<char>(rawdata_ifs_), std::istreambuf_iterator<char>());
    ERS_INFO("Available elements: " << element_count_ << " | In bytes: " << input_buffer_.size());

    // Prepare latency buffer and rate limiter
    latency_buffer_ = std::make_unique<WIBLatencyBuffer>(qsize_);
    rate_limiter_ = std::make_unique<RateLimiter>(rate_);

    // Mark configured
    configured_ = true;
  }
}

void 
FakeLinkDAQModule::do_start(const data_t& /*args*/)
{
  run_marker_.store(true);
  stats_thread_.set_work(&FakeLinkDAQModule::run_stats, this);
  worker_thread_.start_working_thread();
}

void 
FakeLinkDAQModule::do_stop(const data_t& /*args*/)
{
  run_marker_.store(false);
  worker_thread_.stop_working_thread();
  while (!stats_thread_.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));            
  }
}

void 
FakeLinkDAQModule::do_work(std::atomic<bool>& running_flag)
{
  rate_limiter_->init();
  int offset = 0;
  while (running_flag.load()) {
    if (offset == element_count_) {
      offset = 0;
    } else {
      offset++;
    }
    WIB_SUPERCHUNK_STRUCT superchunk;
    ::memcpy(&superchunk.data, input_buffer_.data()+offset*superchunk_size_, superchunk_size_);
    latency_buffer_->write(std::move(superchunk));
    ++packet_count_;
    rate_limiter_->limit();
  }
}

void
FakeLinkDAQModule::run_stats()
{
  int new_packets = 0;
  auto t0 = std::chrono::high_resolution_clock::now();
  while (run_marker_.load()) {
    auto now = std::chrono::high_resolution_clock::now();
    new_packets = packet_count_.exchange(0);
    double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
    ERS_INFO("Packet rate: " << new_packets/seconds/1000. << " [kHz] "
          << "Latency buffer size: " << latency_buffer_->sizeGuess());
    std::this_thread::sleep_for(std::chrono::seconds(5));
    t0 = now;
  }
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::FakeLinkDAQModule)
