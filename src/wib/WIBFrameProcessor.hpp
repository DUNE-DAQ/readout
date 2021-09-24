/**
 * @file WIBFrameProcessor.hpp WIB specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_WIBFRAMEPROCESSOR_HPP_
#define READOUT_SRC_WIB_WIBFRAMEPROCESSOR_HPP_

#include "appfwk/DAQModuleHelper.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/models/IterableQueueModel.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"
#include "toolbox/ReusableThread.hpp"

#include "dataformats/wib/WIBFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "trigger/TPSet.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include "tpg/DesignFIR.hpp"
#include "tpg/FrameExpand.hpp"
#include "tpg/PdspChannelMapService.hpp"
#include "tpg/ProcessAVX2.hpp"
#include "tpg/ProcessingInfo.hpp"
#include "tpg/TPGConstants.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class WIBFrameProcessor : public TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>;
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using constframeptr = const types::WIB_SUPERCHUNK_STRUCT*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  // Channel map funciton type
  typedef int (*chan_map_fn_t)(int);

  explicit WIBFrameProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>(error_registry)
    , m_sw_tpg_enabled(false)
    , m_coll_primfind_dest(nullptr)
    , m_coll_taps_p(nullptr)
    , m_ind_primfind_dest(nullptr)
    , m_ind_taps_p(nullptr)
  {
    // Setup pre-processing pipeline
    TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>::add_preprocess_task(
      std::bind(&WIBFrameProcessor::timestamp_check, this, std::placeholders::_1));
  }

  ~WIBFrameProcessor()
  {
    if (m_coll_taps_p) {
      delete[] m_coll_taps_p;
    }
    if (m_coll_primfind_dest) {
      delete[] m_coll_primfind_dest;
    }
    if (m_ind_taps_p) {
      delete[] m_ind_taps_p;
    }
    if (m_ind_primfind_dest) {
      delete[] m_ind_primfind_dest;
    }
  }

  void start(const nlohmann::json& args) override
  {
    // Reset software TPG resources
    if (m_sw_tpg_enabled) {
      m_coll_taps = swtpg::firwin_int(7, 0.1, m_coll_multiplier);
      m_coll_taps.push_back(0);
      m_ind_taps = swtpg::firwin_int(7, 0.1, m_ind_multiplier);
      m_ind_taps.push_back(0);

      if (m_coll_taps_p == nullptr) {
        m_coll_taps_p = new int16_t[m_coll_taps.size()];
      }
      for (size_t i = 0; i < m_coll_taps.size(); ++i) {
        m_coll_taps_p[i] = m_coll_taps[i];
      }

      if (m_ind_taps_p == nullptr) {
        m_ind_taps_p = new int16_t[m_ind_taps.size()];
      }
      for (size_t i = 0; i < m_ind_taps.size(); ++i) {
        m_ind_taps_p[i] = m_ind_taps[i];
      }

      // Temporary place to stash the hits
      if (m_coll_primfind_dest == nullptr) {
        m_coll_primfind_dest = new uint16_t[100000]; // NOLINT(build/unsigned)
      }
      if (m_ind_primfind_dest == nullptr) {
        m_ind_primfind_dest = new uint16_t[100000]; // NOLINT(build/unsigned)
      }

      TLOG() << "COLL TAPS SIZE: " << m_coll_taps.size() << " threshold:" << m_coll_threshold
             << " exponent:" << m_coll_tap_exponent;

      m_coll_tpg_pi = std::make_unique<swtpg::ProcessingInfo<swtpg::REGISTERS_PER_FRAME>>(
        nullptr,
        swtpg::FRAMES_PER_MSG,
        0,
        swtpg::REGISTERS_PER_FRAME,
        m_coll_primfind_dest,
        m_coll_taps_p,
        (uint8_t)m_coll_taps.size(), // NOLINT(build/unsigned)
        m_coll_tap_exponent,
        m_coll_threshold,
        0,
        0);

      m_ind_tpg_pi = std::make_unique<swtpg::ProcessingInfo<swtpg::REGISTERS_PER_FRAME>>(
        nullptr,
        swtpg::FRAMES_PER_MSG,
        0,
        10,
        m_ind_primfind_dest,
        m_ind_taps_p,
        (uint8_t)m_ind_taps.size(), // NOLINT(build/unsigned)
        m_ind_tap_exponent,
        m_ind_threshold,
        0,
        0);
    }

    while (!m_tp_buffer.empty()) {
      m_tp_buffer.pop();
    }

    // Reset timestamp check
    m_previous_ts = 0;
    m_current_ts = 0;
    m_first_ts_missmatch = true;
    m_problem_reported = false;
    m_ts_error_ctr = 0;

    // Reset stats
    m_first_coll = true;
    m_t0 = std::chrono::high_resolution_clock::now();
    m_new_hits = 0;
    m_new_tps = 0;
    m_coll_hits_count.exchange(0);
    m_num_tps_pushed.exchange(0);
    m_next_tpset_seqno = 0;
    m_sent_tps = 0;
    m_sent_tpsets = 0;
    m_dropped_tps = 0;
    inherited::start(args);
  }

  void stop(const nlohmann::json& args) override
  {
    inherited::stop(args);
    if (m_sw_tpg_enabled) {
      // Make temp. buffers reusable on next start.
      if (m_coll_taps_p) {
        delete[] m_coll_taps_p;
        m_coll_taps_p = nullptr;
      }
      if (m_coll_primfind_dest) {
        delete[] m_coll_primfind_dest;
        m_coll_primfind_dest = nullptr;
      }
      if (m_ind_taps_p) {
        delete[] m_ind_taps_p;
        m_ind_taps_p = nullptr;
      }
      if (m_ind_primfind_dest) {
        delete[] m_ind_primfind_dest;
        m_ind_primfind_dest = nullptr;
      }
    }
  }

  unsigned int getOfflineChannel(swtpg::PdspChannelMapService& channelMap, // NOLINT(build/unsigned)
                                 const dunedaq::dataformats::WIBFrame* frame,
                                 unsigned int ch) // NOLINT(build/unsigned)
  {
    // handle 256 channels on two fibers -- use the channel
    // map that assumes 128 chans per fiber (=FEMB) (Copied
    // from PDSPTPCRawDecoder_module.cc)
    int crate = frame->get_wib_header()->crate_no;
    int slot = frame->get_wib_header()->slot_no;
    int fiber = frame->get_wib_header()->fiber_no;

    unsigned int fiberloc = 0; // NOLINT(build/unsigned)
    if (fiber == 1) {
      fiberloc = 1;
    } else if (fiber == 2) {
      fiberloc = 3;
    } else {
      TLOG() << " Fiber number " << fiber << " is expected to be 1 or 2 -- revisit logic";
      fiberloc = 1;
    }

    unsigned int chloc = ch; // NOLINT(build/unsigned)
    if (chloc > 127) {
      chloc -= 128;
      fiberloc++;
    }

    unsigned int crateloc = crate; // NOLINT(build/unsigned)
    unsigned int offline =         // NOLINT(build/unsigned)
      channelMap.GetOfflineNumberFromDetectorElements(
        crateloc, slot, fiberloc, chloc, swtpg::PdspChannelMapService::kFELIX);
    // printf("crate=%d slot=%d fiber=%d fiberloc=%d chloc=%d offline=%d\n",
    //        crate, slot, fiber, fiberloc, chloc, offline);
    return offline;
  }

  void init(const nlohmann::json& args) override
  {
    try {
      auto queue_index = appfwk::queue_index(args, {});
      if (queue_index.find("tp_out") != queue_index.end()) {
        m_tp_sink.reset(new appfwk::DAQSink<types::TP_READOUT_TYPE>(queue_index["tp_out"].inst));
      }
      if (queue_index.find("tpset_out") != queue_index.end()) {
        m_tpset_sink.reset(new appfwk::DAQSink<trigger::TPSet>(queue_index["tpset_out"].inst));
      }
    } catch (const ers::Issue& excpt) {
      throw ResourceQueueError(ERS_HERE, "tp queue", "DefaultRequestHandlerModel", excpt);
    }
  }

  void conf(const nlohmann::json& cfg) override
  {
    auto config = cfg.get<datalinkhandler::Conf>();
    m_geoid.element_id = config.link_number;
    m_geoid.region_id = config.apa_number;
    m_geoid.system_type = types::WIB_SUPERCHUNK_STRUCT::system_type;

    m_tp_timeout = config.tp_timeout;
    m_tpset_window_size = config.tpset_window_size;

    if (config.enable_software_tpg) {
      m_sw_tpg_enabled = true;

      const char* readout_share_cstr = getenv("READOUT_SHARE");
      if (!readout_share_cstr) {
        throw ConfigurationError(ERS_HERE, m_geoid, "Environment variable READOUT_SHARE is not set");
      }
      std::string readout_share(readout_share_cstr);
      std::string channel_map_rce(config.channel_map_rce);
      std::string channel_map_felix(config.channel_map_felix);
      if (channel_map_rce == "") {
        channel_map_rce = readout_share + "/config/protoDUNETPCChannelMap_RCE_v4.txt";
      }
      if (channel_map_felix == "") {
        channel_map_felix = readout_share + "/config/protoDUNETPCChannelMap_FELIX_v4.txt";
      }
      m_channel_map.reset(new swtpg::PdspChannelMapService(channel_map_rce, channel_map_felix));

      m_induction_items_to_process =
        std::make_unique<IterableQueueModel<InductionItemToProcess>>(200000, false, 0, true, 64); // 64 byte aligned

      // Setup parallel post-processing
      TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>::add_postprocess_task(
        std::bind(&WIBFrameProcessor::find_collection_hits, this, std::placeholders::_1));
    }

    TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>::conf(cfg);
  }

  void get_info(datalinkhandlerinfo::Info& info)
  {
    info.num_tps_sent = m_sent_tps.exchange(0);
    info.num_tpsets_sent = m_sent_tpsets.exchange(0);
    info.num_tps_dropped = m_dropped_tps.exchange(0);

    auto now = std::chrono::high_resolution_clock::now();
    if (m_sw_tpg_enabled) {
      int new_hits = m_coll_hits_count.exchange(0);
      int new_tps = m_num_tps_pushed.exchange(0);
      double seconds = std::chrono::duration_cast<std::chrono::microseconds>(now - m_t0).count() / 1000000.;
      TLOG_DEBUG(TLVL_TAKE_NOTE) << "Hit rate: " << std::to_string(new_hits / seconds / 1000.) << " [kHz]";
      TLOG_DEBUG(TLVL_TAKE_NOTE) << "Total new hits: " << new_hits << " new pushes: " << new_tps;
      info.rate_tp_hits = new_hits / seconds / 1000.;
    }
    m_t0 = now;

    TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>::get_info(info);
  }

protected:
  // Internals
  timestamp_t m_previous_ts = 0;
  timestamp_t m_current_ts = 0;
  bool m_first_ts_missmatch = true;
  bool m_problem_reported = false;
  std::atomic<int> m_ts_error_ctr{ 0 };

  void postprocess_example(const types::WIB_SUPERCHUNK_STRUCT* fp)
  {
    TLOG() << "Postprocessing: " << fp->get_timestamp();
  }

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
      m_error_registry->add_error(FrameErrorRegistry::FrameError(m_previous_ts + 300, m_current_ts));
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
  void find_collection_hits(constframeptr fp)
  {
    if (!fp)
      return;

    auto wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>((uint8_t*)fp); // NOLINT
    uint64_t timestamp = wfptr->get_wib_header()->get_timestamp();                // NOLINT(build/unsigned)

    swtpg::MessageRegistersCollection collection_registers;

    // InductionItemToProcess* ind_item = &m_dummy_induction_item;
    InductionItemToProcess ind_item;
    expand_message_adcs_inplace(fp, &collection_registers, &ind_item.registers);
    m_induction_items_to_process->write(std::move(ind_item));

    if (m_first_coll) {
      m_coll_tpg_pi->setState(collection_registers);

      m_fiber_no = wfptr->get_wib_header()->fiber_no;
      m_crate_no = wfptr->get_wib_header()->crate_no;
      m_slot_no = wfptr->get_wib_header()->slot_no;

      m_offline_channel_base = getOfflineChannel(*m_channel_map, wfptr, 48);
      m_offline_channel_base_induction = getOfflineChannel(*m_channel_map, wfptr, 248);

      TLOG() << "Got first item, fiber/crate/slot=" << m_fiber_no << "/" << m_crate_no << "/" << m_slot_no;
    }

    m_coll_tpg_pi->input = &collection_registers;
    *m_coll_primfind_dest = swtpg::MAGIC;
    swtpg::process_window_avx2(*m_coll_tpg_pi);

    // uint32_t offline_channel_base = (view_type == kInduction) ? m_offline_channel_base_induction :
    // m_offline_channel_base;

    uint16_t chan[16], hit_end[16], hit_charge[16], hit_tover[16]; // NOLINT(build/unsigned)
    unsigned int nhits = 0;

    uint16_t* primfind_it = m_coll_primfind_dest; // NOLINT(build/unsigned)

    constexpr int clocksPerTPCTick = 25;

    // process_window_avx2 stores its output in the buffer pointed to
    // by m_coll_primfind_dest in a (necessarily) complicated way: for
    // every set of 16 channels (one AVX2 register) that has at least
    // one hit which ends at this tick, the full 16-channel registers
    // of channel number, hit end time, hit charge and hit t-o-t are
    // stored. This is done for each of the (6 collection registers
    // per tick) x (12 ticks per superchunk), and the end of valid
    // hits is indicated by the presence of the value "MAGIC" (defined
    // in TPGConstants.h).
    //
    // Since not all channels in a register will have hits ending at
    // this tick, we look at the stored hit_charge to determine
    // whether this channel in the register actually had a hit ending
    // in it: for channels which *did* have a hit ending, the value of
    // hit_charge is nonzero.
    while (*primfind_it != swtpg::MAGIC) {
      // First, get all of the register values (including those with no hit) into local variables
      for (int i = 0; i < 16; ++i) {
        chan[i] = *primfind_it++; // NOLINT(runtime/increment_decrement)
      }
      for (int i = 0; i < 16; ++i) {
        hit_end[i] = *primfind_it++; // NOLINT(runtime/increment_decrement)
      }
      for (int i = 0; i < 16; ++i) {
        hit_charge[i] = *primfind_it++; // NOLINT(runtime/increment_decrement)
      }
      for (int i = 0; i < 16; ++i) {
        hit_tover[i] = *primfind_it++; // NOLINT(runtime/increment_decrement)
      }

      // Now that we have all the register values in local
      // variables, loop over the register index (ie, channel) and
      // find the channels which actually had a hit, as indicated by
      // nonzero value of hit_charge
      for (int i = 0; i < 16; ++i) {
        if (hit_charge[i] && chan[i] != swtpg::MAGIC) {
          // This channel had a hit ending here, so we can create and output the hit here
          const uint16_t online_channel = swtpg::collection_index_to_channel(chan[i]); // NOLINT(build/unsigned)
          uint64_t tp_t_begin =                                                        // NOLINT(build/unsigned)
            timestamp + clocksPerTPCTick * (int64_t(hit_end[i]) - hit_tover[i]);       // NOLINT(build/unsigned)
          uint64_t tp_t_end = timestamp + clocksPerTPCTick * int64_t(hit_end[i]);      // NOLINT(build/unsigned)

          // May be needed for TPSet:
          // uint64_t tspan = clocksPerTPCTick * hit_tover[i]; // is/will be this needed?
          //

          // For quick n' dirty debugging: print out time/channel of hits.
          // Can then make a text file suitable for numpy plotting with, eg:
          //
          // sed -n -e 's/.*Hit: \(.*\) \(.*\).*/\1 \2/p' log.txt  > hits.txt
          //
          // TLOG() << "Hit: " << hit_start << " " << offline_channel;

          // if (should_send) {
          //
          triggeralgs::TriggerPrimitive trigprim;
          trigprim.time_start = tp_t_begin;
          trigprim.time_peak = (tp_t_begin + tp_t_end) / 2;
          trigprim.time_over_threshold = hit_tover[i] * clocksPerTPCTick;
          trigprim.channel = online_channel;
          trigprim.adc_integral = hit_charge[i];
          trigprim.adc_peak = hit_charge[i] / 20;
          trigprim.detid =
            m_fiber_no; // TODO: convert crate/slot/fiber to GeoID Roland Sipos rsipos@cern.ch July-22-2021
          trigprim.type = triggeralgs::TriggerPrimitive::Type::kTPC;
          trigprim.algorithm = triggeralgs::TriggerPrimitive::Algorithm::kTPCDefault;
          trigprim.version = 1;

          if (m_first_coll) {
            TLOG() << "TP makes sense? -> hit_t_begin:" << tp_t_begin << " hit_t_end:" << tp_t_end
                   << " time_peak:" << (tp_t_begin + tp_t_end) / 2;
          }

          if (trigprim.time_start + m_tp_timeout > timestamp + 300) {
            m_tp_buffer.push(trigprim);
          } else {
            m_dropped_tps++;
          }

          //} else {

          //}
          m_new_tps++;
          ++nhits;
        }
      }
    }

    // if (nhits > 0) {
    // TLOG() << "NON null hits: " << nhits << " for ts: " << timestamp;
    // TLOG() << *wfptr;
    //}

    m_num_hits_coll += nhits;
    m_coll_hits_count += nhits;

    if (m_first_coll) {
      TLOG() << "Total hits in first superchunk: " << nhits;
      m_first_coll = false;
    }

    // Check if we can send out a TPSet
    if (!m_tp_buffer.empty() && m_tp_buffer.top().time_start + m_tpset_window_size + m_tp_timeout < timestamp + 300) {
      trigger::TPSet tpset;
      tpset.start_time = (m_tp_buffer.top().time_start / m_tpset_window_size) * m_tpset_window_size;
      tpset.end_time = tpset.start_time + m_tpset_window_size;
      tpset.seqno = m_next_tpset_seqno++; // NOLINT(runtime/increment_decrement)
      tpset.type = trigger::TPSet::Type::kPayload;
      tpset.origin = m_geoid;

      while (!m_tp_buffer.empty() && m_tp_buffer.top().time_start < tpset.end_time) {
        triggeralgs::TriggerPrimitive tp = m_tp_buffer.top();
        types::TP_READOUT_TYPE* tp_readout_type = reinterpret_cast<types::TP_READOUT_TYPE*>(&tp); // NOLINT
        try {
          m_tp_sink->push(*tp_readout_type);
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          ers::error(CannotWriteToQueue(ERS_HERE, m_geoid, "m_tp_sink"));
        }
        tpset.objects.emplace_back(std::move(tp));
        m_tp_buffer.pop();
        m_sent_tps++;
      }

      try {
        m_tpset_sink->push(std::move(tpset));
        m_num_tps_pushed++;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ers::error(CannotWriteToQueue(ERS_HERE, m_geoid, "m_tpset_sink"));
      }
      m_sent_tpsets++;
    }
  }

  // Stage: induction hit finding port
  void find_induction_hits(frameptr /*fp*/)
  {
    m_induction_items_to_process->popFront();

    // while (m_running()) { // if spawned as thread.

    // InductionItemToProcess item = nullptr;

    // if (m_first_ind) {
    //  m_ind_tpg_pi->setState(
    //}
  }

private:
  bool m_sw_tpg_enabled;

  struct InductionItemToProcess
  {
    // Horribly, `registers` has to be the first item in the
    // struct, because the first item in the queue has to be
    // correctly aligned, and we're going to put this in an
    // AlignedProducerConsumerQueue, which aligns the *starts* of
    // the contained objects to 64-byte boundaries, not any later
    // items
    swtpg::MessageRegistersInduction registers;
    uint64_t timestamp; // NOLINT(build/unsigned)

    static constexpr uint64_t END_OF_MESSAGES = UINT64_MAX; // NOLINT(build/unsigned)
  };

  std::unique_ptr<IterableQueueModel<InductionItemToProcess>> m_induction_items_to_process;

  std::unique_ptr<swtpg::PdspChannelMapService> m_channel_map;

  size_t m_num_msg = 0;
  size_t m_num_push_fail = 0;

  size_t m_num_hits_coll = 0;
  size_t m_num_hits_ind = 0;

  std::atomic<int> m_coll_hits_count{ 0 };
  std::atomic<int> m_indu_hits_count{ 0 };
  std::atomic<int> m_num_tps_pushed{ 0 };

  bool m_first_coll = true;
  bool m_first_ind = true;

  InductionItemToProcess m_dummy_induction_item;

  uint8_t m_fiber_no; // NOLINT(build/unsigned)
  uint8_t m_slot_no;  // NOLINT(build/unsigned)
  uint8_t m_crate_no; // NOLINT(build/unsigned)

  uint32_t m_offline_channel_base;           // NOLINT(build/unsigned)
  uint32_t m_offline_channel_base_induction; // NOLINT(build/unsigned)

  // Collection
  const uint16_t m_coll_threshold = 5;                    // units of sigma // NOLINT(build/unsigned)
  const uint8_t m_coll_tap_exponent = 6;                  // NOLINT(build/unsigned)
  const int m_coll_multiplier = 1 << m_coll_tap_exponent; // 64
  std::vector<int16_t> m_coll_taps;                       // firwin_int(7, 0.1, multiplier);
  uint16_t* m_coll_primfind_dest;                         // NOLINT(build/unsigned)
  int16_t* m_coll_taps_p;
  std::unique_ptr<swtpg::ProcessingInfo<swtpg::REGISTERS_PER_FRAME>> m_coll_tpg_pi;

  // Induction
  const uint16_t m_ind_threshold = 3;                   // units of sigma // NOLINT(build/unsigned)
  const uint8_t m_ind_tap_exponent = 6;                 // NOLINT(build/unsigned)
  const int m_ind_multiplier = 1 << m_ind_tap_exponent; // 64
  std::vector<int16_t> m_ind_taps;                      // firwin_int(7, 0.1, multiplier);
  uint16_t* m_ind_primfind_dest;                        // NOLINT(build/unsigned)
  int16_t* m_ind_taps_p;
  std::unique_ptr<swtpg::ProcessingInfo<swtpg::REGISTERS_PER_FRAME>> m_ind_tpg_pi;

  std::unique_ptr<appfwk::DAQSink<types::TP_READOUT_TYPE>> m_tp_sink;
  std::unique_ptr<appfwk::DAQSink<trigger::TPSet>> m_tpset_sink;

  class Comparator
  {
  public:
    bool operator()(triggeralgs::TriggerPrimitive& left, triggeralgs::TriggerPrimitive& right)
    {
      return left.time_start > right.time_start;
    }
  };
  std::priority_queue<triggeralgs::TriggerPrimitive, std::vector<triggeralgs::TriggerPrimitive>, Comparator>
    m_tp_buffer;

  dataformats::GeoID m_geoid;
  uint64_t m_tp_timeout = 10000;       // NOLINT(build/unsigned)
  uint64_t m_tpset_window_size = 1000; // NOLINT(build/unsigned)
  uint64_t m_next_tpset_seqno = 0;     // NOLINT(build/unsigned)

  std::atomic<uint64_t> m_sent_tps{ 0 };    // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_sent_tpsets{ 0 }; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_dropped_tps{ 0 }; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_new_hits{ 0 };    // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_new_tps{ 0 };     // NOLINT(build/unsigned)

  std::chrono::time_point<std::chrono::high_resolution_clock> m_t0;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB_WIBFRAMEPROCESSOR_HPP_
