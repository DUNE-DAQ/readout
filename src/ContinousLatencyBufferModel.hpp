/**
* @file ContinousLatencyBufferModel.hpp Buffers objects for some time
* Software defined latency buffer to temporarily store objects from the
* frontend apparatus. It wraps a bounded SPSC queue from Folly for
* aligned memory access, and convenient frontPtr loads.
* It's especially useful for fix rate and sized FE raw types, like WIB frames.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
#define READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "LatencyBufferConcept.hpp"

//#include <folly/ProducerConsumerQueue.h>
#include "AccessableProducerConsumerQueue.hpp"

#include <memory>

namespace dunedaq {
namespace readout {

template<class RawType>
class ContinousLatencyBufferModel : public LatencyBufferConcept<RawType>,
                                    //public folly::ProducerConsumerQueue<RawType> {
                                    public AccessableProducerConsumerQueue<RawType> {
public:
  ContinousLatencyBufferModel(const size_t qsize) // NOLINT
//  : folly::ProducerConsumerQueue<RawType>(qsize)
  : AccessableProducerConsumerQueue<RawType>(qsize)
  {

  }

  size_t occupancy() override {
    return AccessableProducerConsumerQueue<RawType>::sizeGuess();
  }

  void lock() override {
    AccessableProducerConsumerQueue<RawType>::lock();
  }

  void unlock() override {
    AccessableProducerConsumerQueue<RawType>::unlock();
  }

  // For the continous buffer, the data is moved into the Folly queue.
  bool
  write(RawType new_element) override
  {
    return AccessableProducerConsumerQueue<RawType>::write( std::move(new_element) );
  }

  bool 
  read(RawType& element) override
  {
    return AccessableProducerConsumerQueue<RawType>::read(element);
  }

  void 
  pop(unsigned num) override// NOLINT
  {
    for (unsigned i=0; i<num; ++i) { // NOLINT
      AccessableProducerConsumerQueue<RawType>::popFront();
    }
  }

  RawType* 
  front(unsigned idx) override// NOLINT
  {
    if (idx == 0)
      return AccessableProducerConsumerQueue<RawType>::frontPtr();
    else
      return AccessableProducerConsumerQueue<RawType>::readPtr(idx); // Only with accessable SPSC
  }

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
