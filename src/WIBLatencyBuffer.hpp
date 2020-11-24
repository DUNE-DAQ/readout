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
#ifndef UDAQ_READOUT_SRC_LATENCYBUFFER_HPP_
#define UDAQ_READOUT_SRC_LATENCYBUFFER_HPP_

#include "ReadoutTypes.hpp"
#include "LatencyBufferInterface.hpp"

#include <folly/ProducerConsumerQueue.h>

#include <atomic>

namespace dunedaq {
namespace readout {

class WIBLatencyBuffer : public LatencyBufferInterface,
                         public folly::ProducerConsumerQueue<WIB_SUPERCHUNK_STRUCT> {
public:
  explicit WIBLatencyBuffer(const size_t qsize)
  : folly::ProducerConsumerQueue<WIB_SUPERCHUNK_STRUCT>(qsize)
  {
    ERS_INFO("WIBLatencyBuffer created...");
  }

  WIBLatencyBuffer(const WIBLatencyBuffer&) 
    = delete; ///< WIBLatencyBuffer is not copy-constructible
  WIBLatencyBuffer& operator=(const WIBLatencyBuffer&) 
    = delete; ///< WIBLatencyBuffer is not copy-assginable
  WIBLatencyBuffer(WIBLatencyBuffer&&) 
    = delete; ///< WIBLatencyBuffer is not move-constructible
  WIBLatencyBuffer& operator=(WIBLatencyBuffer&&) 
    = delete; ///< WIBLatencyBuffer is not move-assignable

private:

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_LATENCYBUFFER_HPP_
