/**
 * @file TPGConstants.hpp TPG specific constants
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_TPG_TPGCONSTANTS_HPP_
#define READOUT_SRC_WIB_TPG_TPGCONSTANTS_HPP_

#include <limits>

namespace swtpg {

static const size_t SUPERCHUNK_FRAME_SIZE = 5568; // for 12: 5568  for 6: 2784
struct SUPERCHUNK_CHAR_STRUCT {
  char fragments[SUPERCHUNK_FRAME_SIZE];
};
static_assert(sizeof(struct SUPERCHUNK_CHAR_STRUCT) == 5568, "Check your assumptions on SUPERCHUNK_CHAR_STRUCT");

static const constexpr unsigned short magic = std::numeric_limits<unsigned short>::max();
static const constexpr int16_t threshold = 2000;

// How many frames are concatenated in one netio message
static constexpr size_t FRAMES_PER_MSG = 12;

// How many collection-wire AVX2 registers are returned per
// frame.
static constexpr size_t REGISTERS_PER_FRAME = 6;

// How many bytes are in an AVX2 register
static constexpr size_t BYTES_PER_REGISTER = 32;

// How many samples are in a register
static constexpr size_t SAMPLES_PER_REGISTER = 16;

} // namespace swtpg

#endif // READOUT_SRC_WIB_TPG_TPGCONSTANTS_HPP_
