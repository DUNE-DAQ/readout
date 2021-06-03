/**
 * @file LatencyBufferBase.hpp Latency buffer interface class
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_LATENCYBUFFERCONCEPT_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_LATENCYBUFFERCONCEPT_HPP_

namespace dunedaq {
namespace readout {

/**
 * Concept of a LatencyBuffer.
 *
 * @tparam RawType the type of contained elements
 * @tparam KeyType the type of key for searchability
 */
template<class RawType>
class LatencyBufferConcept
{

public:
  LatencyBufferConcept() {}
  LatencyBufferConcept(const LatencyBufferConcept&) = delete; ///< LatencyBufferConcept is not copy-constructible
  LatencyBufferConcept& operator=(const LatencyBufferConcept&) =
    delete;                                                         ///< LatencyBufferConcept is not copy-assginable
  LatencyBufferConcept(LatencyBufferConcept&&) = delete;            ///< LatencyBufferConcept is not move-constructible
  LatencyBufferConcept& operator=(LatencyBufferConcept&&) = delete; ///< LatencyBufferConcept is not move-assignable

  virtual void resize(uint32_t) = 0;

  //! Occupancy of LB
  virtual size_t occupancy() const = 0;

  //! Move referenced object into LB
  virtual bool write(RawType&&) = 0;

  //! Move object from LB to referenced
  virtual bool read(RawType&) = 0;

  virtual bool put(RawType&) = 0;

  //! Pop N amount of elements from LB
  virtual void pop(unsigned) = 0;

  //! Lock LB
  virtual void lock() = 0;

  //! Unlock LB
  virtual void unlock() = 0;

private:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_LATENCYBUFFERCONCEPT_HPP_
