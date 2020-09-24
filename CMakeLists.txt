cmake_minimum_required(VERSION 3.12)
project(udaq-readout VERSION 0.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../daq-buildtools/cmake ${CMAKE_MODULE_PATH})
include(DAQ)

daq_setup_environment()

find_package(appfwk REQUIRED)

##############################################################################
daq_point_build_to( src )

add_library(udaq-readout_CardReaderDAQModule_duneDAQModule src/CardReaderDAQModule.cpp)
target_link_libraries(udaq-readout_CardReaderDAQModule_duneDAQModule appfwk:appfwk)

##############################################################################
daq_point_build_to( test )

#file(COPY test/driver.json DESTINATION test)

##############################################################################
daq_point_build_to( unittest )

daq_install(TARGETS udaq-readout_CardReaderDAQModule_duneDAQModule)

