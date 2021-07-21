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

#include "readout/ReadoutIssues.hpp"
#include "readout/concepts/RequestHandlerConcept.hpp"
#include "readout/utils/ReusableThread.hpp"

#include "readout/datalinkhandler/Structs.hpp"

#include "dataformats/Fragment.hpp"
#include "dataformats/Types.hpp"
#include "dfmessages/DataRequest.hpp"
#include "logging/Logging.hpp"
#include "readout/FrameErrorRegistry.hpp"
#include "readout/ReadoutLogging.hpp"

#include <boost/asio.hpp>

#include <folly/concurrency/UnboundedQueue.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <iomanip>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <chrono>

using dunedaq::readout::logging::TLVL_HOUSEKEEPING;
using dunedaq::readout::logging::TLVL_QUEUE_PUSH;
using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class ReadoutType, class LatencyBufferType>
class DefaultRequestHandlerModel : public RequestHandlerConcept<ReadoutType, LatencyBufferType>
{
public:
  explicit DefaultRequestHandlerModel(
    std::unique_ptr<LatencyBufferType>& latency_buffer,
    std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
    std::unique_ptr<appfwk::DAQSink<ReadoutType>>& snb_sink,
    std::unique_ptr<FrameErrorRegistry>& error_registry)
    : m_latency_buffer(latency_buffer)
    , m_fragment_sink(fragment_sink)
    , m_snb_sink(snb_sink)
    , m_waiting_requests()
    , m_waiting_requests_lock()
    , m_pop_reqs(0)
    , m_pops_count(0)
    , m_occupancy(0)
    , m_pop_limit_pct(0.0f)
    , m_pop_size_pct(0.0f)
    , m_pop_limit_size(0)
    , m_pop_counter{ 0 }
    , m_buffer_capacity(0)
    , m_response_time_log()
    , m_response_time_log_lock()
    , m_error_registry(error_registry)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "DefaultRequestHandlerModel created...";
  }

  using RequestResult = typename dunedaq::readout::RequestHandlerConcept<ReadoutType, LatencyBufferType>::RequestResult;
  using ResultCode = typename dunedaq::readout::RequestHandlerConcept<ReadoutType, LatencyBufferType>::ResultCode;

  void conf(const nlohmann::json& args)
  {
    auto conf = args.get<datalinkhandler::Conf>();
    m_pop_limit_pct = conf.pop_limit_pct;
    m_pop_size_pct = conf.pop_size_pct;
    m_buffer_capacity = conf.latency_buffer_size;
    m_num_request_handling_threads = conf.num_request_handling_threads;
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
    m_geoid.system_type = ReadoutType::system_type;

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
    m_stats_thread = std::thread(&DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>::run_stats, this);
    m_waiting_queue_thread =
      std::thread(&DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>::check_waiting_requests, this);
    m_request_handler_thread_pool = std::make_unique<boost::asio::thread_pool>(m_num_request_handling_threads);
  }

  void stop(const nlohmann::json& /*args*/)
  {
    m_run_marker.store(false);
    // if (m_recording) throw CommandError(ERS_HERE, "Recording is still ongoing!");
    if (m_future_recording_stopper.valid())
      m_future_recording_stopper.wait();
    m_stats_thread.join();
    m_waiting_queue_thread.join();
    m_request_handler_thread_pool->join();
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

  void cleanup_check() override
  {
    std::unique_lock<std::mutex> lock(m_cv_mutex);
    if (m_latency_buffer->occupancy() > m_pop_limit_size && !m_cleanup_requested.exchange(true)) {
      m_cv.wait(lock, [&] { return m_requests_running == 0; });
      cleanup();
      m_cleanup_requested = false;
      m_cv.notify_all();
    }
  }

  void issue_request(dfmessages::DataRequest datarequest) override
  {
    std::unique_lock<std::mutex> lock(m_cv_mutex);
    m_cv.wait(lock, [&] { return !m_cleanup_requested; });
    m_requests_running++;
    boost::asio::post(*m_request_handler_thread_pool, [&, datarequest]() { // start a thread from pool
      auto t_req_begin = std::chrono::high_resolution_clock::now();
      auto result = data_request(datarequest);
      {
        std::lock_guard<std::mutex> lock(m_cv_mutex);
        m_requests_running--;
      }
      m_cv.notify_all();
      if (result.result_code == ResultCode::kFound) {
        try { // Push to Fragment queue
          TLOG_DEBUG(TLVL_QUEUE_PUSH) << "Sending fragment with trigger_number "
                                      << result.fragment->get_trigger_number() << ", run number "
                                      << result.fragment->get_run_number() << ", and GeoID "
                                      << result.fragment->get_element_id();
          m_fragment_sink->push(std::move(result.fragment));
        } catch (const ers::Issue& excpt) {
          std::ostringstream oss;
          oss << "fragments output queue for link " << m_geoid.element_id;
          ers::warning(CannotWriteToQueue(ERS_HERE, oss.str(), excpt));
        }
      } else if (result.result_code == ResultCode::kNotYet) {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "Re-queue request. "
                                    << "With timestamp=" << result.data_request.trigger_timestamp;
        std::lock_guard<std::mutex> wait_lock_guard(m_waiting_requests_lock);
        m_waiting_requests.push(datarequest);
      }
      auto t_req_end = std::chrono::high_resolution_clock::now();
      auto us_req_took = std::chrono::duration_cast<std::chrono::microseconds>(t_req_end - t_req_begin);
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Responding to data request took: " << us_req_took.count() << "[us]";
      if (result.result_code == ResultCode::kFound) {
        std::lock_guard<std::mutex> time_lock_guard(m_response_time_log_lock);
        m_response_time_log.push_back( std::make_pair<int, int>(result.data_request.trigger_number, us_req_took.count()) );
      }
    });
  }

  void get_info(datalinkhandlerinfo::Info& info) override
  {
    info.found_requested = m_found_requested_count;
    info.bad_requested = m_bad_requested_count;
    info.request_window_too_old = m_request_gone;
    info.retry_request = m_retry_request;
    info.uncategorized_request = m_uncategorized_request;
    info.cleanups = m_cleanups;
    info.num_waiting_requests = m_waiting_requests.size();
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
    fh.sequence_number = dr.sequence_number;
    fh.element_id = { m_geoid.system_type, m_geoid.region_id, m_geoid.element_id };
    fh.fragment_type = static_cast<dataformats::fragment_type_t>(ReadoutType::fragment_type);
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

  void cleanup()
  {
    // auto now_s = time::now_as<std::chrono::seconds>();
    auto size_guess = m_latency_buffer->occupancy();
    if (size_guess > m_pop_limit_size) {
      ++m_pop_reqs;
      unsigned to_pop = m_pop_size_pct * m_latency_buffer->occupancy();

      // SNB handling
      if (m_recording) {
        ReadoutType element;
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
      m_error_registry->remove_errors_until(m_latency_buffer->front()->get_timestamp());
    }
    m_cleanups++;
  }

  void check_waiting_requests()
  {
    while (m_run_marker.load()) {
      {
        std::lock_guard<std::mutex> lock_guard(m_waiting_requests_lock);
        if (m_latency_buffer->occupancy() != 0) {
          auto last_frame = m_latency_buffer->back();       // NOLINT
          uint64_t newest_ts = last_frame->get_timestamp(); // NOLINT(build/unsigned)
          while (!m_waiting_requests.empty() && m_waiting_requests.top().window_end < newest_ts) {
            dfmessages::DataRequest request = m_waiting_requests.top();
            issue_request(request);
            m_waiting_requests.pop();
          }
        }
      }
      cleanup_check();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

      std::unique_lock<std::mutex> time_lock_guard(m_response_time_log_lock);
      if (!m_response_time_log.empty()) {
        std::ostringstream oss;
        oss << "Completed data requests [trig id, took us]: ";
        while (!m_response_time_log.empty()) {
        //for (int i = 0; i < m_response_time_log.size(); ++i) {
          auto& fr = m_response_time_log.front();
          oss << fr.first << ". in " << fr.second << " | ";
          m_response_time_log.pop_front();
        }
        TLOG_DEBUG(TLVL_HOUSEKEEPING) << oss.str();
      }
      time_lock_guard.unlock();
      for (int i = 0; i < 50 && m_run_marker.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      t0 = now;
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread stopped...";
  }

  RequestResult data_request(dfmessages::DataRequest dr) override
  {
    // Prepare response
    RequestResult rres(ResultCode::kUnknown, dr);

    // Prepare FragmentHeader and empty Fragment pieces list
    auto frag_header = create_fragment_header(dr);
    std::vector<std::pair<void*, size_t>> frag_pieces;
    std::ostringstream oss;

    if (m_latency_buffer->occupancy() != 0) {
      // Data availability is calculated here
      auto front_frame = m_latency_buffer->front();     // NOLINT
      auto last_frame = m_latency_buffer->back();       // NOLINT
      uint64_t last_ts = front_frame->get_timestamp();  // NOLINT(build/unsigned)
      uint64_t newest_ts = last_frame->get_timestamp(); // NOLINT(build/unsigned)

      uint64_t start_win_ts = dr.window_begin; // NOLINT(build/unsigned)
      uint64_t end_win_ts = dr.window_end;     // NOLINT(build/unsigned)
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Data request for "
                                  << "Trigger TS=" << dr.trigger_timestamp << " "
                                  << "Oldest stored TS=" << last_ts << " "
                                  << "Newest stored TS=" << newest_ts << " "
                                  << "Start of window TS=" << start_win_ts << " "
                                  << "End of window TS=" << end_win_ts;

      // List of safe-extraction conditions
      if (last_ts <= start_win_ts && end_win_ts <= newest_ts) { // data is there
        ReadoutType request_element;
        request_element.set_timestamp(start_win_ts);
        auto start_iter = m_error_registry->has_error() ? m_latency_buffer->lower_bound(request_element, true)
                                                        : m_latency_buffer->lower_bound(request_element, false);
        if (start_iter == m_latency_buffer->end()) {
          // Due to some concurrent access, the start_iter could not be retrieved successfully, try again
          ++m_retry_request;
          rres.result_code = ResultCode::kNotYet; // give it another chance
        } else {
          rres.result_code = ResultCode::kFound;
          ++m_found_requested_count;

          auto elements_handled = 0;

          ReadoutType* element = &(*start_iter);
          while (start_iter.good() && element->get_timestamp() < end_win_ts) {
            if (element->get_timestamp() < start_win_ts ||
                element->get_timestamp() + (ReadoutType::frames_per_element - 1) * ReadoutType::tick_dist >=
                  end_win_ts) {
              // We don't need the whole superchunk
              for (auto frame_iter = element->begin(); frame_iter != element->end(); frame_iter++) {
                if ((*frame_iter).get_timestamp() >= start_win_ts && (*frame_iter).get_timestamp() < end_win_ts) {
                  frag_pieces.emplace_back(std::make_pair<void*, size_t>(static_cast<void*>(&(*frame_iter)),
                                                                         std::size_t(ReadoutType::frame_size)));
                }
              }
            } else {
              // We are somewhere in the middle -> the whole superchunk can be copied
              frag_pieces.emplace_back(std::make_pair<void*, size_t>(static_cast<void*>(&(*start_iter)),
                                                                     std::size_t(ReadoutType::element_size)));
            }

            elements_handled++;
            ++start_iter;
            element = &(*start_iter);
          }
        }
      } else if (last_ts > start_win_ts) { // data is gone.
        frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
        rres.result_code = ResultCode::kNotFound;
        ++m_request_gone;
        ++m_bad_requested_count;
      } else if (newest_ts < end_win_ts) {
        ++m_retry_request;
        rres.result_code = ResultCode::kNotYet; // give it another chance
      } else {
        TLOG() << "Don't know how to categorise this request";
        frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
        rres.result_code = ResultCode::kNotFound;
        ++m_uncategorized_request;
        ++m_bad_requested_count;
      }

      // Requeue if needed
      if (rres.result_code == ResultCode::kNotYet) {
        if (m_run_marker.load()) {
          return rres; // If kNotYet, return immediately, don't check for fragment pieces.
        } else {
          frag_header.error_bits |= (0x1 << static_cast<size_t>(dataformats::FragmentErrorBits::kDataNotFound));
          rres.result_code = ResultCode::kNotFound;
          ++m_bad_requested_count;
        }
      }

      // Build fragment
      oss << "TS match result on link " << m_geoid.element_id << ": " << ' ' << "Trigger number=" << dr.trigger_number
          << " "
          << "Oldest stored TS=" << last_ts << " "
          << "Start of window TS=" << start_win_ts << " "
          << "End of window TS=" << end_win_ts << " "
          << "Estimated newest stored TS=" << newest_ts;
      TLOG_DEBUG(TLVL_WORK_STEPS) << oss.str();
    } else {
      ++m_bad_requested_count;
    }

    if (rres.result_code != ResultCode::kFound) {
      ers::warning(dunedaq::readout::TrmWithEmptyFragment(ERS_HERE, oss.str()));
    }

    // Create fragment from pieces
    rres.fragment = std::make_unique<dataformats::Fragment>(frag_pieces);

    // Set header
    rres.fragment->set_header_fields(frag_header);

    return rres;
  }

  // Data access (LB)
  std::unique_ptr<LatencyBufferType>& m_latency_buffer;

  // Request source and Fragment sink
  std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& m_fragment_sink;

  // Sink for SNB data
  std::unique_ptr<appfwk::DAQSink<ReadoutType>>& m_snb_sink;

  // Requests
  std::size_t m_max_requested_elements;

  std::mutex m_cv_mutex;
  std::condition_variable m_cv;

  std::atomic<bool> m_cleanup_requested = false;
  std::atomic<int> m_requests_running = 0;

  // Priority queue for waiting requests
  class Comparator
  {
  public:
    bool operator()(dfmessages::DataRequest& left, dfmessages::DataRequest& right)
    {
      return left.window_end > right.window_end;
    }
  };
  std::priority_queue<dfmessages::DataRequest, std::vector<dfmessages::DataRequest>, Comparator> m_waiting_requests;
  std::mutex m_waiting_requests_lock;

  // The run marker
  std::atomic<bool> m_run_marker = false;

  // Stats
  std::atomic<int> m_pop_reqs;
  std::atomic<int> m_pops_count;
  std::atomic<int> m_occupancy;
  std::thread m_stats_thread;
  std::thread m_waiting_queue_thread;

  std::atomic<int> m_cleanups{ 0 };

private:
  // For recording
  std::atomic<bool> m_recording = false;
  std::future<void> m_future_recording_stopper;

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
  std::atomic<int> m_request_gone{ 0 };
  std::atomic<int> m_retry_request{ 0 };
  std::atomic<int> m_uncategorized_request{ 0 };

  // Request response time log
  std::deque<std::pair<int, int>> m_response_time_log;
  std::mutex m_response_time_log_lock;

  // Data extractor threads pool 
  std::unique_ptr<boost::asio::thread_pool> m_request_handler_thread_pool;
  size_t m_num_request_handling_threads = 0;

  std::unique_ptr<FrameErrorRegistry>& m_error_registry;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_DEFAULTREQUESTHANDLERMODEL_HPP_
