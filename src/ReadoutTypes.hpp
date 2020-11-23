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

#include <cstdint> // uint64_t
#include <memory> // unique_ptr

namespace dunedaq {
namespace readout {

/**
 * @brief A FULLMODE Elink is identified by the following: 
 * - card id (physical card ID)
 * - link tag (elink_id * 64 + 2048 * logic_region)
*/
struct LinkId {
  uint8_t card_id_;
  uint32_t link_tag_;
};

/** 
 * @brief SuperChunk concept: The FELIX user payloads are called CHUNKs.
 * There is mechanism in firmware to aggregate WIB frames to a user payload 
 * that is called a SuperChunk. Default mode is with 12 frames:
 * 12[WIB frames] x 464[Bytes] = 5568[Bytes] 
*/
const constexpr std::size_t SUPERCHUNK_SIZE = 5568; // for 12: 5568
struct SUPERCHUNK_STRUCT {
  char fragments[SUPERCHUNK_SIZE];
};

typedef dunedaq::appfwk::DAQSink<std::uint64_t> BlockPtrSink; // NOLINT uint64_t
typedef std::unique_ptr<BlockPtrSink> UniqueBlockPtrSink; 

typedef dunedaq::appfwk::DAQSource<std::uint64_t> BlockPtrSource; // NOLINT uint64_t
typedef std::unique_ptr<BlockPtrSource> UniqueBlockPtrSource;

typedef dunedaq::appfwk::DAQSink<SUPERCHUNK_STRUCT> FrameSink;
typedef std::unique_ptr<FrameSink> UniqueFrameSink;

typedef dunedaq::appfwk::DAQSource<SUPERCHUNK_STRUCT> FrameSource;
typedef std::unique_ptr<FrameSource> UniqueFrameSource;

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTTYPES_HPP_
