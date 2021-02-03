/*
* @file LatencyBuffer.hpp Buffers objects for some time
* Software defined latency buffer to temporarily store objects from the
* frontend apparatus. It wraps a bounded SPSC queue from Folly for
* aligned memory access, and convenient frontPtr loads.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_
#define UDAQ_READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "RequestHandlerConcept.hpp"
#include "readout/ReusableThread.hpp"

#include "readout/datalinkhandler/Structs.hpp"

#include "dfmessages/DataRequest.hpp"
#include "dataformats/Fragment.hpp"

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>

namespace dunedaq {
namespace readout {

template<class RawType>
class DefaultRequestHandlerModel : public RequestHandlerConcept {
public:
  explicit DefaultRequestHandlerModel(const std::string& rawtype,
                                      std::atomic<bool>& marker,
                                      std::function<size_t()>& occupancy_callback,
                                      std::function<bool(RawType&)>& read_callback,
                                      std::function<void(unsigned)>& pop_callback,
                                      std::function<RawType*(unsigned)>& front_callback,
                                      std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink)
  : occupancy_callback_(occupancy_callback)
  , read_callback_(read_callback)
  , pop_callback_(pop_callback)
  , front_callback_(front_callback)
  , fragment_sink_(fragment_sink)
  , run_marker_(marker)
  , raw_type_name_(rawtype)
  , pop_limit_pct_(0.0f)
  , pop_size_pct_(0.0f)
  , pop_limit_size_(0)
  , pop_counter_{0}
  , buffer_capacity_(0)
  , pop_reqs_(0)
  , pops_count_(0)
  , occupancy_(0)
  , stats_thread_(0)
  {
    ERS_INFO("DefaultRequestHandlerModel created...");
    cleanup_request_callback_ = std::bind(&DefaultRequestHandlerModel<RawType>::cleanup_request, 
      this, std::placeholders::_1, std::placeholders::_2);
    data_request_callback_ = std::bind(&DefaultRequestHandlerModel<RawType>::data_request,
      this, std::placeholders::_1, std::placeholders::_2);
  }

  void conf(const nlohmann::json& args)
  {
    auto conf = args.get<datalinkhandler::Conf>();
    pop_limit_pct_ = conf.pop_limit_pct;
    pop_size_pct_ = conf.pop_size_pct;
    buffer_capacity_ = conf.latency_buffer_size;
    //if (configured_) {
    //  ers::error(ConfigurationError(ERS_HERE, "This object is already configured!"));
    if (pop_limit_pct_ < 0.0f || pop_limit_pct_ > 1.0f ||
        pop_size_pct_ < 0.0f || pop_size_pct_ > 1.0f) {
      ers::error(ConfigurationError(ERS_HERE, "Auto-pop percentage out of range."));
    } else {
      pop_limit_size_ = pop_limit_pct_ * buffer_capacity_;
      max_requested_elements_ = pop_limit_size_ - pop_limit_size_ * pop_size_pct_;
    }
    ERS_INFO("RequestHandler configured. " << std::fixed << std::setprecision(2)
          << "auto-pop limit: "<< pop_limit_pct_*100.0f << "% "
          << "auto-pop size: " << pop_size_pct_*100.0f  << "% "
          << "max requested elements: " << max_requested_elements_);
  }

  void start(const nlohmann::json& /*args*/)
  {
    stats_thread_.set_work(&DefaultRequestHandlerModel<RawType>::run_stats, this);
    executor_ = std::thread(&DefaultRequestHandlerModel<RawType>::executor, this);
  }

  void stop(const nlohmann::json& /*args*/)
  {
    executor_.join();
    while (!stats_thread_.get_readiness()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }
  }
 
  void auto_cleanup_check()
  {
    auto size_guess = occupancy_callback_();
    if (size_guess > pop_limit_size_) {
      dfmessages::DataRequest dr;
      auto delay_us = 0;
      auto execfut = std::async(std::launch::deferred, cleanup_request_callback_, dr, delay_us);
      completion_queue_.push(std::move(execfut));              
    }
  }

  // DataRequest struct!?
  void issue_request(dfmessages::DataRequest datarequest, unsigned delay_us = 0)
  {
    auto reqfut = std::async(std::launch::async, data_request_callback_, datarequest, delay_us);
    completion_queue_.push(std::move(reqfut));
  }

protected:
  RequestResult
  cleanup_request(dfmessages::DataRequest dr, unsigned /*delay_us*/ = 0)
  {
    //auto now_s = time::now_as<std::chrono::seconds>();
    auto size_guess = occupancy_callback_();
    if (size_guess > pop_limit_size_) {
      ++pop_reqs_;
      unsigned to_pop = pop_size_pct_ * occupancy_callback_();
      pops_count_ += to_pop;
      pop_callback_(to_pop);
      occupancy_ = occupancy_callback_();
      pops_count_.store(pops_count_.load()+to_pop);
    }
    return RequestResult(ResultCode::kCleanup, dr);
  }

  RequestResult 
  data_request(dfmessages::DataRequest dr, unsigned /*delay_us*/ = 0)
  {
    ers::error(DefaultImplementationCalled(ERS_HERE, " DefaultRequestHandlerModel ", " data_request "));
    return RequestResult(ResultCode::kUnknown, dr);
  }

  void executor()
  {
    std::future<RequestResult> fut;
    while (run_marker_.load()) {
      if (completion_queue_.empty()) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      } else {
        bool success = completion_queue_.try_pop(fut);
        if (!success) {
          //ers::error(CommandFacilityError(ERS_HERE, "Can't get from completion queue."));
        } else {
          fut.wait(); // trigger execution
          auto reqres = fut.get();
          if (reqres.result_code == ResultCode::kNotYet) { // give it another chance
            ERS_DEBUG(1, "Re-queue request. "
              << "With timestamp=" << reqres.data_request.m_trigger_timestamp
              << "delay [us] " << reqres.request_delay_us);
            issue_request(reqres.data_request, reqres.request_delay_us);
          }
        }
      }
    }
    while (!completion_queue_.empty()) { // cleanup completion queue
      bool success = completion_queue_.try_pop(fut);
      if (!success) {

      } else {
        fut.wait(); // trigger execution
        auto reqres = fut.get();
        if (reqres.result_code != ResultCode::kFound) {

        }
      }
    }
  }

  void run_stats() {
    ERS_INFO("Statistics thread started...");
    int new_pop_reqs = 0;
    int new_pop_count = 0;
    int new_occupancy = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (run_marker_.load()) {
      auto now = std::chrono::high_resolution_clock::now();
      new_pop_reqs = pop_reqs_.exchange(0);
      new_pop_count = pops_count_.exchange(0);
      new_occupancy = occupancy_;
      double seconds =  std::chrono::duration_cast<std::chrono::microseconds>(now-t0).count()/1000000.;
      ERS_DEBUG(1, "Cleanup request rate: " << new_pop_reqs/seconds/1. << " [Hz]"
          << " Dropped: " << new_pop_count
          << " Occupancy: " << new_occupancy);
      for(int i=0; i<50 && run_marker_.load(); ++i){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      t0 = now;
    }
    ERS_INFO("Statistics thread stopped...");
  }

  // Data access (LB interfaces)
  std::function<size_t()>& occupancy_callback_;
  std::function<bool(RawType&)>& read_callback_; 
  std::function<void(unsigned)>& pop_callback_;
  std::function<RawType*(unsigned)>& front_callback_;

  // Request source and Fragment sink
  std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink_;

  // Requests
  typedef std::function<RequestResult(dfmessages::DataRequest, unsigned)> RequestCallback;
  RequestCallback cleanup_request_callback_;
  RequestCallback data_request_callback_;
  std::size_t max_requested_elements_;

  // Completion of requests requests
  typedef tbb::concurrent_queue<std::future<RequestResult>> CompletionQueue;
  CompletionQueue completion_queue_;

private:
  // Executor
  std::thread executor_;
  std::atomic<bool>& run_marker_;

  // Configuration
  std::string raw_type_name_;
  bool configured_;
  float pop_limit_pct_; // buffer occupancy percentage to issue a pop request
  float pop_size_pct_;  // buffer percentage to pop
  unsigned pop_limit_size_;  // pop_limit_pct * buffer_capacity
  stats::counter_t pop_counter_;
  size_t buffer_capacity_;

  // Stats
  stats::counter_t pop_reqs_;
  stats::counter_t pops_count_;
  stats::counter_t occupancy_;
  ReusableThread stats_thread_;

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_DEFAULTREQUESTHANDLERMODEL_HPP_
