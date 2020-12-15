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
#ifndef UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
#define UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "LatencyBufferConcept.hpp"

#include <folly/ProducerConsumerQueue.h>

namespace dunedaq {
namespace readout {

template<class RawType>
class ContinousLatencyBufferModel : public LatencyBufferConcept,
                                    public folly::ProducerConsumerQueue<RawType> {
public:
  ContinousLatencyBufferModel(const size_t qsize,
                              std::function<size_t()>& occupancy_override,
                              std::function<void(std::unique_ptr<RawType>)>& write_override,
                              std::function<bool(RawType&)>& read_override,
                              std::function<void(unsigned)>& pop_override,
                              std::function<RawType*()>& front_override)
  : folly::ProducerConsumerQueue<RawType>(qsize)
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
    front_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::front_data_of_buffer, this);
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
  pop_data_from_buffer(unsigned num) 
  {
    for (unsigned i=0; i<num; ++i) {
      this->popFront();
    }
  }

  RawType* 
  front_data_of_buffer() 
  {
    return this->frontPtr();
  }

private:
  std::function<size_t()>& occupancy_override_;
  std::function<void(std::unique_ptr<RawType>)>& write_override_;  
  std::function<bool(RawType&)>& read_override_;
  std::function<void(unsigned)>& pop_override_;
  std::function<RawType*()>& front_override_;

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
