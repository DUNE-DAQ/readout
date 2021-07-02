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

namespace dunedaq {
  namespace readout {

    class FrameErrorRegistry {
    public:
      FrameErrorRegistry()
      : m_missing_frames()
      {

      }

      void add_missing_frame_sequence(uint64_t start, uint64_t end) {
        m_missing_frames.emplace_back(start, end);
      }

      void update_latest_frame_in_buffer(uint64_t ts) {
        while (has_error() && m_missing_frames.front().second < ts) {
          m_missing_frames.erase(m_missing_frames.begin());
        }
      }

      bool has_error() {
        return !m_missing_frames.empty();
      }


    private:
      std::vector<std::pair<uint64_t, uint64_t>> m_missing_frames;
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_
