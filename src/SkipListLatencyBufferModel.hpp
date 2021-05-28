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
#ifndef READOUT_SRC_SKIPLISTLATENCYBUFFERMODEL_HPP_
#define READOUT_SRC_SKIPLISTLATENCYBUFFERMODEL_HPP_

#include "LatencyBufferConcept.hpp"
#include "ReadoutIssues.hpp"

#include "folly/ConcurrentSkipList.h"
#include "folly/memory/Arena.h"

#include <memory>
#include <utility>
#include <functional>
#include <assert.h>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

/*
namespace dunedaq {
namespace readout {

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

  void deallocate(char* ptr, size_t n) {
    assert(1 == allocated_.erase(ptr));
    arena_.get().deallocate(ptr, n);
  }

  bool isEmpty() const { return allocated_.empty(); }

  std::reference_wrapper<ParentAlloc> arena_;
  std::set<void*> allocated_;
};

} // namespace readout
} // namespace dunedaq
*/

/*
namespace folly {
template <typename ParentAlloc>
struct AllocatorHasTrivialDeallocate<dunedaq::readout::ParanoidArenaAlloc<ParentAlloc>>
  : AllocatorHasTrivialDeallocate<ParentAlloc> {};
} // namespace folly
*/

namespace dunedaq {
namespace readout {

template<class RawType, class KeyType, class KeyGetter>
class SkipListLatencyBufferModel : public LatencyBufferConcept<RawType, KeyType>
{
public:

  // Paranoid Arena Allocator for RawType
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
  
    void deallocate(RawType* ptr, size_t n) {
      assert(1 == allocated_.erase(ptr));
      arena_.get().deallocate(ptr, n);
    }
  
    bool isEmpty() const { return allocated_.empty(); }
  
    std::reference_wrapper<ParentAlloc> arena_;
    std::set<void*> allocated_;
  };

  // Folly typenames
  using SkipListT = typename folly::ConcurrentSkipList<RawType>;
  using SkipListTAcc = typename folly::ConcurrentSkipList<RawType>::Accessor; // SKL Accessor
  using SkipListTSkip = typename folly::ConcurrentSkipList<RawType>::Skipper; // Skipper accessor

  // Custom allocator
  using Allocator = typename folly::SysArenaAllocator<RawType>;
  using ParanoidAlloc = ParanoidArenaAlloc<Allocator>;
  using CxxAlloc = folly::CxxAllocatorAdaptor<RawType, ParanoidAlloc>;
  using ParanoidSkipList = typename folly::ConcurrentSkipList<RawType, std::less<RawType>, CxxAlloc>;
  using ParanoidSkipListAcc = typename ParanoidSkipList::Accessor; // SKL Accessor
  using ParanoidSkipListSkip = typename ParanoidSkipList::Skipper; // Skipper accessor

  SkipListLatencyBufferModel()
    : m_parent_allocator(m_arena)
    , m_paranoid_allocator(m_parent_allocator)
    , m_cxx_alloc(m_paranoid_allocator)
    , m_skip_list(ParanoidSkipList::createInstance(unconfigured_head_height, m_cxx_alloc))
  {
    TLOG(TLVL_WORK_STEPS) << "Initializing non configured latency buffer";
  }

  void conf(const nlohmann::json& /*cfg*/) override
  {
    // auto params = cfg.get<datalinkhandler::Conf>();
    // m_queue.reset(new SearchableProducerConsumerQueue<RawType, KeyType, KeyGetter>(params.latency_buffer_size));
  }

  std::shared_ptr<ParanoidSkipList>& get_skip_list() { return std::ref(m_skip_list); }

  size_t occupancy() override
  {
    auto occupancy = 0;
    {
      ParanoidSkipListAcc acc(m_skip_list);
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
      ParanoidSkipListAcc acc(m_skip_list);
      auto ret = acc.insert(std::move(new_element)); // ret T = std::pair<iterator, bool>
      success = ret.second;
    }
    return success;
  }

  /* Reads closest match. */
  bool read(RawType& element) override
  {
    bool found = false;
    {
      ParanoidSkipListAcc acc(m_skip_list);
      auto lb_node = acc.lower_bound(element);
      found = (lb_node == acc.end()) ? false : true;
      if (found) {
        element = *lb_node;
      }
    }
    return found;
  }

  bool place(RawType&& new_element, KeyType& /*key*/) override { return write(std::move(new_element)); }

  /* Finds exact match, contrary to read(). */
  bool find(RawType& element, KeyType& /*key*/) override
  {
    bool found = false;
    {
      ParanoidSkipListAcc acc(m_skip_list);
      auto node = acc.find(element);
      found = (node == acc.end()) ? false : true;
      if (found) {
        element = *node;
      }
    }
    return found;
  }

  void pop(unsigned num = 1) override // NOLINT
  {
    {
      ParanoidSkipListAcc acc(m_skip_list);
      for (unsigned i = 0; i < num; ++i) {
        acc.pop_back();
      }
    }
  }

  RawType* get_ptr(unsigned /*idx*/) override // NOLINT
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
  static constexpr uint32_t unconfigured_head_height = 2; // NOLINT
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SKIPLISTLATENCYBUFFERMODEL_HPP_
