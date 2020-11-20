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

#include <folly/ProducerConsumerQueue.h>

#include "ReadoutTypes.hpp"

#include <atomic>

namespace dunedaq {
namespace readout {

template <class T>
class LatencyBuffer : public folly::ProducerConsumerQueue<T> {
public: 
  LatencyBuffer(const LatencyBuffer&) 
    = delete; ///< LatencyBuffer is not copy-constructible
  LatencyBuffer& operator=(const LatencyBuffer&) 
    = delete; ///< LatencyBuffer is not copy-assginable
  LatencyBuffer(LatencyBuffer&&) 
    = delete; ///< LatencyBuffer is not move-constructible
  LatencyBuffer& operator=(LatencyBuffer&&) 
    = delete; ///< LatencyBuffer is not move-assignable

private:

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_LATENCYBUFFER_HPP_
