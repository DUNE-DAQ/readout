cmake_minimum_required(VERSION 3.12)
project(readout VERSION 1.4.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(logging REQUIRED)
find_package(opmonlib REQUIRED)
find_package(dataformats REQUIRED)
find_package(dfmessages REQUIRED)
find_package(triggeralgs REQUIRED)
find_package(timinglibs REQUIRED)
find_package(trigger REQUIRED)
find_package(folly REQUIRED)
find_package(Boost COMPONENTS iostreams REQUIRED)
set(BOOST_LIBS Boost::iostreams ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_LIBRARIES})

daq_codegen( datalinkhandler.jsonnet  datarecorder.jsonnet  fakecardreader.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )
daq_codegen( *info.jsonnet DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

set(USE_INTRINSICS ON)

if(${USE_INTRINSICS})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
endif()

##############################################################################
# Dependency sets
set(READOUT_DEPENDENCIES 
  # system
  numa
 
  # tools
  Folly::folly
  ers::ers
  logging::logging

  # dunedaq
  opmonlib::opmonlib
  dataformats::dataformats
  dfmessages::dfmessages
  triggeralgs::triggeralgs
  trigger::trigger
)

##############################################################################
# Main library
daq_add_library(
  wib/tpg/*.cpp
  LINK_LIBRARIES appfwk::appfwk ${READOUT_DEPENDENCIES}
)

##############################################################################
# Plugins
daq_add_plugin(FakeCardReader duneDAQModule LINK_LIBRARIES appfwk::appfwk readout)
daq_add_plugin(DataLinkHandler duneDAQModule LINK_LIBRARIES appfwk::appfwk readout ${BOOST_LIBS})
daq_add_plugin(DataRecorder duneDAQModule LINK_LIBRARIES appfwk::appfwk readout ${BOOST_LIBS})
daq_add_plugin(FragmentConsumer duneDAQModule LINK_LIBRARIES appfwk::appfwk readout)
daq_add_plugin(TimeSyncConsumer duneDAQModule LINK_LIBRARIES appfwk::appfwk readout)

##############################################################################
# Integration tests
daq_add_application(readout_test_ratelimiter test_ratelimiter_app.cxx TEST LINK_LIBRARIES readout)
#daq_add_application(readout_test_lb_allocation test_lb_allocation_app.cxx TEST LINK_LIBRARIES readout)
daq_add_application(readout_test_bufferedfilewriter test_bufferedfilewriter_app.cxx TEST LINK_LIBRARIES readout ${BOOST_LIBS})
daq_add_application(readout_test_bufferedfilereader test_bufferedfilereader_app.cxx TEST LINK_LIBRARIES readout ${BOOST_LIBS})
daq_add_application(readout_test_skiplist test_skiplist_app.cxx TEST LINK_LIBRARIES readout ${BOOST_LIBS})
daq_add_application(readout_test_fast_expand_wib2frame test_fast_expand_wib2frame_app.cxx TEST LINK_LIBRARIES readout ${BOOST_LIBS})


##############################################################################
# Unit Tests
daq_add_unit_test(RawWIBTp_test                LINK_LIBRARIES readout)
daq_add_unit_test(BufferedReadWrite_test       LINK_LIBRARIES readout ${BOOST_LIBS})
#daq_add_unit_test(VariableSizeElementQueue_test LINK_LIBRARIES readout)

##############################################################################
# Installation
daq_install()
