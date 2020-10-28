/**
* @file ReadoutTypes.hpp Common types in udaq-readout
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTTYPES_HPP_
#define UDAQ_READOUT_SRC_READOUTTYPES_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include <memory> // unique_ptr

namespace dunedaq {
namespace readout {

/** 
 * @brief SuperChunk concept: The FELIX user payloads are called CHUNKs.
 * There is mechanism in firmware to aggregate WIB frames to a user payload 
 * that is called a SuperChunk. Default mode is with 12 frames:
 * 12[WIB frames] x 464[Bytes] = 5568[Bytes] 
*/
const constexpr std::size_t SUPERCHUNK_FRAME_SIZE = 5568; // for 12: 5568
struct SUPERCHUNK_CHAR_STRUCT {
  char fragments[SUPERCHUNK_FRAME_SIZE];
};

typedef dunedaq::appfwk::DAQSink<uint64_t> BlockPtrSink; // NOLINT uint64_t
typedef std::unique_ptr<BlockPtrSink> UniqueBlockPtrSink; 

typedef dunedaq::appfwk::DAQSource<uint64_t> BlockPtrSource; // NOLINT uint64_t
typedef std::unique_ptr<BlockPtrSource> UniqueBlockPtrSource;

typedef dunedaq::appfwk::DAQSink<SUPERCHUNK_CHAR_STRUCT> FrameSink;
typedef std::unique_ptr<FrameSink> UniqueFrameSink;

typedef dunedaq::appfwk::DAQSource<SUPERCHUNK_CHAR_STRUCT> FrameSource;
typedef std::unique_ptr<FrameSource> UniqueFrameSource;

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTTYPES_HPP_
