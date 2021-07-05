/**
 * @file FrameError.hpp Frame error type
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_FRAMEERROR_HPP_
#define READOUT_INCLUDE_READOUT_FRAMEERROR_HPP_

#include <vector>

namespace dunedaq {
  namespace readout {

    struct FrameError {
    public:
      enum class ErrorType {
        MISSING_FRAMES,
        FAULTY_FRAMES
      };

      FrameError(ErrorType error_type, uint64_t start_ts, uint64_t end_ts) : error_type(error_type), start_ts(start_ts), end_ts(end_ts)
      {

      }

      ErrorType error_type;
      uint64_t start_ts;
      uint64_t end_ts;

      bool operator<(const FrameError& other) const {
        return end_ts < other.end_ts;
      }

      bool operator>(const FrameError& other) const {
        return end_ts > other.end_ts;
      }
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_FRAMEERROR_HPP_
