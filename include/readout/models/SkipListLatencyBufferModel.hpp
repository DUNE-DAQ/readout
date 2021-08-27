/**
 * @file SkipListLatencyBufferModel.hpp A folly concurrent SkipList wrapper
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_

#include "readout/concepts/LatencyBufferConcept.hpp"
#include "readout/ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "folly/ConcurrentSkipList.h"

#include <memory>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class T>
class SkipListLatencyBufferModel : public LatencyBufferConcept<T>
{

public:
  // Folly typenames
  using SkipListT = typename folly::ConcurrentSkipList<T>;
  using SkipListTIter = typename SkipListT::iterator;
  using SkipListTAcc = typename folly::ConcurrentSkipList<T>::Accessor; // SKL Accessor
  using SkipListTSkip = typename folly::ConcurrentSkipList<T>::Skipper; // Skipper accessor

  SkipListLatencyBufferModel()
    : m_skip_list(folly::ConcurrentSkipList<T>::createInstance(unconfigured_head_height))
  {
    TLOG(TLVL_WORK_STEPS) << "Initializing non configured latency buffer";
  }

  struct Iterator
  {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    Iterator(SkipListTAcc&& acc, SkipListTIter iter)
      : m_acc(std::move(acc))
      , m_iter(iter)
    {}

    reference operator*() const { return *m_iter; }
    pointer operator->() { return &(*m_iter); }
    Iterator& operator++() // NOLINT(runtime/increment_decrement) :)
    {
      m_iter++;
      return *this;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_iter == b.m_iter; }
    friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_iter != b.m_iter; }

    bool good() { return m_iter.good(); }

  private:
    SkipListTAcc m_acc;
    SkipListTIter m_iter;
  };

  void conf(const nlohmann::json& /*cfg*/) override {
    // Reset datastructure
    m_skip_list = folly::ConcurrentSkipList<T>::createInstance(unconfigured_head_height);
  }

  size_t occupancy() const override
  {
    auto occupancy = 0;
    {
      SkipListTAcc acc(m_skip_list);
      occupancy = acc.size();
    }
    return occupancy;
  }

  void flush() override
  {
    pop(occupancy());
  }

  std::shared_ptr<SkipListT>& get_skip_list() { return std::ref(m_skip_list); }

  // For the continous buffer, the data is moved into the Folly queue.
  bool write(T&& new_element) override
  {
    bool success = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto ret = acc.insert(std::move(new_element)); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  bool put(T& new_element) override
  {
    bool success = false;
    {
      SkipListTAcc acc(m_skip_list);
      auto ret = acc.insert(new_element); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  bool read(T& element) override
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

  Iterator begin()
  {
    SkipListTAcc acc = SkipListTAcc(m_skip_list);
    SkipListTIter iter = acc.begin();
    return std::move(Iterator(std::move(acc), iter));
  }

  Iterator end()
  {
    SkipListTAcc acc = SkipListTAcc(m_skip_list);
    SkipListTIter iter = acc.end();
    return std::move(Iterator(std::move(acc), iter));
  }

  Iterator lower_bound(T& element, bool /*with_errors=false*/)
  {
    SkipListTAcc acc = SkipListTAcc(m_skip_list);
    SkipListTIter iter = acc.lower_bound(element);
    return std::move(Iterator(std::move(acc), iter));
  }

  const T* front() override
  {
    SkipListTAcc acc(m_skip_list);
    return acc.first();
  }

  const T* back() override
  {
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
