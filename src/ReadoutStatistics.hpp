/**
 * @file ReadoutStatistics.hpp Readout system metrics
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_READOUTSTATISTICS_HPP_
#define READOUT_SRC_READOUTSTATISTICS_HPP_

#include <atomic>

namespace dunedaq::readout::stats {

using counter_t = std::atomic<int>;

} // namespace dunedaq::readout::stats

#endif // READOUT_SRC_READOUTSTATISTICS_HPP_
