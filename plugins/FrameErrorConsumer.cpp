/**
 * @file FrameErrorConsumer.cpp Module that consumes frames with errors from a queue
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licencing/copyright details are in the COPYING file that you should have
 * recieved with this code.
 * */

#ifndef READOUT_PLUGINS_FRAMEERRORCONSUMER_HPP_
#define READOUT_PLUGINS_FRAMEERRORCONSUMER_HPP_

#include "readout/ReadoutLogging.hpp"
#include "DummyConsumer.cpp"
#include "DummyConsumer.hpp"
#include "logging/Logging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "readout/datalinkhandler/Structs.hpp"

#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

class FrameErrorConsumer : public DummyConsumer<std::unique_ptr<dunedaq::readout::datalinkhandler::ErrorMessage>>
{
public:
  explicit FrameErrorConsumer(const std::string name)
  : DummyConsumer<std::unique_ptr<dunedaq::readout::datalinkhandler::ErrorMessage>>(name)
  {}

  void packet_callback(std::unique_ptr<dunedaq::readout::datalinkhandler::ErrorMessage>& packet) override
  {
    // Handle ErrorMessage
  }

};
} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::FrameErrorConsumer)

#endif // READOUT_PLUGINS_FRAMEERRORCONSUMER_HPP_