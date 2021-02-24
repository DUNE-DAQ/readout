/**
* @file LatencyBufferBase.hpp Latency buffer interface class 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_
#define READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_

namespace dunedaq {
namespace readout {

class LatencyBufferConcept {

public:
  LatencyBufferConcept() {}
  LatencyBufferConcept(const LatencyBufferConcept&)
    = delete; ///< LatencyBufferConcept is not copy-constructible
  LatencyBufferConcept& operator=(const LatencyBufferConcept&)
    = delete; ///< LatencyBufferConcept is not copy-assginable
  LatencyBufferConcept(LatencyBufferConcept&&)
    = delete; ///< LatencyBufferConcept is not move-constructible
  LatencyBufferConcept& operator=(LatencyBufferConcept&&)
    = delete; ///< LatencyBufferConcept is not move-assignable

  virtual size_t occupancy_of_buffer() = 0;   

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_
