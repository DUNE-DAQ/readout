/**
* @file DefaultRequestHandlerModel.hpp Default mode of operandy for 
* request handling: 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_
#define READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "RequestHandlerConcept.hpp"
#include "readout/ReusableThread.hpp"

#include "readout/datalinkhandler/Structs.hpp"

#include "dfmessages/DataRequest.hpp"
#include "dataformats/Fragment.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include <folly/concurrency/UnboundedQueue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>
#include <utility>
#include <memory>
#include <string>

using dunedaq::readout::logging::TLVL_WORK_STEPS;
using dunedaq::readout::logging::TLVL_HOUSEKEEPING;

namespace dunedaq {
namespace readout {

template<class RawType, class LatencyBufferType>
class DefaultRequestHandlerModel : public RequestHandlerConcept<RawType, LatencyBufferType> {
public:
  explicit DefaultRequestHandlerModel(std::unique_ptr<LatencyBufferType>& latency_buffer,
                                      std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                                      std::unique_ptr<appfwk::DAQSink<RawType>>& snb_sink)
  : m_latency_buffer(latency_buffer)
  , m_fragment_sink(fragment_sink)
  , m_snb_sink(snb_sink)
  , m_pop_limit_pct(0.0f)
  , m_pop_size_pct(0.0f)
  , m_pop_limit_size(0)
  , m_pop_counter{0}
  , m_buffer_capacity(0)
  , m_pop_reqs(0)
  , m_pops_count(0)
  , m_occupancy(0)
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
    //if (m_configured) {
    //  ers::error(ConfigurationError(ERS_HERE, "This object is already configured!"));
    if (m_pop_limit_pct < 0.0f || m_pop_limit_pct > 1.0f ||
        m_pop_size_pct < 0.0f || m_pop_size_pct > 1.0f) {
      ers::error(ConfigurationError(ERS_HERE, "Auto-pop percentage out of range."));
    } else {
      m_pop_limit_size = m_pop_limit_pct * m_buffer_capacity;
      m_max_requested_elements = m_pop_limit_size - m_pop_limit_size * m_pop_size_pct;
    }
	std::ostringstream oss;
	oss << "RequestHandler configured. " << std::fixed << std::setprecision(2)
		<< "auto-pop limit: "<< m_pop_limit_pct*100.0f << "% "
		<< "auto-pop size: " << m_pop_size_pct*100.0f  << "% "
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
    //if (m_recording) throw CommandError(ERS_HERE, "Recording is still ongoing!");
    if (m_future_recording_stopper.valid()) m_future_recording_stopper.wait();
    m_executor.join();
    m_stats_thread.join();
  }

  void record(const nlohmann::json& args) override {
    auto conf = args.get<datalinkhandler::RecordingParams>();
    if (m_recording.load()) {
      TLOG() << "A recording is still running, no new recording was started!" << std::endl;
      return;
    }
    m_future_recording_stopper = std::async([&]() {
      TLOG() << "Start recording" << std::endl;
      m_recording.exchange(true);
      std::this_thread::sleep_for(std::chrono::seconds(conf.duration));
      TLOG() << "Stop recording" << std::endl;
      m_recording.exchange(false);
    });
  }
 
  void auto_cleanup_check()
  {
    //TLOG_DEBUG(TLVL_WORK_STEPS) << "Enter auto_cleanup_check";
    auto size_guess = m_latency_buffer->occupancy();
    if (!m_cleanup_requested && size_guess > m_pop_limit_size) {
      dfmessages::DataRequest dr;
      auto delay_us = 0;
      auto execfut = std::async(std::launch::deferred, &DefaultRequestHandlerModel<RawType, LatencyBufferType>::cleanup_request, this, dr, delay_us);
      m_completion_queue.enqueue(std::move(execfut));
      m_cleanup_requested = true;
    }
  }

  // DataRequest struct!?
  void issue_request(dfmessages::DataRequest datarequest, unsigned delay_us = 0) // NOLINT
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Enter issue_request";
    auto reqfut = std::async(std::launch::async, &DefaultRequestHandlerModel<RawType, LatencyBufferType>::data_request, this, datarequest, delay_us);
    m_completion_queue.enqueue(std::move(reqfut));
  }

protected:
  RequestResult
  cleanup_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0) // NOLINT
  {
    //auto now_s = time::now_as<std::chrono::seconds>();
    auto size_guess = m_latency_buffer->occupancy();
    if (size_guess > m_pop_limit_size) {
      ++m_pop_reqs;
      unsigned to_pop = m_pop_size_pct * m_latency_buffer->occupancy();

      // SNB handling
      if (m_recording) {
        RawType element;
        for (unsigned i = 0; i < to_pop; ++i) { // NOLINT
          if (m_latency_buffer->read(element)) {
            try {
              if (m_recording) m_snb_sink->push(element, std::chrono::milliseconds(0));
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
      m_pops_count += to_pop;     

      m_occupancy = m_latency_buffer->occupancy();
      m_pops_count.store(m_pops_count.load()+to_pop);
    }
    m_cleanup_requested = false;
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
          //m_lock_callback();
          fut.wait(); // trigger execution
          //m_unlock_callback();
          auto reqres = fut.get();
          //TLOG() << "Request result handled: " << resultCodeAsString(reqres.result_code);

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
              << "With timestamp=" << reqres.data_request.trigger_timestamp
              << "delay [us] " << reqres.request_delay_us;
            issue_request(reqres.data_request, 0);
          }
        }
      }
    }
  }

  void run_stats() {
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
      double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
      TLOG_DEBUG(TLVL_HOUSEKEEPING) << "Cleanup request rate: " << new_pop_reqs/seconds/1. << " [Hz]"
          << " Dropped: " << new_pop_count
          << " Occupancy: " << new_occupancy;
      for(int i=0; i<50 && m_run_marker.load(); ++i){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      t0 = now;
    }
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Statistics thread stopped...";
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

private:
  // For recording
  std::atomic<bool> m_recording = false;
  std::future<void> m_future_recording_stopper;

  // Executor
  std::thread m_executor;

  // Configuration
  bool m_configured;
  float m_pop_limit_pct; // buffer occupancy percentage to issue a pop request
  float m_pop_size_pct;  // buffer percentage to pop
  unsigned m_pop_limit_size;  // pop_limit_pct * buffer_capacity
  stats::counter_t m_pop_counter;
  size_t m_buffer_capacity;

  // Stats
  stats::counter_t m_pop_reqs;
  stats::counter_t m_pops_count;
  stats::counter_t m_occupancy;
  std::thread m_stats_thread;

  std::atomic<bool> m_cleanup_requested = false;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_
