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

namespace dunedaq {
namespace readout {
namespace time {

/*
 * Timestamp and constants
 * */
typedef std::uint64_t timestamp_t;

static const timestamp_t ns = 1;
static const timestamp_t us = 1000 * ns;
static const timestamp_t ms = 1000 * us;
static const timestamp_t s = 1000 * ms;

} // namespace time
} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_TIME_HPP_
