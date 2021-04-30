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
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class RawType>
class ContinousLatencyBufferModel : public LatencyBufferConcept<RawType> {

static constexpr uint32_t unconfigured_buffer_size = 2; // NOLINT
public:

  ContinousLatencyBufferModel() : m_queue(new AccessableProducerConsumerQueue<RawType>(unconfigured_buffer_size)) {
    TLOG(TLVL_WORK_STEPS) << "Initializing non configured latency buffer";
  }

  void conf(const nlohmann::json& cfg) override {
    auto params = cfg.get<datalinkhandler::Conf>();
    m_queue.reset(new AccessableProducerConsumerQueue<RawType>(params.latency_buffer_size));
  }

  size_t occupancy() override {
    return m_queue->sizeGuess();
  }

  void lock() override {
    m_queue->lock();
  }

  void unlock() override {
    m_queue->unlock();
  }

  // For the continous buffer, the data is moved into the Folly queue.
  bool
  write(RawType&& new_element) override
  {
    return m_queue->write( std::move(new_element) );
  }

  bool 
  read(RawType& element) override
  {
    return m_queue->read(element);
  }

  void 
  pop(unsigned num = 1) override// NOLINT
  {
    for (unsigned i=0; i<num; ++i) { // NOLINT
      m_queue->popFront();
    }
  }

  RawType* 
  getPtr(unsigned idx) override// NOLINT
  {
    if (idx == 0) {
      return m_queue->frontPtr();
    } else {
      return m_queue->readPtr(idx); // Only with accessable SPSC
    }
  }

private:
  std::unique_ptr<AccessableProducerConsumerQueue<RawType>> m_queue;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_CONTINOUSLATENCYBUFFERMODEL_HPP_
