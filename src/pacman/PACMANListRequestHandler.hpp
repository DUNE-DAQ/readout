/**
 * @file PACMANListRequestHandler.hpp Trigger matching mechanism for PACMAN frames.
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_PACMAN_PACMANLISTREQUESTHANDLER_HPP_
#define READOUT_SRC_PACMAN_PACMANLISTREQUESTHANDLER_HPP_

#include "ReadoutIssues.hpp"
#include "readout/models/DefaultRequestHandlerModel.hpp"
#include "readout/models/SkipListLatencyBufferModel.hpp"

#include "dataformats/pacman/PACMANFrame.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/NDReadoutTypes.hpp"

#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {

class PACMANListRequestHandler
  : public DefaultRequestHandlerModel<types::PACMAN_MESSAGE_STRUCT,
                                      SkipListLatencyBufferModel<types::PACMAN_MESSAGE_STRUCT>>
{
public:
  using inherited = DefaultRequestHandlerModel<types::PACMAN_MESSAGE_STRUCT,
                                               SkipListLatencyBufferModel<types::PACMAN_MESSAGE_STRUCT>>;
  using SkipListAcc = typename folly::ConcurrentSkipList<types::PACMAN_MESSAGE_STRUCT>::Accessor;
  using SkipListSkip = typename folly::ConcurrentSkipList<types::PACMAN_MESSAGE_STRUCT>::Skipper;

  PACMANListRequestHandler(std::unique_ptr<SkipListLatencyBufferModel<types::PACMAN_MESSAGE_STRUCT>>& latency_buffer,
                           std::unique_ptr<appfwk::DAQSink<std::unique_ptr<dataformats::Fragment>>>& fragment_sink,
                           std::unique_ptr<appfwk::DAQSink<types::PACMAN_MESSAGE_STRUCT>>& snb_sink)
    : DefaultRequestHandlerModel<types::PACMAN_MESSAGE_STRUCT,
                                 SkipListLatencyBufferModel<types::PACMAN_MESSAGE_STRUCT>>(latency_buffer,
                                                                                              fragment_sink,
                                                                                              snb_sink)
  {
    TLOG_DEBUG(TLVL_WORK_STEPS) << "PACMANistRequestHandler created...";
  }

protected:

private:

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_PACMAN_PACMANLISTREQUESTHANDLER_HPP_
