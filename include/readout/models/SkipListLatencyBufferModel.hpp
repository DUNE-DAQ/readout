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

template<class RawType>
class SkipListLatencyBufferModel : public LatencyBufferConcept<RawType>
{

public:
  // Folly typenames
  using SkipListT = typename folly::ConcurrentSkipList<RawType>;
  using SkipListTIter = typename SkipListT::iterator;
  using SkipListTAcc = typename folly::ConcurrentSkipList<RawType>::Accessor; // SKL Accessor
  using SkipListTSkip = typename folly::ConcurrentSkipList<RawType>::Skipper; // Skipper accessor

  SkipListLatencyBufferModel()
    : m_skip_list(folly::ConcurrentSkipList<RawType>::createInstance(unconfigured_head_height))
  {
    TLOG(TLVL_WORK_STEPS) << "Initializing non configured latency buffer";
  }

  struct Iterator
  {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = RawType;
    using pointer           = RawType*;
    using reference         = RawType&;

    Iterator(std::unique_ptr<SkipListTAcc>&& acc, SkipListTIter iter) : m_acc(std::move(acc)), m_iter(iter) {

    }

    reference operator*() const {
      return *m_iter;
    }
    pointer operator->() {
      return &(*m_iter);
    }
    Iterator& operator++() {
      m_iter++;
      return *this;
    }
    /*
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }
     */
    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.m_iter == b.m_iter;
    };
    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return a.m_iter != b.m_iter;
    };

    bool good() {
      return m_iter.good();
    }

  private:
    std::unique_ptr<SkipListTAcc> m_acc;
    SkipListTIter m_iter;
  };

  void resize(size_t /*new_size*/) override {

  }

  size_t occupancy() const override {
    auto occupancy = 0;
    {
      SkipListTAcc acc(m_skip_list);
      occupancy = acc.size();
    }
    return occupancy;
  }

  std::shared_ptr<SkipListT>& get_skip_list() { return std::ref(m_skip_list); }


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

  bool put(RawType& new_element) override {
    bool success = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto ret = acc.insert(new_element); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  bool read(RawType& element) override
  {
    bool found = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto lb_node = acc.begin();
      found = (lb_node == acc.end()) ? false : true;
      if (found) {
        element = *lb_node;
      }
    }
    return found;
  }

  Iterator begin() {
    std::unique_ptr<SkipListTAcc> acc = std::make_unique<SkipListTAcc>(m_skip_list);
    SkipListTIter iter = acc->begin();
    return std::move(Iterator(std::move(acc), iter));
  }

  Iterator end() {
    std::unique_ptr<SkipListTAcc> acc = std::make_unique<SkipListTAcc>(m_skip_list);
    SkipListTIter iter = acc->end();
    return std::move(Iterator(std::move(acc), iter));
  }

  Iterator lower_bound(RawType& element) {
    std::unique_ptr<SkipListTAcc> acc = std::make_unique<SkipListTAcc>(m_skip_list);
    SkipListTIter iter = acc->lower_bound(element);
    return std::move(Iterator(std::move(acc), iter));
  }

  const RawType* front() override {
    SkipListTAcc acc(m_skip_list);
    return acc.first();
  }

  const RawType* back() override {
    SkipListTAcc acc(m_skip_list);
    return acc.last();
  }

  void pop(size_t num = 1) override // NOLINT(build/unsigned)
  {
    {
      SkipListTAcc acc(m_skip_list);
      for (unsigned i = 0; i < num; ++i) {
        acc.pop_back();
      }
    }
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
