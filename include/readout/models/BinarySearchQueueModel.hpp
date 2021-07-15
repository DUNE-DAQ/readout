/**
 * @file BinarySearchQueueModel.hpp Queue that can be searched
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_BINARYSEARCHQUEUEMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_BINARYSEARCHQUEUEMODEL_HPP_

#include "readout/ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "IterableQueueModel.hpp"

namespace dunedaq {
namespace readout {

template<class T>
class BinarySearchQueueModel : public IterableQueueModel<T>
{
public:
  BinarySearchQueueModel()
    : IterableQueueModel<T>()
  {}

  explicit BinarySearchQueueModel(uint32_t size) // NOLINT(build/unsigned)
    : IterableQueueModel<T>(size)
  {}

  typename IterableQueueModel<T>::Iterator lower_bound(T& element, bool /*with_errors=false*/)
  {
    unsigned int start_index =
      IterableQueueModel<T>::readIndex_.load(std::memory_order_relaxed); // NOLINT(build/unsigned)
    unsigned int end_index =
      IterableQueueModel<T>::writeIndex_.load(std::memory_order_acquire); // NOLINT(build/unsigned)

    if (start_index == end_index) {
      TLOG() << "Queue is empty" << std::endl;
      return IterableQueueModel<T>::end();
    }
    end_index = end_index == 0 ? IterableQueueModel<T>::size_ - 1 : end_index - 1;

    T& right_element = IterableQueueModel<T>::records_[end_index];

    if (right_element < element) {
      TLOG() << "Could not find element" << std::endl;
      return IterableQueueModel<T>::end();
    }

    while (true) {
      unsigned int diff =
        start_index <= end_index ? end_index - start_index : IterableQueueModel<T>::size_ + end_index - start_index;
      unsigned int middle_index = start_index + ((diff + 1) / 2);
      if (middle_index >= IterableQueueModel<T>::size_)
        middle_index -= IterableQueueModel<T>::size_;
      T& element_between = IterableQueueModel<T>::records_[middle_index];
      if (diff == 0) {
        return typename IterableQueueModel<T>::Iterator(*this, middle_index);
      } else if (element < element_between) {
        end_index = middle_index != 0 ? middle_index - 1 : IterableQueueModel<T>::size_ - 1;
      } else {
        start_index = middle_index;
      }
    }
  }
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_BINARYSEARCHQUEUEMODEL_HPP_
