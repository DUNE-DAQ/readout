/**
 * @file PDSListRequestHandler.hpp Trigger matching mechanism for WIB frames.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_DAPHNE_DAPHNELISTREQUESTHANDLER_HPP_
#define READOUT_SRC_DAPHNE_DAPHNELISTREQUESTHANDLER_HPP_

#include "readout/models/DefaultRequestHandlerModel.hpp"
#include "ReadoutIssues.hpp"
#include "readout/models/SkipListLatencyBufferModel.hpp"

#include "dataformats/pds/PDSFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"

#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

class DaphneListRequestHandler
  : public DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT,
                                      SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT>>
{
public:
  using inherited = DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT,
                                               SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT>>;
  using SkipListAcc = typename folly::ConcurrentSkipList<types::PDS_SUPERCHUNK_STRUCT>::Accessor;
  using SkipListSkip = typename folly::ConcurrentSkipList<types::PDS_SUPERCHUNK_STRUCT>::Skipper;

  DaphneListRequestHandler(std::unique_ptr<SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT>>& latency_buffer,
                        std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                        std::unique_ptr<appfwk::DAQSink<types::PDS_SUPERCHUNK_STRUCT>>& snb_sink)
    : DefaultRequestHandlerModel<types::PDS_SUPERCHUNK_STRUCT,
                                 SkipListLatencyBufferModel<types::PDS_SUPERCHUNK_STRUCT>>(latency_buffer,
                                                                                        fragment_sink,
                                                                                        snb_sink)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "PDSListRequestHandler created...";
  }
  

protected:
  RequestResult cleanup_request(dfmessages::DataRequest dr,
                                unsigned /** delay_us */ = 0) override // NOLINT(build/unsigned)
  {
    return pds_cleanup_request(dr);
  }

  RequestResult pds_cleanup_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0)
  {
    // size_t occupancy_guess = m_latency_buffer->occupancy();
    size_t removed_ctr = 0;
    uint64_t tailts = 0; // oldest // NOLINT(build/unsigned)
    uint64_t headts = 0; // newest // NOLINT(build/unsigned)
    {
      SkipListAcc acc(m_latency_buffer->get_skip_list());
      auto tail = acc.last();
      auto head = acc.first();
      if (tail && head) {
        // auto tailptr = reinterpret_cast<const dataformats::PDSFrame*>(tail); // NOLINT
        // auto headptr = reinterpret_cast<const dataformats::PDSFrame*>(head); // NOLINT
        tailts = (*tail).get_timestamp(); // tailptr->get_timestamp();
        headts = (*head).get_timestamp(); // headptr->get_timestamp();
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Cleanup REQUEST with "
                                    << "Oldest stored TS=" << tailts << " "
                                    << "Newest stored TS=" << headts;
        if (headts - tailts > m_max_ts_diff) { // ts differnce exceeds maximum
          ++(inherited::m_pop_reqs);
          uint64_t timediff = m_max_ts_diff; // NOLINT(build/unsigned)
          while (timediff >= m_max_ts_diff) {
            bool removed = acc.remove(*tail);
            if (!removed) {
              TLOG_DEBUG(TLVL_WORK_STEPS) << "Unsuccesfull remove from SKL during cleanup: " << removed;
            } else {
              ++removed_ctr;
            }
            tail = acc.last();
            // headptr = reinterpret_cast<const dataformats::PDSFrame*>(head);
            tailts = (*tail).get_timestamp(); // headptr->get_timestamp();
            timediff = headts - tailts;
          }
          inherited::m_pops_count += removed_ctr;
        }
      } else {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Didn't manage to get SKL head and tail!";
      }
    }
    inherited::m_cleanup_requested = false;
    return RequestResult(ResultCode::kCleanup, dr);
  }


private:
  // Constants
  static const constexpr uint64_t m_max_ts_diff = 10000000; // NOLINT(build/unsigned)

  // Stats
  std::atomic<int> m_found_requested_count{ 0 };
  std::atomic<int> m_bad_requested_count{ 0 };
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_DAPHNE_DAPHNELISTREQUESTHANDLER_HPP_
