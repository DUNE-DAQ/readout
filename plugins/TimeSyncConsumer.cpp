/**
 * @file TimeSyncConsumer.cpp Module that consumes TimeSync's from a queue
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_PLUGINS_TIMESYNCCONSUMER_HPP_
#define READOUT_PLUGINS_TIMESYNCCONSUMER_HPP_

#include "DummyConsumer.cpp"
#include "DummyConsumer.hpp"
#include "dfmessages/TimeSync.hpp"

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DummyConsumer<dunedaq::dfmessages::TimeSync>)

#endif // READOUT_PLUGINS_TIMESYNCCONSUMER_HPP_
