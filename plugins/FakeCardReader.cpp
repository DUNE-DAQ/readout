/**
 * @file FakeCardReader.cpp FakeCardReader class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/fakecardreader/Nljs.hpp"
#include "readout/fakecardreaderinfo/Nljs.hpp"
#include "readout/ReadoutLogging.hpp"

#include "FakeCardReader.hpp"
#include "ReadoutIssues.hpp"
#include "ReadoutConstants.hpp"

//#include "dataformats/wib/WIBFrame.hpp"         // FIXME move to dataformats repo
#include "logging/Logging.hpp"
#include "dataformats/wib/WIBFrame.hpp"                   // FIXME now using local copy
#include "readout/RawWIBTp.hpp"                   // FIXME now using local copy
#include "appfwk/app/Nljs.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <utility>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout { 

FakeCardReader::FakeCardReader(const std::string& name)
  : DAQModule(name)
  , m_configured(false)
  , m_run_marker{false}
  , m_packet_count{0}
  , m_packet_count_tot{0}
  , m_stat_packet_count{0}
  , m_stats_thread(0)
{
  register_command("conf", &FakeCardReader::do_conf);
  register_command("scrap", &FakeCardReader::do_scrap);
  register_command("start", &FakeCardReader::do_start);
  register_command("stop", &FakeCardReader::do_stop);
}

void
FakeCardReader::init(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  auto ini = args.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.dir != "output") {
      continue;
    }

    try {
      TLOG_DEBUG(TLVL_WORK_STEPS) << get_name() << ": Setting up queue: " << qi.inst;
      auto& name = qi.name;
      if (name.find("tp_") != std::string::npos) {
        m_tp_output_queues.emplace_back(new appfwk::DAQSink<std::unique_ptr<types::RAW_WIB_TP_STRUCT>>(qi.inst));
      } else {
        m_output_queues.emplace_back(new appfwk::DAQSink<types::WIB_SUPERCHUNK_STRUCT>(qi.inst));
      }
    }
    catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, get_name(), qi.name, excpt);
    }
  }
  TLOG_DEBUG(TLVL_BOOKKEEPING) << "Number of WIB output queues: " << m_output_queues.size() 
                               << "; Number of raw WIB TP output queues: " << m_tp_output_queues.size();
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
FakeCardReader::get_info(opmonlib::InfoCollector& ci, int /*level*/) {
  fakecardreaderinfo::Info fcr;

  fcr.packets = m_packet_count_tot.load();
  fcr.new_packets = m_packet_count.exchange(0);
  ci.add(fcr);
}

void 
FakeCardReader::do_conf(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_conf() method";

  if (m_configured) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "This module is already configured!";
  } else {
    m_cfg = args.get<fakecardreader::Conf>();

    // Read input
    m_source_buffer = std::make_unique<FileSourceBuffer>(m_cfg.input_limit, constant::WIB_SUPERCHUNK_SIZE);
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Reading binary file: " << m_cfg.data_filename;
    try {
      m_source_buffer->read(m_cfg.data_filename);
    }
    catch (const ers::Issue& ex) {
      ers::fatal(ex);
      throw ConfigurationError(ERS_HERE, "", ex);
    }

    if (m_cfg.tp_enabled == "true") {
      m_tp_source_buffer = std::make_unique<FileSourceBuffer>(m_cfg.input_limit, constant::RAW_WIB_TP_SUBFRAME_SIZE);
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Reading binary file: " << m_cfg.tp_data_filename;
      try {
        m_tp_source_buffer->read(m_cfg.tp_data_filename);
      } 
      catch (const ers::Issue& ex) {
        ers::fatal(ex);
        throw ConfigurationError(ERS_HERE, "", ex);
      }
    }

    // Mark configured
    m_configured = true;
    TLOG_DEBUG(TLVL_WORK_STEPS) << "This module is successfully configured!";
  }
 
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_conf() method";
}

void
FakeCardReader::do_scrap(const data_t& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";

  m_configured = false;

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}
void 
FakeCardReader::do_start(const data_t& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";

  m_run_marker.store(true);
  m_packet_count = 0;
  m_packet_count_tot = 0;
  m_stat_packet_count = 0;
  m_stats_thread.set_work(&FakeCardReader::run_stats, this);

  if(m_source_buffer->num_elements() == 0) {
    ers::fatal(EmptySourceBuffer(ERS_HERE, m_cfg.data_filename));
  } else {
    int idx=0;
    for (auto my_queue : m_output_queues) {
      m_worker_threads.emplace_back(&FakeCardReader::generate_data, this, my_queue, m_cfg.link_ids[idx]);
      ++idx;
    }
    if (m_cfg.tp_enabled == "true") {
      for (auto my_queue : m_tp_output_queues) {
        m_worker_threads.emplace_back(&FakeCardReader::generate_tp_data, this, my_queue, m_cfg.link_ids[idx]);
      }
      ++idx;
    }
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void 
FakeCardReader::do_stop(const data_t& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method"; 

  m_run_marker.store(false);
  for (auto& work_thread : m_worker_threads) {
    work_thread.join();
  }
  m_worker_threads.clear();
  while (!m_stats_thread.get_readiness()) {	
    std::this_thread::sleep_for(std::chrono::milliseconds(100));            	
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method"; 
}

void 
FakeCardReader::generate_data(appfwk::DAQSink<types::WIB_SUPERCHUNK_STRUCT>* myqueue, int linkid) 
{
  TLOG_DEBUG(TLVL_WORK_STEPS) << "WIB data generation thread " << linkid << " started";

  pthread_setname_np(pthread_self(), get_name().c_str());
  // Init ratelimiter, element offset and source buffer ref
  dunedaq::readout::RateLimiter rate_limiter(m_cfg.rate_khz);
  rate_limiter.init();
  int offset = 0;
  auto& source = m_source_buffer->get();

  // This should be changed in case of a generic Fake ELink reader (exercise with TPs dumps)
  int num_elem = m_source_buffer->num_elements();
  if (num_elem == 0) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "No WIB elements to read from buffer! Sleeping...";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    num_elem = m_source_buffer->num_elements();
  }
  auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(source.data()); // NOLINT
  TLOG_DEBUG(TLVL_BOOKKEEPING) << "Number of elements to read from buffer: " << num_elem
                               << "; wfptr is: " << wfptr; 

  // set the initial timestamp to a configured value, otherwise just use the timestamp from the wib header
  uint64_t ts_0 = (m_cfg.set_t0_to >= 0) ? m_cfg.set_t0_to : wfptr->get_wib_header()->get_timestamp(); // NOLINT
  TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp in the WIB source file: " << ts_0
                               << "; linkid is: " << linkid;
  uint64_t ts_next = ts_0; // NOLINT

  // Run until stop marker
  
  while (m_run_marker.load()) {
      // Which element to push to the buffer
      if (offset == num_elem) {
        offset = 0;
      } 
      // Create next superchunk
      //std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT> payload_ptr = std::make_unique<types::WIB_SUPERCHUNK_STRUCT>();
      types::WIB_SUPERCHUNK_STRUCT payload;
      // Memcpy from file buffer to flat char array
      ::memcpy(static_cast<void*>(payload.data),
               static_cast<void*>(source.data() + offset * constant::WIB_SUPERCHUNK_SIZE), 
               constant::WIB_SUPERCHUNK_SIZE);

      // fake the timestamp, act like a real WIB link
      for (unsigned int i=0; i<12; ++i) { // NOLINT
        auto wf = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(((uint8_t*)payload.data)+i*464); // NOLINT
        auto wfh = const_cast<dunedaq::dataformats::WIBHeader*>(wf->get_wib_header()); 
        wfh->set_timestamp(ts_next);
        ts_next += 25;
      } 

      // queue in to actual DAQSink
      try {
        myqueue->push(std::move(payload), m_queue_timeout_ms);
      } catch (ers::Issue &excpt) {
        ers::warning(CannotWriteToQueue(ERS_HERE, "raw data input queue", excpt));
        // std::runtime_error("Queue timed out...");
      }

    // Count packet and limit rate if needed.
    ++offset;
    ++m_packet_count;
    ++m_packet_count_tot;
    ++m_stat_packet_count;
    rate_limiter.limit();
  }
  TLOG_DEBUG(TLVL_WORK_STEPS) << "WIB data generation thread " << linkid << " finished";
}

void
FakeCardReader::generate_tp_data(appfwk::DAQSink<std::unique_ptr<types::RAW_WIB_TP_STRUCT>>* myqueue, int linkid)
{
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Raw WIB TP data generation thread " << linkid << " started";

  pthread_setname_np(pthread_self(), get_name().c_str());
  // Init ratelimiter, element offset and source buffer ref
  dunedaq::readout::RateLimiter rate_limiter(m_cfg.tp_rate_khz);
  rate_limiter.init();
  int offset = 0;
  auto& source = m_tp_source_buffer->get();

  // This should be changed in case of a generic Fake ELink reader (exercise with TPs dumps)
  int num_elem = m_tp_source_buffer->num_elements();
  if (num_elem == 0) {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "No raw WIB TP elements to read from buffer! Sleeping...";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    num_elem = m_tp_source_buffer->num_elements();
  }
  auto rwtpptr = reinterpret_cast<dunedaq::dataformats::RawWIBTp*>(source.data()); // NOLINT
  TLOG_DEBUG(TLVL_BOOKKEEPING) << "Number of raw WIB TP elements to read from buffer: " << num_elem
                               << "; rwtpptr is: " << rwtpptr;

  uint64_t ts_0 = (m_cfg.set_t0_to >= 0) ? m_cfg.set_t0_to : rwtpptr->get_header()->get_timestamp(); // NOLINT
  TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp in the raw WIB TP source file: " << ts_0
                               << "; linkid is: " << linkid;
  uint64_t ts_next = ts_0; // NOLINT

  dunedaq::dataformats::RawWIBTp* tf{nullptr};
  while (m_run_marker.load()) {
    // Which element to push to the buffer
    if (offset == num_elem * static_cast<int>(constant::RAW_WIB_TP_SUBFRAME_SIZE)) {
      offset = 0;
    }
    // Count number of subframes in a TP frame
    int n = 1;
    while (reinterpret_cast<types::TpSubframe*>(((uint8_t*)source.data()) // NOLINT
           + offset + (n-1)*constant::RAW_WIB_TP_SUBFRAME_SIZE)->word3 != 0xDEADBEEF) {
      n++;
    }
    // Create next TP frame
    std::unique_ptr<types::RAW_WIB_TP_STRUCT> payload_ptr = std::make_unique<types::RAW_WIB_TP_STRUCT>();

    for (int i=0; i<n; ++i) {
     auto* sp = reinterpret_cast<types::TpSubframe*>( // NOLINT
       ((uint8_t*)source.data())+offset+i*constant::RAW_WIB_TP_SUBFRAME_SIZE); // NOLINT 
     if (!m_found_tp_header) {
        tf = new dunedaq::dataformats::RawWIBTp();
        const dunedaq::dataformats::TpHeader* tfh = tf->get_header();
        tfh = reinterpret_cast<dunedaq::dataformats::TpHeader*>(sp); // NOLINT
        tf->set_timestamp(ts_next);
        ts_next += 25;
        m_found_tp_header = true;
        payload_ptr->head = *tfh;
        continue;
      }
      if (sp->word3 == 0xDEADBEEF) {
        const dunedaq::dataformats::TpPedinfo* tpi = tf->get_pedinfo();
        tpi = reinterpret_cast<dunedaq::dataformats::TpPedinfo*>(sp); // NOLINT
        m_found_tp_header = false;
        payload_ptr->ped = *tpi;
        continue;
      }
      dunedaq::dataformats::TpData* td = reinterpret_cast<dunedaq::dataformats::TpData*>(sp); // NOLINT
      payload_ptr->block.set_tp(*td);
    }
 
    // queue in to actual DAQSink
    try {
      myqueue->push(std::move(payload_ptr), m_queue_timeout_ms);
    } catch (const ers::Issue& excpt) {
         ers::warning(CannotWriteToQueue(ERS_HERE, "TP input queue", excpt));
      // std::runtime_error("Queue timed out...");
    }

    // Count packet and limit rate if needed.
    offset += n*constant::RAW_WIB_TP_SUBFRAME_SIZE;
    //if (m_alloc_) { free(m_data_); }
    ++m_packet_count;
    ++m_packet_count_tot;
    ++m_stat_packet_count;
    rate_limiter.limit();
  }
  TLOG_DEBUG(TLVL_WORK_STEPS) << "Raw WIB TP data generation thread " << linkid << " finished";
}

void	
FakeCardReader::run_stats()	
{	
  // Temporarily, for debugging, a rate checker thread...	
  int new_packets = 0;	
  auto t0 = std::chrono::high_resolution_clock::now();	
  while (m_run_marker.load()) {	
    auto now = std::chrono::high_resolution_clock::now();	
    new_packets = m_stat_packet_count.exchange(0);	
    double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;	
    TLOG_DEBUG(TLVL_TAKE_NOTE) << "Produced Packet rate: " << new_packets/seconds/1000. << " [kHz]";
    for(int i=0; i<100 && m_run_marker.load(); ++i){ // 10 seconds sleep	
      std::this_thread::sleep_for(std::chrono::milliseconds(100));	
    }	
    t0 = now;	
  }	
}

} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::FakeCardReader)
