/**
 * @file NDReadoutTypes.hpp Payload type structures for the DUNE Near Detector
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_NDREADOUTTYPES_HPP_
#define READOUT_INCLUDE_READOUT_NDREADOUTTYPES_HPP_

#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"

#include "dataformats/FragmentHeader.hpp"
#include "dataformats/GeoID.hpp"
#include "dataformats/pacman/PACMANFrame.hpp"

#include <cstdint> // uint_t types
#include <memory>  // unique_ptr

namespace dunedaq {
namespace readout {
namespace types {

/**
 * @brief PACMAN frame
 * Size = 816[Bytes] (12*64+1*32+2*8)
 * */
const constexpr std::size_t PACMAN_FRAME_SIZE = 1024 * 1024; 
struct PACMAN_MESSAGE_STRUCT
{
  using FrameType = PACMAN_MESSAGE_STRUCT;
  // data
  char data[PACMAN_FRAME_SIZE];
  // comparable based on first timestamp
  bool operator<(const PACMAN_MESSAGE_STRUCT& other) const
  {
    auto thisptr = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data);       
    auto otherptr = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&other.data); 
    return thisptr->get_msg_header((void *) &data)->unix_ts < otherptr->get_msg_header((void *) &other.data)->unix_ts ? true : false;
  }

  // message UNIX timestamp - NOT individual packet timestamps
  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_header((void *) &data)->unix_ts;
  }

  // FIX ME - implement this in the frame later
  void set_timestamp(uint64_t /*ts*/) // NOLINT(build/unsigned)
  {
    //reinterpret_cast<dunedaq::dataformats::PACMANFrame*>(&data)->set_timestamp(ts);
  }

  uint64_t get_message_type() const // NOLINT(build/unsigned)
  {
    return reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_header((void *) &data)->type;
  }

  void inspect_message() const
  {

    std::cout << "Message timestamp: " << get_timestamp() << std::endl;

    std::cout << "Message Type: " << (char)get_message_type() << std::endl;

    uint16_t numWords = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_header((void *) &data)->words;

    std::cout << "Num words in message: " << numWords << std::endl;

    for(unsigned int i = 0; i < numWords; i++)
    {

      std::cout << "Inspecting word " << i << std::endl;

      dunedaq::dataformats::PACMANFrame::PACMANMessageWord* theWord = reinterpret_cast<const dunedaq::dataformats::PACMANFrame*>(&data)->get_msg_word((void *) &data, i);

      
      std::cout << "Word type: " << (char)theWord->data_word.type << std::endl;
      std::cout << "PACMAN I/O Channel: " << (char)theWord->data_word.channel_id << std::endl;
      std::cout << "Word receipt timestamp: " << theWord->data_word.receipt_timestamp << std::endl;
      
      dunedaq::dataformats::PACMANFrame::LArPixPacket* thePacket = &(theWord->data_word.larpix_word);

      std::cout << "Inspecting packet" << std::endl;

      std::cout << "Packet Type: " << thePacket->data_packet.type << std::endl;
      std::cout << "Packet Chip ID: " << thePacket->data_packet.chipid << std::endl;  
      std::cout << "Packet Channel ID: " << thePacket->data_packet.channelid << std::endl;

      std::cout << "packet timestamp: " << thePacket->data_packet.timestamp<< std::endl;
    }
  }

  FrameType* begin()
  {
    return reinterpret_cast<FrameType*>(&data[0]);
  }

  FrameType* end()
  {
    return reinterpret_cast<FrameType*>(data+PACMAN_FRAME_SIZE);
  }

  static const constexpr dataformats::GeoID::SystemType system_type = dataformats::GeoID::SystemType::kNDLArTPC;
  static const constexpr dataformats::FragmentType fragment_type = dataformats::FragmentType::kNDLArTPC;
  static const constexpr size_t frames_per_element = 1;
  static const constexpr size_t tick_dist = 1;
  static const constexpr size_t frame_size = PACMAN_FRAME_SIZE;
  static const constexpr size_t element_size = PACMAN_FRAME_SIZE;
};

/**
 * Key finder for LBs.
 * */
struct PACMANTimestampGetter
{
  uint64_t operator()(PACMAN_MESSAGE_STRUCT& data) // NOLINT(build/unsigned)
  {
    return data.get_timestamp();
  }
};

typedef dunedaq::appfwk::DAQSink<PACMAN_MESSAGE_STRUCT> PACMANFrameSink;
typedef std::unique_ptr<PACMANFrameSink> UniquePACMANFrameSink;
using PACMANFramePtrSink = appfwk::DAQSink<std::unique_ptr<types::PACMAN_MESSAGE_STRUCT>>;
using UniquePACMANFramePtrSink = std::unique_ptr<PACMANFramePtrSink>;

} // namespace types
} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_NDREADOUTTYPES_HPP_
