/**
 * @file frame_expand.h WIB specific frame expansion
 * @author Philip Rodrigues (rodriges@fnal.gov)
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_TPG_FRAMEEXPAND_HPP_
#define READOUT_SRC_WIB_TPG_FRAMEEXPAND_HPP_

#include "TPGConstants.hpp"
#include "detdataformats/wib/WIBFrame.hpp"
#include "readout/ReadoutTypes.hpp"

#include <array>
#include <immintrin.h>

namespace swtpg {

struct MessageCollectionADCs
{
  char fragments[COLLECTION_ADCS_SIZE];
};

template<size_t NREGISTERS>
struct WindowCollectionADCs
{
  WindowCollectionADCs(size_t numMessages_, MessageCollectionADCs* fragments_)
    : numMessages(numMessages_)
    , fragments(fragments_)
  {}

  // Get a pointer to register `ireg` at time `itime`, as an AVX2 int register
  const __m256i* get256(size_t ireg, size_t itime) const
  {
    const size_t msg_index = itime / 12;
    const size_t msg_time_offset = itime % 12;
    const size_t index = msg_index * NREGISTERS * FRAMES_PER_MSG + FRAMES_PER_MSG * ireg + msg_time_offset;
    const __m256i* rawp = reinterpret_cast<const __m256i*>(fragments) + index; // NOLINT
    return rawp;
  }

  uint16_t get16(size_t ichan, size_t itime) const // NOLINT(build/unsigned)
  {
    const size_t register_index = ichan / SAMPLES_PER_REGISTER;
    const size_t register_offset = ichan % SAMPLES_PER_REGISTER;
    const size_t register_t0_start = register_index * SAMPLES_PER_REGISTER * FRAMES_PER_MSG;
    const size_t msg_index = itime / 12;
    const size_t msg_time_offset = itime % 12;
    // The index in uint16_t of the start of the message we want // NOLINT(build/unsigned)
    const size_t msg_start_index =
      msg_index * sizeof(MessageCollectionADCs) / sizeof(uint16_t); // NOLINT(build/unsigned)
    const size_t offset_within_msg = register_t0_start + SAMPLES_PER_REGISTER * msg_time_offset + register_offset;
    const size_t index = msg_start_index + offset_within_msg;
    return *(reinterpret_cast<uint16_t*>(fragments) + index); // NOLINT
  }

  size_t numMessages;
  MessageCollectionADCs* __restrict__ fragments;
};

// A little wrapper around an array of 256-bit registers, so that we
// can explicitly access it as an array of 256-bit registers or as an
// array of uint16_t
template<size_t N>
class RegisterArray
{
public:
  // RegisterArray() = default;

  // RegisterArray(RegisterArray& other)
  // {
  //     memcpy(m_array, other.m_array, N*sizeof(uint16_t)); NOLINT(build/unsigned)
  // }

  // RegisterArray(RegisterArray&& other) = default;

  // Get the value at the ith position as a 256-bit register
  inline __m256i ymm(size_t i) const
  {
    return _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(m_array) + i); // NOLINT
  }
  inline void set_ymm(size_t i, __m256i val)
  {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(m_array) + i, val); // NOLINT
  }
  inline uint16_t uint16(size_t i) const { return m_array[i]; }        // NOLINT(build/unsigned)
  inline void set_uint16(size_t i, uint16_t val) { m_array[i] = val; } // NOLINT(build/unsigned)

  // Access the jth entry in the ith register
  inline uint16_t uint16(size_t i, size_t j) const { return m_array[16 * i + j]; }        // NOLINT(build/unsigned)
  inline void set_uint16(size_t i, size_t j, uint16_t val) { m_array[16 * i + j] = val; } // NOLINT(build/unsigned)

  inline uint16_t* data() { return m_array; }             // NOLINT(build/unsigned)
  inline const uint16_t* data() const { return m_array; } // NOLINT(build/unsigned)

  inline size_t size() const { return N; }

private:
  alignas(32) uint16_t __restrict__ m_array[N * 16]; // NOLINT(build/unsigned)
};

typedef RegisterArray<6> FrameRegistersCollection;
typedef RegisterArray<10> FrameRegistersInduction;

typedef RegisterArray<6 * FRAMES_PER_MSG> MessageRegistersCollection;
typedef RegisterArray<10 * FRAMES_PER_MSG> MessageRegistersInduction;

struct FrameRegisters
{
  FrameRegistersCollection collection_registers;
  FrameRegistersInduction induction_registers;
};

struct MessageRegisters
{
  MessageRegistersCollection collection_registers;
  MessageRegistersInduction induction_registers;
};

//==============================================================================
// Print a 256-bit register interpreting it as packed 8-bit values
void
print256(__m256i var);

//==============================================================================
// Print a 256-bit register interpreting it as packed 16-bit values
void
print256_as16(__m256i var);

//==============================================================================
// Print a 256-bit register interpreting it as packed 16-bit values
void
print256_as16_dec(__m256i var);

//==============================================================================
// Abortive attempt at expanding just the collection channels, instead
// of expanding all channels and then picking out just the collection
// ones.
RegisterArray<2>
expand_segment_collection(const dunedaq::detdataformats::ColdataBlock& block);

//==============================================================================
// Take the raw memory containing 12-bit ADCs in the shuffled WIB
// format and rearrange them into 16-bit values in channel order. A
// 256-bit register holds 21-and-a-bit 12-bit values: we expand 16 of
// them into 16-bit values
inline __m256i
expand_two_segments(const dunedaq::detdataformats::ColdataSegment* __restrict__ first_segment)
{
  const __m256i* __restrict__ segments_start = reinterpret_cast<const __m256i*>(first_segment); // NOLINT
  __m256i raw = _mm256_lddqu_si256(segments_start);

  // First: the ADCs are arranged in a repeating pattern of 3 32-bit
  // words. The instructions we use later act won't shuffle items
  // across 128-bit lanes, so first we move the second block of 3
  // 32-bit words so it starts on the 128-bit boundary. Then we can
  // expand both blocks by doing the same thing on each 128-bit lane

  // The items at indices 3 and 7 are arbitrary: we won't be using
  // their values later
  const __m256i lane_shuffle = _mm256_setr_epi32(0, 1, 2, 0, 3, 4, 5, 0);
  raw = _mm256_permutevar8x32_epi32(raw, lane_shuffle);

  // We eventually want to end up with one vector containing all the
  // low-order nibbles with zeros elsewhere, and the other
  // containing all the high-order nibbles with zeros elsewhere. The
  // smallest unit in which we can fetch items is 8 bits, so we
  // fetch 8-bit units, shuffle them and mask
  const __m256i shuffle_mask_lo = _mm256_setr_epi8(
    0, 2, 2, 4, 6, 8, 8, 10, 1, 3, 3, 5, 7, 9, 9, 11, 0, 2, 2, 4, 6, 8, 8, 10, 1, 3, 3, 5, 7, 9, 9, 11);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
  const __m256i shuffle_mask_hi = _mm256_setr_epi8(0,
                                                   0xff,
                                                   4,
                                                   0xff,
                                                   6,
                                                   0xff,
                                                   10,
                                                   0xff,
                                                   1,
                                                   0xff,
                                                   5,
                                                   0xff,
                                                   7,
                                                   0xff,
                                                   11,
                                                   0xff,
                                                   0,
                                                   0xff,
                                                   4,
                                                   0xff,
                                                   6,
                                                   0xff,
                                                   10,
                                                   0xff,
                                                   1,
                                                   0xff,
                                                   5,
                                                   0xff,
                                                   7,
                                                   0xff,
                                                   11,
                                                   0xff);
#pragma GCC diagnostic pop
  // Get the 8-bit units into the 256-bit register in roughly the places they'll end up
  __m256i lo = _mm256_shuffle_epi8(raw, shuffle_mask_lo);
  __m256i hi = _mm256_shuffle_epi8(raw, shuffle_mask_hi);
  // Some nibbles in `lo` are in the high-order place when they
  // should be in the low-order place, and vice versa in `hi`. So we
  // make copies of the vectors that are shifted by one nibble, and
  // then use `blend` to select the shifted or unshifted version of
  // each item.
  __m256i lo_shift = _mm256_srli_epi16(lo, 4);
  __m256i hi_shift = _mm256_slli_epi16(hi, 4);
  // Whether we want the shifted or unshifted version alternates, so
  // the appropriate blend mask is 0xaa
  __m256i lo_blended = _mm256_blend_epi16(lo, lo_shift, 0xaa);
  __m256i hi_blended = _mm256_blend_epi16(hi, hi_shift, 0xaa);
  // Now we mask out the high-order nibbles in `lo`, and the low-order nibbles in `hi`, so that we can OR them together.
  // Effectively: (hi & 0xf0) | (lo & 0x0f)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
  __m256i lo_nibble_mask = _mm256_set1_epi16(0x0f0f);
  __m256i hi_nibble_mask = _mm256_set1_epi16(0xf0f0);
#pragma GCC diagnostic pop
  __m256i lo_masked = _mm256_and_si256(lo_blended, lo_nibble_mask);
  __m256i hi_masked = _mm256_and_si256(hi_blended, hi_nibble_mask);
  __m256i final = _mm256_or_si256(lo_masked, hi_masked);
  // The low 128 bits of `final` contain 8 16-bit adc values
  return final;
}

inline RegisterArray<5>
get_block_divided_adcs(const dunedaq::detdataformats::ColdataBlock& __restrict__ block)
{
  // First expand all of the channels into `expanded_all`
  __m256i expanded_all[4];
  for (int j = 0; j < 4; ++j) {
    expanded_all[j] = expand_two_segments(&block.segments[2 * j]);
  }

  // -------------------------------------------------------------------------
  // Select the collection channels into the first 2 registers of the output array
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
  // Now select just the collection channels, using a blend
  __m256i coll_blend_mask = _mm256_setr_epi16(0xffff,
                                              0xffff,
                                              0xffff,
                                              0xffff, // 4 from the second segment, please Carol
                                              0xffff,
                                              0xffff, // 2 don't care
                                              0x0,
                                              0x0, // 2 from the first
                                              0xffff,
                                              0xffff, // 2 from the second
                                              0x0,
                                              0x0, // 2 don't care
                                              0x0,
                                              0x0,
                                              0x0,
                                              0x0); // 4 from the first
#pragma GCC diagnostic pop
  RegisterArray<5> divided;
  // Blend, and shuffle so that the "don't care"s are on the end
  const __m256i shuffle_mask = _mm256_setr_epi32(0, 1, 3, 4, 6, 7, 0, 0);
  divided.set_ymm(
    0,
    _mm256_permutevar8x32_epi32(_mm256_blendv_epi8(expanded_all[0], expanded_all[1], coll_blend_mask), shuffle_mask));
  divided.set_ymm(
    1,
    _mm256_permutevar8x32_epi32(_mm256_blendv_epi8(expanded_all[2], expanded_all[3], coll_blend_mask), shuffle_mask));

  // Set the dummy values to zero so they stand out more easily later on
  const int dummy_blend_mask = 0xc0;
  const __m256i zero = _mm256_setzero_si256();
  divided.set_ymm(0, _mm256_blend_epi32(divided.ymm(0), zero, dummy_blend_mask));
  divided.set_ymm(1, _mm256_blend_epi32(divided.ymm(1), zero, dummy_blend_mask));

  // -------------------------------------------------------------------------
  // Select the induction channels into the last 3 registers of the output array

  // Induction channels are a bit more complicated because the items
  // at positions 4,5,10,11 are induction channels in *both* of the
  // arrays returned by expand_two_segments, whereas there was no
  // such overlap with collection channels

  // The induction blend mask is just the NOT of the collection
  // blend mask. There's no single NOT intrinsic, so use NOTAND as
  // suggested here:
  //  https://stackoverflow.com/a/42636183/8881282
  const __m256i ind_blend_mask =
    _mm256_andnot_si256(coll_blend_mask, _mm256_cmpeq_epi8(coll_blend_mask, coll_blend_mask));
  divided.set_ymm(2, _mm256_blendv_epi8(expanded_all[0], expanded_all[1], ind_blend_mask));
  divided.set_ymm(3, _mm256_blendv_epi8(expanded_all[2], expanded_all[3], ind_blend_mask));
  // Now get the 8 items we missed: shuffle them into their final positions and then blend
  const __m256i tmp0 = _mm256_permutevar8x32_epi32(expanded_all[0], _mm256_setr_epi32(5, 0, 0, 0, 0, 0, 0, 0));
  // The item in expanded_all[1] is already in a valid position, so we elide what would be:
  //
  // const __m256i tmp1=_mm256_permutevar8x32_epi32(expanded_all[1], _mm256_setr_epi32(0, 0, 2, 0, 0, 0, 0, 0));
  //
  const __m256i tmp2 = _mm256_permutevar8x32_epi32(expanded_all[2], _mm256_setr_epi32(0, 5, 0, 0, 0, 0, 0, 0));
  const __m256i tmp3 = _mm256_permutevar8x32_epi32(expanded_all[3], _mm256_setr_epi32(0, 0, 0, 2, 0, 0, 0, 0));
  const __m256i tmpblend0 = _mm256_blend_epi32(tmp0, expanded_all[1], 0x04);
  const __m256i tmpblend1 = _mm256_blend_epi32(tmp2, tmp3, 0x08);

  // Blend the two registers together and zero out the dummy values as we did with the collection channels
  divided.set_ymm(4, _mm256_blend_epi32(_mm256_blend_epi32(tmpblend0, tmpblend1, 0xa), zero, 0xf0));

  return divided;
}

//==============================================================================

// Get all of the ADC values in the frame, expanded into 16-bit values
// and split into registers containing only collection or only
// induction channels.  There are 6 collection registers followed by
// 10 induction registers
inline FrameRegisters
get_frame_divided_adcs(const dunedaq::detdataformats::WIBFrame* __restrict__ frame)
{
  // First, get all items block-by-block. Each block produces
  // registers that are not full, so we will "compress" them when we
  // have more blocks
  RegisterArray<20> adcs_tmp;
  for (int i = 0; i < 4; ++i) {
    RegisterArray<5> block_adcs = get_block_divided_adcs(frame->get_block(i));
    adcs_tmp.set_ymm(2 * i, block_adcs.ymm(0));
    adcs_tmp.set_ymm(2 * i + 1, block_adcs.ymm(1));
    adcs_tmp.set_ymm(8 + 3 * i + 0, block_adcs.ymm(2));
    adcs_tmp.set_ymm(8 + 3 * i + 1, block_adcs.ymm(3));
    adcs_tmp.set_ymm(8 + 3 * i + 2, block_adcs.ymm(4));
  }

  // ---------------------------------------------------------------------------

  // "Compress" the collection registers

  // Now adcs_tmp contains 96 values in 8 registers, but we can fit
  // those values in 6 registers, so do that. This way, the
  // downstream processing code has less to do.

  // We'll take advantage of the fact that there are 4 "null" values
  // (ie 4*16 bits = 64 bits) in the registers at the end, so we can
  // just treat the registers as containing 64-bit items.

  // We're going to move values from the last two registers (indices
  // 6 and 7) into the spaces in registers 0-5, so we first shuffle
  // register 6 so it has the items we want in the right place to
  // blend, and then blend
  FrameRegisters adcs;

  // A register where every 64-bit word is the 0th 64-bit word from the original register
  __m256i reg0_quad0 = _mm256_permute4x64_epi64(adcs_tmp.ymm(6), 0x00);
  // A register where every 64-bit word is the 1st 64-bit word from the original register
  __m256i reg0_quad1 = _mm256_permute4x64_epi64(adcs_tmp.ymm(6), 0x55);
  // A register where every 64-bit word is the 2nd 64-bit word from the original register
  __m256i reg0_quad2 = _mm256_permute4x64_epi64(adcs_tmp.ymm(6), 0xaa);

  // Likewise for register 7
  __m256i reg1_quad0 = _mm256_permute4x64_epi64(adcs_tmp.ymm(7), 0x00);
  __m256i reg1_quad1 = _mm256_permute4x64_epi64(adcs_tmp.ymm(7), 0x55);
  __m256i reg1_quad2 = _mm256_permute4x64_epi64(adcs_tmp.ymm(7), 0xaa);

  // Treating the register as containing 8 32-bit items, this blend
  // mask takes 6 from the first register and 2 from the second
  // register, which is what we want
  int blend_mask = 0xc0;

  adcs.collection_registers.set_ymm(0, _mm256_blend_epi32(adcs_tmp.ymm(0), reg0_quad0, blend_mask));
  adcs.collection_registers.set_ymm(1, _mm256_blend_epi32(adcs_tmp.ymm(1), reg0_quad1, blend_mask));
  adcs.collection_registers.set_ymm(2, _mm256_blend_epi32(adcs_tmp.ymm(2), reg0_quad2, blend_mask));

  adcs.collection_registers.set_ymm(3, _mm256_blend_epi32(adcs_tmp.ymm(3), reg1_quad0, blend_mask));
  adcs.collection_registers.set_ymm(4, _mm256_blend_epi32(adcs_tmp.ymm(4), reg1_quad1, blend_mask));
  adcs.collection_registers.set_ymm(5, _mm256_blend_epi32(adcs_tmp.ymm(5), reg1_quad2, blend_mask));

  // ---------------------------------------------------------------------------
  // "Compress" the induction registers

  // This is a little easier than the collection case, because each
  // block gives two registers full of valid values, which we can
  // just copy straight to the output array, plus a register that's
  // exactly half-full, which we can combine with the half-full
  // register from another block

  // Already full registers: 8, 9, 11, 12, 14, 15, 17, 18
  // Half-full registers: 10, 13, 16, 19

  // Straight copy of the registers that are already full
  adcs.induction_registers.set_ymm(0, adcs_tmp.ymm(8));
  adcs.induction_registers.set_ymm(1, adcs_tmp.ymm(9));
  adcs.induction_registers.set_ymm(2, adcs_tmp.ymm(11));
  adcs.induction_registers.set_ymm(3, adcs_tmp.ymm(12));
  adcs.induction_registers.set_ymm(4, adcs_tmp.ymm(14));
  adcs.induction_registers.set_ymm(5, adcs_tmp.ymm(15));
  adcs.induction_registers.set_ymm(6, adcs_tmp.ymm(17));
  adcs.induction_registers.set_ymm(7, adcs_tmp.ymm(18));

  // Pair up the half-full registers and put them in the remaining spaces
  adcs.induction_registers.set_ymm(
    8, _mm256_blend_epi32(adcs_tmp.ymm(10), _mm256_permute4x64_epi64(adcs_tmp.ymm(13), 0x4f), 0xf0));
  adcs.induction_registers.set_ymm(
    9, _mm256_blend_epi32(adcs_tmp.ymm(16), _mm256_permute4x64_epi64(adcs_tmp.ymm(19), 0x4f), 0xf0));

  return adcs;
}

//==============================================================================

// Get all the collection channel values from a dune::ColdataBlock as 16-bit
// values into 2 256-bit registers. Implemented by expanding all the
// values using expand_two_segments, and then picking out the
// collection channels with a blend. There are only 12 collection
// channels in a dune::ColdataBlock, so we shuffle valid values into the
// 0-11 entries of the register, and leave 4 invalid values at the end of each
// register
inline RegisterArray<2>
get_block_collection_adcs(const dunedaq::detdataformats::ColdataBlock& __restrict__ block)
{
  // First expand all of the channels into `expanded_all`
  __m256i expanded_all[4];
  for (int j = 0; j < 4; ++j) {
    expanded_all[j] = expand_two_segments(&block.segments[2 * j]);
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
  // Now select just the collection channels, using a blend
  __m256i blend_mask = _mm256_setr_epi16(0xffff,
                                         0xffff,
                                         0xffff,
                                         0xffff, // 4 from the second segment, please Carol
                                         0x0,
                                         0x0, // 2 don't care
                                         0x0,
                                         0x0, // 2 from the first
                                         0xffff,
                                         0xffff, // 2 from the second
                                         0x0,
                                         0x0, // 2 don't care
                                         0x0,
                                         0x0,
                                         0x0,
                                         0x0); // 4 from the first
#pragma GCC diagnostic pop
  RegisterArray<2> expanded_coll;
  // Blend, and shuffle so that the "don't care"s are on the end
  const __m256i shuffle_mask = _mm256_setr_epi32(0, 1, 3, 4, 6, 7, 0, 0);
  expanded_coll.set_ymm(
    0, _mm256_permutevar8x32_epi32(_mm256_blendv_epi8(expanded_all[0], expanded_all[1], blend_mask), shuffle_mask));
  expanded_coll.set_ymm(
    1, _mm256_permutevar8x32_epi32(_mm256_blendv_epi8(expanded_all[2], expanded_all[3], blend_mask), shuffle_mask));

  // Set the dummy values to zero so they stand out more easily later on
  const int dummy_blend_mask = 0xc0;
  const __m256i zero = _mm256_setzero_si256();
  expanded_coll.set_ymm(0, _mm256_blend_epi32(expanded_coll.ymm(0), zero, dummy_blend_mask));
  expanded_coll.set_ymm(1, _mm256_blend_epi32(expanded_coll.ymm(1), zero, dummy_blend_mask));

  return expanded_coll;
}

//==============================================================================
inline RegisterArray<4>
get_block_all_adcs(const dunedaq::detdataformats::ColdataBlock& __restrict__ block)
{
  RegisterArray<4> expanded_all;
  for (int j = 0; j < 4; ++j) {
    expanded_all.set_ymm(j, expand_two_segments(&block.segments[2 * j]));
  }
  return expanded_all;
}

//==============================================================================
//
inline RegisterArray<REGISTERS_PER_FRAME>
get_frame_collection_adcs(const dunedaq::detdataformats::WIBFrame* __restrict__ frame)
{
  // Each coldata block has 24 collection channels, so we have to
  // put it in two registers, using 12 of the 16 slots in each
  RegisterArray<8> adcs_tmp;
  for (int i = 0; i < 4; ++i) {
    RegisterArray<2> block_adcs = get_block_collection_adcs(frame->get_block(i));
    adcs_tmp.set_ymm(2 * i, block_adcs.ymm(0));
    adcs_tmp.set_ymm(2 * i + 1, block_adcs.ymm(1));
  }

  // Now adcs_tmp contains 96 values in 8 registers, but we can fit
  // those values in 6 registers, so do that. This way, the
  // downstream processing code has less to do.

  // We'll take advantage of the fact that there are 4 "null" values
  // (ie 4*16 bits = 64 bits) in the registers at the end, so we can
  // just treat the registers as containing 64-bit items.

  // We're going to move values from the last two registers (indices
  // 6 and 7) into the spaces in registers 0-5, so we first shuffle
  // register 6 so it has the items we want in the right place to
  // blend, and then blend
  RegisterArray<REGISTERS_PER_FRAME> adcs;

  // A register where every 64-bit word is the 0th 64-bit word from the original register
  __m256i reg0_quad0 = _mm256_permute4x64_epi64(adcs_tmp.ymm(6), 0x00);
  // A register where every 64-bit word is the 1st 64-bit word from the original register
  __m256i reg0_quad1 = _mm256_permute4x64_epi64(adcs_tmp.ymm(6), 0x55);
  // A register where every 64-bit word is the 2nd 64-bit word from the original register
  __m256i reg0_quad2 = _mm256_permute4x64_epi64(adcs_tmp.ymm(6), 0xaa);

  // Likewise for register 7
  __m256i reg1_quad0 = _mm256_permute4x64_epi64(adcs_tmp.ymm(7), 0x00);
  __m256i reg1_quad1 = _mm256_permute4x64_epi64(adcs_tmp.ymm(7), 0x55);
  __m256i reg1_quad2 = _mm256_permute4x64_epi64(adcs_tmp.ymm(7), 0xaa);

  // Treating the register as containing 8 32-bit items, this blend
  // mask takes 6 from the first register and 2 from the second
  // register, which is what we want
  int blend_mask = 0xc0;

  adcs.set_ymm(0, _mm256_blend_epi32(adcs_tmp.ymm(0), reg0_quad0, blend_mask));
  adcs.set_ymm(1, _mm256_blend_epi32(adcs_tmp.ymm(1), reg0_quad1, blend_mask));
  adcs.set_ymm(2, _mm256_blend_epi32(adcs_tmp.ymm(2), reg0_quad2, blend_mask));

  adcs.set_ymm(3, _mm256_blend_epi32(adcs_tmp.ymm(3), reg1_quad0, blend_mask));
  adcs.set_ymm(4, _mm256_blend_epi32(adcs_tmp.ymm(4), reg1_quad1, blend_mask));
  adcs.set_ymm(5, _mm256_blend_epi32(adcs_tmp.ymm(5), reg1_quad2, blend_mask));

  return adcs;
}

//==============================================================================
inline RegisterArray<16>
get_frame_all_adcs(const dunedaq::detdataformats::WIBFrame* __restrict__ frame)
{
  RegisterArray<16> adcs;
  for (int i = 0; i < 4; ++i) {
    RegisterArray<4> block_adcs = get_block_all_adcs(frame->get_block(i));
    adcs.set_ymm(4 * i, block_adcs.ymm(0));
    adcs.set_ymm(4 * i + 1, block_adcs.ymm(1));
    adcs.set_ymm(4 * i + 2, block_adcs.ymm(2));
    adcs.set_ymm(4 * i + 3, block_adcs.ymm(3));
  }
  return adcs;
}

//======================================================================
inline MessageRegisters
expand_message_adcs(const SUPERCHUNK_CHAR_STRUCT& __restrict__ ucs)
{
  MessageRegisters adcs;
  for (size_t iframe = 0; iframe < FRAMES_PER_MSG; ++iframe) {
    const dunedaq::detdataformats::WIBFrame* frame =
      reinterpret_cast<const dunedaq::detdataformats::WIBFrame*>(&ucs) + iframe; // NOLINT
    FrameRegisters frame_regs = get_frame_divided_adcs(frame);
    for (size_t iblock = 0; iblock < REGISTERS_PER_FRAME; ++iblock) {
      // Arrange it so that adjacent times are adjacent in
      // memory, which will hopefully make the trigger primitive
      // finding code itself a little easier
      //
      // So the memory now looks like:
      // (register 0, time 0) (register 0, time 1) ... (register 0, time 11)
      // (register 1, time 0) (register 1, time 1) ... (register 1, time 11)
      // ...
      // (register 5, time 0) (register 5, time 1) ... (register 5, time 11)
      adcs.collection_registers.set_ymm(iframe + iblock * FRAMES_PER_MSG, frame_regs.collection_registers.ymm(iblock));
    }
    // Same for induction registers
    for (size_t iblock = 0; iblock < 10; ++iblock) {
      adcs.induction_registers.set_ymm(iframe + iblock * FRAMES_PER_MSG, frame_regs.induction_registers.ymm(iblock));
    }
  }
  return adcs;
}

//======================================================================
inline void
expand_message_adcs_inplace(const dunedaq::readout::types::WIB_SUPERCHUNK_STRUCT* __restrict__ ucs,
                            MessageRegistersCollection* __restrict__ collection_registers,
                            MessageRegistersInduction* __restrict__ induction_registers)
{
  for (size_t iframe = 0; iframe < FRAMES_PER_MSG; ++iframe) {
    const dunedaq::detdataformats::WIBFrame* frame =
      reinterpret_cast<const dunedaq::detdataformats::WIBFrame*>(ucs) + iframe; // NOLINT
    FrameRegisters frame_regs = get_frame_divided_adcs(frame);
    for (size_t iblock = 0; iblock < REGISTERS_PER_FRAME; ++iblock) {
      // Arrange it so that adjacent times are adjacent in
      // memory, which will hopefully make the trigger primitive
      // finding code itself a little easier
      //
      // So the memory now looks like:
      // (register 0, time 0) (register 0, time 1) ... (register 0, time 11)
      // (register 1, time 0) (register 1, time 1) ... (register 1, time 11)
      // ...
      // (register 5, time 0) (register 5, time 1) ... (register 5, time 11)
      collection_registers->set_ymm(iframe + iblock * FRAMES_PER_MSG, frame_regs.collection_registers.ymm(iblock));
    }
    // Same for induction registers
    for (size_t iblock = 0; iblock < 10; ++iblock) {
      induction_registers->set_ymm(iframe + iblock * FRAMES_PER_MSG, frame_regs.induction_registers.ymm(iblock));
    }
  }
}

//==============================================================================
// Take the raw memory containing 12-bit ADCs in the shuffled WIB
// format and rearrange them into 16-bit values in channel order. A
// 256-bit register holds 21-and-a-bit 12-bit values: we expand 16 of
// them into 16-bit values
// __m256i expand_two_segments(const dune::ColdataSegment* __restrict__ first_segment);

//==============================================================================

// Get all of the ADCs in the frame, expanded into 16-bit values and
// split into registers containing only collection or only induction
// channels.  There are 6 collection registers followed by 10
// induction registers
// FrameRegisters get_frame_divided_adcs(const dunedaq::detdataformats::WIBFrame* __restrict__ frame);

//==============================================================================

// Get all the collection channel values from a dune::ColdataBlock as 16-bit
// values into 2 256-bit registers. Implemented by expanding all the
// values using expand_two_segments, and then picking out the
// collection channels with a blend. There are only 12 collection
// channels in a dune::ColdataBlock, so we shuffle valid values into the
// 0-11 entries of the register, and leave 4 invalid values at the end of each
// register
// inline RegisterArray<2> get_block_collection_adcs(const dune::ColdataBlock& __restrict__ block);

//==============================================================================
// As above, for all collection and induction ADCs
// RegisterArray<4> get_block_all_adcs(const dune::ColdataBlock& __restrict__ block);

//==============================================================================
// Expand all the collection channels into 6 AVX2 registers
// RegisterArray<REGISTERS_PER_FRAME> get_frame_collection_adcs(const dunedaq::detdataformats::WIBFrame* __restrict__
// frame);

//==============================================================================
// As above, for all collection and induction ADCs
// RegisterArray<16> get_frame_all_adcs(const dunedaq::detdataformats::WIBFrame* __restrict__ frame);

//==============================================================================
int
collection_index_to_offline(int index);
int
induction_index_to_offline(int index);

//==============================================================================
int
collection_index_to_channel(int index);
int
induction_index_to_channel(int index);

//======================================================================
// MessageRegisters expand_message_adcs(const SUPERCHUNK_CHAR_STRUCT& __restrict__ ucs);

} // namespace swtpg

#endif // READOUT_SRC_WIB_TPG_FRAMEEXPAND_HPP_
