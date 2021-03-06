/**
 * @file RateLimiter.hpp Simple RateLimiter implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_RATELIMITER_HPP_
#define READOUT_SRC_RATELIMITER_HPP_

#include "Time.hpp"

#include <ctime>
#include <unistd.h>

namespace dunedaq {
namespace readout {

/** RateLimiter usage:
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
  explicit RateLimiter(double kilohertz)
  : m_kilohertz(kilohertz)
  , m_max_overshoot(10 * time::ms)
  , m_period( static_cast<time::timestamp_t>((1000.f/m_kilohertz) * static_cast<double>(time::us)) )
  {
    init();
  }

  void init() {
    m_now = gettime();
    m_deadline = m_now + m_period;
  }

  void limit() {
    if(m_now > m_deadline + m_max_overshoot) {
      m_deadline = m_now + m_period;
    } else {
      while(m_now < m_deadline) {
        m_now = gettime();            
      }
      m_deadline += m_period;
    }
  }

protected:
  time::timestamp_t gettime() {
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return time::timestamp_t(ts.tv_sec)*time::s + time::timestamp_t(ts.tv_nsec)*time::ns;            
  }

private:
  double m_kilohertz;
  time::timestamp_t m_max_overshoot;
  time::timestamp_t m_period;
  time::timestamp_t m_now;
  time::timestamp_t m_deadline;
};

} // namespace readout
} // namespace dunedaq
 
#endif // READOUT_SRC_RATELIMITER_HPP_
