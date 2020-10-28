/**
 * @file LinkReaderDAQModule.cpp LinkReaderDAQModule class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "LinkReaderDAQModule.hpp"
#include "ReadoutIssues.hpp"

#include <sstream>
#include <memory>
#include <string>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "LinkReaderDAQModule" // NOLINT

namespace dunedaq {
namespace readout { 

LinkReaderDAQModule::LinkReaderDAQModule(const std::string& name)
  : DAQModule(name)
  , configured_(false)
  , link_id_(0)
  , link_tag_(0)
  , parser_type_("")
  , id_str_("")
  , block_ptr_source_(nullptr)
  , parser_sink_(nullptr)
  , source_timeout_(100)
  , link_processor_{0}
  , parser_impl_()
  , parser_(parser_impl_)
{
  register_command("start", &LinkReaderDAQModule::do_start);
  register_command("stop", &LinkReaderDAQModule::do_stop);
}

void
LinkReaderDAQModule::init()
{
  link_id_ = get_config().value<unsigned>("link_id", 0);
  link_tag_ = get_config().value<unsigned>("link_tag", 0);
  parser_impl_.set_ids(link_id_, link_tag_);
  parser_type_ = get_config().value<std::string>("parser_type", "frame"); 

  std::ostringstream osstr;
  osstr << "LinkReader[" << link_id_ << "," << link_tag_ << ',' << parser_type_ << ']';
  id_str_ = osstr.str(); 
  configured_ = true;
  try
  {
    block_ptr_source_ = std::make_unique<BlockPtrSource>(get_config()["input"].get<std::string>());
  } 
  catch (const ers::Issue& excpt)
  {
    throw ResourceQueueError(ERS_HERE, get_name(), "input", excpt);
  }
  try 
  {
    parser_sink_ = std::make_unique<FrameSink>(get_config()["output"].get<std::string>());
  }
  catch (const ers::Issue& excpt)
  {
    throw ResourceQueueError(ERS_HERE, get_name(), "output", excpt);
  }
}

void 
LinkReaderDAQModule::do_start(const std::vector<std::string>& /*args*/)
{
  if (!active_.load()) {
    link_processor_.setWork(&LinkReaderDAQModule::processLink, this);
    active_.store(true);
    ERS_INFO(id_str_ << " started.");
  } else {
    ERS_INFO(id_str_ << " is already running!");
  }
}

void 
LinkReaderDAQModule::do_stop(const std::vector<std::string>& /*args*/)
{
  if (active_.load()) {
    while(!link_processor_.getReadiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    active_.store(false);
    ERS_INFO(id_str_ << " stopped.");
  } else {
    ERS_INFO(id_str_ << " already stopped!");
  }
}

void 
LinkReaderDAQModule::processLink()
{
  ERS_INFO(id_str_ << " spawning processLink()...");
  int expected = -1;
  while (active_) {
    uint64_t block_addr; // NOLINT
    try
    {
      block_ptr_source_->pop(block_addr, source_timeout_);
    }
    catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
    {
      // it is perfectly reasonable that there might be no data in the queue 
      // some fraction of the times that we check, so we just continue on and try again
      continue;
    }
    const felix::packetformat::block* block = const_cast<felix::packetformat::block*>(
      felix::packetformat::block_from_bytes(reinterpret_cast<const char*>(block_addr)) // NOLINT
    );
    if (expected >= 0) {
      if (expected != block->seqnr) {
        // Not expected sequence number! Expected is expected, but got block->seqnr
      }
    }
    expected = (block->seqnr + 1) % 32;
    parser_.process(block);
  }
}

}
} // namespace dunedaq::readout

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::LinkReaderDAQModule)
