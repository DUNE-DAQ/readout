/**
 * @file SkipListLatencyBufferModel.hpp Buffers objects for some time
 * Software defined latency buffer to temporarily store objects from the
 * frontend apparatus. It wraps a bounded SPSC queue from Folly for
 * aligned memory access, and convenient frontPtr loads.
 * It's especially useful for fix rate and sized FE raw types, like WIB frames.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_

#include "readout/concepts/LatencyBufferConcept.hpp"
#include "ReadoutIssues.hpp"

#include "folly/ConcurrentSkipList.h"

#include <memory>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class RawType, class KeyType, class KeyGetter>
class SkipListLatencyBufferModel : public LatencyBufferConcept<RawType, KeyType>
{

public:
  // Folly typenames
  using SkipListT = typename folly::ConcurrentSkipList<RawType>;
  using SkipListTAcc = typename folly::ConcurrentSkipList<RawType>::Accessor; // SKL Accessor
  using SkipListTSkip = typename folly::ConcurrentSkipList<RawType>::Skipper; // Skipper accessor

  SkipListLatencyBufferModel()
    : m_skip_list(folly::ConcurrentSkipList<RawType>::createInstance(unconfigured_head_height))
  {
    TLOG(TLVL_WORK_STEPS) << "Initializing non configured latency buffer";
  }

  void conf(const nlohmann::json& /*cfg*/) override
  {
    // auto params = cfg.get<datalinkhandler::Conf>();
    // m_queue.reset(new SearchableProducerConsumerQueue<RawType, KeyType, KeyGetter>(params.latency_buffer_size));
  }

  std::shared_ptr<SkipListT>& get_skip_list() { return std::ref(m_skip_list); }

  size_t occupancy() override
  {
    auto occupancy = 0;
    {
      SkipListTAcc acc(m_skip_list);
      occupancy = acc.size();
    }
    return occupancy;
  }

  void lock() override
  {
    // m_queue->lock();
  }

  void unlock() override
  {
    // m_queue->unlock();
  }

  // For the continous buffer, the data is moved into the Folly queue.
  bool write(RawType&& new_element) override
  {
    bool success = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto ret = acc.insert(std::move(new_element)); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  // Reads closest match.
  bool read(RawType& element) override
  {
    bool found = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto lb_node = acc.lower_bound(element);
      found = (lb_node == acc.end()) ? false : true;
      if (found) {
        element = *lb_node;
      }
    }
    return found;
  }

  bool place(RawType&& new_element, KeyType& /*key*/) override { return write(std::move(new_element)); }

  // Finds exact match, contrary to read().
  bool find(RawType& element, KeyType& /*key*/) override
  {
    bool found = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto node = acc.find(element);
      found = (node == acc.end()) ? false : true;
      if (found) {
        element = *node;
      }
    }
    return found;
  }

  void pop(unsigned num = 1) override // NOLINT(build/unsigned)
  {
    {
      SkipListTAcc acc(m_skip_list);
      for (unsigned i = 0; i < num; ++i) {
        acc.pop_back();
      }
    }
  }

  RawType* get_ptr(unsigned /*idx*/) override // NOLINT(build/unsigned)
  {
    TLOG(TLVL_WORK_STEPS) << "Undefined behavior for SkipListLatencyBufferModel!";
    return nullptr;
  }

  RawType* find_ptr(KeyType& /*key*/) override
  {
    return nullptr;
    // return m_queue->find_element(key);
  }

  int find_index(KeyType& /*key*/) override
  {
    return -1;
    // return m_queue->find_index(key);
  }

private:
  // Concurrent SkipList
  std::shared_ptr<SkipListT> m_skip_list;

  // Conf
  static constexpr uint32_t unconfigured_head_height = 2; // NOLINT(build/unsigned)
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_
