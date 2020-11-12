/**
 * @file ReadoutStatistics.hpp Readout system metrics
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTSTATISTICS_HPP_
#define UDAQ_READOUT_SRC_READOUTSTATISTICS_HPP_

#include <atomic>

namespace dunedaq::readout {

struct ParserStats {
  std::atomic<int> packet_ctr_{0};
  std::atomic<int> short_ctr_{0};
  std::atomic<int> chunk_ctr_{0};
  std::atomic<int> subchunk_ctr_{0};
  std::atomic<int> block_ctr_{0};
  std::atomic<int> error_short_ctr_{0};
  std::atomic<int> error_chunk_ctr_{0};
  std::atomic<int> error_subchunk_ctr_{0};
  std::atomic<int> error_block_ctr_{0};
  std::atomic<int> last_chunk_size_{0};
};

} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_READOUTSTATISTICS_HPP_
