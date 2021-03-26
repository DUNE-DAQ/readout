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
#include "readout/ReadoutLogging.hpp"

#include "dfmessages/DataRequest.hpp"
#include "dataformats/Fragment.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>
#include <utility>
#include <memory>
#include <string>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

template<class RawType>
class DefaultRequestHandlerModel : public RequestHandlerConcept {
public:
  explicit DefaultRequestHandlerModel(const std::string& rawtype,
                                      std::atomic<bool>& marker,
                                      std::function<size_t()>& m_occupancycallback,
                                      std::function<bool(RawType&)>& read_callback,
                                      std::function<void(unsigned)>& pop_callback,
                                      std::function<RawType*(unsigned)>& front_callback,
                                      std::function<void()>& lock_callback,
                                      std::function<void()>& unlock_callback,
                                      std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                                      std::unique_ptr<appfwk::DAQSink<RawType>>& snb_sink)
  : m_occupancy_callback(m_occupancycallback)
  , m_read_callback(read_callback)
  , m_pop_callback(pop_callback)
  , m_front_callback(front_callback)
  , m_lock_callback(lock_callback)
  , m_unlock_callback(unlock_callback)
  , m_fragment_sink(fragment_sink)
  , m_snb_sink(snb_sink)
  , m_run_marker(marker)
  , m_raw_type_name(rawtype)
  , m_pop_limit_pct(0.0f)
  , m_pop_size_pct(0.0f)
  , m_pop_limit_size(0)
  , m_pop_counter{0}
  , m_buffer_capacity(0)
  , m_pop_reqs(0)
  , m_pops_count(0)
  , m_occupancy(0)
  , m_stats_thread(0)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "DefaultRequestHandlerModel created...";
    m_cleanup_request_callback = std::bind(&DefaultRequestHandlerModel<RawType>::cleanup_request, 
      this, std::placeholders::_1, std::placeholders::_2);
    m_data_request_callback = std::bind(&DefaultRequestHandlerModel<RawType>::data_request,
      this, std::placeholders::_1, std::placeholders::_2);
  }

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
    //m_run_marker.store(true);
    m_stats_thread.set_work(&DefaultRequestHandlerModel<RawType>::run_stats, this);
    m_executor = std::thread(&DefaultRequestHandlerModel<RawType>::executor, this);
  }

  void stop(const nlohmann::json& /*args*/)
  {
    //m_run_marker.store(false);
    m_executor.join();
    while (!m_stats_thread.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
  }
 
  void auto_cleanup_check()
  {
    //TLOG_DEBUG(TLVL_WORK_STEPS) << "Enter auto_cleanup_check";
    auto size_guess = m_occupancy_callback();
    if (size_guess > m_pop_limit_size) {
      dfmessages::DataRequest dr;
      auto delay_us = 0;
      auto execfut = std::async(std::launch::deferred, m_cleanup_request_callback, dr, delay_us);
      m_completion_queue.push(std::move(execfut));              
    }
  }

  // DataRequest struct!?
  void issue_request(dfmessages::DataRequest datarequest, unsigned delay_us = 0) // NOLINT
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "Enter issue_request";
    auto reqfut = std::async(std::launch::async, m_data_request_callback, datarequest, delay_us);
    m_completion_queue.push(std::move(reqfut));
  }

protected:
  RequestResult
  cleanup_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0) // NOLINT
  {
    //auto now_s = time::now_as<std::chrono::seconds>();
    auto size_guess = m_occupancy_callback();
    if (size_guess > m_pop_limit_size) {
      ++m_pop_reqs;
      unsigned to_pop = m_pop_size_pct * m_occupancy_callback();
      m_pops_count += to_pop;

      // SNB handling
      RawType element;
      for (uint i = 0; i < to_pop; ++i) {
        if (m_read_callback(element)) {
          //try {
          std::cout << "Push to queue" << std::endl;
            m_snb_sink->push(element, std::chrono::milliseconds(0));
          //} catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
            //TLOG_DEBUG(TLVL_WORK_STEPS) << "Could not write to queue";
          //}
        } else {
          // Change this to throw an error
          TLOG_DEBUG(TLVL_WORK_STEPS) << "Could not read from buffer";
        }
        m_pop_callback(1);
      }

      m_occupancy = m_occupancy_callback();
      m_pops_count.store(m_pops_count.load()+to_pop);
    }
    return RequestResult(ResultCode::kCleanup, dr);
  }

  RequestResult 
  data_request(dfmessages::DataRequest dr, unsigned /** delay_us */ = 0) // NOLINT
  {
    ers::error(DefaultImplementationCalled(ERS_HERE, " DefaultRequestHandlerModel ", " data_request "));
    return RequestResult(ResultCode::kUnknown, dr);
  }

  void executor()
  {
    std::future<RequestResult> fut;
    while (m_run_marker.load() || !m_completion_queue.empty()) {
      if (m_completion_queue.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      } else {
        bool success = m_completion_queue.try_pop(fut);
        if (!success) {
          ers::error(CannotReadFromQueue(ERS_HERE, "RequestsCompletionQueue."));
        } else {
          //m_lock_callback();
          fut.wait(); // trigger execution
          //m_unlock_callback();
          auto reqres = fut.get();
          //TLOG() << "Request result handled: " << resultCodeAsString(reqres.result_code);
          if (reqres.result_code == ResultCode::kNotYet && m_run_marker.load()) { // give it another chance
            TLOG_DEBUG(TLVL_WORK_STEPS) << "Re-queue request. "
              << "With timestamp=" << reqres.data_request.trigger_timestamp
              << "delay [us] " << reqres.request_delay_us;
            issue_request(reqres.data_request, reqres.request_delay_us);
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

  // Data access (LB interfaces)
  std::function<size_t()>& m_occupancy_callback;
  std::function<bool(RawType&)>& m_read_callback; 
  std::function<void(unsigned)>& m_pop_callback;
  std::function<RawType*(unsigned)>& m_front_callback;
  std::function<void()>& m_lock_callback;
  std::function<void()>& m_unlock_callback;

  // Request source and Fragment sink
  std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& m_fragment_sink;

  // Sink for SNB data
  std::unique_ptr<appfwk::DAQSink<RawType>>& m_snb_sink;

  // Requests
  using request_callback_t = std::function<RequestResult(dfmessages::DataRequest, unsigned)>;
  request_callback_t m_cleanup_request_callback;
  request_callback_t m_data_request_callback;
  std::size_t m_max_requested_elements;

  // Completion of requests requests
  using completion_queue_t = tbb::concurrent_queue<std::future<RequestResult>>;
  completion_queue_t m_completion_queue;

  // The run marker
  std::atomic<bool>& m_run_marker;

private:
  // Executor
  std::thread m_executor;

  // Configuration
  std::string m_raw_type_name;
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
  ReusableThread m_stats_thread;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_
