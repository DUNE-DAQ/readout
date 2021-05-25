/**
 * @file DummyConsumerFragment.cpp Module that consumes fragments from a queue
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/

#ifndef READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_
#define READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_

#include "DummyConsumer.hpp"
#include "DummyConsumer.cpp"
#include "dataformats/Fragment.hpp"

#include <memory>

namespace dunedaq {
  namespace readout {
  class DummyConsumerFragment : public DummyConsumer<std::unique_ptr<dunedaq::dataformats::Fragment>> {
  public:
    DummyConsumerFragment(const std::string name) : DummyConsumer<std::unique_ptr<dunedaq::dataformats::Fragment>>(name) {

    }

    void packet_callback(std::unique_ptr<dunedaq::dataformats::Fragment> &packet) override {
      dunedaq::dataformats::FragmentHeader header = packet->get_header();
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Received fragment with size " << header.size << ", trigger_number: "
        << header.trigger_number << ", trigger_timestamp: " << header.trigger_timestamp << ", window_begin: "
        << header.window_begin << ", window_end: " << header.window_end;
    }
  };
  }
}

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DummyConsumerFragment)

#endif //READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_
