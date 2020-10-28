/**
 * @file CardReaderDAQModule.cc CardReaderDAQModule class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CardReaderDAQModule.hpp"
#include "ReadoutIssues.hpp"

#include "flxcard/FlxException.h"

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <TRACE/trace.h>
/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "CardReaderDAQModule" // NOLINT

namespace dunedaq {
namespace readout {

CardReaderDAQModule::CardReaderDAQModule(const std::string& name)
  : DAQModule(name)
  , configured_(false)
  , block_ptr_sinks_{ } 
  , dma_processor_{0}
{
  register_command("configure", &CardReaderDAQModule::do_configure);
  register_command("start", &CardReaderDAQModule::do_start);
  register_command("stop", &CardReaderDAQModule::do_stop);
}

void
CardReaderDAQModule::init()
{
  card_id_ = get_config().value<uint8_t>("card_id", 0);
  card_offset_ = get_config().value<uint8_t>("card_offset", 0);
  dma_id_ = get_config().value<uint8_t>("dma_id", 0);
  numa_id_ = get_config().value<uint8_t>("numa_id", 0);
  num_sources_ = get_config().value<uint8_t>("num_sources", 1);
  num_links_ = get_config().value<uint8_t>("num_links", M_MAX_LINKS_PER_CARD);
  std::vector<std::string> queue_names = get_config()["outputs"].get<std::vector<std::string>>();
  ERS_INFO("Base parameters initialized: " << get_config().dump()); 

  if (queue_names.size() != num_links_) {
    ers::error(readout::ConfigurationError(ERS_HERE, "Number of links does not match number of output queues."));
  } else {
    for (unsigned i=0; i<queue_names.size(); ++i){
     block_ptr_sinks_[i] = std::make_unique<BlockPtrSink>(queue_names[i]);
     ERS_INFO("Added BlockPtr DAQSink for link[" << i << "].");
    }
  }
}

void
CardReaderDAQModule::do_configure(const std::vector<std::string>& /*args*/)
{
  if (configured_) {
    ERS_INFO("Card is already configured! Won't touch it.");
  } else {
    ERS_INFO("Configuring CardReaderDAQModule of card[" << std::to_string(card_id_) << "].");
    // Open card
    openCard();
    ERS_INFO("Card[" << card_id_ << "] opened.");
    // Allocate CMEM
    cmem_handle_ = allocateCMEM(numa_id_, dma_memory_size_, &phys_addr_, &virt_addr_);  
    ERS_INFO("Card[" << card_id_ << "] CMEM memory allocated with " << dma_memory_size_ << " Bytes.");
    // Stop currently running DMA
    stopDMA();
    ERS_INFO("Card[" << card_id_ << "] DMA interactions force stopped.");
    // Init DMA between software and card
    initDMA();
    ERS_INFO("Card[" << card_id_ << "] DMA access initialized.");
    // The rest was some CPU pinning.
    ERS_INFO("Card[" << card_id_ << "] is configured for datataking.");
    configured_=true;
  }
}

void
CardReaderDAQModule::do_start(const std::vector<std::string>& /*args*/)
{
  //thread_.start_working_thread();
  ERS_INFO("Starting CardReader of card " << card_id_ << "...");
  if (!active_.load()) { 
    startDMA();
    setRunning(true);
    dma_processor_.setWork(&CardReaderDAQModule::processDMA, this);
    ERS_INFO("Started CardReader of card " << card_id_ << "...");
  } else {
    ERS_INFO("CardReader of card " << card_id_ << " is already running!");
  }
}

void
CardReaderDAQModule::do_stop(const std::vector<std::string>& /*args*/)
{
  //thread_.stop_working_thread();
  ERS_INFO("Stopping CardReader of card " << card_id_ << "...");
  if (active_.load()) {
    stopDMA();
    setRunning(false);
    while (!dma_processor_.getReadiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ERS_INFO("Stopped CardReader of card " << card_id_ << "!"); 
  } else {
    ERS_INFO("CardReader of card " << card_id_ << " is already stopped!"); 
  }
}

void 
CardReaderDAQModule::setRunning(bool shouldRun) 
{ 
  bool wasRunning = active_.exchange(shouldRun);
  ERS_INFO("Active state was toggled from " << wasRunning << " to " << shouldRun);
}

void 
CardReaderDAQModule::openCard()
{
  ERS_INFO("Opening FELIX card " << card_id_);
  try {
    card_mutex_.lock();
    flx_card_->card_open(static_cast<int>(card_id_), LOCK_NONE);
    card_mutex_.unlock();
  }
  catch(FlxException& ex) {
    ers::error(readout::FelixError(ERS_HERE, ex.what()));
    exit(EXIT_FAILURE);
  }
}

void 
CardReaderDAQModule::closeCard()
{
  ERS_INFO("Closing FELIX card " << card_id_);
  try {
    card_mutex_.lock();
    flx_card_->card_close();
    card_mutex_.unlock();
  }
  catch(FlxException& ex) {
    ers::error(readout::FelixError(ERS_HERE, ex.what()));
    exit(EXIT_FAILURE);
  }
}

int 
CardReaderDAQModule::allocateCMEM(uint8_t numa, u_long bsize, u_long* paddr, u_long* vaddr)
{
  ERS_INFO("Allocating CMEM buffer " << card_id_ << " dma id:" << dma_id_);
  int handle;
  unsigned ret = CMEM_Open();
  if (!ret) {
    ret = CMEM_NumaSegmentAllocate(bsize, numa, const_cast<char*>("FelixRO"), &handle);
    //ret = CMEM_GFPBPASegmentAllocate(bsize, const_cast<char*>("FelixRO"), &handle); 
  }
  if (!ret) {
    ret = CMEM_SegmentPhysicalAddress(handle, paddr);
  }
  if (!ret) {
    ret = CMEM_SegmentVirtualAddress(handle, vaddr);
  }
  if (ret) {
    //rcc_error_print(stdout, ret);
    card_mutex_.lock();
    flx_card_->card_close();
    card_mutex_.unlock();
    ers::error(readout::FelixError(ERS_HERE, 
      "Not enough CMEM memory allocated or the application demands too much CMEM memory.\n"
      "Fix the CMEM memory reservation in the driver or change the module's configuration."));
    exit(EXIT_FAILURE);
  }
  return handle;
}

void 
CardReaderDAQModule::initDMA()
{
  ERS_INFO("InitDMA issued...");
  card_mutex_.lock();

  //flx_card_->irq_disable();
  //std::cout <<"flxCard.irq_disable issued.");

  flx_card_->dma_reset();
  ERS_INFO("flxCard.dma_reset issued.");

  flx_card_->soft_reset();
  ERS_INFO("flxCard.soft_reset issued.");

  //flxCard.irq_disable(ALL_IRQS);
  //DEBUG("flxCard.irq_diable(ALL_IRQS) issued.");

#warning RS: Investigate when did fifo_flush vanished?
  //flx_card_->dma_fifo_flush();
  //DEBUG("flxCard.dma_fifo_flush issued.");

  //flx_card_->irq_enable(IRQ_DATA_AVAILABLE);
  //DEBUG("flxCard.irq_enable(IRQ_DATA_AVAILABLE) issued.");
  card_mutex_.unlock();
  
  current_addr_ = phys_addr_;
  destination_ = phys_addr_;
  read_index_ = 0;
  ERS_INFO("flxCard initDMA done card[" << card_id_ << "]");
}

void 
CardReaderDAQModule::startDMA()
{
  ERS_INFO("Issuing flxCard.dma_to_host for card " << card_id_ << " dma id:" << dma_id_);
  card_mutex_.lock();
  flx_card_->dma_to_host(dma_id_, phys_addr_, dma_memory_size_, FLX_DMA_WRAPAROUND); // FlxCard.h
  card_mutex_.unlock();
} 

void 
CardReaderDAQModule::stopDMA()
{
  ERS_INFO("Issuing flxCard.dma_stop for card " << card_id_ << " dma id:" << dma_id_);
  card_mutex_.lock();
  flx_card_->dma_stop(dma_id_);
  card_mutex_.unlock();
} 

uint64_t 
CardReaderDAQModule::bytesAvailable() 
{ 
  return (current_addr_ - ((read_index_ * M_BLOCK_SIZE) + phys_addr_) + dma_memory_size_) 
          % dma_memory_size_;
}

void 
CardReaderDAQModule::readCurrentAddress() 
{
  card_mutex_.lock();
  current_addr_ = flx_card_->m_bar0->DMA_DESC_STATUS[dma_id_].current_address;
  card_mutex_.unlock();
}


void 
CardReaderDAQModule::processDMA()
{
  ERS_INFO("CardReader starts processing blocks...");
  while (active_) {

    // Loop until read address makes sense
    while((current_addr_ < phys_addr_) || (phys_addr_+dma_memory_size_ < current_addr_))
    {
      if (active_.load()) {
        readCurrentAddress();
        std::this_thread::sleep_for(std::chrono::microseconds(5000)); //cfg.poll_time = 5000
      } else {
        ERS_INFO("Stop issued during poll! Returning...");
        return;
      }
    }
    // Loop while there are not enough data
    while (bytesAvailable() < M_BLOCK_THRESHOLD*M_BLOCK_SIZE)
    {
      if (active_.load()) {
        std::this_thread::sleep_for(std::chrono::microseconds(5000)); // cfg.poll_time = 5000
        readCurrentAddress();
      } else {
        ERS_INFO("Stop issued during poll! Returning...");
        return;
      }
    }

    // Set write index and start DMA advancing
    u_long write_index = (current_addr_ - phys_addr_) / M_BLOCK_SIZE;
    uint64_t bytes = 0;
    while (read_index_ != write_index) {
      uint64_t from_address = virt_addr_ + (read_index_ * M_BLOCK_SIZE);

      // Interpret block
      const felix::packetformat::block* block = const_cast<felix::packetformat::block*>(
        felix::packetformat::block_from_bytes(reinterpret_cast<const char*>(from_address))
      );

      // Get ELink ID
      unsigned block_elink_to_id = static_cast<unsigned>(block->elink)/64;
      //ERS_INFO("BLOCK ELINK: " << block_elink);

#warning RS: Add here the proper error handling via appfwk
      // Queue block pointer for processing
      block_ptr_sinks_[block_elink_to_id]->push(from_address);

      //if ( !block_ptr_queues_[block_elink_to_id]->push(from_address) ) {
      //  elink_metrics_[block_elink]++;
      //  ERROR("Could not queue in block for elink[" << block_elink << "]");
      //}

      // Advance
      read_index_ = (read_index_ + 1) % (dma_memory_size_ / M_BLOCK_SIZE);
      bytes += M_BLOCK_SIZE; 
    }      

    // here check if we can move the read pointer in the circular buffer
    destination_ = phys_addr_ + (write_index * M_BLOCK_SIZE) - (M_MARGIN_BLOCKS * M_BLOCK_SIZE);
    if (destination_ < phys_addr_) {
      destination_ += dma_memory_size_;
    }

    // Finally, set new pointer
    card_mutex_.lock();
    flx_card_->dma_set_ptr(dma_id_, destination_);
    card_mutex_.unlock();

  }
  ERS_INFO("CardReader processor thread finished.");
}



} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::CardReaderDAQModule)
