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

template <class RawType>
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

  virtual void conf(const nlohmann::json& cfg) = 0;
  virtual size_t occupancy() = 0;
  virtual bool write(RawType&&) = 0;
  virtual bool read(RawType&) = 0;
  virtual void pop(unsigned) = 0;
  virtual RawType* getPtr(unsigned) = 0;
  virtual void lock() = 0;
  virtual void unlock() = 0;

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_LATENCYBUFFERCONCEPT_HPP_
