/**
* @file LatencyBufferBase.hpp Latency buffer base class 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_
#define UDAQ_READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_

namespace dunedaq {
namespace readout {

class LatencyBufferConcept {

public:
  explicit LatencyBufferConcept() {}
  LatencyBufferConcept(const LatencyBufferConcept&)
    = delete; ///< LatencyBufferConcept is not copy-constructible
  LatencyBufferConcept& operator=(const LatencyBufferConcept&)
    = delete; ///< LatencyBufferConcept is not copy-assginable
  LatencyBufferConcept(LatencyBufferConcept&&)
    = delete; ///< LatencyBufferConcept is not move-constructible
  LatencyBufferConcept& operator=(LatencyBufferConcept&&)
    = delete; ///< LatencyBufferConcept is not move-assignable

private:
  
};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_
