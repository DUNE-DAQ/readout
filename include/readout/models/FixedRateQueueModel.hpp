/**
 * @file FixedRateQueueModel.hpp Queue that can be searched
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_FIXEDRATEQUEUEMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_FIXEDRATEQUEUEMODEL_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include "IterableQueueModel.hpp"

namespace dunedaq {
  namespace readout {

    template<class T>
    class FixedRateQueueModel : public IterableQueueModel<T>
    {
    public:
      FixedRateQueueModel() : IterableQueueModel<T>() {

      }

      explicit FixedRateQueueModel(uint32_t size) // NOLINT(build/unsigned)
          : IterableQueueModel<T>(size)
      {}

      typename IterableQueueModel<T>::Iterator lower_bound(T& element)
      {
        uint64_t timestamp = element.get_timestamp();
        unsigned int start_index =
            IterableQueueModel<T>::readIndex_.load(std::memory_order_relaxed); // NOLINT(build/unsigned)
        size_t occupancy_guess = IterableQueueModel<T>::occupancy();
        uint64_t last_ts = IterableQueueModel<T>::records_[start_index].get_timestamp();
        uint64_t newest_ts = last_ts + occupancy_guess * T::tick_dist * T::frames_per_element;

        if (last_ts > timestamp || timestamp > newest_ts) {
          return IterableQueueModel<T>::end();
        }

        int64_t time_tick_diff = (timestamp - last_ts) / T::tick_dist;
        uint32_t num_element_offset = time_tick_diff / T::frames_per_element;
        uint32_t target_index = start_index + num_element_offset;
        if (target_index >= IterableQueueModel<T>::size_) {
          target_index -= IterableQueueModel<T>::size_;
        }
        return typename IterableQueueModel<T>::Iterator(*this, target_index);
      }

    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT__MODELS_FIXEDRATEQUEUEMODEL_HPP_
