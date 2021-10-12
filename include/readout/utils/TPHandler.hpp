/**
 * @file TPHandler.hpp Buffer for TPSets
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_UTILS_TPHANDLER_HPP_
#define READOUT_INCLUDE_READOUT_UTILS_TPHANDLER_HPP_

#include "appfwk/DAQModuleHelper.hpp"
#include "readout/ReadoutIssues.hpp"
#include "trigger/TPSet.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <queue>
#include <utility>
#include <vector>

namespace dunedaq {
namespace readout {

class TPHandler
{
public:
  explicit TPHandler(appfwk::DAQSink<types::SW_WIB_TRIGGERPRIMITIVE_STRUCT>& tp_sink,
                     appfwk::DAQSink<trigger::TPSet>& tpset_sink,
                     uint64_t tp_timeout,		// NOLINT(build/unsigned)
                     uint64_t tpset_window_size,	// NOLINT(build/unsigned)
                     dataformats::GeoID geoId)
    : m_tp_sink(tp_sink)
    , m_tpset_sink(tpset_sink)
    , m_tp_timeout(tp_timeout)
    , m_tpset_window_size(tpset_window_size)
    , m_geoid(geoId)
  {}

  bool add_tp(triggeralgs::TriggerPrimitive trigprim, uint64_t currentTime) // NOLINT(build/unsigned)
  {
    if (trigprim.time_start + m_tp_timeout > currentTime) {
      m_tp_buffer.push(trigprim);
      return true;
    } else {
      return false;
    }
  }

  void try_sending_tpsets(uint64_t currentTime) // NOLINT(build/unsigned)
  {
    if (!m_tp_buffer.empty() && m_tp_buffer.top().time_start + m_tpset_window_size + m_tp_timeout < currentTime) {
      trigger::TPSet tpset;
      tpset.start_time = (m_tp_buffer.top().time_start / m_tpset_window_size) * m_tpset_window_size;
      tpset.end_time = tpset.start_time + m_tpset_window_size;
      tpset.seqno = m_next_tpset_seqno++; // NOLINT(runtime/increment_decrement)
      tpset.type = trigger::TPSet::Type::kPayload;
      tpset.origin = m_geoid;

      while (!m_tp_buffer.empty() && m_tp_buffer.top().time_start < tpset.end_time) {
        triggeralgs::TriggerPrimitive tp = m_tp_buffer.top();
        types::SW_WIB_TRIGGERPRIMITIVE_STRUCT* tp_readout_type =
          reinterpret_cast<types::SW_WIB_TRIGGERPRIMITIVE_STRUCT*>(&tp); // NOLINT
        try {
          m_tp_sink.push(*tp_readout_type);
          m_sent_tps++;
        } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
          ers::error(CannotWriteToQueue(ERS_HERE, m_geoid, "m_tp_sink"));
        }
        tpset.objects.emplace_back(std::move(tp));
        m_tp_buffer.pop();
      }

      try {
        m_tpset_sink.push(std::move(tpset));
        m_sent_tpsets++;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        ers::error(CannotWriteToQueue(ERS_HERE, m_geoid, "m_tpset_sink"));
      }
    }
  }

  void reset()
  {
    while (!m_tp_buffer.empty()) {
      m_tp_buffer.pop();
    }
    m_next_tpset_seqno = 0;
    m_sent_tps = 0;
    m_sent_tpsets = 0;
  }

  size_t get_and_reset_num_sent_tps() { return m_sent_tps.exchange(0); }

  size_t get_and_reset_num_sent_tpsets() { return m_sent_tpsets.exchange(0); }

private:
  appfwk::DAQSink<types::SW_WIB_TRIGGERPRIMITIVE_STRUCT>& m_tp_sink;
  appfwk::DAQSink<trigger::TPSet>& m_tpset_sink;
  uint64_t m_tp_timeout; 	    // NOLINT(build/unsigned)
  uint64_t m_tpset_window_size;     // NOLINT(build/unsigned)
  uint64_t m_next_tpset_seqno = 0;  // NOLINT(build/unsigned)
  dataformats::GeoID m_geoid;

  std::atomic<size_t> m_sent_tps{ 0 };    // NOLINT(build/unsigned)
  std::atomic<size_t> m_sent_tpsets{ 0 }; // NOLINT(build/unsigned)

  class TPComparator
  {
  public:
    bool operator()(triggeralgs::TriggerPrimitive& left, triggeralgs::TriggerPrimitive& right)
    {
      return left.time_start > right.time_start;
    }
  };
  std::priority_queue<triggeralgs::TriggerPrimitive, std::vector<triggeralgs::TriggerPrimitive>, TPComparator>
    m_tp_buffer;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_UTILS_TPHANDLER_HPP_
