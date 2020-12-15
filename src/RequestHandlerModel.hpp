/**
* @file LatencyBuffer.hpp Buffers objects for some time
* Software defined latency buffer to temporarily store objects from the
* frontend apparatus. It wraps a bounded SPSC queue from Folly for
* aligned memory access, and convenient frontPtr loads.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_REQUESTHANDLERMODEL_HPP_
#define UDAQ_READOUT_SRC_REQUESTHANDLERMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "RequestHandlerConcept.hpp"

#include "readout/datalinkhandler/Structs.hpp"

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <future>
#include <iomanip>

namespace dunedaq {
namespace readout {

template<class RawType>
class RequestHandlerModel : public RequestHandlerConcept {
public:
  explicit RequestHandlerModel(const std::string& rawtype,
                               std::atomic<bool>& marker,
                               std::function<size_t()>& occupancy_callback,
                               std::function<bool(RawType&)>& read_callback,
                               std::function<void(unsigned)>& pop_callback,
                               std::function<RawType*()>& front_callback)
  : raw_type_name_(rawtype)
  , run_marker_(marker)
  , occupancy_callback_(occupancy_callback)
  , read_callback_(read_callback)
  , pop_callback_(pop_callback)
  , front_callback_(front_callback)
  , pop_limit_pct_(0.0f)
  , pop_size_pct_(0.0f)
  , pop_limit_size_(0)
  , pop_counter_{0}
  , buffer_capacity_(0)
  {
    ERS_INFO("RequestHandlerModel created...");
    auto_pop_callback_ = std::bind(&RequestHandlerModel<RawType>::auto_pop, this);
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
    }
    ERS_INFO("RequestHandler configured. " << std::fixed << std::setprecision(2)
          << "auto-pop limit: "<< pop_limit_pct_*100.0f << "% "
          << "auto-pop size: " << pop_size_pct_*100.0f  << "%");
  }

  void start(const nlohmann::json& args)
  {
    executor_ = std::thread(&RequestHandlerModel<RawType>::executor, this);
  }

  void stop(const nlohmann::json& args)
  {
    executor_.join();
  }
 
  void auto_pop_check()
  {
    auto size_guess = occupancy_callback_();
    if (size_guess > pop_limit_size_) {
      auto execfut = std::async(std::launch::deferred, auto_pop_callback_);
      request_queue_.push(std::move(execfut));              
    }
  }

  // DataRequest struct!?
  void issue_request() 
  {
  
  }

protected:
  void auto_pop()
  {
    auto now_s = time::now_as<std::chrono::seconds>();
    auto size_guess = occupancy_callback_();
    if (size_guess > pop_limit_size_) {
      auto to_pop = pop_size_pct_ * occupancy_callback_();
      pop_callback_(to_pop);
      pop_counter_.store(pop_counter_.load()+to_pop);
      ERS_INFO("Popped " << to_pop << " elements. Total: " << pop_counter_.load() 
          << " occup: " << occupancy_callback_());
    }
  }

  void executor()
  {
   std::future<void> fut;
   while (run_marker_.load()) {
     if (request_queue_.empty()) {
       std::this_thread::sleep_for(std::chrono::microseconds(50));
     } else {
       bool success = request_queue_.try_pop(fut);
       if (!success) {
         //ers::error(CommandFacilityError(ERS_HERE, "Can't get from completion queue."));
       } else {
         fut.wait(); // trigger execution
       }
     }
   }
  }

private:
  // Configuration
  std::string raw_type_name_;
  bool configured_;

  float pop_limit_pct_; // buffer occupancy percentage to issue a pop request
  float pop_size_pct_; // buffer percentage to pop
  int pop_limit_size_;   // pop_limit_pct
  stats::counter_t pop_counter_;
  size_t buffer_capacity_;

  // Data access (LB interfaces)
  std::function<size_t()>& occupancy_callback_;
  std::function<bool(RawType&)>& read_callback_; 
  std::function<void(unsigned)>& pop_callback_;
  std::function<RawType*()>& front_callback_;

  // Internals
  typedef tbb::concurrent_queue<std::future<void>> RequestQueue;
  RequestQueue request_queue_;

  // Pop on buffer is a special request
  typedef std::function<void()> AutoPopCallback;
  AutoPopCallback auto_pop_callback_;

  // Executor
  std::thread executor_;
  std::atomic<bool>& run_marker_;
};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_REQUESTHANDLERMODEL_HPP_
