/**
 * @file FakeElinkReader.cpp FakeElinkReader class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/

#include "readout/fakeelinkreader/Nljs.hpp"

#include "FakeElinkReader.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutConstants.hpp"

#include "appfwk/cmd/Nljs.hpp"

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
#define TRACE_NAME "FakeElinkReader" // NOLINT

namespace dunedaq {
namespace readout { 

FakeElinkReader::FakeElinkReader(const std::string& name)
  : DAQModule(name)
  , configured_(false)
  , output_queue_(nullptr)
  , rate_limiter_(nullptr)
  , worker_thread_(std::bind(&FakeElinkReader::do_work, this, std::placeholders::_1))
  , run_marker_{false}
  , packet_count_{0}
  , stats_thread_(0)
{
  register_command("conf", &FakeElinkReader::do_conf);
  register_command("start", &FakeElinkReader::do_start);
  register_command("stop", &FakeElinkReader::do_stop);
}

void
FakeElinkReader::init(const data_t& args)
{
  ERS_INFO("Resetting queue fakelink-out");
  auto ini = args.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.dir != "output") {
      continue;
    }
    try {
      output_queue_.reset(new appfwk::DAQSink<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>(qi.inst));
    }
    catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, get_name(), qi.name, excpt);
    }
  }
}

void 
FakeElinkReader::do_conf(const data_t& args)
{
  if (configured_) {
    ers::error(ConfigurationError(ERS_HERE, "This module is already configured!"));
  } else {
    cfg_ = args.get<fakeelinkreader::Conf>();

    // Read input
    source_buffer_ = std::make_unique<FileSourceBuffer>(cfg_.input_limit, constant::WIB_SUPERCHUNK_SIZE);
    source_buffer_->read(cfg_.data_filename);

    // Prepare ratelimiter
    rate_limiter_ = std::make_unique<RateLimiter>(cfg_.rate_khz);

    // Mark configured
    configured_ = true;
  }
}

void 
FakeElinkReader::do_start(const data_t& /*args*/)
{
  run_marker_.store(true);
  stats_thread_.set_work(&FakeElinkReader::run_stats, this);
  worker_thread_.start_working_thread();
}

void 
FakeElinkReader::do_stop(const data_t& /*args*/)
{
  run_marker_.store(false);
  worker_thread_.stop_working_thread();
  while (!stats_thread_.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));            
  }
}

void 
FakeElinkReader::do_work(std::atomic<bool>& running_flag)
{
  // Init ratelimiter, element offset and source buffer ref
  rate_limiter_->init();
  int offset = 0;
  const auto& source = source_buffer_->get();

  // Run until stop marker
  while (running_flag.load()) {
    // Which element to push to the buffer
    if (offset == source_buffer_->num_elements()) {
      offset = 0;
    } else {
      offset++;
    }
    // Create next superchunk
    std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT> payload_ptr = std::make_unique<types::WIB_SUPERCHUNK_STRUCT>();
    // Memcpy from file buffer to flat char array
    ::memcpy(&payload_ptr->data, source.data()+offset*constant::WIB_SUPERCHUNK_SIZE, constant::WIB_SUPERCHUNK_SIZE);
    // queue in to actual DAQSink
    try {
      output_queue_->push(std::move(payload_ptr), queue_timeout_ms_);
    } catch (...) { // RS TODO: ERS issues
      std::runtime_error("Queue timed out...");
    }
    // Count packet and limit rate if needed.
    ++packet_count_;
    rate_limiter_->limit();
  }
}

void
FakeElinkReader::run_stats()
{
  // Temporarily, for debugging, a rate checker thread...
  int new_packets = 0;
  auto t0 = std::chrono::high_resolution_clock::now();
  while (run_marker_.load()) {
    auto now = std::chrono::high_resolution_clock::now();
    new_packets = packet_count_.exchange(0);
    double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
    ERS_INFO("Packet rate: " << new_packets/seconds/1000. << " [kHz]");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    t0 = now;
  }
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::FakeElinkReader)
