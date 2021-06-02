/**
 * @file SearchableLatencyBufferModel.hpp Buffers objects for some time
 * Software defined latency buffer to temporarily store objects from the
 * frontend apparatus. It wraps a bounded SPSC queue from Folly for
 * aligned memory access, and convenient frontPtr loads.
 * It's especially useful for fix rate and sized FE raw types, like WIB frames.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_SEARCHABLELATENCYBUFFERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_SEARCHABLELATENCYBUFFERMODEL_HPP_

#include "readout/concepts/LatencyBufferConcept.hpp"
#include "ReadoutIssues.hpp"

#include "readout/utils/SearchableProducerConsumerQueue.hpp"

#include <memory>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class RawType, class KeyType, class KeyGetter>
class SearchableLatencyBufferModel : public LatencyBufferConcept<RawType, KeyType>
{

  static constexpr uint32_t unconfigured_buffer_size = 2; // NOLINT(build/unsigned)
public:
  SearchableLatencyBufferModel()
    : m_queue(new SearchableProducerConsumerQueue<RawType, KeyType, KeyGetter>(unconfigured_buffer_size))
  {
    TLOG(TLVL_WORK_STEPS) << "Initializing non configured latency buffer";
  }

  void conf(const nlohmann::json& cfg) override
  {
    auto params = cfg.get<datalinkhandler::Conf>();
    m_queue.reset(new SearchableProducerConsumerQueue<RawType, KeyType, KeyGetter>(params.latency_buffer_size));
  }

  size_t occupancy() override { return m_queue->sizeGuess(); }

  void lock() override { m_queue->lock(); }

  void unlock() override { m_queue->unlock(); }

  // For the continous buffer, the data is moved into the Folly queue.
  bool write(RawType&& new_element) override { return m_queue->write(std::move(new_element)); }

  bool read(RawType& element) override { return m_queue->read(element); }

  bool place(RawType&& /*new_element*/, KeyType& /*key*/) override
  {
    TLOG(TLVL_WORK_STEPS) << "Undefined behavior for SearchableLatencyBufferModel!";
    return false;
  }

  bool find(RawType& element, KeyType& key) override
  {
    auto elptr = m_queue->find_element(key);
    if (elptr != nullptr) {
      element = *elptr;
      return true;
    }
    return false;
  }

  void pop(unsigned num = 1) override // NOLINT(build/unsigned)
  {
    for (unsigned i = 0; i < num; ++i) { // NOLINT(build/unsigned)
      m_queue->popFront();
    }
  }

  RawType* get_ptr(unsigned idx) override // NOLINT(build/unsigned)
  {
    if (idx == 0) {
      return m_queue->frontPtr();
    } else {
      return m_queue->readPtr(idx); // Only with accessable SPSC
    }
  }

  RawType* find_ptr(KeyType& key) override { return m_queue->find_element(key); }

  int find_index(KeyType& key) override { return m_queue->find_index(key); }

  RawType* at(int index) { return m_queue->at(index); }

  int next_index(int index) { return m_queue->next_index(index); }

private:
  std::unique_ptr<SearchableProducerConsumerQueue<RawType, KeyType, KeyGetter>> m_queue;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_SEARCHABLELATENCYBUFFERMODEL_HPP_
