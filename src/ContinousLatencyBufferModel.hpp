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
class ContinousLatencyBufferModel : public LatencyBufferConcept,
                                    //public folly::ProducerConsumerQueue<RawType> {
                                    public AccessableProducerConsumerQueue<RawType> {
public:
  ContinousLatencyBufferModel(const size_t qsize,
                              std::function<size_t()>& occupancy_override,
                              std::function<void(std::unique_ptr<RawType>)>& write_override,
                              std::function<bool(RawType&)>& read_override,
                              std::function<void(unsigned)>& pop_override,       // NOLINT
                              std::function<RawType*(unsigned)>& front_override) // NOLINT
//  : folly::ProducerConsumerQueue<RawType>(qsize)
  : AccessableProducerConsumerQueue<RawType>(qsize)
  , occupancy_override_(occupancy_override)
  , write_override_(write_override)
  , read_override_(read_override)
  , pop_override_(pop_override)
  , front_override_(front_override)
  {
    // Bind custom post process of data
    occupancy_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::occupancy_of_buffer, this);
    write_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::write_data_to_buffer, this, std::placeholders::_1);
    read_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::read_data_from_buffer, this, std::placeholders::_1);
    pop_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::pop_data_from_buffer, this, std::placeholders::_1);
    front_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::front_data_of_buffer, this, std::placeholders::_1);
  }

  size_t occupancy_of_buffer() {
    return this->sizeGuess();
  }

  // For the continous buffer, the data is moved into the Folly queue.
  void 
  write_data_to_buffer(std::unique_ptr<RawType> new_element) 
  {
    this->write( *new_element );
  }

  bool 
  read_data_from_buffer(RawType& element) 
  {
    return this->read(element);
  }

  void 
  pop_data_from_buffer(unsigned num) // NOLINT
  {
    for (unsigned i=0; i<num; ++i) { // NOLINT
      this->popFront();
    }
  }

  RawType* 
  front_data_of_buffer(unsigned idx) // NOLINT
  {
    if (idx == 0)
      return this->frontPtr();
    else
      return this->readPtr(idx); // Only with accessable SPSC
  }

private:
  std::function<size_t()>& occupancy_override_;
  std::function<void(std::unique_ptr<RawType>)>& write_override_;  
  std::function<bool(RawType&)>& read_override_;
  std::function<void(unsigned)>& pop_override_;       // NOLINT
  std::function<RawType*(unsigned)>& front_override_; // NOLINT

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
