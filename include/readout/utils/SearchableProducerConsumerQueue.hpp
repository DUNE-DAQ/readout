/**
 * @file SearchableProducerConsumerQueue.hpp Queue that can be searched
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_UTILS_SEARCHABLEPRODUCERCONSUMERQUEUE_HPP_
#define READOUT_INCLUDE_READOUT_UTILS_SEARCHABLEPRODUCERCONSUMERQUEUE_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "AccessableProducerConsumerQueue.hpp"

namespace dunedaq {
namespace readout {

template<class T, class Key, class KeyGetter>
class SearchableProducerConsumerQueue : public AccessableProducerConsumerQueue<T>
{
public:
  explicit SearchableProducerConsumerQueue(uint32_t size) // NOLINT(build/unsigned)
    : AccessableProducerConsumerQueue<T>(size)
  {}

  typename AccessableProducerConsumerQueue<T>::Iterator lower_bound(T& element)
  {
    Key key = (m_key_getter)(element);
    unsigned int start_index =
      AccessableProducerConsumerQueue<T>::readIndex_.load(std::memory_order_relaxed); // NOLINT(build/unsigned)
    unsigned int end_index =
      AccessableProducerConsumerQueue<T>::writeIndex_.load(std::memory_order_acquire); // NOLINT(build/unsigned)

    if (start_index == end_index) {
      TLOG() << "Queue is empty" << std::endl;
      return end();
    }
    end_index = end_index == 0 ? AccessableProducerConsumerQueue<T>::size_ - 1 : end_index - 1;

    Key right_key = (m_key_getter)(AccessableProducerConsumerQueue<T>::records_[end_index]);

    if (key > right_key) {
      TLOG() << "Could not find element" << std::endl;
      return end();
    }

    while (true) {
      unsigned int diff = start_index <= end_index
                            ? end_index - start_index
                            : AccessableProducerConsumerQueue<T>::size_ + end_index - start_index;
      unsigned int middle_index = start_index + ((diff + 1) / 2);
      if (middle_index >= AccessableProducerConsumerQueue<T>::size_)
        middle_index -= AccessableProducerConsumerQueue<T>::size_;
      T* element_between = &AccessableProducerConsumerQueue<T>::records_[middle_index];
      Key middle_key = (m_key_getter)(*element_between);
      if (middle_key == key) {
        return typename AccessableProducerConsumerQueue<T>::Iterator(*this, middle_index);
      } else if (middle_key > key) {
        end_index = middle_index != 0 ? middle_index - 1 : AccessableProducerConsumerQueue<T>::size_ - 1;
      } else {
        start_index = middle_index;
      }
      if (diff == 0) {
        return typename AccessableProducerConsumerQueue<T>::Iterator(*this, middle_index);
      }
    }
  }

  typename AccessableProducerConsumerQueue<T>::Iterator end() {
    return typename AccessableProducerConsumerQueue<T>::Iterator(*this, -1);
  }

private:
  KeyGetter m_key_getter;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_UTILS_SEARCHABLEPRODUCERCONSUMERQUEUE_HPP_
