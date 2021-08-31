/**
 * @file SkipListLatencyBufferModel.hpp A folly concurrent SkipList wrapper
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_

#include "readout/ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/concepts/LatencyBufferConcept.hpp"

#include "logging/Logging.hpp"

#include "folly/ConcurrentSkipList.h"
#include "folly/memory/Arena.h"

#include <memory>
#include <utility>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

template<class T>
class SkipListLatencyBufferModel : public LatencyBufferConcept<T>
{

public:

  // Paranoid Arena Allocator for T
  template <typename ParentAlloc>
  struct ParanoidArenaAlloc {
    explicit ParanoidArenaAlloc(ParentAlloc& arena) : arena_(arena) {}
    ParanoidArenaAlloc(ParanoidArenaAlloc const&) = delete;
    ParanoidArenaAlloc(ParanoidArenaAlloc&&) = delete;
    ParanoidArenaAlloc& operator=(ParanoidArenaAlloc const&) = delete;
    ParanoidArenaAlloc& operator=(ParanoidArenaAlloc&&) = delete;

    void* allocate(size_t size) {
      void* result = arena_.get().allocate(size);
      allocated_.insert(result);
      return result;
    }

    void deallocate(T* ptr, size_t n) {
      assert(1 == allocated_.erase(ptr));
      arena_.get().deallocate(ptr, n);
    }

    bool isEmpty() const { return allocated_.empty(); }

    std::reference_wrapper<ParentAlloc> arena_;
    std::set<void*> allocated_;
  };

  // Folly typenames
  using SkipListT = typename folly::ConcurrentSkipList<T>;
  using SkipListTIter = typename SkipListT::iterator;
  using SkipListTAcc = typename folly::ConcurrentSkipList<T>::Accessor; // SKL Accessor
  using SkipListTSkip = typename folly::ConcurrentSkipList<T>::Skipper; // Skipper accessor

  // Custom allocator
  using Allocator = typename folly::SysArenaAllocator<T>;
  using ParanoidAlloc = ParanoidArenaAlloc<Allocator>;
  using CxxAlloc = folly::CxxAllocatorAdaptor<T, ParanoidAlloc>;
  using ParanoidSkipList = typename folly::ConcurrentSkipList<T, std::less<T>, CxxAlloc>;
  using ParanoidSkipListTIter = typename ParanoidSkipList::iterator;
  using ParanoidSkipListTAcc = typename ParanoidSkipList::Accessor; // SKL Accessor
  using ParanoidSkipListTSkip = typename ParanoidSkipList::Skipper; // Skipper accessor

  SkipListLatencyBufferModel()
    : m_parent_allocator(m_arena)
    , m_paranoid_allocator(m_parent_allocator)
    , m_cxx_alloc(m_paranoid_allocator)
    , m_skip_list(ParanoidSkipList::createInstance(unconfigured_head_height, m_cxx_alloc))
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

    Iterator(ParanoidSkipListTAcc&& acc, ParanoidSkipListTIter iter)
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
    ParanoidSkipListTAcc m_acc;
    ParanoidSkipListTIter m_iter;
  };

  void conf(const nlohmann::json& /*cfg*/) override
  {
    // Reset datastructure
    m_skip_list = folly::ConcurrentSkipList<T>::createInstance(unconfigured_head_height);
  }

  size_t occupancy() const override
  {
    auto occupancy = 0;
    {
      ParanoidSkipListTAcc acc(m_skip_list);
      occupancy = acc.size();
    }
    return occupancy;
  }


  void flush() override { pop(occupancy()); }

  std::shared_ptr<ParanoidSkipList>& get_skip_list() { return std::ref(m_skip_list); }

  // For the continous buffer, the data is moved into the Folly queue.
  bool write(T&& new_element) override
  {
    bool success = false;
    {
      ParanoidSkipListTAcc acc(m_skip_list);
      auto ret = acc.insert(std::move(new_element)); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  bool put(T& new_element) override
  {
    bool success = false;
    {
      ParanoidSkipListTAcc acc(m_skip_list);
      auto ret = acc.insert(new_element); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  bool read(T& element) override
  {
    bool found = false;
    {
      ParanoidSkipListTAcc acc(m_skip_list);
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
    ParanoidSkipListTAcc acc = ParanoidSkipListTAcc(m_skip_list);
    ParanoidSkipListTIter iter = acc.begin();
    return std::move(Iterator(std::move(acc), iter));
  }

  Iterator end()
  {
    ParanoidSkipListTAcc acc = ParanoidSkipListTAcc(m_skip_list);
    ParanoidSkipListTIter iter = acc.end();
    return std::move(Iterator(std::move(acc), iter));
  }

  Iterator lower_bound(T& element, bool /*with_errors=false*/)
  {
    ParanoidSkipListTAcc acc = ParanoidSkipListTAcc(m_skip_list);
    ParanoidSkipListTIter iter = acc.lower_bound(element);
    return std::move(Iterator(std::move(acc), iter));
  }

  const T* front() override
  {
    ParanoidSkipListTAcc acc(m_skip_list);
    return acc.first();
  }

  const T* back() override
  {
    ParanoidSkipListTAcc acc(m_skip_list);
    return acc.last();
  }

  void pop(size_t num = 1) override // NOLINT(build/unsigned)
  {
    {
      ParanoidSkipListTAcc acc(m_skip_list);
      for (unsigned i = 0; i < num; ++i) {
        acc.pop_back();
      }
    }
  }

private:
  // Arena
  folly::SysArena m_arena;

  // Arena allocator
  Allocator m_parent_allocator;

  // Paranoid allocator
  ParanoidAlloc m_paranoid_allocator;

  // Cxx Alloc
  CxxAlloc m_cxx_alloc;

  // Concurrent SkipList
  std::shared_ptr<ParanoidSkipList> m_skip_list;

  // Conf
  static constexpr uint32_t unconfigured_head_height = 2; // NOLINT(build/unsigned)
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_SKIPLISTLATENCYBUFFERMODEL_HPP_
