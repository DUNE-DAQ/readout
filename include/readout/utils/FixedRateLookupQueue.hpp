/**
 * @file SearchableProducerConsumerQueue.hpp Queue that can be searched
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_UTILS_FIXEDRATELOOKUPQUEUE_HPP_
#define READOUT_INCLUDE_READOUT_UTILS_FIXEDRATELOOKUPQUEUE_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "AccessableProducerConsumerQueue.hpp"

namespace dunedaq {
  namespace readout {

    template<class T>
    class FixedRateLookupQueue : public AccessableProducerConsumerQueue<T>
    {
    public:
      FixedRateLookupQueue() : AccessableProducerConsumerQueue<T>() {

      }

      explicit FixedRateLookupQueue(uint32_t size) // NOLINT(build/unsigned)
          : AccessableProducerConsumerQueue<T>(size)
      {}

      typename FixedRateLookupQueue<T>::Iterator lower_bound(T& element)
      {
        uint64_t timestamp = element.get_timestamp();
        unsigned int start_index =
            AccessableProducerConsumerQueue<T>::readIndex_.load(std::memory_order_relaxed); // NOLINT(build/unsigned)
        size_t occupancy_guess = AccessableProducerConsumerQueue<T>::occupancy();
        uint64_t last_ts = AccessableProducerConsumerQueue<T>::records_[start_index].get_timestamp();
        uint64_t newest_ts = last_ts + (occupancy_guess - m_safe_num_elements_margin) * T::tick_dist * T::frames_per_element;

        if (last_ts > timestamp || timestamp > newest_ts) {
          return AccessableProducerConsumerQueue<T>::end();
        }

        int64_t time_tick_diff = (timestamp - last_ts) / T::tick_dist;
        uint32_t num_element_offset = time_tick_diff / T::frames_per_element;
        uint32_t target_index = start_index + num_element_offset;
        if (target_index >= AccessableProducerConsumerQueue<T>::size_) {
          target_index -= AccessableProducerConsumerQueue<T>::size_;
        }
        return typename AccessableProducerConsumerQueue<T>::Iterator(*this, target_index);
      }

    private:
      static const constexpr uint64_t m_safe_num_elements_margin = 10; // NOLINT(build/unsigned)

    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_UTILS_FIXEDRATELOOKUPQUEUE_HPP_
