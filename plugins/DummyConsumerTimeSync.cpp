/**
 * @file DummyConsumerTimeSync.cpp Module that consumes TimeSync's from a queue
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/

#ifndef READOUT_PLUGINS_DUMMYCONSUMERTIMESYNC_HPP_
#define READOUT_PLUGINS_DUMMYCONSUMERTIMESYNC_HPP_

#include "DummyConsumer.hpp"
#include "DummyConsumer.cpp"
#include "dfmessages/TimeSync.hpp"

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DummyConsumer<dunedaq::dfmessages::TimeSync>)

#endif //READOUT_PLUGINS_DUMMYCONSUMERTIMESYNC_HPP_
