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
  ContinousLatencyBufferModel(const size_t qsize, std::function<void(std::unique_ptr<RawType>)>& write_override)
  : folly::ProducerConsumerQueue<RawType>(qsize)
  , write_override_(write_override)
  {
    // Bind custom post process of data
    write_override_ = std::bind(&ContinousLatencyBufferModel<RawType>::write_data_to_buffer, this, std::placeholders::_1);
  }

  // For the continous buffer, the data is moved into the Folly queue.
  void write_data_to_buffer(std::unique_ptr<RawType> new_element) {
    this->write( *new_element );
  }

private:
  std::function<void(std::unique_ptr<RawType>)>& write_override_;  

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
