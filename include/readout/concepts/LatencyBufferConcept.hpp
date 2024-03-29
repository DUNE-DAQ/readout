/**
 * @file LatencyBufferBase.hpp Latency buffer interface class
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_LATENCYBUFFERCONCEPT_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_LATENCYBUFFERCONCEPT_HPP_

#include <nlohmann/json.hpp>

#include <cstddef>

namespace dunedaq {
namespace readout {

/**
 * Concept of a LatencyBuffer.
 *
 * @tparam RawType the type of contained elements
 * @tparam KeyType the type of key for searchability
 */
template<class T>
class LatencyBufferConcept
{

public:
  LatencyBufferConcept() {}
  LatencyBufferConcept(const LatencyBufferConcept&) = delete; ///< LatencyBufferConcept is not copy-constructible
  LatencyBufferConcept& operator=(const LatencyBufferConcept&) =
    delete;                                                         ///< LatencyBufferConcept is not copy-assginable
  LatencyBufferConcept(LatencyBufferConcept&&) = delete;            ///< LatencyBufferConcept is not move-constructible
  LatencyBufferConcept& operator=(LatencyBufferConcept&&) = delete; ///< LatencyBufferConcept is not move-assignable

  //! Configure the LB
  virtual void conf(const nlohmann::json& cfg) = 0;

  //! Occupancy of LB
  virtual std::size_t occupancy() const = 0;

  //! Move referenced object into LB
  virtual bool write(T&& element) = 0;

  //! Move object from LB to referenced
  virtual bool read(T& element) = 0;

  //! Write referenced object into LB without moving it
  // virtual bool put(T& element) = 0;

  //! Get pointer to the front of the LB
  virtual const T* front() = 0;

  //! Get pointer to the back of the LB
  virtual const T* back() = 0;

  //! Pop specified amount of elements from LB
  virtual void pop(std::size_t amount) = 0;

  //! Flush all elements from the latency buffer
  virtual void flush() = 0;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_LATENCYBUFFERCONCEPT_HPP_
