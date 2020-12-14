/**
* @file Time.hpp Common Time related constants in UDAQ
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_TIME_HPP_
#define UDAQ_READOUT_SRC_TIME_HPP_

#include <cstdint> // uint64_t
#include <chrono>

namespace dunedaq {
namespace readout {
namespace time {

/*
 * Timestamp and constants
 * */
typedef std::uint64_t timestamp_t;

static constexpr timestamp_t ns = 1;
static constexpr timestamp_t us = 1000 * ns;
static constexpr timestamp_t ms = 1000 * us;
static constexpr timestamp_t s = 1000 * ms;

template<typename ChronoType>
inline int64_t now_as() { // NOLINT
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<ChronoType>(duration).count();
}

} // namespace time
} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_TIME_HPP_
