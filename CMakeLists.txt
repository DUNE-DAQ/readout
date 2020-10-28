cmake_minimum_required(VERSION 3.12)
project(udaq-readout VERSION 0.0.0)

find_package(daq-buildtools 1.2.1 REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
#find_package(udaq_readout_deps REQUIRED)

##############################################################################

set(FLXCARD_DEPENDENCIES $ENV{UDAQ_READOUT_DEPS_LIB}/libFlxCard.so $ENV{UDAQ_READOUT_DEPS_LIB}/libcmem_rcc.so)
set(FLXUTIL_DEPENDENCIES $ENV{UDAQ_READOUT_DEPS_LIB}/libpacketformat.so)

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
