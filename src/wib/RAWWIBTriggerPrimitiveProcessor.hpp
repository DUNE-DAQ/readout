/**
 * @file RAWWIBTriggerPrimitiveProcessor.hpp WIB TP specific Task based raw processor
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_RAWWIBTRIGGERPRIMITIVEPROCESSOR_HPP_
#define READOUT_SRC_WIB_RAWWIBTRIGGERPRIMITIVEPROCESSOR_HPP_

#include "appfwk/DAQModuleHelper.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/models/TaskRawDataProcessorModel.hpp"

#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/RawWIBTp.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "trigger/TPSet.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class RAWWIBTriggerPrimitiveProcessor : public TaskRawDataProcessorModel<types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT>
{

public:
  using inherited = TaskRawDataProcessorModel<types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT>;
  using frame_ptr = types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT*;
  using rwtp_ptr = dunedaq::detdataformats::RawWIBTp*;
  using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

  explicit RAWWIBTriggerPrimitiveProcessor(std::unique_ptr<FrameErrorRegistry>& error_registry)
    : TaskRawDataProcessorModel<types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT>(error_registry)
  {
    TaskRawDataProcessorModel<types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT>::add_preprocess_task(
      std::bind(&RAWWIBTriggerPrimitiveProcessor::tp_stitch, this, std::placeholders::_1));
  }

  void init(const nlohmann::json& args) override
  {
    try {
      auto queue_index = appfwk::queue_index(args, {});
      if (queue_index.find("tp") != queue_index.end()) {
        m_tp_source.reset(new appfwk::DAQSource<types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT>(queue_index["tp"].inst));
      }
    } catch (const ers::Issue& excpt) {
      // error
    }
  }

  void tp_stitch(frame_ptr fr)
  {
    auto& rwtp = fr->rwtp;
    m_frames++;
    int nhits = rwtp->get_nhits();
    uint64_t ts_0 = rwtp->get_timestamp(); // NOLINT
    uint8_t m_channel_no = rwtp->m_head.m_wire_no;
    uint8_t m_fiber_no = rwtp->m_head.m_fiber_no;
    for (int i = 0; i < nhits; ++i) {
      triggeralgs::TriggerPrimitive trigprim;
      trigprim.time_start = ts_0 + rwtp->m_blocks[i].m_start_time * m_time_tick;
      trigprim.time_peak = ts_0 + rwtp->m_blocks[i].m_peak_time * m_time_tick;
      trigprim.time_over_threshold = rwtp->m_blocks[i].m_end_time * m_time_tick - trigprim.time_start;
      trigprim.channel = m_channel_no; // TODO: get offline channel number
      trigprim.adc_integral = rwtp->m_blocks[i].m_sum_adc;
      trigprim.adc_peak = rwtp->m_blocks[i].m_peak_adc;
      trigprim.detid = m_fiber_no; // TODO: convert crate/slot/fiber to GeoID Roland Sipos rsipos@cern.ch July-22-2021
      trigprim.type = triggeralgs::TriggerPrimitive::Type::kTPC;
      trigprim.algorithm = triggeralgs::TriggerPrimitive::Algorithm::kTPCDefault;

      // stitch current hit to previous hit
      if (m_A[m_channel_no].size() == 1) {
        if (rwtp->m_blocks[i].m_start_time == 0 &&
            trigprim.time_start - m_A[m_channel_no][0].time_start <= m_stitch_constant) {
          // current hit is continuation of previous hit
          if (trigprim.adc_peak > m_A[m_channel_no][0].adc_peak) {
            m_A[m_channel_no][0].time_peak += trigprim.time_peak;
            m_A[m_channel_no][0].adc_peak = trigprim.adc_peak;
          }
          m_A[m_channel_no][0].time_over_threshold += trigprim.time_over_threshold;
          m_A[m_channel_no][0].adc_integral += trigprim.adc_integral;
        } else {
          // current hit is not continuation of previous hit
          // add previous hit to TriggerPrimitives
          m_tps.push_back(std::move(m_A[m_channel_no][0]));
          m_trigprims++;
          m_sent_tps++;
          m_A[m_channel_no].clear();
        }
      }
      // NB for TPSets: this assumes hits come ordered in time
      // current hit (is, completes or starts) one TriggerPrimitive
      uint8_t m_tp_continue = rwtp->m_blocks[i].m_hit_continue;
      if (m_tp_continue == 0) {
        if (m_A[m_channel_no].size() == 1) {
          m_tps.push_back(std::move(m_A[m_channel_no][0]));
          m_trigprims++;
          m_sent_tps++;
          m_A[m_channel_no].clear();
        } else {
          m_trigprims++;
          m_sent_tps++;
        }
      } else {
        m_A[m_channel_no].push_back(trigprim);
      }
    }
  }

protected:
  timestamp_t m_current_ts = 0;
  timestamp_t m_nhits = 0;
  int m_time_tick = 25;

private:
  using source_t = appfwk::DAQSource<types::RAW_WIB_TRIGGERPRIMITIVE_STRUCT>;
  std::unique_ptr<source_t> m_tp_source;

  // stitching algorithm
  std::vector<triggeralgs::TriggerPrimitive> m_A[255]; // keep track of TPs to stitch per channel
  std::vector<triggeralgs::TriggerPrimitive> m_tps;
  int m_trigprims = 0;
  int m_frames = 0;
  uint64_t m_stitch_constant = 1600; // one packet = 64 * 25 ns

  // info
  std::atomic<uint64_t> m_sent_tps{ 0 }; // NOLINT(build/unsigned)
  std::chrono::time_point<std::chrono::high_resolution_clock> m_t0;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_WIB_RAWWIBTRIGGERPRIMITIVEPROCESSOR_HPP_
