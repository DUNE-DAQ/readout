/**
* @file ReadoutTypes.hpp Common types in udaq-readout
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_INCLUDE_READOUT_READOUTTYPES_HPP_
#define READOUT_INCLUDE_READOUT_READOUTTYPES_HPP_

#include "RawWIBTp.hpp"

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "dataformats/pds/PDSFrame.hpp"

#include <cstdint> // uint_t types
#include <memory> // unique_ptr

namespace dunedaq {
namespace readout {
namespace types {

/**
 * @brief A FULLMODE Elink is identified by the following: 
 * - card id (physical card ID)
 * - link tag (elink_id * 64 + 2048 * logic_region)
*/
struct LinkId {
  uint8_t card_id_;   // NOLINT
  uint32_t link_tag_; // NOLINT
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

/**
 * @brief For WIB2 the numbers are different.
 * 12[WIB2 frames] x 468[Bytes] = 5616[Bytes]
 * */
const constexpr std::size_t WIB2_SUPERCHUNK_SIZE = 5616; // for 12: 5616
struct WIB2_SUPERCHUNK_STRUCT {
  char data[WIB2_SUPERCHUNK_SIZE];
};

/**
 * @brief For PDS the numbers are different.
 * 12[PDS frames] x 584[Bytes] = 7008[Bytes]
 * */
const constexpr std::size_t PDS_SUPERCHUNK_SIZE = 7008; // for 12: 7008
struct PDS_SUPERCHUNK_STRUCT {
  char data[PDS_SUPERCHUNK_SIZE];
};

/**
 * Key finder for LBs.
 * */
struct PDSTimestampGetter {
  uint64_t operator() (PDS_SUPERCHUNK_STRUCT& pdss) { // NOLINT
    auto pdsfptr = reinterpret_cast<dunedaq::dataformats::PDSFrame*>(&pdss);
    return pdsfptr->get_timestamp();
  }
};

/**
 * @brief Convencience wrapper to take ownership over char pointers with 
 * corresponding allocated memory size.
 * */
struct VariableSizePayloadWrapper {
  VariableSizePayloadWrapper() {}
  VariableSizePayloadWrapper(size_t size, char* data) : size(size), data(data) {}

  size_t size = 0;
  std::unique_ptr<char> data = nullptr;
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


// raw WIB TP
struct RAW_WIB_TP_STRUCT {
  dunedaq::dataformats::TpHeader head;
  dunedaq::dataformats::TpDataBlock block;
  dunedaq::dataformats::TpPedinfo ped;
};

struct TpSubframe
{
  uint32_t word1; // NOLINT
  uint32_t word2; // NOLINT
  uint32_t word3; // NOLINT
};

} // namespace types
} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_READOUTTYPES_HPP_
