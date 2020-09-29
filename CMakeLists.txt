cmake_minimum_required(VERSION 3.12)
project(udaq-readout VERSION 0.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../daq-buildtools/cmake ${CMAKE_MODULE_PATH})
include(DAQ)

daq_setup_environment()

find_package(appfwk REQUIRED)

#find_package(felix-deps REQUIRED)

#link_directories(${FELIX_LIB})
#target_link_libraries(${module_name} PUBLIC ${FELIX_LIB}/FlxCard.so)

##############################################################################
daq_point_build_to( src )

message("FELIX INC -> " $ENV{FELIX_INC})
message("FELIX LIB -> " $ENV{FELIX_LIB})
add_library(udaq-readout_CardReaderDAQModule_duneDAQModule src/CardReaderDAQModule.cpp)
target_include_directories (udaq-readout_CardReaderDAQModule_duneDAQModule PUBLIC $ENV{FELIX_INC})
target_link_libraries(udaq-readout_CardReaderDAQModule_duneDAQModule appfwk::appfwk
  $ENV{FELIX_LIB}/libFlxCard.so
  $ENV{FELIX_LIB}/libcmem_rcc.so
  $ENV{FELIX_LIB}/libpacketformat.so
)

##############################################################################
daq_point_build_to( test )

file(COPY test/felix_readout_card0_app.json DESTINATION test)

##############################################################################
daq_point_build_to( unittest )

daq_install(TARGETS udaq-readout_CardReaderDAQModule_duneDAQModule)

