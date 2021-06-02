/**
 * @file ReadoutConstants.hpp Common constants in udaq-readout
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_READOUTCONSTANTS_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_READOUTCONSTANTS_HPP_

namespace dunedaq {
namespace readout {
namespace constant {

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
const constexpr std::size_t RAW_WIB_TP_SUBFRAME_SIZE =
  12; // same size for header, tp data, pedinfo: 3 words * 4 bytes/word

} // namespace constant
} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_READOUTCONSTANTS_HPP_
