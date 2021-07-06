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

#include "dataformats/FragmentHeader.hpp"
#include "dataformats/GeoID.hpp"
#include "dataformats/daphne/DAPHNEFrame.hpp"
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
    // auto thisptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&data);        // NOLINT
    // auto otherptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&other.data); // NOLINT
    return this->get_timestamp() < other.get_timestamp();
  }

  bool operator==(const WIB_SUPERCHUNK_STRUCT& other) const { return this->get_timestamp() == other.get_timestamp(); }

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

  static const constexpr dataformats::GeoID::SystemType system_type = dataformats::GeoID::SystemType::kTPC;
  static const constexpr dataformats::FragmentType fragment_type = dataformats::FragmentType::kTPCData;
  static const constexpr uint64_t tick_dist = 25; // 2 MHz@50MHz clock // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = 464;
  static const constexpr uint8_t frames_per_element = 12; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size * frames_per_element;
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

  bool operator==(const WIB2_SUPERCHUNK_STRUCT& other) const { return this->get_timestamp() == other.get_timestamp(); }

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

  static const constexpr dataformats::GeoID::SystemType system_type = dataformats::GeoID::SystemType::kTPC;
  static const constexpr dataformats::FragmentType fragment_type = dataformats::FragmentType::kTPCData;
  static const constexpr uint64_t tick_dist = 32; // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = 468;
  static const constexpr uint8_t frames_per_element = 12; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size * frames_per_element;
};

/**
 * @brief For DAPHNE the numbers are different.
 * 12[DAPHNE frames] x 584[Bytes] = 7008[Bytes]
 * */
const constexpr std::size_t DAPHNE_SUPERCHUNK_SIZE = 7008; // for 12: 7008
struct DAPHNE_SUPERCHUNK_STRUCT
{
  // data
  char data[DAPHNE_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const DAPHNE_SUPERCHUNK_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::dataformats::DAPHNEFrame*>(&data);        // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::dataformats::DAPHNEFrame*>(&other.data); // NOLINT
    return thisptr->get_timestamp() < otherptr->get_timestamp() ? true : false;
  }

  bool operator==(const DAPHNE_SUPERCHUNK_STRUCT& other) const
  {
    return this->get_timestamp() == other.get_timestamp();
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::DAPHNEFrame*>(&data)->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    auto frame = reinterpret_cast<dunedaq::dataformats::DAPHNEFrame*>(&data); // NOLINT
    frame->header.timestamp_wf_1 = ts;
    frame->header.timestamp_wf_2 = ts >> 32;
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto df = reinterpret_cast<dunedaq::dataformats::DAPHNEFrame*>(((uint8_t*)(&data)) + i * 584); // NOLINT
      df->header.timestamp_wf_1 = ts_next;
      df->header.timestamp_wf_2 = ts_next >> 32;
      ts_next += offset;
    }
  }

  static const constexpr dataformats::GeoID::SystemType system_type = dataformats::GeoID::SystemType::kPDS;
  static const constexpr dataformats::FragmentType fragment_type = dataformats::FragmentType::kPDSData;
  static const constexpr uint64_t tick_dist = 16; // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = 584;
  static const constexpr uint8_t frames_per_element = 12; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size * frames_per_element;
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
