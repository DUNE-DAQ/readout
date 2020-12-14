// From Module
#include "RequestHandler.hpp"
#include "ReadoutIssues.hpp"
#include "Time.hpp"

#include <ers/ers.h>

// From STD
#include <iomanip>
#include <chrono>

using namespace std::chrono_literals;

namespace dunedaq {
namespace readout {

void 
RequestHandler::configure(float pop_limit_pct, float pop_size_pct)
{
  if (configured_) {
    ers::error(ConfigurationError(ERS_HERE, "This object is already configured!"));
  } else if (pop_limit_pct < 0.0f || pop_limit_pct > 1.0f ||
             pop_size_pct < 0.0f || pop_size_pct > 1.0f) {
    ers::error(ConfigurationError(ERS_HERE, "Auto-pop percentage out of range."));
  } else {
    buffer_capacity_ = latency_buffer_->capacity();
    pop_limit_pct_ = pop_limit_pct;
    pop_size_pct_ = pop_size_pct;
    pop_limit_size_ = pop_limit_pct_ * buffer_capacity_;
    // Pop callback
    auto_pop_callback_ = std::bind(&RequestHandler::auto_pop, this);
  }  
}

void 
RequestHandler::start()
{
  executor_ = std::thread(&RequestHandler::executor, this);
}

void 
RequestHandler::stop()
{
  executor_.join();
}

void
RequestHandler::auto_pop_request()
{
  auto size_guess = latency_buffer_->sizeGuess();
  if (size_guess > pop_limit_size_) {
    auto execfut = std::async(std::launch::deferred, auto_pop_callback_);
    request_queue_.push(std::move(execfut));
  }
}

void
RequestHandler::issue_request(/*appfwk::Request ?*/)
{

}

void
RequestHandler::auto_pop() 
{
  auto now_s = time::now_as<std::chrono::seconds>();
  auto size_guess = latency_buffer_->sizeGuess();
  if (size_guess > pop_limit_size_) {
    auto to_pop = pop_size_pct_ * latency_buffer_->sizeGuess();
    for (int i=0; i<to_pop; ++i) {
      latency_buffer_->popFront();
    }
    ++pop_counter_;
  }
}

void
RequestHandler::executor()
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

}
} // namespace dunedaq::readout
