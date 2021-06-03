/**
 * @file DefaultRequestHandlerModel.hpp Default mode of operandy for
 * request handling:
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_DEFAULTREQUESTHANDLERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_DEFAULTREQUESTHANDLERMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "readout/concepts/RequestHandlerConcept.hpp"
#include "readout/utils/ReusableThread.hpp"

#include "readout/datalinkhandler/Structs.hpp"

#include "dataformats/Fragment.hpp"
#include "dataformats/Types.hpp"
#include "dfmessages/DataRequest.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include <folly/concurrency/UnboundedQueue.h>

#include <atomic>
#include <functional>
#include <future>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using dunedaq::readout::logging::TLVL_HOUSEKEEPING;
using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class RawType, class LatencyBufferType>
class DefaultRequestHandlerModel : public RequestHandlerConcept<RawType, LatencyBufferType>
{
public:
  explicit DefaultRequestHandlerModel(
    std::unique_ptr<LatencyBufferType>& latency_buffer,
    std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
    std::unique_ptr<appfwk::DAQSink<RawType>>& snb_sink)
    : m_latency_buffer(latency_buffer)
    , m_fragment_sink(fragment_sink)
    , m_snb_sink(snb_sink)
    , m_pop_reqs(0)
    , m_pops_count(0)
    , m_occupancy(0)
    , m_pop_limit_pct(0.0f)
    , m_pop_size_pct(0.0f)
    , m_pop_limit_size(0)
    , m_pop_counter{ 0 }
    , m_buffer_capacity(0)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "DefaultRequestHandlerModel created...";
  }

  using RequestResult = typename dunedaq::readout::RequestHandlerConcept<RawType, LatencyBufferType>::RequestResult;
  using ResultCode = typename dunedaq::readout::RequestHandlerConcept<RawType, LatencyBufferType>::ResultCode;

  void conf(const nlohmann::json& args)
  {
    auto conf = args.get<datalinkhandler::Conf>();
    m_pop_limit_pct = conf.pop_limit_pct;
    m_pop_size_pct = conf.pop_size_pct;
    m_buffer_capacity = conf.latency_buffer_size;
    // if (m_configured) {
    //  ers::error(ConfigurationError(ERS_HERE, "This object is already configured!"));
    if (m_pop_limit_pct < 0.0f || m_pop_limit_pct > 1.0f || m_pop_size_pct < 0.0f || m_pop_size_pct > 1.0f) {
      ers::error(ConfigurationError(ERS_HERE, "Auto-pop percentage out of range."));
    } else {
      m_pop_limit_size = m_pop_limit_pct * m_buffer_capacity;
      m_max_requested_elements = m_pop_limit_size - m_pop_limit_size * m_pop_size_pct;
    }

    m_geoid.element_id = conf.link_number;
    m_geoid.region_id = conf.apa_number;
    m_geoid.system_type = RawType::system_type;

    std::ostringstream oss;
    oss << "RequestHandler configured. " << std::fixed << std::setprecision(2)
        << "auto-pop limit: " << m_pop_limit_pct * 100.0f << "% "
        << "auto-pop size: " << m_pop_size_pct * 100.0f << "% "
        << "max requested elements: " << m_max_requested_elements;
    TLOG_DEBUG(TLVL_WORK_STEPS) << oss.str();
  }

  void start(const nlohmann::json& /*args*/)
  {
    m_run_marker.store(true);
    m_stats_thread = std::thread(&DefaultRequestHandlerModel<RawType, LatencyBufferType>::run_stats, this);
    m_executor = std::thread(&DefaultRequestHandlerModel<RawType, LatencyBufferType>::executor, this);
  }

  void stop(const nlohmann::json& /*args*/)
  {
    m_run_marker.store(false);
    // if (m_recording) throw CommandError(ERS_HERE, "Recording is still ongoing!");
    if (m_future_recording_stopper.valid())
      m_future_recording_stopper.wait();
    m_executor.join();
    m_stats_thread.join();
  }

  void record(const nlohmann::json& args) override
  {
    if (m_snb_sink.get() == nullptr) {
      TLOG() << "Recording could not be started because output queue is not set up";
      return;
    }
    auto conf = args.get<datalinkhandler::RecordingParams>();
    if (m_recording.load()) {
      TLOG() << "A recording is still running, no new recording was started!" << std::endl;
      return;
    }
    m_future_recording_stopper = std::async(
      [&](int duration) {
        TLOG() << "Start recording for " << duration << " second(s)" << std::endl;
        m_recording.exchange(true);
        std::this_thread::sleep_for(std::chrono::seconds(duration));
        TLOG() << "Stop recording" << std::endl;
        m_recording.exchange(false);
      },
      conf.duration);
  }

  void auto_cleanup_check()
  {
    // TLOG_DEBUG(TLVL_WORK_STEPS) << "Enter auto_cleanup_check";
    auto size_guess = m_latency_buffer->occupancy();
    if (!m_cleanup_requested.load(std::memory_order_acquire) && size_guess > m_pop_limit_size) {
      dfmessages::DataRequest dr;
      auto delay_us = 0;
      // 10-May-2021, KAB: moved the assignment of m_cleanup_requested so that is is *before* the creation of
      // the future and the addition of the future to the completion queue in order to avoid the race condition
      // in which the future gets run before we have a chance to set the m_cleanup_requested flag here.
      m_cleanup_requested.store(true);
      auto execfut = std::async(std::launch::deferred,
                                &DefaultRequestHandlerModel<RawType, LatencyBufferType>::cleanup_request,
                                this,
                                dr,
                                delay_us);
      m_completion_queue.enqueue(std::move(execfut));
    }
  }

  // DataRequest struct!?
  void issue_request(dfmessages::DataRequest datarequest, unsigned delay_us = 0) // NOLINT(build/unsigned)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Enter issue_request";
    auto reqfut = std::async(std::launch::async,
                             &DefaultRequestHandlerModel<RawType, LatencyBufferType>::data_request,
                             this,
                             datarequest,
                             delay_us);
    m_completion_queue.enqueue(std::move(reqfut));
  }

protected:
  inline dataformats::FragmentHeader create_fragment_header(const dfmessages::DataRequest& dr)
  {
    dataformats::FragmentHeader fh;
    fh.size = sizeof(fh);
    fh.trigger_number = dr.trigger_number;
    fh.trigger_timestamp = dr.trigger_timestamp;
    fh.window_begin = dr.window_begin;
    fh.window_end = dr.window_end;
    fh.run_number = dr.run_number;
    fh.element_id = { m_geoid.system_type, m_geoid.region_id, m_geoid.element_id };
    fh.fragment_type = static_cast<dataformats::fragment_type_t>(RawType::fragment_type);
    return std::move(fh);
  }

  inline void dump_to_buffer(const void* data,
                             std::size_t size,
                             void* buffer,
                             uint32_t buffer_pos, // NOLINT(build/unsigned)
                             const std::size_t& buffer_size)
  {
    auto bytes_to_copy = size;
    while (bytes_to_copy > 0) {
      auto n = std::min(bytes_to_copy, buffer_size - buffer_pos);
      std::memcpy(static_cast<char*>(buffer) + buffer_pos, static_cast<const char*>(data), n);
      buffer_pos += n;
      bytes_to_copy -= n;
      if (buffer_pos == buffer_size) {
        buffer_pos = 0;
      }
    }
  }

  RequestResult cleanup_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0) // NOLINT(build/unsigned)
  {
    // auto now_s = time::now_as<std::chrono::seconds>();
    auto size_guess = m_latency_buffer->occupancy();
    if (size_guess > m_pop_limit_size) {
      ++m_pop_reqs;
      unsigned to_pop = m_pop_size_pct * m_latency_buffer->occupancy();

      // SNB handling
      if (m_recording) {
        RawType element;
        for (unsigned i = 0; i < to_pop; ++i) { // NOLINT(build/unsigned)
          if (m_latency_buffer->read(element)) {
            try {
              if (m_recording)
                m_snb_sink->push(element, std::chrono::milliseconds(0));
            } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
              ers::error(CannotWriteToQueue(ERS_HERE, "SNB Writer"));
            }
          } else {
            throw InternalError(ERS_HERE, "Could not read from latency buffer");
          }
        }
      } else {
        m_latency_buffer->pop(to_pop);
      }
      // m_pops_count += to_pop;
      m_occupancy = m_latency_buffer->occupancy();
      m_pops_count.store(m_pops_count.load() + to_pop);
    }
    m_cleanup_requested.store(false);
    return RequestResult(ResultCode::kCleanup, dr);
  }

  void executor()
  {
    std::future<RequestResult> fut;
    while (m_run_marker.load() || !m_completion_queue.empty()) {
      if (m_completion_queue.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      } else {
        bool success = m_completion_queue.try_dequeue(fut);
        if (!success) {
          ers::error(CannotReadFromQueue(ERS_HERE, "RequestsCompletionQueue."));
        } else {
          // m_lock_callback();
          fut.wait(); // trigger execution
          // m_unlock_callback();
          auto reqres = fut.get();
          // TLOG() << "Request result handled: " << resultCodeAsString(reqres.result_code);

          // 28-Apr-2021, KAB: I believe that a test on m_run_marker when reqres.result_code is kNotYet
          // leads to missing fragments in TriggerRecords at the end of a run (i.e. at Stop time).
          // In the case of the WIBRequestHandler::tcp_data_request() method, that method is smart enough to
          // recognize that a Stop has been requested and create an empty Fragment if the data can not
          // be found. Checking on the status of the run_marker here prevents that code from doing that
          // valuable service.  So, in this candidate change, I have commented out the check on the
          // run_marker here.  With this change, I see fewer (maybe even zero) TriggerRecords with
          // missing fragments at the end of runs.  Of course, those Fragments may be empty, but at
          // least they are not missing.
          if (reqres.result_code == ResultCode::kNotYet) { // && m_run_marker.load()) { // give it another chance
            TLOG_DEBUG(TLVL_WORK_STEPS) << "Re-queue request. "
                                        << "With timestamp=" << reqres.data_request.trigger_timestamp << "delay [us] "
                                        << reqres.request_delay_us;
            issue_request(reqres.data_request, 0);
          }
        }
      }
    }
  }

  void run_stats()
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread started...";
    int new_pop_reqs = 0;
    int new_pop_count = 0;
    int new_occupancy = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (m_run_marker.load()) {
      auto now = std::chrono::high_resolution_clock::now();
      new_pop_reqs = m_pop_reqs.exchange(0);
      new_pop_count = m_pops_count.exchange(0);
      new_occupancy = m_occupancy;
      double seconds = std::chrono::duration_cast<std::chrono::microseconds>(now - t0).count() / 1000000.;
      TLOG_DEBUG(TLVL_HOUSEKEEPING) << "Cleanup request rate: " << new_pop_reqs / seconds / 1. << " [Hz]"
                                    << " Dropped: " << new_pop_count << " Occupancy: " << new_occupancy;
      for (int i = 0; i < 50 && m_run_marker.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      t0 = now;
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread stopped...";
  }

  RequestResult data_request(dfmessages::DataRequest dr, unsigned delay_us = 0) override // NOLINT(build/unsigned)
  {
    if (delay_us > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
    }

    // Prepare response
    RequestResult rres(ResultCode::kUnknown, dr);

    // Data availability is calculated here
    size_t occupancy_guess = m_latency_buffer->occupancy();
    auto front_frame = *(reinterpret_cast<const RawType*>(&(*m_latency_buffer->front()))); // NOLINT
    auto last_frame =
        *(reinterpret_cast<const RawType*>(&(*m_latency_buffer->back()))); // NOLINT
    uint64_t last_ts = front_frame.get_timestamp();  // NOLINT(build/unsigned)
    uint64_t newest_ts = last_frame.get_timestamp(); // NOLINT(build/unsigned)

    uint64_t start_win_ts = dr.window_begin; // NOLINT(build/unsigned)
    uint64_t end_win_ts = dr.window_end;     // NOLINT(build/unsigned)
    RawType request_element;
    request_element.set_timestamp(start_win_ts);
    auto start_iter = m_latency_buffer->lower_bound(request_element);
    request_element.set_timestamp(end_win_ts);
    auto end_iter = m_latency_buffer->lower_bound(request_element);
    // std::cout << start_idx << ", " << end_idx << std::endl;
    // std::cout << "Timestamps: " << start_win_ts << ", " << end_win_ts << std::endl;
    // std::cout << "Found timestamps: " << m_latency_buffer->at(start_idx)->get_timestamp() << ", " <<
    // m_latency_buffer->at(end_idx)->get_timestamp() << std::endl;

    TLOG_DEBUG(TLVL_WORK_STEPS) << "Data request for "
                                << "Trigger TS=" << dr.trigger_timestamp << " "
                                << "Oldest stored TS=" << last_ts << " "
                                << "Newest stored TS=" << newest_ts << " "
                                << "Start of window TS=" << start_win_ts << " "
                                << "End of window TS=" << end_win_ts;

    // Prepare FragmentHeader and empty Fragment pieces list
    auto frag_header = create_fragment_header(dr);

    // List of safe-extraction conditions

    if (start_iter != m_latency_buffer->end() && end_iter == m_latency_buffer->end()) { // data is not fully in buffer yet
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kInvalidWindow));
      rres.result_code = ResultCode::kPass;
      ++m_bad_requested_count;
    } else if (last_ts <= start_win_ts && end_win_ts <= newest_ts) { // data is there
      if (start_iter != m_latency_buffer->end() && end_iter != m_latency_buffer->end()) {                          // data is there (double check)
        rres.result_code = ResultCode::kFound;
        ++m_found_requested_count;
      } else {
      }
    } else if (start_iter != m_latency_buffer->end() && end_iter != m_latency_buffer->end()) { // data is there
      rres.result_code = ResultCode::kFound;
      ++m_found_requested_count;
    } else if (start_iter == m_latency_buffer->end() && end_iter != m_latency_buffer->end()) { // data is partially gone.
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    } else if (newest_ts < end_win_ts) {
      rres.result_code = ResultCode::kNotYet; // give it another chance
    } else {
      TLOG() << "Don't know how to categorise this request";
      frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
      rres.result_code = ResultCode::kNotFound;
      ++m_bad_requested_count;
    }

    // Requeue if needed
    if (rres.result_code == ResultCode::kNotYet) {
      if (m_run_marker.load()) {
        // rres.request_delay_us = (min_num_elements - occupancy_guess) * m_frames_per_element * m_tick_dist / 1000.;
        if (rres.request_delay_us < m_min_delay_us) { // minimum delay protection
          rres.request_delay_us = m_min_delay_us;
        }
        return rres; // If kNotYet, return immediately, don't check for fragment pieces.
      } else {
        frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
        rres.result_code = ResultCode::kNotFound;
        ++m_bad_requested_count;
      }
    }

    // Build fragment
    std::vector<std::pair<void*, size_t>> frag_pieces;
    std::ostringstream oss;
    oss << "TS match result on link " << m_geoid.element_id << ": " << ' '
        << "Trigger number=" << dr.trigger_number << " "
        << "Oldest stored TS=" << last_ts << " "
        << "Start of window TS=" << start_win_ts << " "
        << "End of window TS=" << end_win_ts << " "
        << "Estimated newest stored TS=" << newest_ts;
    TLOG_DEBUG(TLVL_WORK_STEPS) << oss.str();

    if (rres.result_code != ResultCode::kFound) {
      ers::warning(dunedaq::readout::TrmWithEmptyFragment(ERS_HERE, oss.str()));
    } else {

      auto elements_handled = 0;

      auto last_chunk =
          end_iter->get_timestamp() == end_win_ts ? ++end_iter : end_iter;
      last_chunk++;
      for (auto iter = start_iter; iter != last_chunk; ++iter) {
        void* element = static_cast<void*>(&(*iter));

        if (element != nullptr) {
          frag_pieces.emplace_back(std::make_pair<void*, size_t>(static_cast<void*>(&(*iter)),
                                                                 std::size_t(RawType::element_size)));
          elements_handled++;
          // TLOG() << "Elements handled: " << elements_handled;
        } else {
          TLOG() << "NULLPTR NOHANDLE";
          break;
        }
      }
    }
    // Create fragment from pieces
    auto frag = std::make_unique<dataformats::Fragment>(frag_pieces);

    // Set header
    frag->set_header_fields(frag_header);
    // Push to Fragment queue
    try {
      m_fragment_sink->push(std::move(frag));
    } catch (const ers::Issue& excpt) {
      std::ostringstream oss;
      oss << "fragments output queueu for link " << m_geoid.element_id;
      ers::warning(CannotWriteToQueue(ERS_HERE, oss.str(), excpt));
    }

    return rres;
  }

  // Data access (LB)
  std::unique_ptr<LatencyBufferType>& m_latency_buffer;

  // Request source and Fragment sink
  std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& m_fragment_sink;

  // Sink for SNB data
  std::unique_ptr<appfwk::DAQSink<RawType>>& m_snb_sink;

  // Requests
  std::size_t m_max_requested_elements;

  // Completion of requests requests
  using completion_queue_t = folly::UMPSCQueue<std::future<RequestResult>, true>;
  completion_queue_t m_completion_queue;

  // The run marker
  std::atomic<bool> m_run_marker = false;

  // Stats
  std::atomic<int> m_pop_reqs;
  std::atomic<int> m_pops_count;
  std::atomic<int> m_occupancy;
  std::thread m_stats_thread;

  std::atomic<bool> m_cleanup_requested = false;

private:
  // For recording
  std::atomic<bool> m_recording = false;
  std::future<void> m_future_recording_stopper;

  // Executor
  std::thread m_executor;

  // Configuration
  bool m_configured;
  float m_pop_limit_pct;     // buffer occupancy percentage to issue a pop request
  float m_pop_size_pct;      // buffer percentage to pop
  unsigned m_pop_limit_size; // pop_limit_pct * buffer_capacity
  std::atomic<int> m_pop_counter;
  size_t m_buffer_capacity;
  dataformats::GeoID m_geoid;
  static const constexpr uint32_t m_min_delay_us = 30000; // NOLINT(build/unsigned)

  // Stats
  std::atomic<int> m_found_requested_count{ 0 };
  std::atomic<int> m_bad_requested_count{ 0 };
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_DEFAULTREQUESTHANDLERMODEL_HPP_
