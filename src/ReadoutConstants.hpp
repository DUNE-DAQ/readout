/**
* @file ReadoutConstants.hpp Common constants in udaq-readout
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTCONSTANTS_HPP_
#define UDAQ_READOUT_SRC_READOUTCONSTANTS_HPP_

namespace dunedaq {
namespace readout {
namespace constant {

/**
 * @brief Card related constants that are not supposed to change
 * except firmware constraints are altered.
*/
/*
static constexpr size_t M_MAX_LINKS_PER_CARD=6;
static constexpr size_t M_MARGIN_BLOCKS=4;
static constexpr size_t M_BLOCK_THRESHOLD=256;
static constexpr size_t M_BLOCK_SIZE=felix::packetformat::BLOCKSIZE;
*/

/** 
 * @brief SuperChunk concept: The FELIX user payloads are called CHUNKs.
 * There is mechanism in firmware to aggregate WIB frames to a user payload 
 * that is called a SuperChunk. Default mode is with 12 frames:
 * 12[WIB frames] x 464[Bytes] = 5568[Bytes] 
*/
const constexpr std::size_t WIB_FRAME_SIZE = 464;

const constexpr std::size_t FLX_SUPERCHUNK_FACTOR = 12;

const constexpr std::size_t WIB_SUPERCHUNK_SIZE = 5568; // for 12: 5568

// Raw WIB TP 
const constexpr std::size_t RAW_WIB_TP_SUBFRAME_SIZE = 12; // header, data, pedinfo have same size: 3 words * 4 bytes/word  

} // namespace constant
} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTCONSTANTS_HPP_
