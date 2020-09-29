/**
 * @file CardReaderDAQModule.hpp
 *
 * CardReaderDAQModule creates vectors of integers of a given length, starting with the given start integer and
 * counting up to the given ending integer. Its current position is persisted between generated vectors, so if the
 * parameters are chosen correctly, the generated vectors will "walk" through the valid range.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef APPFWK_UDAQ_READOUT_CARDREADERDAQMODULE_HPP_
#define APPFWK_UDAQ_READOUT_CARDREADERDAQMODULE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

// Module internals
#include "ReusableThread.hpp"

// FELIX Software Suite provided
#include "flxcard/FlxCard.h"
#include "packetformat/block_format.hpp"

#include <future>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace appfwk {
namespace readout {
/**
 * @brief CardReaderDAQModule reads FELIX DMA block pointers
 *
 * @author Roland Sipos
 * @date   2020-2021
 *
 */
class CardReaderDAQModule : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief CardReaderDAQModule Constructor
   * @param name Instance name for this CardReaderDAQModule instance
   */
  explicit CardReaderDAQModule(const std::string& name);

  CardReaderDAQModule(const CardReaderDAQModule&) =
    delete; ///< CardReaderDAQModule is not copy-constructible
  CardReaderDAQModule& operator=(const CardReaderDAQModule&) =
    delete; ///< CardReaderDAQModule is not copy-assignable
  CardReaderDAQModule(CardReaderDAQModule&&) =
    delete; ///< CardReaderDAQModule is not move-constructible
  CardReaderDAQModule& operator=(CardReaderDAQModule&&) =
    delete; ///< CardReaderDAQModule is not move-assignable

  void init() override;

private:
  // Commands
  void do_configure(const std::vector<std::string>& args);
  void do_start(const std::vector<std::string>& args);
  void do_stop(const std::vector<std::string>& args);

  // Configuration
  bool configured_;
  typedef std::unique_ptr<DAQSink<uint64_t>> UniqueBlockPtrQueue;
  std::map<uint8_t, UniqueBlockPtrQueue> block_ptr_queues_;

  // Card control
  typedef std::unique_ptr<FlxCard> UniqueFlxCard;
  UniqueFlxCard flx_card_;
  std::mutex card_mutex_;

  // Constants
  static constexpr size_t M_MAX_LINKS_PER_CARD=6;
  static constexpr size_t M_MARGIN_BLOCKS=4;
  static constexpr size_t M_BLOCK_THRESHOLD=256;
  static constexpr size_t M_BLOCK_SIZE=felix::packetformat::BLOCKSIZE; 

  // Internals
  uint8_t card_id_;
  uint8_t card_offset_;
  uint8_t dma_id_;
  uint8_t numa_id_;
  uint8_t num_sources_;
  uint8_t num_links_;
  std::string info_str_;

  // CMEM
  uint64_t dma_memory_size_; // size of CMEM (driver) memory to allocate
  int cmem_handle_;         // handle to the DMA memory block
  uint64_t virt_addr_;      // virtual address of the DMA memory block
  uint64_t phys_addr_;      // physical address of the DMA memory block
  uint64_t current_addr_;   // pointer to the current write position for the card
  unsigned read_index_;
  u_long destination_;      // FlxCard.h

  // Processor
  inline static const std::string dma_processor_name_ = "flx-dma";
  std::atomic<bool> run_lock_{false};
  std::atomic<bool> active_{false};
  ReusableThread dma_processor_; //
  void processDMA();

  // Functionalities
  void setRunning(bool running);
  void openCard();
  void closeCard();
  int allocateCMEM(uint8_t numa, u_long bsize, u_long* paddr, u_long* vaddr);
  void initDMA();
  void startDMA();
  void stopDMA();
  uint64_t bytesAvailable();  
  void readCurrentAddress();

};
} // namespace readout
} // namespace appfwk

ERS_DECLARE_ISSUE_BASE(appfwk,
                       ProducerProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))
} // namespace dunedaq

#endif // APPFWK_UDAQ_READOUT_CARDREADERDAQMODULE_HPP_
