/**
* @file LatencyBufferInterface.hpp Buffers objects for some time
* Software defined latency buffer to temporarily store objects from the
* frontend apparatus. It wraps a bounded SPSC queue from Folly for
* aligned memory access, and convenient frontPtr loads.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_LATENCYBUFFERINTERFACE_HPP_
#define UDAQ_READOUT_SRC_LATENCYBUFFERINTERFACE_HPP_

#include <atomic>

namespace dunedaq {
namespace readout {

class LatencyBufferInterface {
public:
  explicit LatencyBufferInterface() { }

  LatencyBufferInterface(const LatencyBufferInterface&) 
    = delete; ///< LatencyBufferInterface is not copy-constructible
  LatencyBufferInterface& operator=(const LatencyBufferInterface&) 
    = delete; ///< LatencyBufferInterface is not copy-assginable
  LatencyBufferInterface(LatencyBufferInterface&&) 
    = delete; ///< LatencyBufferInterface is not move-constructible
  LatencyBufferInterface& operator=(LatencyBufferInterface&&) 
    = delete; ///< LatencyBufferInterface is not move-assignable

private:

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_LATENCYBUFFERINTERFACE_HPP_
