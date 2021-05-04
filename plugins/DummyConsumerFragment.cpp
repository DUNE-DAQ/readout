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

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DummyConsumer<std::unique_ptr<dunedaq::dataformats::Fragment>>)

#endif //READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_