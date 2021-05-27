/**
 * @file Time.hpp Common Time related constants in UDAQ
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_TIME_HPP_
#define READOUT_SRC_TIME_HPP_

#include <chrono>
#include <cstdint> // uint64_t

namespace dunedaq {
namespace readout {
namespace time {

using timestamp_t = std::uint64_t; // NOLINT(build/unsigned)

inline constexpr timestamp_t ns = 1;
inline constexpr timestamp_t us = 1000 * ns;
inline constexpr timestamp_t ms = 1000 * us;
inline constexpr timestamp_t s = 1000 * ms;

template<typename ChronoType>
inline int64_t
now_as()
{
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<ChronoType>(duration).count();
}

} // namespace time
} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_TIME_HPP_
