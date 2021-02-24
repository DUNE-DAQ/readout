/**
 * @file FakeCardReader.cpp FakeCardReader class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/fakecardreader/Nljs.hpp"
#include "FakeCardReader.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutConstants.hpp"

//#include "dataformats/wib/WIBFrame.hpp"         // FIXME move to dataformats repo
#include "logging/Logging.hpp"
#include "readout/WIBFrame.hpp"                   // FIXME now using local copy
#include "readout/RawWIBTp.hpp"                   // FIXME now using local copy
#include "appfwk/cmd/Nljs.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "FakeCardReader" // NOLINT

/**
* @brief TRACE debug levels used in this source file
 */
enum
{
  TLVL_ENTER_EXIT_METHODS = 5,
  TLVL_WORK_STEPS = 10,
  TLVL_BOOKKEEPING = 15
};

namespace dunedaq {
namespace readout { 

FakeCardReader::FakeCardReader(const std::string& name)
  : DAQModule(name)
  , configured_(false)
  , run_marker_{false}
  , packet_count_{0}
  , stats_thread_(0)
{
  register_command("conf", &FakeCardReader::do_conf);
  register_command("scrap", &FakeCardReader::do_scrap);
  register_command("start", &FakeCardReader::do_start);
  register_command("stop", &FakeCardReader::do_stop);
}

void
FakeCardReader::init(const data_t& args)
{

  auto ini = args.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.dir != "output") {
      continue;
    }

    try {
      TLOG() << "Setting up queue: " << qi.inst;
      if (qi.name == "tp_output") {
        tp_output_queues_.emplace_back(new appfwk::DAQSink<std::unique_ptr<types::RAW_WIB_TP_STRUCT>>(qi.inst));
      } else {
        output_queues_.emplace_back(new appfwk::DAQSink<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>(qi.inst));
      }
    }
    catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, get_name(), qi.name, excpt);
    }
  }
  TLOG() << "Init: # output queues: " << output_queues_.size() << "; # tp_output queues: " << tp_output_queues_.size();
}

void 
FakeCardReader::do_conf(const data_t& args)
{
  if (configured_) {
    ers::error(ConfigurationError(ERS_HERE, "This module is already configured!"));
  } else {
    cfg_ = args.get<fakecardreader::Conf>();

    // Read input
    source_buffer_ = std::make_unique<FileSourceBuffer>(cfg_.input_limit, constant::WIB_SUPERCHUNK_SIZE);
    TLOG() << "Reading binary file: " << cfg_.data_filename;
    try {
      source_buffer_->read(cfg_.data_filename);
    }
    catch (const ers::Issue& ex) {
      ers::fatal(ex);
      throw ConfigurationError(ERS_HERE, "", ex);
    }

    if (cfg_.tp_enabled == "true") {
      tp_source_buffer_ = std::make_unique<FileSourceBuffer>(cfg_.input_limit, constant::RAW_WIB_TP_SUBFRAME_SIZE);
      tp_source_buffer_->read(cfg_.tp_data_filename);
    }

    // Mark configured
    configured_ = true;
  }
}

void
FakeCardReader::do_scrap(const data_t& /*args*/)
{
     configured_ = false;
}
void 
FakeCardReader::do_start(const data_t& /*args*/)
{
  run_marker_.store(true);
  packet_count_ = 0;
  stats_thread_.set_work(&FakeCardReader::run_stats, this);

  if(source_buffer_->num_elements() == 0) {
    ers::fatal(EmptySourceBuffer(ERS_HERE, cfg_.data_filename));
  } else {
    int idx=0;
    for (auto my_queue : output_queues_) {
      worker_threads_.emplace_back(&FakeCardReader::generate_data, this, my_queue, cfg_.link_ids[idx]);
      ++idx;
    }
    if (cfg_.tp_enabled == "true") {
      for (auto my_queue : tp_output_queues_) {
        worker_threads_.emplace_back(&FakeCardReader::generate_tp_data, this, my_queue, cfg_.link_ids[idx]);
      }
      ++idx;
    }
  }

}

void 
FakeCardReader::do_stop(const data_t& /*args*/)
{
  run_marker_.store(false);
  for (auto& work_thread : worker_threads_) {
    work_thread.join();
  }
  
  worker_threads_.clear();

  while (!stats_thread_.get_readiness()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));            
  }
}

void 
FakeCardReader::generate_data(appfwk::DAQSink<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>* myqueue, int linkid) 
{
  pthread_setname_np(pthread_self(), get_name().c_str());
  // Init ratelimiter, element offset and source buffer ref
  dunedaq::readout::RateLimiter rate_limiter(cfg_.rate_khz);
  rate_limiter.init();
  int offset = 0;
  auto& source = source_buffer_->get();

  // This should be changed in case of a generic Fake ELink reader (exercise with TPs dumps)
  int num_elem = source_buffer_->num_elements();
  if (num_elem == 0) {
	  TLOG_DEBUG(2) << "num_elem=0; sleeping...";
	  usleep(100000);
	  num_elem = source_buffer_->num_elements();
  }
  auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(source.data());
  TLOG_DEBUG(2) << "num_elem=" << num_elem << " wfptr=" << wfptr;
  // set the initial timestamp to a configured value, otherwise just use the timestamp from the wib header
  uint64_t ts_0 = (cfg_.set_t0_to >= 0) ? cfg_.set_t0_to : wfptr->get_wib_header()->get_timestamp();
  TLOG() << "First timestamp in the source file: " << ts_0 << "; linkid is: " << linkid;
  uint64_t ts_next = ts_0;

  // Run until stop marker
  
  while (run_marker_.load()) {
      // Which element to push to the buffer
      if (offset == num_elem) {
        offset = 0;
      } 
      // Create next superchunk
      std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT> payload_ptr = std::make_unique<types::WIB_SUPERCHUNK_STRUCT>();
      // Memcpy from file buffer to flat char array
      ::memcpy((void*)&payload_ptr->data, (void*)(source.data()+offset*constant::WIB_SUPERCHUNK_SIZE), constant::WIB_SUPERCHUNK_SIZE);

      // fake the timestamp
      for (unsigned int i=0; i<12; ++i) {
        auto wf = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(((uint8_t*)payload_ptr.get())+i*464);
	auto wfh = const_cast<dunedaq::dataformats::WIBHeader*>(wf->get_wib_header()); 
        wfh->set_timestamp(ts_next);
        ts_next += 25;
      }  

      // queue in to actual DAQSink
      try {
        myqueue->push(std::move(payload_ptr), queue_timeout_ms_);
      } catch (...) { // RS TODO: ERS issues
        std::runtime_error("Queue timed out...");
      }

    // Count packet and limit rate if needed.
    ++offset;
    ++packet_count_;
    rate_limiter.limit();
  }
  TLOG_DEBUG(0) << "Data generation thread " << linkid << " finished";
}

void
FakeCardReader::generate_tp_data(appfwk::DAQSink<std::unique_ptr<types::RAW_WIB_TP_STRUCT>>* myqueue, int linkid)
{
  std::stringstream ss;
  ss << "card-reader-" << linkid;
  pthread_setname_np(pthread_self(), ss.str().c_str());
  // Init ratelimiter, element offset and source buffer ref
  dunedaq::readout::RateLimiter rate_limiter(cfg_.tp_rate_khz);
  rate_limiter.init();
  int offset = 0;
  auto& source = tp_source_buffer_->get();

  // This should be changed in case of a generic Fake ELink reader (exercise with TPs dumps)
  int num_elem = tp_source_buffer_->num_elements();
  uint64_t ts_0 = reinterpret_cast<dunedaq::dataformats::RawWIBTp*>(source.data())->get_header()->timestamp();
  TLOG() << "First timestamp in the source file: " << ts_0 << "; linkid is: " << linkid;
  uint64_t ts_next = ts_0;

  dunedaq::dataformats::RawWIBTp* tf{nullptr};
  while (run_marker_.load()) {
    // Which element to push to the buffer
    if (offset == num_elem * static_cast<int>(constant::RAW_WIB_TP_SUBFRAME_SIZE)) {
      offset = 0;
    }
    // Count number of subframes in a TP frame
    int n = 1;
    while (reinterpret_cast<types::TpSubframe*>(((uint8_t*)source.data())
           + offset + (n-1)*constant::RAW_WIB_TP_SUBFRAME_SIZE)->word3 != 0xDEADBEEF) {
      n++;
    }
    // Create next TP frame
    std::unique_ptr<types::RAW_WIB_TP_STRUCT> payload_ptr = std::make_unique<types::RAW_WIB_TP_STRUCT>();

    for (int i=0; i<n; ++i) {
     auto* sp = reinterpret_cast<types::TpSubframe*>(((uint8_t*)source.data())+offset+i*constant::RAW_WIB_TP_SUBFRAME_SIZE);
     if (!m_found_tp_header) {
        tf = new dunedaq::dataformats::RawWIBTp();
        const dunedaq::dataformats::TpHeader* tfh = tf->get_header();
        tfh = reinterpret_cast<dunedaq::dataformats::TpHeader*>(sp);
        tf->set_timestamp(ts_next);
        ts_next += 25;
        m_found_tp_header = true;
        payload_ptr->head = *tfh;
        continue;
      }
      if (sp->word3 == 0xDEADBEEF) {
        const dunedaq::dataformats::TpPedinfo* tpi = tf->get_pedinfo();
        tpi = reinterpret_cast<dunedaq::dataformats::TpPedinfo*>(sp);
        m_found_tp_header = false;
        payload_ptr->ped = *tpi;
        continue;
      }
      dunedaq::dataformats::TpData* td = reinterpret_cast<dunedaq::dataformats::TpData*>(sp);
      payload_ptr->block.set_tp(*td);
    }
 
    // queue in to actual DAQSink
    try {
      myqueue->push(std::move(payload_ptr), queue_timeout_ms_);
    } catch (...) { // RS TODO: ERS issues
      std::runtime_error("Queue timed out...");
    }

    // Count packet and limit rate if needed.
    offset += n*constant::RAW_WIB_TP_SUBFRAME_SIZE;
    //if (m_alloc_) { free(m_data_); }
    ++packet_count_;
    rate_limiter.limit();
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Data generation thread " << linkid << " finished";
}




void
FakeCardReader::run_stats()
{
  // Temporarily, for debugging, a rate checker thread...
  int new_packets = 0;
  auto t0 = std::chrono::high_resolution_clock::now();
  while (run_marker_.load()) {
    auto now = std::chrono::high_resolution_clock::now();
    new_packets = packet_count_.exchange(0);
    double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
    TLOG() << "Produced Packet rate: " << new_packets/seconds/1000. << " [kHz]";
    for(int i=0; i<100 && run_marker_.load(); ++i){ // 10 seconds sleep
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    t0 = now;
  }
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::FakeCardReader)
