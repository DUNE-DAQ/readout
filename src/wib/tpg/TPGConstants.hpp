/**
 * @file TPGConstants.hpp TPG specific constants
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_TPG_TPGCONSTANTS_HPP_
#define READOUT_SRC_WIB_TPG_TPGCONSTANTS_HPP_

#include <cstdint>
#include <cstdlib>
#include <limits>

namespace swtpg {

const constexpr std::size_t SUPERCHUNK_FRAME_SIZE = 5568; // for 12: 5568  for 6: 2784
struct SUPERCHUNK_CHAR_STRUCT
{
  char fragments[SUPERCHUNK_FRAME_SIZE];
};
static_assert(sizeof(struct SUPERCHUNK_CHAR_STRUCT) == 5568, "Check your assumptions on SUPERCHUNK_CHAR_STRUCT");

const constexpr std::uint16_t MAGIC = std::numeric_limits<std::uint16_t>::max(); // NOLINT

const constexpr std::int16_t THRESHOLD = 2000;

// How many frames are concatenated in one netio message
const constexpr std::size_t FRAMES_PER_MSG = 12;

// How many collection-wire AVX2 registers are returned per
// frame.
const constexpr std::size_t COLLECTION_REGISTERS_PER_FRAME = 6;

// How many induction-wire AVX2 registers are returned per
// frame.
const constexpr std::size_t INDUCTION_REGISTERS_PER_FRAME = 10;

// How many bytes are in an AVX2 register
const constexpr std::size_t BYTES_PER_REGISTER = 32;

// How many samples are in a register
const constexpr std::size_t SAMPLES_PER_REGISTER = 16;

// One netio message's worth of collection channel ADCs after
// expansion: 12 frames per message times 8 registers per frame times
// 32 bytes (256 bits) per register
const constexpr std::size_t COLLECTION_ADCS_SIZE = BYTES_PER_REGISTER * COLLECTION_REGISTERS_PER_FRAME * FRAMES_PER_MSG;

} // namespace swtpg

#endif // READOUT_SRC_WIB_TPG_TPGCONSTANTS_HPP_
