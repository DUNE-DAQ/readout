cmake_minimum_required(VERSION 3.12)
project(udaq-readout VERSION 0.0.0)

#find_package(daq-buildtools 1.2.1 REQUIRED)
find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(udaq_readout_deps REQUIRED)
find_package(folly REQUIRED)

##############################################################################

set(FLXCARD_DEPENDENCIES udaq_readout_deps::FlxCard udaq_readout_deps::cmem_rcc)
set(FLXUTIL_DEPENDENCIES udaq_readout_deps::packetformat Folly::folly)

##############################################################################
# Main library

daq_add_library(DefaultParserImpl.cpp LINK_LIBRARIES appfwk ${FLXUTIL_DEPENDENCIES}) 

##############################################################################
# Plugins

daq_add_plugin(CardReaderDAQModule duneDAQModule LINK_LIBRARIES appfwk udaq-readout ${FLXCARD_DEPENDENCIES})
daq_add_plugin(LinkReaderDAQModule duneDAQModule LINK_LIBRARIES appfwk udaq-readout ${FLXUTIL_DEPENDENCIES})

##############################################################################
# Integration tests
# TBA

##############################################################################
# Test configurations
file(COPY test/felix_readout_card0_app.json DESTINATION test)

##############################################################################

daq_install()
