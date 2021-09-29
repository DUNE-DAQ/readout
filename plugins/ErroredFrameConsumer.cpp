/**
 * @file ErroredFrameConsumer.cpp Module that consumes frames with errors from a queue
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licencing/copyright details are in the COPYING file that you should have
 * recieved with this code.
 * */

#ifndef READOUT_PLUGINS_ERROREDFRAMECONSUMER_HPP_
#define READOUT_PLUGINS_ERROREDFRAMECONSUMER_HPP_

#include "readout/ReadoutLogging.hpp"
#include "DummyConsumer.cpp"
#include "DummyConsumer.hpp"
#include "logging/Logging.hpp"
#include "dataformats/wib/WIBFrame.hpp"

#include <bitset>

#include <memory>
#include <string>

namespace dunedaq {
namespace readout {

class ErroredFrameConsumer : public DummyConsumer<dataformats::WIBFrame>
{
public:
  explicit ErroredFrameConsumer(const std::string name)
  : DummyConsumer<dataformats::WIBFrame>(name)
  {}

  void packet_callback(dataformats::WIBFrame& packet) override
  {
    if (packet.get_wib_header()->wib_errors){
//      m_error_count += std::bitset<16>(packet.get_wib_header()->wib_errors).count();
    }
  }
private:
//  int m_error_count;
};
} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::ErroredFrameConsumer)

#endif // READOUT_PLUGINS_ERROREDFRAMECONSUMER_HPP_