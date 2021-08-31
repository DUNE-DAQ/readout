/**
 * @file RawWIBTp.hpp Error registry for missing frames
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_
#define READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_

#include <cstdint> // uint_t types
#include <deque>

namespace dunedaq {
namespace readout {

class FrameErrorRegistry
{
public:
  struct FrameError
  {
  public:
    FrameError(uint64_t start_ts, uint64_t end_ts) // NOLINT(build/unsigned)
      : start_ts(start_ts)
      , end_ts(end_ts)
    {}

    uint64_t start_ts; // NOLINT(build/unsigned)
    uint64_t end_ts;   // NOLINT(build/unsigned)

    bool operator<(const FrameError& other) const { return end_ts < other.end_ts; }

    bool operator>(const FrameError& other) const { return end_ts > other.end_ts; }
  };

  FrameErrorRegistry()
    : m_errors()
  {}

  void add_error(FrameError error) { m_errors.push_back(error); }

  void remove_errors_until(uint64_t ts) // NOLINT(build/unsigned)
  {
    while (!m_errors.empty() && m_errors.front().end_ts < ts) {
      m_errors.pop_front();
    }
  }

  bool has_error() { return !m_errors.empty(); }

private:
  std::deque<FrameError> m_errors;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_FRAMEERRORREGISTRY_HPP_
