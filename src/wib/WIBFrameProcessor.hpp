/**
 * @file WIBFrameProcessor.hpp WIB specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_WIBFRAMEPROCESSOR_HPP_
#define READOUT_SRC_WIB_WIBFRAMEPROCESSOR_HPP_

#include "ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include "tpg/PdspChannelMapService.h"
#include "tpg/frame_expand.h"
#include "tpg/ProcessingInfo.h"
#include "tpg/design_fir.h"
#include "tpg/constants.h"
#include "tpg/process_avx2.h"

#include <atomic>
#include <functional>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class WIBFrameProcessor : public TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>;
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  // Channel map funciton type
  typedef int (*chan_map_fn_t)(int);

  WIBFrameProcessor()
    : TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>()
    , m_channel_map("/tmp/protoDUNETPCChannelMap_RCE_v4.txt",
                    "/tmp/protoDUNETPCChannelMap_FELIX_v4.txt")
  {
    m_taps = firwin_int(7, 0.1, m_multiplier);
    m_taps.push_back(0);

    m_taps_p = new int16_t[m_taps.size()];
    for (size_t i=0; i<m_taps.size(); ++i) {
      m_taps_p[i] = m_taps[i];
    }
    // Temporary place to stash the hits
    m_primfind_dest = new uint16_t[100000];

    TLOG() << "TAPS SIZE: " << m_taps.size() << " threshold:" << m_threshold << " exponent:" << m_tap_exponent;

    m_tpg_pi = std::make_unique<ProcessingInfo<REGISTERS_PER_FRAME>>(nullptr, FRAMES_PER_MSG, 0, REGISTERS_PER_FRAME, 
      m_primfind_dest, m_taps_p, (uint8_t)m_taps.size(), m_tap_exponent, m_threshold, 0, 0);

    m_tasklist.push_back(std::bind(&WIBFrameProcessor::timestamp_check, this, std::placeholders::_1));
    m_tasklist.push_back(std::bind(&WIBFrameProcessor::find_hits, this, std::placeholders::_1));
    // m_tasklist.push_back( std::bind(&WIBFrameProcessor::frame_error_check, this, std::placeholders::_1) );
  }

  ~WIBFrameProcessor() {
    delete[] m_taps_p;
    delete[] m_primfind_dest;
  }

  unsigned int getOfflineChannel(PdspChannelMapService& channelMap, const dunedaq::dataformats::WIBFrame* frame, unsigned int ch)
  {
    // handle 256 channels on two fibers -- use the channel
    // map that assumes 128 chans per fiber (=FEMB) (Copied
    // from PDSPTPCRawDecoder_module.cc)
    int crate = frame->get_wib_header()->crate_no;
    int slot = frame->get_wib_header()->slot_no;
    int fiber = frame->get_wib_header()->fiber_no;
 
    unsigned int fiberloc = 0;
    if (fiber == 1) {
      fiberloc = 1;
    } else if (fiber == 2) {
      fiberloc = 3;
    } else {
      TLOG() << " Fiber number " << (int)fiber << " is expected to be 1 or 2 -- revisit logic";
      fiberloc = 1;
    }
 
    unsigned int chloc = ch;
    if (chloc > 127) {
      chloc -= 128;
      fiberloc++;
    }

    unsigned int crateloc = crate;
    unsigned int offline = channelMap.GetOfflineNumberFromDetectorElements(crateloc, slot, fiberloc, chloc, PdspChannelMapService::kFELIX);
    /* printf("crate=%d slot=%d fiber=%d fiberloc=%d chloc=%d offline=%d\n", */
    /*        crate, slot, fiber, fiberloc, chloc, offline); */
    return offline;
  }


protected:
  // Internals
  timestamp_t m_previous_ts = 0;
  timestamp_t m_current_ts = 0;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  std::atomic<int> m_ts_error_ctr{ 0 };

  /**
   * Pipeline Stage 1.: Check proper timestamp increments in WIB frame
   * */
  void timestamp_check(frameptr fp)
  {
    // If EMU data, emulate perfectly incrementing timestamp
    if (inherited::m_emulator_mode) {         // emulate perfectly incrementing timestamp
      uint64_t ts_next = m_previous_ts + 300; // NOLINT(build/unsigned)
      for (unsigned int i = 0; i < 12; ++i) { // NOLINT(build/unsigned)
        auto wf = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(((uint8_t*)fp) + i * 464); // NOLINT
        auto wfh = const_cast<dunedaq::dataformats::WIBHeader*>(wf->get_wib_header());
        wfh->set_timestamp(ts_next);
        ts_next += 25;
      }
    }

    // Acquire timestamp
    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(fp); // NOLINT
    m_current_ts = wfptr->get_wib_header()->get_timestamp();

    // Check timestamp
    if (m_current_ts - m_previous_ts != 300) {
      ++m_ts_error_ctr;
      if (m_first_ts_missmatch) { // log once
        TLOG_DEBUG(TLVL_BOOKKEEPING) << "First timestamp MISSMATCH! -> | previous: " << std::to_string(m_previous_ts)
                                     << " current: " + std::to_string(m_current_ts);
        m_first_ts_missmatch = false;
      }
    }

    if (m_ts_error_ctr > 1000) {
      if (!m_problem_reported) {
        TLOG() << "*** Data Integrity ERROR *** Timestamp continuity is completely broken! "
               << "Something is wrong with the FE source or with the configuration!";
        m_problem_reported = true;
      }
    }

    m_previous_ts = m_current_ts;
    m_last_processed_daq_ts = m_current_ts;
  }

  /**
   * Pipeline Stage 2.: Check WIB headers for error flags
   * */
  void frame_error_check(frameptr /*fp*/)
  {
    // check error fields
  }

  /**
   * Pipeline Stage 3.: Do software TPG
   * */
  void find_hits(frameptr fp) {
    if (!fp)
      return;

    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>((uint8_t*)fp); // NOLINT 
    uint64_t timestamp=wfptr->get_wib_header()->get_timestamp();

    MessageRegistersCollection collection_registers;
    InductionItemToProcess* ind_item = &m_dummy_induction_item;
    expand_message_adcs_inplace(fp, &collection_registers, &ind_item->registers); 

    if (m_first) {
      m_tpg_pi->setState(collection_registers);

      m_fiber_no = wfptr->get_wib_header()->fiber_no;
      m_crate_no = wfptr->get_wib_header()->crate_no;
      m_slot_no = wfptr->get_wib_header()->slot_no;

      m_offline_channel_base = getOfflineChannel(m_channel_map, wfptr, 48);
      m_offline_channel_base_induction = getOfflineChannel(m_channel_map, wfptr, 248);

      TLOG() << "Got first item, fiber/crate/slot=" << (int)m_fiber_no << "/" << (int)m_crate_no << "/" << (int)m_slot_no;
    }

    m_tpg_pi->input = &collection_registers;
    *m_primfind_dest = MAGIC;
    process_window_avx2(*m_tpg_pi);

    //uint32_t offline_channel_base = (view_type == kInduction) ? m_offline_channel_base_induction : m_offline_channel_base;

    uint16_t chan[16], hit_end[16], hit_charge[16], hit_tover[16];
    unsigned int nhits=0;
    size_t n_sent_hits=0; // The number of "sendable" hits we produced (ie, that weren't suppressed as bad/noisy)
    size_t sent_adcsum=0; // The adc sum for "sendable" hits

    while (*m_primfind_dest != MAGIC) {
        for (int i=0; i<16; ++i) {
          chan[i] = *m_primfind_dest++;
        }
        for (int i=0; i<16; ++i) {
          hit_end[i] = *m_primfind_dest++;
        }
        for (int i=0; i<16; ++i) {
          hit_charge[i] = *m_primfind_dest++;
        }
        for (int i=0; i<16; ++i) {
          hit_tover[i] = *m_primfind_dest++;
        }

        for (int i=0; i<16; ++i) { 
          if (hit_charge[i] && chan[i] != MAGIC) {
            const uint16_t online_channel = collection_index_to_channel(chan[i]);
            int multiplier = (m_fiber_no == 1) ? 1 : -1;
            const uint32_t offline_channel = m_offline_channel_base + multiplier * collection_index_to_offline(chan[i]);
            
          }
          ++nhits;
        }
    }

    if (nhits > 0) {
      TLOG() << "NON null hits: " << nhits << " for ts: " << timestamp;
      TLOG() << *wfptr;
    }

    if (m_first) {
      TLOG() << "Total hits in first superchunk: " << nhits;
      m_first = false;
    } 

  }

private:

  struct InductionItemToProcess
  {
    // Horribly, `registers` has to be the first item in the
    // struct, because the first item in the queue has to be
    // correctly aligned, and we're going to put this in an
    // AlignedProducerConsumerQueue, which aligns the *starts* of
    // the contained objects to 64-byte boundaries, not any later
    // items
    MessageRegistersInduction registers;
    uint64_t timestamp;

    static constexpr uint64_t END_OF_MESSAGES=UINT64_MAX;
  };

  PdspChannelMapService m_channel_map;

  size_t m_num_hits = 0;
  size_t m_num_msg = 0;
  size_t m_num_push_fail = 0;
  bool m_first = true;

  InductionItemToProcess m_dummy_induction_item;

  uint8_t m_fiber_no;
  uint8_t m_slot_no;
  uint8_t m_crate_no;

  uint32_t m_offline_channel_base;
  uint32_t m_offline_channel_base_induction;

  const uint16_t m_threshold = 5; // units of sigma
  const uint8_t m_tap_exponent = 6;
  const int m_multiplier = 1 << m_tap_exponent; // 64
  std::vector<int16_t> m_taps; // firwin_int(7, 0.1, multiplier);

  uint16_t* m_primfind_dest;
  int16_t* m_taps_p;
  
  std::unique_ptr<ProcessingInfo<REGISTERS_PER_FRAME>> m_tpg_pi;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB_WIBFRAMEPROCESSOR_HPP_
