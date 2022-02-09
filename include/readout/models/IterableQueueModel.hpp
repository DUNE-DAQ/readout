/** @file IterableQueueModel.hpp
 *
 * Copyright 2012-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @author Bo Hu (bhu@fb.com)
// @author Jordan DeLong (delong.j@fb.com)

// Modification by Roland Sipos and Florian Till Groetschla
// for DUNE-DAQ software framework

#ifndef READOUT_INCLUDE_READOUT_MODELS_ITERABLEQUEUEMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_ITERABLEQUEUEMODEL_HPP_

#include "readout/ReadoutIssues.hpp"
#include "readout/concepts/LatencyBufferConcept.hpp"
#include "readout/readoutconfig/Nljs.hpp"
#include "readout/readoutconfig/Structs.hpp"

#include "logging/Logging.hpp"

#include <folly/lang/Align.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cxxabi.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

#include <xmmintrin.h>

#ifdef WITH_LIBNUMA_SUPPORT
#include <numa.h>
#endif

namespace dunedaq {
namespace readout {

/**
 * IterableQueueModel is a one producer and one consumer queue without locks.
 * Modified version of the folly::ProducerConsumerQueue via adding a readPtr function.
 * Requires  well defined and followed constraints on the consumer side.
 */
template<class T>
struct IterableQueueModel : public LatencyBufferConcept<T>
{
  typedef T value_type;

  IterableQueueModel(const IterableQueueModel&) = delete;
  IterableQueueModel& operator=(const IterableQueueModel&) = delete;

  IterableQueueModel()
    : LatencyBufferConcept<T>()
    , numa_aware_(false)
    , numa_node_(0)
    , intrinsic_allocator_(false)
    , alignment_size_(0)
    , invalid_configuration_requested_(false)
    , size_(2)
    , records_(static_cast<T*>(std::malloc(sizeof(T) * 2)))
    , readIndex_(0)
    , writeIndex_(0)
  {}

  // size must be >= 2.
  //
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (size-1), so if you start with an empty queue,
  // isFull() will return true after size-1 insertions.
  explicit IterableQueueModel(std::size_t size)
    : LatencyBufferConcept<T>() // NOLINT(build/unsigned)
    , numa_aware_(false)
    , numa_node_(0)
    , intrinsic_allocator_(false)
    , alignment_size_(0)
    , invalid_configuration_requested_(false)
    , size_(size)
    , records_(static_cast<T*>(std::malloc(sizeof(T) * size)))
    , readIndex_(0)
    , writeIndex_(0)
  {
    assert(size >= 2);
    if (!records_) {
      throw std::bad_alloc();
    }
#if 0
        ptrlogger = std::thread([&](){
          while(true) {
            auto const currentRead = readIndex_.load(std::memory_order_relaxed);
            auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
            TLOG() << "BEG:" << std::hex << &records_[0] << " END:" << &records_[size] << std::dec
                      << " R:" << currentRead << " - W:" << currentWrite
                      << " OFLOW:" << overflow_ctr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
        });
#endif
  }

  // size must be >= 2.
  // Aligned strategies
  IterableQueueModel(std::size_t size,
                     bool numa_aware = false,
                     uint8_t numa_node = 0, // NOLINT (build/unsigned)
                     bool intrinsic_allocator = false,
                     std::size_t alignment_size = 0)
    : LatencyBufferConcept<T>() // NOLINT(build/unsigned)
    , numa_aware_(numa_aware)
    , numa_node_(numa_node)
    , intrinsic_allocator_(intrinsic_allocator)
    , alignment_size_(alignment_size)
    , invalid_configuration_requested_(false)
    , size_(size)
    , readIndex_(0)
    , writeIndex_(0)
  {
    assert(size >= 2);
    allocate_memory(size, numa_aware, numa_node, intrinsic_allocator, alignment_size);

    if (!records_) {
      throw std::bad_alloc();
    }
#if 0
        ptrlogger = std::thread([&](){
          while(true) {
            auto const currentRead = readIndex_.load(std::memory_order_relaxed);
            auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
            TLOG() << "BEG:" << std::hex << &records_[0] << " END:" << &records_[size] << std::dec
                      << " R:" << currentRead << " - W:" << currentWrite
                      << " OFLOW:" << overflow_ctr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
        });
#endif
  }

  ~IterableQueueModel() { free_memory(); }

  void free_memory()
  {
    // We need to destruct anything that may still exist in our queue.
    // (No real synchronization needed at destructor time: only one
    // thread can be doing this.)
    if (!std::is_trivially_destructible<T>::value) {
      std::size_t readIndex = readIndex_;
      std::size_t endIndex = writeIndex_;
      while (readIndex != endIndex) {
        records_[readIndex].~T();
        if (++readIndex == size_) { // NOLINT(runtime/increment_decrement)
          readIndex = 0;
        }
      }
    }

    if (intrinsic_allocator_) {
      _mm_free(records_);
    } else if (numa_aware_) {
#ifdef WITH_LIBNUMA_SUPPORT
      numa_free(records_, sizeof(T) * size_);
#endif
    } else {
      std::free(records_);
    }
  }

  void allocate_memory(std::size_t size,
                       bool numa_aware = false,
                       uint8_t numa_node = 0, // NOLINT (build/unsigned)
                       bool intrinsic_allocator = false,
                       std::size_t alignment_size = 0)
  {
    assert(size >= 2);
    // TODO: check for valid alignment sizes! | July-21-2021 | Roland Sipos | rsipos@cern.ch

    if (intrinsic_allocator && alignment_size > 0) { // _mm allocator
      records_ = static_cast<T*>(_mm_malloc(sizeof(T) * size, alignment_size));

    } else if (!intrinsic_allocator && alignment_size > 0) { // std aligned allocator
      records_ = static_cast<T*>(std::aligned_alloc(alignment_size, sizeof(T) * size));

    } else if (numa_aware && numa_node >= 0 && numa_node < 8) { // numa allocator from libnuma
#ifdef WITH_LIBNUMA_SUPPORT
      records_ = static_cast<T*>(numa_alloc_onnode(sizeof(T) * size, numa_node));
#else
      throw GenericConfigurationError(ERS_HERE,
                                      "NUMA allocation was requested but program was built without USE_LIBNUMA");
#endif

    } else if (!numa_aware && !intrinsic_allocator && alignment_size == 0) {
      // Standard allocator
      records_ = static_cast<T*>(std::malloc(sizeof(T) * size));

    } else {
      // Let it fail, as expected combination might be invalid
      // records_ = static_cast<T*>(std::malloc(sizeof(T) * size_);
    }

    size_ = size;
    numa_aware_ = numa_aware;
    numa_node_ = numa_node;
    intrinsic_allocator_ = intrinsic_allocator;
    alignment_size_ = alignment_size;
  }

  bool put(T& record) { return write_(std::move(record)); }

  bool write(T&& record) override { return write_(std::move(record)); }

  // move (or copy) the value at the front of the queue to given variable
  bool read(T& record) override
  {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      // queue is empty
      return false;
    }

    auto nextRecord = currentRead + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    record = std::move(records_[currentRead]);
    records_[currentRead].~T();
    readIndex_.store(nextRecord, std::memory_order_release);
    return true;
  }

  // queue must not be empty
  void popFront()
  {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    assert(currentRead != writeIndex_.load(std::memory_order_acquire));

    auto nextRecord = currentRead + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }

    records_[currentRead].~T();
    readIndex_.store(nextRecord, std::memory_order_release);
  }

  // RS: Will this work?
  void pop(std::size_t x)
  {
    for (std::size_t i = 0; i < x; i++) {
      popFront();
    }
  }

  bool isEmpty() const
  {
    return readIndex_.load(std::memory_order_acquire) == writeIndex_.load(std::memory_order_acquire);
  }

  bool isFull() const
  {
    auto nextRecord = writeIndex_.load(std::memory_order_acquire) + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
      return false;
    }
    // queue is full
    return true;
  }

  // * If called by consumer, then true size may be more (because producer may
  //   be adding items concurrently).
  // * If called by producer, then true size may be less (because consumer may
  //   be removing items concurrently).
  // * It is undefined to call this from any other thread.
  std::size_t occupancy() const override
  {
    int ret = static_cast<int>(writeIndex_.load(std::memory_order_acquire)) -
              static_cast<int>(readIndex_.load(std::memory_order_acquire));
    if (ret < 0) {
      ret += static_cast<int>(size_);
    }
    return static_cast<std::size_t>(ret);
  }

  // maximum number of items in the queue.
  std::size_t capacity() const { return size_ - 1; }

  struct Iterator
  {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    Iterator(IterableQueueModel<T>& queue, uint32_t index) // NOLINT(build/unsigned)
      : m_queue(queue)
      , m_index(index)
    {}

    reference operator*() const { return m_queue.records_[m_index]; }
    pointer operator->() { return &m_queue.records_[m_index]; }
    Iterator& operator++() // NOLINT(runtime/increment_decrement) :)
    {
      if (good()) {
        m_index++;
        if (m_index == m_queue.size_) {
          m_index = 0;
        }
      }
      if (!good()) {
        m_index = std::numeric_limits<uint32_t>::max(); // NOLINT(build/unsigned)
      }
      return *this;
    }
    Iterator operator++(int amount) // NOLINT(runtime/increment_decrement) :)
    {
      Iterator tmp = *this;
      for (int i = 0; i < amount; ++i) {
        ++(*this);
      }
      return tmp;
    }
    friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_index == b.m_index; }
    friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_index != b.m_index; }

    bool good()
    {
      auto const currentRead = m_queue.readIndex_.load(std::memory_order_relaxed);
      auto const currentWrite = m_queue.writeIndex_.load(std::memory_order_relaxed);
      return (*this != m_queue.end()) &&
             ((m_index >= currentRead && m_index < currentWrite) ||
              (m_index >= currentRead && currentWrite < currentRead) ||
              (currentWrite < currentRead && m_index < currentRead && m_index < currentWrite));
    }

    uint32_t get_index() { return m_index; } // NOLINT(build/unsigned)

  private:
    IterableQueueModel<T>& m_queue;
    uint32_t m_index; // NOLINT(build/unsigned)
  };

  Iterator begin()
  {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      // queue is empty
      return end();
    }
    return Iterator(*this, currentRead);
  }

  const T* front() override
  {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      return nullptr;
    }
    return &records_[currentRead];
  }

  const T* back() override
  {
    auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
    if (currentWrite == readIndex_.load(std::memory_order_acquire)) {
      return nullptr;
    }
    int currentLast = currentWrite;
    if (currentLast == 0) {
      currentLast = size_ - 1;
    } else {
      currentLast--;
    }
    return &records_[currentLast];
  };

  Iterator end()
  {
    return Iterator(*this, std::numeric_limits<uint32_t>::max()); // NOLINT(build/unsigned)
  }

  void conf(const nlohmann::json& cfg) override
  {
    auto conf = cfg["latencybufferconf"].get<readoutconfig::LatencyBufferConf>();
    assert(conf.latency_buffer_size >= 2);
    free_memory();

    allocate_memory(conf.latency_buffer_size,
                    conf.latency_buffer_numa_aware,
                    conf.latency_buffer_numa_node,
                    conf.latency_buffer_intrinsic_allocator,
                    conf.latency_buffer_alignment_size);
    readIndex_ = 0;
    writeIndex_ = 0;

    if (!records_) {
      throw std::bad_alloc();
    }

    if (conf.latency_buffer_preallocation) {
      T element;
      for (size_t i = 0; i < size_ - 1; ++i) {
        write_(std::move(element));
      }
      flush();
    }
  }

  void flush() override { pop(occupancy()); }

protected:
  template<class... Args>
  bool write_(Args&&... recordArgs)
  {
    // const std::lock_guard<std::mutex> lock(m_mutex);
    auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
    auto nextRecord = currentWrite + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    // if (nextRecord == readIndex_.load(std::memory_order_acquire)) {
    // std::cout << "SPSC WARNING -> Queue is full! WRITE PASSES READ!!! \n";
    //}
    //    new (&records_[currentWrite]) T(std::forward<Args>(recordArgs)...);
    // writeIndex_.store(nextRecord, std::memory_order_release);
    // return true;

    // ORIGINAL:
    if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
      new (&records_[currentWrite]) T(std::forward<Args>(recordArgs)...);
      writeIndex_.store(nextRecord, std::memory_order_release);
      return true;
    }
    // queue is full

    ++overflow_ctr;

    return false;
  }

  // hardware_destructive_interference_size is set to 128.
  // (Assuming cache line size of 64, so we use a cache line pair size of 128 )
  std::atomic<int> overflow_ctr{ 0 };

  // NUMA awareness and aligned allocator usage
  bool numa_aware_;
  uint8_t numa_node_; // NOLINT (build/unsigned)
  bool intrinsic_allocator_;
  std::size_t alignment_size_;
  bool invalid_configuration_requested_;

  std::thread ptrlogger;

  char pad0_[folly::hardware_destructive_interference_size]; // NOLINT(runtime/arrays)
  uint32_t size_;                                            // NOLINT(build/unsigned)
  T* records_;

  alignas(folly::hardware_destructive_interference_size) std::atomic<unsigned int> readIndex_; // NOLINT(build/unsigned)
  alignas(
    folly::hardware_destructive_interference_size) std::atomic<unsigned int> writeIndex_; // NOLINT(build/unsigned)

  char pad1_[folly::hardware_destructive_interference_size - sizeof(writeIndex_)]; // NOLINT(runtime/arrays)
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_ITERABLEQUEUEMODEL_HPP_
