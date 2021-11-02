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

#include "daqdataformats/FragmentHeader.hpp"
#include "daqdataformats/GeoID.hpp"
#include "detdataformats/daphne/DAPHNEFrame.hpp"
#include "detdataformats/ssp/SSPTypes.hpp"
#include "detdataformats/wib/WIBFrame.hpp"
#include "detdataformats/wib2/WIB2Frame.hpp"
#include "triggeralgs/TriggerPrimitive.hpp"

#include <cstdint> // uint_t types
#include <memory>  // unique_ptr

namespace dunedaq {
namespace readout {
namespace types {

// Location of this struct is very bad very bad
struct TriggerPrimitive
{
  TriggerPrimitive(uint64_t messageTimestamp_,  // NOLINT(build/unsigned)
                   uint16_t channel_,           // NOLINT(build/unsigned)
                   uint16_t endTime_,           // NOLINT(build/unsigned)
                   uint16_t charge_,            // NOLINT(build/unsigned)
                   uint16_t timeOverThreshold_) // NOLINT(build/unsigned)
    : messageTimestamp(messageTimestamp_)
    , channel(channel_)
    , endTime(endTime_)
    , charge(charge_)
    , timeOverThreshold(timeOverThreshold_)
  {}

  // The timestamp of the netio message that this hit comes from
  uint64_t messageTimestamp; // NOLINT(build/unsigned)
  // The electronics channel number within the (crate, slot, fiber)
  uint16_t channel; // NOLINT(build/unsigned)
  // In TPC ticks relative to the start of the netio message
  uint16_t endTime; // NOLINT(build/unsigned)
  // In ADC
  uint16_t charge; // NOLINT(build/unsigned)
  // In *TPC* clock ticks
  uint16_t timeOverThreshold; // NOLINT(build/unsigned)
};

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
  using FrameType = dunedaq::detdataformats::WIBFrame;

  // data
  char data[WIB_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const WIB_SUPERCHUNK_STRUCT& other) const
  {
    // auto thisptr = reinterpret_cast<const dunedaq::detdataformats::WIBHeader*>(&data);        // NOLINT
    // auto otherptr = reinterpret_cast<const dunedaq::detdataformats::WIBHeader*>(&other.data); // NOLINT
    return this->get_timestamp() < other.get_timestamp();
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::detdataformats::WIBFrame*>(&data)->get_wib_header()->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    reinterpret_cast<dunedaq::detdataformats::WIBFrame*>(&data)->get_wib_header()->set_timestamp(ts); // NOLINT
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto wf = reinterpret_cast<dunedaq::detdataformats::WIBFrame*>(((uint8_t*)(&data)) + i * 464); // NOLINT
      auto wfh = const_cast<dunedaq::detdataformats::WIBHeader*>(wf->get_wib_header());
      wfh->set_timestamp(ts_next);
      ts_next += offset;
    }
  }

  FrameType* begin()
  {
    return reinterpret_cast<FrameType*>(&data[0]); // NOLINT
  }

  FrameType* end()
  {
    return reinterpret_cast<FrameType*>(data + WIB_SUPERCHUNK_SIZE); // NOLINT
  }

  static const constexpr daqdataformats::GeoID::SystemType system_type = daqdataformats::GeoID::SystemType::kTPC;
  static const constexpr daqdataformats::FragmentType fragment_type = daqdataformats::FragmentType::kTPCData;
  static const constexpr uint64_t tick_dist = 25; // 2 MHz@50MHz clock // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = 464;
  static const constexpr uint8_t frames_per_element = 12; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size * frames_per_element;
};
static_assert(sizeof(struct WIB_SUPERCHUNK_STRUCT) == 5568, "Check your assumptions on WIB_SUPERCHUNK_STRUCT");

static_assert(sizeof(struct WIB_SUPERCHUNK_STRUCT) == WIB_SUPERCHUNK_SIZE,
              "Check your assumptions on WIB_SUPERCHUNK_STRUCT");

/**
 * @brief For WIB2 the numbers are different.
 * 12[WIB2 frames] x 468[Bytes] = 5616[Bytes]
 * */
const constexpr std::size_t WIB2_SUPERCHUNK_SIZE = 5616; // for 12: 5616
struct WIB2_SUPERCHUNK_STRUCT
{
  using FrameType = dunedaq::detdataformats::WIB2Frame;
  // data
  char data[WIB2_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const WIB2_SUPERCHUNK_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::detdataformats::WIB2Frame*>(&data);        // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::detdataformats::WIB2Frame*>(&other.data); // NOLINT
    return thisptr->get_timestamp() < otherptr->get_timestamp() ? true : false;
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::detdataformats::WIB2Frame*>(&data)->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    auto frame = reinterpret_cast<dunedaq::detdataformats::WIB2Frame*>(&data); // NOLINT
    frame->header.timestamp_1 = ts;
    frame->header.timestamp_2 = ts >> 32;
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto w2f = reinterpret_cast<dunedaq::detdataformats::WIB2Frame*>(((uint8_t*)(&data)) + i * 468); // NOLINT
      w2f->header.timestamp_1 = ts_next;
      w2f->header.timestamp_2 = ts_next >> 32;
      ts_next += offset;
    }
  }

  FrameType* begin()
  {
    return reinterpret_cast<FrameType*>(&data[0]); // NOLINT
  }

  FrameType* end()
  {
    return reinterpret_cast<FrameType*>(data + WIB2_SUPERCHUNK_SIZE); // NOLINT
  }

  static const constexpr daqdataformats::GeoID::SystemType system_type = daqdataformats::GeoID::SystemType::kTPC;
  static const constexpr daqdataformats::FragmentType fragment_type = daqdataformats::FragmentType::kTPCData;
  static const constexpr uint64_t tick_dist = 32; // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = 468;
  static const constexpr uint8_t frames_per_element = 12; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size * frames_per_element;
};

static_assert(sizeof(struct WIB2_SUPERCHUNK_STRUCT) == WIB2_SUPERCHUNK_SIZE,
              "Check your assumptions on WIB2_SUPERCHUNK_STRUCT");

/**
 * @brief For DAPHNE the numbers are different.
 * 12[DAPHNE frames] x 584[Bytes] = 7008[Bytes]
 * */
const constexpr std::size_t DAPHNE_SUPERCHUNK_SIZE = 7008; // for 12: 7008
struct DAPHNE_SUPERCHUNK_STRUCT
{
  using FrameType = dunedaq::detdataformats::DAPHNEFrame;
  // data
  char data[DAPHNE_SUPERCHUNK_SIZE];
  // comparable based on first timestamp
  bool operator<(const DAPHNE_SUPERCHUNK_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::detdataformats::DAPHNEFrame*>(&data);        // NOLINT
    auto otherptr = reinterpret_cast<const dunedaq::detdataformats::DAPHNEFrame*>(&other.data); // NOLINT
    return thisptr->get_timestamp() < otherptr->get_timestamp() ? true : false;
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::detdataformats::DAPHNEFrame*>(&data)->get_timestamp(); // NOLINT
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    auto frame = reinterpret_cast<dunedaq::detdataformats::DAPHNEFrame*>(&data); // NOLINT
    frame->header.timestamp_wf_1 = ts;
    frame->header.timestamp_wf_2 = ts >> 32;
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t offset = 25) // NOLINT(build/unsigned)
  {
    uint64_t ts_next = first_timestamp; // NOLINT(build/unsigned)
    for (unsigned int i = 0; i < 12; ++i) {
      auto df = reinterpret_cast<dunedaq::detdataformats::DAPHNEFrame*>(((uint8_t*)(&data)) + i * 584); // NOLINT
      df->header.timestamp_wf_1 = ts_next;
      df->header.timestamp_wf_2 = ts_next >> 32;
      ts_next += offset;
    }
  }

  FrameType* begin()
  {
    return reinterpret_cast<FrameType*>(&data[0]); // NOLINT
  }

  FrameType* end()
  {
    return reinterpret_cast<FrameType*>(data + DAPHNE_SUPERCHUNK_SIZE); // NOLINT
  }

  static const constexpr daqdataformats::GeoID::SystemType system_type = daqdataformats::GeoID::SystemType::kPDS;
  static const constexpr daqdataformats::FragmentType fragment_type = daqdataformats::FragmentType::kPDSData;
  static const constexpr uint64_t tick_dist = 16; // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = 584;
  static const constexpr uint8_t frames_per_element = 12; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = frame_size * frames_per_element;
};

static_assert(sizeof(struct DAPHNE_SUPERCHUNK_STRUCT) == DAPHNE_SUPERCHUNK_SIZE,
              "Check your assumptions on DAPHNE_SUPERCHUNK_STRUCT");

const constexpr std::size_t TP_SIZE = sizeof(triggeralgs::TriggerPrimitive);
struct TP_READOUT_TYPE
{
  using FrameType = TP_READOUT_TYPE;
  // data
  triggeralgs::TriggerPrimitive tp;
  // comparable based on start timestamp
  bool operator<(const TP_READOUT_TYPE& other) const { return this->tp.time_start < other.tp.time_start; }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return tp.time_start;
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    tp.time_start = ts;
  }

  void fake_timestamp(uint64_t first_timestamp, uint64_t /*offset = 25*/) // NOLINT(build/unsigned)
  {
    tp.time_start = first_timestamp;
  }

  FrameType* begin() { return this; }

  FrameType* end() { return (this + 1); } // NOLINT

  static const constexpr daqdataformats::GeoID::SystemType system_type = daqdataformats::GeoID::SystemType::kTPC;
  static const constexpr daqdataformats::FragmentType fragment_type = daqdataformats::FragmentType::kTriggerPrimitives;
  static const constexpr uint64_t tick_dist = 25; // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = TP_SIZE;
  static const constexpr uint8_t frames_per_element = 1; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = TP_SIZE;
};

static_assert(sizeof(struct TP_READOUT_TYPE) == sizeof(triggeralgs::TriggerPrimitive),
              "Check your assumptions on TP_READOUT_TYPE");

const constexpr std::size_t SSP_FRAME_SIZE = 1012;
struct SSP_FRAME_STRUCT
{
  using FrameType = SSP_FRAME_STRUCT;

  // header
  detdataformats::EventHeader header;

  // data
  char data[SSP_FRAME_SIZE];

  // comparable based on start timestamp
  bool operator<(const SSP_FRAME_STRUCT& other) const
  {
    return this->get_timestamp() < other.get_timestamp() ? true : false;
  }

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    auto ehptr = &header;
    unsigned long ts = 0; // NOLINT(runtime/int)
    for (unsigned int iword = 0; iword <= 3; ++iword) {
      ts += ((unsigned long)(ehptr->timestamp[iword])) << 16 * iword; //NOLINT(runtime/int)
    }
    return ts;
  }

  void set_timestamp(uint64_t ts) // NOLINT(build/unsigned)
  {
    uint64_t bitmask = (1 << 16) - 1; // NOLINT(build/unsigned)
    for (unsigned int iword = 0; iword <= 3; ++iword) {
      header.timestamp[iword] = static_cast<uint16_t>((ts & bitmask)); // NOLINT(build/unsigned)
      ts = ts >> 16;
    }
  }

  void fake_timestamp(uint64_t /*first_timestamp*/, uint64_t /*offset = 25*/) // NOLINT(build/unsigned)
  {
    // tp.time_start = first_timestamp;
  }

  FrameType* begin() { return this; }

  FrameType* end() { return (this + 1); } // NOLINT

  static const constexpr daqdataformats::GeoID::SystemType system_type = daqdataformats::GeoID::SystemType::kPDS;
  static const constexpr daqdataformats::FragmentType fragment_type = daqdataformats::FragmentType::kPDSData;
  static const constexpr uint64_t tick_dist = 25; // NOLINT(build/unsigned)
  static const constexpr size_t frame_size = SSP_FRAME_SIZE;
  static const constexpr uint8_t frames_per_element = 1; // NOLINT(build/unsigned)
  static const constexpr size_t element_size = SSP_FRAME_SIZE;
};

static_assert(sizeof(struct SSP_FRAME_STRUCT) == sizeof(detdataformats::EventHeader) + SSP_FRAME_SIZE,
              "Check your assumptions on TP_READOUT_TYPE");

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

// raw WIB TP
struct RAW_WIB_TP_STRUCT
{
  dunedaq::detdataformats::TpHeader head;
  dunedaq::detdataformats::TpDataBlock block;
  dunedaq::detdataformats::TpPedinfo ped;
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
