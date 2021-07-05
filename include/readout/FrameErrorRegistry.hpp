/**
 * @file RawWIBTp.hpp Error registry for missing frames
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_
#define READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_

#include <vector>
#include <queue>
#include <readout/FrameError.hpp>

namespace dunedaq {
  namespace readout {

    class FrameErrorRegistry {
    public:
      FrameErrorRegistry()
      : m_errors()
      {

      }

      void add_error(FrameError error) {
        m_errors.push(error);
      }

      void update_latest_frame_in_buffer(uint64_t ts) {
        while (has_error() && m_errors.top().end_ts < ts) {
          m_errors.pop();
        }
      }

      bool has_error() {
        return !m_errors.empty();
      }


    private:
      std::priority_queue<FrameError, std::vector<FrameError>, std::greater<FrameError>> m_errors;
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_
