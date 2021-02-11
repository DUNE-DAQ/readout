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

#include "nlohmann/json.hpp"

#include <cstdint> // uint_t types
#include <memory> // unique_ptr

namespace dunedaq {
namespace readout {
namespace types {

using cfg_data_t = nlohmann::json;

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
const constexpr std::size_t WIB_SUPERCHUNK_SIZE = 5568; // for 12: 5568
struct WIB_SUPERCHUNK_STRUCT {
  char data[WIB_SUPERCHUNK_SIZE];
};

typedef dunedaq::appfwk::DAQSink<std::uint64_t> BlockPtrSink; // NOLINT uint64_t
typedef std::unique_ptr<BlockPtrSink> UniqueBlockPtrSink; 

typedef dunedaq::appfwk::DAQSource<std::uint64_t> BlockPtrSource; // NOLINT uint64_t
typedef std::unique_ptr<BlockPtrSource> UniqueBlockPtrSource;

typedef dunedaq::appfwk::DAQSink<WIB_SUPERCHUNK_STRUCT> WIBFrameSink;
typedef std::unique_ptr<WIBFrameSink> UniqueWIBFrameSink;
using WIBFramePtrSink = appfwk::DAQSink<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>;
using UniqueWIBFramePtrSink = std::unique_ptr<WIBFramePtrSink>;

typedef dunedaq::appfwk::DAQSource<WIB_SUPERCHUNK_STRUCT> WIBFrameSource;
typedef std::unique_ptr<WIBFrameSource> UniqueWIBFrameSource;
using WIBFramePtrSource = appfwk::DAQSource<std::unique_ptr<types::WIB_SUPERCHUNK_STRUCT>>;
using UniqueWIBFramePtrSource = std::unique_ptr<WIBFramePtrSource>;

// TP
const constexpr std::size_t TP_SUPERCHUNK_SIZE = 5568; // also defined in ReadoutConstants.hpp
struct TP_SUPERCHUNK_STRUCT {
  char data[TP_SUPERCHUNK_SIZE];
};

typedef dunedaq::appfwk::DAQSink<TP_SUPERCHUNK_STRUCT> TPFrameSink;
typedef std::unique_ptr<TPFrameSink> UniqueTPFrameSink;
using TPFramePtrSink = appfwk::DAQSink<std::unique_ptr<types::TP_SUPERCHUNK_STRUCT>>;
using UniqueTPFramePtrSink = std::unique_ptr<TPFramePtrSink>;

typedef dunedaq::appfwk::DAQSource<TP_SUPERCHUNK_STRUCT> TPFrameSource;
typedef std::unique_ptr<TPFrameSource> UniqueTPFrameSource;
using TPFramePtrSource = appfwk::DAQSource<std::unique_ptr<types::TP_SUPERCHUNK_STRUCT>>;
using UniqueTPFramePtrSource = std::unique_ptr<TPFramePtrSource>;



} // namespace types
} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTTYPES_HPP_
