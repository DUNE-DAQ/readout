/**
 * @file VariableSizeElementQueue.hpp Queue that can handle element wrappers
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_VARIABLESIZEELEMENTQUEUE_HPP_
#define READOUT_SRC_VARIABLESIZEELEMENTQUEUE_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "AccessableProducerConsumerQueue.hpp"

using namespace dunedaq::readout::logging;

namespace dunedaq {
  namespace readout {

    template<class T>
    class VariableSizeElementQueue : public AccessableProducerConsumerQueue<T> {
    public:
      VariableSizeElementQueue(uint32_t size) : AccessableProducerConsumerQueue<T>(size) {

      }

      T* find(uint32_t timestamp) {
        unsigned int startIndex = AccessableProducerConsumerQueue<T>::readIndex_.load(std::memory_order_relaxed);
        unsigned int endIndex = AccessableProducerConsumerQueue<T>::writeIndex_.load(std::memory_order_acquire);
        uint32_t left_timestamp;
        uint32_t right_timestamp;

        if (startIndex == endIndex) {
          TLOG() << "Queue is empty" << std::endl;
          return nullptr;
        }
        endIndex -= 1;

        memcpy(&left_timestamp, AccessableProducerConsumerQueue<T>::records_[startIndex].data.get(), sizeof(uint32_t));
        memcpy(&right_timestamp, AccessableProducerConsumerQueue<T>::records_[endIndex].data.get(), sizeof(uint32_t));

        if (timestamp > right_timestamp) {
          TLOG() << "Could not find element" << std::endl;
          return nullptr;
        }

        while (true) {
          unsigned int diff = startIndex <= endIndex ? endIndex - startIndex : AccessableProducerConsumerQueue<T>::size_ + endIndex - startIndex;
          unsigned int indexMiddle = startIndex + ((diff+1)/2);
          if (indexMiddle >= AccessableProducerConsumerQueue<T>::size_) indexMiddle -= AccessableProducerConsumerQueue<T>::size_;
          T* element_between = &AccessableProducerConsumerQueue<T>::records_[indexMiddle];
          uint32_t middle_timestamp;
          memcpy(&middle_timestamp, element_between->data.get(), sizeof(uint32_t));
          if (middle_timestamp == timestamp) {
            return element_between;
          } else if (middle_timestamp > timestamp) {
            endIndex = indexMiddle != 0 ? indexMiddle - 1 : AccessableProducerConsumerQueue<T>::size_-1;
          } else {
            startIndex = indexMiddle;
          }
          if (diff == 0) {
            if (middle_timestamp > timestamp) {
              return element_between;
            } else {
              return (indexMiddle == AccessableProducerConsumerQueue<T>::size_ - 1) ? &AccessableProducerConsumerQueue<T>::records_[0] : element_between+1;
            }
          }
        }
      }
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_VARIABLESIZEELEMENTQUEUE_HPP_
