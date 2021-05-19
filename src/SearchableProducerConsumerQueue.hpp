/**
 * @file SearchableProducerConsumerQueue.hpp Queue that can be searched
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_SEARCHABLEPRODUCERCONSUMERQUEUE_HPP_
#define READOUT_SRC_SEARCHABLEPRODUCERCONSUMERQUEUE_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "AccessableProducerConsumerQueue.hpp"

namespace dunedaq {
namespace readout {

template<class T, class Key, class KeyGetter>
class SearchableProducerConsumerQueue : public AccessableProducerConsumerQueue<T> {
public:
  explicit SearchableProducerConsumerQueue(uint32_t size) : AccessableProducerConsumerQueue<T>(size) { // NOLINT

  }

  T* find_element(Key key) {
    unsigned int start_index = AccessableProducerConsumerQueue<T>::readIndex_.load(std::memory_order_relaxed);
    unsigned int end_index = AccessableProducerConsumerQueue<T>::writeIndex_.load(std::memory_order_acquire);

    if (start_index == end_index) {
      TLOG() << "Queue is empty" << std::endl;
      return nullptr;
    }
    end_index -= 1;

    Key right_key = (m_key_getter)(AccessableProducerConsumerQueue<T>::records_[end_index]);

    if (key > right_key) {
      TLOG() << "Could not find element" << std::endl;
      return nullptr;
    }

    while (true) {
      unsigned int diff = start_index <= end_index ? end_index - start_index : AccessableProducerConsumerQueue<T>::size_ + end_index - start_index;
      unsigned int middle_index = start_index + ((diff+1)/2);
      if (middle_index >= AccessableProducerConsumerQueue<T>::size_) middle_index -= AccessableProducerConsumerQueue<T>::size_;
      T* element_between = &AccessableProducerConsumerQueue<T>::records_[middle_index];
      Key middle_key = (m_key_getter)(*element_between);
      if (middle_key == key) {
        return element_between;
      } else if (middle_key > key) {
        end_index = middle_index != 0 ? middle_index - 1 : AccessableProducerConsumerQueue<T>::size_-1;
      } else {
        start_index = middle_index;
      }
      if (diff == 0) {
        if (middle_key > key) {
          return element_between;
        } else {
          return (middle_index == AccessableProducerConsumerQueue<T>::size_ - 1) ? &AccessableProducerConsumerQueue<T>::records_[0] : element_between+1;
        }
      }
    }
  }

  int find_index(Key key) { // NOLINT
    unsigned int start_index = AccessableProducerConsumerQueue<T>::readIndex_.load(std::memory_order_relaxed); // NOLINT
    unsigned int end_index = AccessableProducerConsumerQueue<T>::writeIndex_.load(std::memory_order_acquire); // NOLINT

    if (start_index == end_index) {
      TLOG() << "Queue is empty" << std::endl;
      return -1;
    }
    end_index -= 1;

    Key right_key = (m_key_getter)(AccessableProducerConsumerQueue<T>::records_[end_index]);

    if (key > right_key) {
      TLOG() << "Could not find element" << std::endl;
      return -1;
    }

    while (true) {
      unsigned int diff = start_index <= end_index ? end_index - start_index : AccessableProducerConsumerQueue<T>::size_ + end_index - start_index;
      unsigned int middle_index = start_index + ((diff+1)/2);
      if (middle_index >= AccessableProducerConsumerQueue<T>::size_) middle_index -= AccessableProducerConsumerQueue<T>::size_;
      T* element_between = &AccessableProducerConsumerQueue<T>::records_[middle_index];
      Key middle_key = (m_key_getter)(*element_between);
      if (middle_key == key) {
        return middle_index;
      } else if (middle_key > key) {
        end_index = middle_index != 0 ? middle_index - 1 : AccessableProducerConsumerQueue<T>::size_-1;
      } else {
        start_index = middle_index;
      }
      if (diff == 0) {
        if (middle_key > key) {
          return middle_index;
        } else {
          return (middle_index == AccessableProducerConsumerQueue<T>::size_ - 1) ? 0 : middle_index+1;
        }
      }
    }
  }

  T* at(int index) {
    return &AccessableProducerConsumerQueue<T>::records_[index];
  }

  int next_index(int index) {
    return (index == AccessableProducerConsumerQueue<T>::size_ - 1) ? 0 : index + 1;
  }

private:
  KeyGetter m_key_getter;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_SEARCHABLEPRODUCERCONSUMERQUEUE_HPP_
