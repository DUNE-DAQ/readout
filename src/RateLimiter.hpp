/**
 * @file RateLimiter.hpp Simple RateLimiter implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef UDAQ_READOUT_SRC_RATELIMITER_HPP_
#define UDAQ_READOUT_SRC_RATELIMITER_HPP_

#include "Time.hpp"

#include <time.h>
#include <unistd.h>

namespace dunedaq {
namespace readout {

/* RateLimiter usage:
 *
 *  auto limiter = RateLimiter(1000); // 1MHz
 *  limiter.init();
 *  while (duration) {
 *    // do work
 *    limiter.limit();
 *  }
 */
class RateLimiter {
public:
  RateLimiter(unsigned long long kilohertz)
  : kilohertz_(kilohertz)
  , max_overshoot_(10 * time::ms)
  , period_((1000./kilohertz_) * time::us)
  {
    init();
  }

  void init() {
    now_ = gettime();
    deadline_ = now_ + period_;
  }

  void limit() {
    if(now_ > deadline_ + max_overshoot_) {
      deadline_ = now_ + period_;
    } else {
      while(now_ < deadline_) {
        now_ = gettime();            
      }
      deadline_ += period_;
    }
  }

protected:
  time::timestamp_t gettime() {
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return time::timestamp_t(ts.tv_sec)*time::s + time::timestamp_t(ts.tv_nsec)*time::ns;            
  }

private:
  unsigned long long kilohertz_;
  time::timestamp_t max_overshoot_;
  time::timestamp_t period_;
  time::timestamp_t now_;
  time::timestamp_t deadline_;
};

} // namespace readout
} // namespace dunedaq
 
#endif // UDAQ_READOUT_SRC_RATELIMITER_HPP_
