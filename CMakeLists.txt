cmake_minimum_required(VERSION 3.12)
project(readout VERSION 1.2.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(logging REQUIRED)
find_package(opmonlib REQUIRED)
find_package(dataformats REQUIRED)
find_package(dfmessages REQUIRED)
find_package(folly REQUIRED)
find_package(TBB REQUIRED)
FIND_PACKAGE(Boost COMPONENTS program_options system log log_setup iostreams REQUIRED)
set(BOOST_LIBS Boost::program_options Boost::log Boost::log_setup Boost::iostreams ${Boost_LIBRARIES})

daq_codegen(*.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

##############################################################################
# Dependency sets
set(READOUT_DEPENDENCIES 
  ers::ers 
  logging::logging
  opmonlib::opmonlib
  dataformats::dataformats
  dfmessages::dfmessages
  Folly::folly ${TBB} ${TBB_IMPORTED_TARGETS}
)

##############################################################################
# Main library
daq_add_library(
  LINK_LIBRARIES appfwk::appfwk ${READOUT_DEPENDENCIES}
)

##############################################################################
# Plugins
daq_add_plugin(FakeCardReader duneDAQModule LINK_LIBRARIES appfwk::appfwk readout)
daq_add_plugin(DataLinkHandler duneDAQModule LINK_LIBRARIES appfwk::appfwk readout)
daq_add_plugin(BufferedFileStreamer duneDAQModule LINK_LIBRARIES appfwk::appfwk readout ${BOOST_LIBS})

##############################################################################
# Integration tests
daq_add_application(readout_test_ratelimiter test_ratelimiter_app.cxx TEST LINK_LIBRARIES readout)

##############################################################################
# Test configurations
file(COPY test/fakereadout-commands.json DESTINATION test)

##############################################################################
# Unit Tests
daq_add_unit_test(RawWIBTp_test                LINK_LIBRARIES readout)

##############################################################################
# Installation
daq_install()
