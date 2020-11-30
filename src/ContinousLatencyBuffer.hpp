/**
* @file LatencyBuffer.hpp Buffers objects for some time
* Software defined latency buffer to temporarily store objects from the
* frontend apparatus. It wraps a bounded SPSC queue from Folly for
* aligned memory access, and convenient frontPtr loads.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFER_HPP_
#define UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFER_HPP_

#include "LatencyBufferBase.hpp"
#include "DefaultParserImpl.hpp"

#include <folly/ProducerConsumerQueue.h>

#include <typeinfo>

namespace dunedaq {
namespace readout {

template<class RawDataType>
class ContinousLatencyBuffer : public LatencyBufferBase,
                               public folly::ProducerConsumerQueue<RawDataType> {
public:
  ContinousLatencyBuffer(const size_t qsize)
  : folly::ProducerConsumerQueue<RawDataType>(qsize)
  {

    // need to implement from POP from Source and push to Folly....
    

    // Bind custom post process of data
    //parser_impl_.post_process_chunk_func =
    //  std::bind(&ContinousLatencyBuffer<RawDataType>::write_data_to_buffer, this, std::placeholders::_1);
  }

  // LatencyBuffer implements type conversion from felix chunk to RawType (store type)
  //void write_data_to_buffer(const felix::packetformat::chunk& chunk);

private:

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_CONTINOUSLATENCYBUFFER_HPP_
