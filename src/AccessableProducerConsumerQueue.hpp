/*
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

// Modification by Roland Sipos for DUNE-DAQ software framework

#pragma once

#include <atomic>
#include <mutex>
#include <cassert>
#include <cstdlib>
#include <cxxabi.h>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <iomanip>

namespace dunedaq {
namespace readout {

/**
 * AccessableProducerConsumerQueue is a one producer and one consumer queue without locks.
 * Modified version of the folly::ProducerConsumerQueue via adding a readPtr function.
 * Requires  well defined and followed constraints on the consumer side.
 */
template <class T> struct AccessableProducerConsumerQueue {
  typedef T value_type;

  AccessableProducerConsumerQueue(const AccessableProducerConsumerQueue &) = delete;
  AccessableProducerConsumerQueue &operator=(const AccessableProducerConsumerQueue &) = delete;

  // size must be >= 2.
  //
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (size-1), so if you start with an empty queue,
  // isFull() will return true after size-1 insertions.
  explicit AccessableProducerConsumerQueue(uint32_t size) // NOLINT
      : size_(size), records_(static_cast<T *>(std::malloc(sizeof(T) * size))), readIndex_(0),
        writeIndex_(0) {
    assert(size >= 2);
    if (!records_) {
      throw std::bad_alloc();
    }
/*
    ptrlogger = std::thread([&](){
      while(true) {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
        std::cout << "BEG:" << std::hex << &records_[0] << " END:" << &records_[size] << std::dec 
                  << " R:" << currentRead << " - W:" << currentWrite 
                  << " OFLOW:" << overflow_ctr << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
*/
  }

  ~AccessableProducerConsumerQueue() {
    // We need to destruct anything that may still exist in our queue.
    // (No real synchronization needed at destructor time: only one
    // thread can be doing this.)
    if (!std::is_trivially_destructible<T>::value) {
      size_t readIndex = readIndex_;
      size_t endIndex = writeIndex_;
      while (readIndex != endIndex) {
        records_[readIndex].~T();
        if (++readIndex == size_) { // NOLINT(runtime/increment_decrement)
          readIndex = 0;
        }
      }
    }
    std::free(records_);
  }

  template <class... Args> bool write(Args &&... recordArgs) {
    //const std::lock_guard<std::mutex> lock(m_mutex);
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

  // move (or copy) the value at the front of the queue to given variable
  bool read(T &record) {
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

  // pointer to the value at the front of the queue (for use in-place) or
  // nullptr if empty.
  T *frontPtr() {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      // queue is empty
      return nullptr;
    }
    return &records_[currentRead];
  }

  // pointer to xth element starting from the front pointer (for use in-place)
  // nullpr if empty
  T *readPtr(size_t index) {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      // RS FIXME -> not enough assumption. More strict check on writeIndex is needed
      // queue is empty
      return nullptr;
    }

    auto recordIdx = currentRead + index;
    if (recordIdx >= size_) {
      recordIdx -= size_;
      if (recordIdx > size_) { // don't stomp out
        return nullptr;
      }
    }
    return &records_[recordIdx];
  }

  size_t readIdx() {
    auto const cr = readIndex_.load(std::memory_order_relaxed);
    return cr;
  } 

  size_t writeIdx() {
    auto const wr = writeIndex_.load(std::memory_order_relaxed);
    return wr;
  } 

  // queue must not be empty
  void popFront() {
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
  void popXFront(size_t x) {
    for (size_t i = 0; i < x; i++) {
      popFront();
    }
  }

  bool isEmpty() const {
    return readIndex_.load(std::memory_order_acquire) ==
           writeIndex_.load(std::memory_order_acquire);
  }

  bool isFull() const {
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
  size_t sizeGuess() const {
    int ret = static_cast<int>(writeIndex_.load(std::memory_order_acquire)) -
              static_cast<int>(readIndex_.load(std::memory_order_acquire));
    if (ret < 0) {
      ret += static_cast<int>(size_);
    }
    return static_cast<size_t>(ret);
  }

  // maximum number of items in the queue.
  size_t capacity() const { return size_ - 1; }

  void lock() { m_mutex.lock(); }
  void unlock() {m_mutex.unlock(); }

protected: // hardware_destructive_interference_size is set to 128.
         // (Assuming cache line size of 64, so we use a cache line pair size of 128 )

  std::mutex m_mutex;
  std::atomic<int> overflow_ctr{0};

  std::thread ptrlogger;

  char pad0_[folly::hardware_destructive_interference_size];  // NOLINT(runtime/arrays)
  const uint32_t size_; // NOLINT
  T *const records_;

  alignas(folly::hardware_destructive_interference_size) std::atomic<unsigned int> readIndex_;  // NOLINT
  alignas(folly::hardware_destructive_interference_size) std::atomic<unsigned int> writeIndex_; // NOLINT

  char pad1_[folly::hardware_destructive_interference_size - sizeof(writeIndex_)]; // NOLINT(runtime/arrays)
};

} // namespace readout
} // namespace dunedaq
