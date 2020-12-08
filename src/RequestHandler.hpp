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
#ifndef UDAQ_READOUT_SRC_REQUESTHANDLER_HPP_
#define UDAQ_READOUT_SRC_REQUESTHANDLER_HPP_

#include "ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"
#include "ContinousLatencyBufferModel.hpp"
#include "DefaultParserImpl.hpp"

#include <tbb/concurrent_queue.h>

#include <atomic>
#include <thread>
#include <functional>
#include <future>

namespace dunedaq {
namespace readout {

class RequestHandlerBase {

};

class RequestHandler { // : appfwk::RequestReceiver ??
public:
  explicit RequestHandler(std::unique_ptr<ContinousLatencyBufferModel<types::WIB_SUPERCHUNK_STRUCT>>& lbuffer,
                          std::atomic<bool>& marker)
  : pop_limit_pct_(0.0f)
  , pop_size_pct_(0.0f)
  , pop_limit_size_(0)
  , pop_counter_{0}
  , buffer_capacity_(0)
  , parser_impl_() // TO BE REMOVED
  , latency_buffer_(lbuffer)
  , run_marker_(marker)
  {
    ERS_INFO("RequestHandler created...");
  }

  RequestHandler(const RequestHandler&) 
    = delete; ///< RequestHandler is not copy-constructible
  RequestHandler& operator=(const RequestHandler&) 
    = delete; ///< RequestHandler is not copy-assginable
  RequestHandler(RequestHandler&&) 
    = delete; ///< RequestHandler is not move-constructible
  RequestHandler& operator=(RequestHandler&&) 
    = delete; ///< RequestHandler is not move-assignable

  void configure(float pop_limit_pct, float pop_size_pct); 
  void start();
  void stop();
  
  // Requests
  void auto_pop_request();
  void issue_request();

protected:
  void auto_pop();
  void executor();

private:
  // Configuration
  bool configured_;
  float pop_limit_pct_; // buffer occupancy percentage to issue a pop request
  float pop_size_pct_; // buffer percentage to pop
  int pop_limit_size_;   // pop_limit_pct
  stats::counter_t pop_counter_;
  int buffer_capacity_;

  // TO BE REMOVED
  DefaultParserImpl parser_impl_;

  // Latency buffer to work on
  std::unique_ptr<ContinousLatencyBufferModel<types::WIB_SUPERCHUNK_STRUCT>>& latency_buffer_;

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

#endif // UDAQ_READOUT_SRC_REQUESTHANDLER_HPP_
