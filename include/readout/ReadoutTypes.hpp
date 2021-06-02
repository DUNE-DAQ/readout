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
#include "dataformats/wib/WIBFrame.hpp"
#include "dataformats/wib2/WIB2Frame.hpp"

#include <cstdint> // uint_t types
#include <memory>  // unique_ptr

namespace dunedaq {
namespace readout {
namespace types {

/**
 * @brief A FULLMODE Elink is identified by the following:
 * - card id (physical card ID)
 * - link tag (elink_id * 64 + 2048 * logic_region)
 */
struct LinkId
{
  uint8_t card_id_;   // NOLINT(build/unsigned)
  uint32_t link_tag_; // NOLINT(build/unsigned)
};

class Timestamped
{
  virtual uint64_t get_timestamp() const = 0; // NOLINT(build/unsigned)
  virtual void set_timestamp(uint64_t) = 0;   // NOLINT(build/unsigned)
};

/**
 * @brief SuperChunk concept: The FELIX user payloads are called CHUNKs.
 * There is mechanism in firmware to aggregate WIB frames to a user payload
 * that is called a SuperChunk. Default mode is with 12 frames:
 * 12[WIB frames] x 464[Bytes] = 5568[Bytes]
 */
const constexpr std::size_t WIB_SUPERCHUNK_SIZE = 5568; // for 12: 5568
struct WIB_SUPERCHUNK_STRUCT
{
  // data
  char data[WIB_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const WIB_SUPERCHUNK_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&data);        // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&other.data); // NOLINT
    return thisptr->get_timestamp() < otherptr->get_timestamp() ? true : false;
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::WIBFrame*>(&data)->get_wib_header()->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    reinterpret_cast<dunedaq::dataformats::WIBFrame*>(&data)->get_wib_header()->set_timestamp(ts); // NOLINT
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto wf = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(((uint8_t*)(&data)) + i * 464); // NOLINT
      auto wfh = const_cast<dunedaq::dataformats::WIBHeader*>(wf->get_wib_header());
      wfh->set_timestamp(ts_next);
      ts_next += offset;
    }
  }
};

/**
 * @brief For WIB2 the numbers are different.
 * 12[WIB2 frames] x 468[Bytes] = 5616[Bytes]
 * */
const constexpr std::size_t WIB2_SUPERCHUNK_SIZE = 5616; // for 12: 5616
struct WIB2_SUPERCHUNK_STRUCT
{
  // data
  char data[WIB2_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const WIB2_SUPERCHUNK_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::dataformats::WIB2Frame*>(&data);        // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::dataformats::WIB2Frame*>(&other.data); // NOLINT
    return thisptr->get_timestamp() < otherptr->get_timestamp() ? true : false;
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::WIB2Frame*>(&data)->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    auto frame = reinterpret_cast<dunedaq::dataformats::WIB2Frame*>(&data); // NOLINT
    frame->header.timestamp_1 = ts;
    frame->header.timestamp_2 = ts >> 32;
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto w2f = reinterpret_cast<dunedaq::dataformats::WIB2Frame*>(((uint8_t*)(&data)) + i * 468); // NOLINT
      w2f->header.timestamp_1 = ts_next;
      w2f->header.timestamp_2 = ts_next >> 32;
      ts_next += offset;
    }
  }
};

/**
 * @brief For PDS the numbers are different.
 * 12[PDS frames] x 584[Bytes] = 7008[Bytes]
 * */
const constexpr std::size_t PDS_SUPERCHUNK_SIZE = 7008; // for 12: 7008
struct PDS_SUPERCHUNK_STRUCT
{
  // data
  char data[PDS_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const PDS_SUPERCHUNK_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::dataformats::PDSFrame*>(&data);        // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::dataformats::PDSFrame*>(&other.data); // NOLINT
    return thisptr->get_timestamp() > otherptr->get_timestamp() ? true : false;
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::PDSFrame*>(&data)->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    auto frame = reinterpret_cast<dunedaq::dataformats::PDSFrame*>(&data); // NOLINT
    frame->header.timestamp_wf_1 = ts;
    frame->header.timestamp_wf_2 = ts >> 32;
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto pdsf = reinterpret_cast<dunedaq::dataformats::PDSFrame*>(((uint8_t*)(&data)) + i * 584); // NOLINT
      pdsf->header.timestamp_wf_1 = ts_next;
      pdsf->header.timestamp_wf_2 = ts_next >> 32;
      ts_next += offset;
    }
  }
};

/**
 * Key finder for LBs.
 * */
struct PDSTimestampGetter
{
  uint64_t operator()(PDS_SUPERCHUNK_STRUCT& pdss) // NOLINT(build/unsigned)
  {
    auto pdsfptr = reinterpret_cast<dunedaq::dataformats::PDSFrame*>(&pdss); // NOLINT
    return pdsfptr->get_timestamp();
  }
};

/**
 * @brief Convencience wrapper to take ownership over char pointers with
 * corresponding allocated memory size.
 * */
struct VariableSizePayloadWrapper
{
  VariableSizePayloadWrapper() {}
  VariableSizePayloadWrapper(size_t size, char* data)
    : size(size)
    , data(data)
  {}

  size_t size = 0;
  std::unique_ptr<char> data = nullptr;
};

typedef dunedaq::appfwk::DAQSink<std::uint64_t> BlockPtrSink; // NOLINT(build/unsigned)
typedef std::unique_ptr<BlockPtrSink> UniqueBlockPtrSink;

typedef dunedaq::appfwk::DAQSource<std::uint64_t> BlockPtrSource; // NOLINT(build/unsigned)
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
struct RAW_WIB_TP_STRUCT
{
  dunedaq::dataformats::TpHeader head;
  dunedaq::dataformats::TpDataBlock block;
  dunedaq::dataformats::TpPedinfo ped;
};

struct TpSubframe
{
  uint32_t word1; // NOLINT(build/unsigned)
  uint32_t word2; // NOLINT(build/unsigned)
  uint32_t word3; // NOLINT(build/unsigned)
};

} // namespace types
} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_READOUTTYPES_HPP_
