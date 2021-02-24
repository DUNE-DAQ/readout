/**
 * @file WIBFrame.hpp WIB1 bit fields and accessors
 * Originally FelixFrame.hpp from protodune.
 * Original authors M. Vermeulen, R.Sipos 2018
 * Modified by P. Rodrigues on June 2020
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef DATAFORMATS_INCLUDE_DATAFORMATS_WIB_WIBFRAME_HPP_
#define DATAFORMATS_INCLUDE_DATAFORMATS_WIB_WIBFRAME_HPP_

#include "ers/ers.h"

#include <bitset>
#include <iostream>
#include <vector>

namespace dunedaq {

/**
 * @brief An ERS Error indicating that the requested index is out of range
 * @param wib_index_supplied Index that caused the error
 * @param wib_index_min Minium valid index for this function
 * @param wib_index_max Maximum valid index for this function
 * @cond Doxygen doesn't like ERS macros
 */
ERS_DECLARE_ISSUE(dataformats,
                  WibFrameRelatedIndexError,
                  "Supplied index " << wib_index_supplied << " is outside the allowed range of " << wib_index_min << " to " << wib_index_max,
                  ((int)wib_index_supplied)((int)wib_index_min)((int)wib_index_max)) // NOLINT
    /// @endcond



namespace dataformats {

using word_t = uint32_t; // NOLINT(build/unsigned)
using adc_t = uint16_t;  // NOLINT(build/unsigned)

/**
 * @brief WIB header struct
 */
struct WIBHeader
{
  word_t m_sof : 8, m_version : 5, m_fiber_no : 3, m_crate_no : 5, m_slot_no : 3, m_reserved_1 : 8;
  word_t m_mm : 1, m_oos : 1, m_reserved_2 : 14, m_wib_errors : 16;
  word_t m_timestamp_1;
  word_t m_timestamp_2 : 16, m_wib_counter_1 : 15, m_z : 1;

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    uint64_t timestamp = m_timestamp_1 | (static_cast<uint64_t>(m_timestamp_2) << 32); // NOLINT(build/unsigned)
    if (!m_z) {
      timestamp |= static_cast<uint64_t>(m_wib_counter_1) << 48; // NOLINT(build/unsigned)
    }
    return timestamp;
  }
  uint16_t get_wib_counter() const { return m_z ? m_wib_counter_1 : 0; } // NOLINT(build/unsigned)

  void set_timestamp(const uint64_t new_timestamp) // NOLINT(build/unsigned)
  {
    m_timestamp_1 = new_timestamp;
    m_timestamp_2 = new_timestamp >> 32;
    if (!m_z) {
      m_wib_counter_1 = new_timestamp >> 48;
    }
  }
  void set_wib_counter(const uint16_t new_wib_counter) // NOLINT(build/unsigned)
  {
    if (m_z) {
      m_wib_counter_1 = new_wib_counter;
    }
  }

  // Print functions for debugging.
  std::ostream& print_hex(std::ostream& o) const
  {
    return o << std::hex << "SOF:" << m_sof << " version:" << m_version << " fiber:" << m_fiber_no
             << " slot:" << m_slot_no << " crate:" << m_crate_no << " mm:" << m_mm << " oos:" << m_oos
             << " wib_errors:" << m_wib_errors << " timestamp: " << get_timestamp() << std::dec << '\n';
  }

  std::ostream& print_bits(std::ostream& o) const
  {
    return o << "SOF:" << std::bitset<8>(m_sof) << " version:" << std::bitset<5>(m_version)
             << " fiber:" << std::bitset<3>(m_fiber_no) << " slot:" << std::bitset<5>(m_slot_no)
             << " crate:" << std::bitset<3>(m_crate_no) << " mm:" << bool(m_mm) << " oos:" << bool(m_oos)
             << " wib_errors:" << std::bitset<16>(m_wib_errors) << " timestamp: " << get_timestamp() << '\n'
             << " Z: " << m_z << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, WIBHeader const& h)
{
  return o << "SOF:" << unsigned(h.m_sof) << " version:" << unsigned(h.m_version) << " fiber:" << unsigned(h.m_fiber_no)
           << " slot:" << unsigned(h.m_slot_no) << " crate:" << unsigned(h.m_crate_no) << " mm:" << unsigned(h.m_mm)
           << " oos:" << unsigned(h.m_oos) << " wib_errors:" << unsigned(h.m_wib_errors)
           << " timestamp: " << h.get_timestamp() << '\n';
}

/**
 * @brief COLDATA header struct
 */
struct ColdataHeader
{
  word_t m_s1_error : 4, m_s2_error : 4, m_reserved_1 : 8, m_checksum_a_1 : 8, m_checksum_b_1 : 8;
  word_t m_checksum_a_2 : 8, m_checksum_b_2 : 8, m_coldata_convert_count : 16;
  word_t m_error_register : 16, m_reserved_2 : 16;
  word_t m_hdr_1 : 4, m_hdr_3 : 4, m_hdr_2 : 4, m_hdr_4 : 4, m_hdr_5 : 4, m_hdr_7 : 4, m_hdr_6 : 4, m_hdr_8 : 4;

  uint16_t get_checksum_a() const { return static_cast<uint16_t>(m_checksum_a_1) | (m_checksum_a_2 << 8); } // NOLINT(build/unsigned)
  uint16_t get_checksum_b() const { return static_cast<uint16_t>(m_checksum_b_1) | (m_checksum_b_2 << 8); } // NOLINT(build/unsigned)
  uint8_t get_hdr(const uint8_t i) const                                                       // NOLINT(build/unsigned)
  {
    switch (i) {
      case 1:
        return m_hdr_1;
      case 2:
        return m_hdr_2;
      case 3:
        return m_hdr_3;
      case 4:
        return m_hdr_4;
      case 5:
        return m_hdr_5;
      case 6:
        return m_hdr_6;
      case 7:
        return m_hdr_7;
      case 8:
        return m_hdr_8;
    }
    return 0;
  }

  void set_checksum_a(const uint16_t new_checksum_a) // NOLINT(build/unsigned)
  {
    m_checksum_a_1 = new_checksum_a;
    m_checksum_a_2 = new_checksum_a >> 8;
  }
  void set_checksum_b(const uint16_t new_checksum_b) // NOLINT(build/unsigned)
  {
    m_checksum_b_1 = new_checksum_b;
    m_checksum_b_2 = new_checksum_b >> 8;
  }
  void set_hdr(const uint8_t i, const uint8_t new_hdr) // NOLINT(build/unsigned)
  {
    switch (i) {
      case 1:
        m_hdr_1 = new_hdr;
        break;
      case 2:
        m_hdr_2 = new_hdr;
        break;
      case 3:
        m_hdr_3 = new_hdr;
        break;
      case 4:
        m_hdr_4 = new_hdr;
        break;
      case 5:
        m_hdr_5 = new_hdr;
        break;
      case 6:
        m_hdr_6 = new_hdr;
        break;
      case 7:
        m_hdr_7 = new_hdr;
        break;
      case 8:
        m_hdr_8 = new_hdr;
        break;
    }
  }

  // Print functions for debugging.
  std::ostream& print_hex(std::ostream& o) const
  {
    o << std::hex << "s1_error:" << m_s1_error << " s2_error:" << m_s2_error << " checksum_a1:" << m_checksum_a_1
      << " checksum_b1:" << m_checksum_b_1 << " checksum_a2:" << m_checksum_a_2 << " checksum_b1:" << m_checksum_b_2
      << " coldata_convert_count:" << m_coldata_convert_count << " error_register:" << m_error_register
      << " hdr_1:" << m_hdr_1 << " hdr_2:" << m_hdr_2 << " hdr_3:" << m_hdr_3 << " hdr_4:" << m_hdr_4
      << " hdr_5:" << m_hdr_5 << " hdr_6:" << m_hdr_6 << " hdr_7:" << m_hdr_7 << " hdr_8:" << m_hdr_8;
    return o << '\n';
  }
  std::ostream& print_bits(std::ostream& o) const
  {
    o << "s1_error:" << std::bitset<4>(m_s1_error) << " s2_error:" << std::bitset<4>(m_s2_error)
      << " checksum_a1:" << std::bitset<8>(m_checksum_a_1) << " checksum_b1:" << std::bitset<8>(m_checksum_b_1)
      << " checksum_a2:" << std::bitset<8>(m_checksum_a_2) << " checksum_b2:" << std::bitset<8>(m_checksum_b_2)
      << " coldata_convert_count:" << std::bitset<16>(m_coldata_convert_count)
      << " error_register:" << std::bitset<16>(m_error_register) << " hdr_1:" << std::bitset<8>(m_hdr_1)
      << " hdr_2:" << std::bitset<8>(m_hdr_2) << " hdr_3:" << std::bitset<8>(m_hdr_3)
      << " hdr_4:" << std::bitset<8>(m_hdr_4) << " hdr_5:" << std::bitset<8>(m_hdr_5)
      << " hdr_6:" << std::bitset<8>(m_hdr_6) << " hdr_7:" << std::bitset<8>(m_hdr_7)
      << " hdr_8:" << std::bitset<8>(m_hdr_8);
    return o << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, ColdataHeader const& hdr)
{
  o << "s1_error:" << unsigned(hdr.m_s1_error) << " s2_error:" << unsigned(hdr.m_s2_error)
    << " checksum_a1:" << unsigned(hdr.m_checksum_a_1) << " checksum_b1:" << unsigned(hdr.m_checksum_b_1)
    << " checksum_a2:" << unsigned(hdr.m_checksum_a_2) << " checksum_b1:" << unsigned(hdr.m_checksum_b_2)
    << " coldata_convert_count:" << unsigned(hdr.m_coldata_convert_count)
    << " error_register:" << unsigned(hdr.m_error_register) << " hdr_1:" << unsigned(hdr.m_hdr_1)
    << " hdr_2:" << unsigned(hdr.m_hdr_2) << " hdr_3:" << unsigned(hdr.m_hdr_3) << " hdr_4:" << unsigned(hdr.m_hdr_4)
    << " hdr_5:" << unsigned(hdr.m_hdr_5) << " hdr_6:" << unsigned(hdr.m_hdr_6) << " hdr_7:" << unsigned(hdr.m_hdr_7)
    << " hdr_8:" << unsigned(hdr.m_hdr_8);
  return o << '\n';
}

/**
 * @brief COLDATA segment struct
 */
struct ColdataSegment
{
  static constexpr int s_num_ch_per_seg = 8;

  // This struct contains three words of ADC values that form the main repeating
  // pattern in the COLDATA block.
  word_t m_adc0ch0_1 : 8, m_adc1ch0_1 : 8, m_adc0ch0_2 : 4, m_adc0ch1_1 : 4, m_adc1ch0_2 : 4, m_adc1ch1_1 : 4;
  word_t m_adc0ch1_2 : 8, m_adc1ch1_2 : 8, m_adc0ch2_1 : 8, m_adc1ch2_1 : 8;
  word_t m_adc0ch2_2 : 4, m_adc0ch3_1 : 4, m_adc1ch2_2 : 4, m_adc1ch3_1 : 4, m_adc0ch3_2 : 8, m_adc1ch3_2 : 8;

  uint16_t get_channel(const uint8_t adc, const uint8_t ch) const // NOLINT(build/unsigned)
  {
    if (adc % 2 == 0) {
      switch (ch % 4) {
        case 0:
          return m_adc0ch0_1 | m_adc0ch0_2 << 8;
        case 1:
          return m_adc0ch1_1 | m_adc0ch1_2 << 4;
        case 2:
          return m_adc0ch2_1 | m_adc0ch2_2 << 8;
        case 3:
          return m_adc0ch3_1 | m_adc0ch3_2 << 4;
      }
    } else if (adc % 2 == 1) {
      switch (ch % 4) {
        case 0:
          return m_adc1ch0_1 | m_adc1ch0_2 << 8;
        case 1:
          return m_adc1ch1_1 | m_adc1ch1_2 << 4;
        case 2:
          return m_adc1ch2_1 | m_adc1ch2_2 << 8;
        case 3:
          return m_adc1ch3_1 | m_adc1ch3_2 << 4;
      }
    }
    throw WibFrameRelatedIndexError(ERS_HERE, adc, 0, 1);
  }

  void set_channel(const uint8_t adc, const uint8_t ch, const uint16_t new_val) // NOLINT(build/unsigned)
  {
    if (adc % 2 == 0) {
      switch (ch % 4) {
        case 0:
          m_adc0ch0_1 = new_val;
          m_adc0ch0_2 = new_val >> 8;
          break;
        case 1:
          m_adc0ch1_1 = new_val;
          m_adc0ch1_2 = new_val >> 4;
          break;
        case 2:
          m_adc0ch2_1 = new_val;
          m_adc0ch2_2 = new_val >> 8;
          break;
        case 3:
          m_adc0ch3_1 = new_val;
          m_adc0ch3_2 = new_val >> 4;
          break;
      }
    } else if (adc % 2 == 1) {
      switch (ch % 4) {
        case 0:
          m_adc1ch0_1 = new_val;
          m_adc1ch0_2 = new_val >> 8;
          break;
        case 1:
          m_adc1ch1_1 = new_val;
          m_adc1ch1_2 = new_val >> 4;
          break;
        case 2:
          m_adc1ch2_1 = new_val;
          m_adc1ch2_2 = new_val >> 8;
          break;
        case 3:
          m_adc1ch3_1 = new_val;
          m_adc1ch3_2 = new_val >> 4;
          break;
      }
    }
  }
};

/**
 * @brief COLDATA block struct
 */
struct ColdataBlock
{
  static constexpr int s_num_seg_per_block = 8;
  static constexpr int s_num_ch_per_adc = 8;
  static constexpr int s_num_adc_per_block =
    ColdataSegment::s_num_ch_per_seg * s_num_seg_per_block / s_num_ch_per_adc;
  static constexpr int s_num_ch_per_block = s_num_seg_per_block * ColdataSegment::s_num_ch_per_seg;

  ColdataHeader m_head;
  ColdataSegment m_segments[s_num_seg_per_block]; // NOLINT

  uint16_t get_channel(const uint8_t adc, const uint8_t ch) const // NOLINT(build/unsigned)
  {
    // Each segment houses one half (four channels) of two subsequent ADCs.
    return m_segments[ get_segment_index_(adc, ch) ].get_channel(adc, ch);
  }

  void set_channel(const uint8_t adc, const uint8_t ch, const uint16_t new_val) // NOLINT(build/unsigned)
  {
    m_segments[ get_segment_index_(adc, ch) ].set_channel(adc, ch, new_val);
  }

private:
  int get_segment_index_(const int adc, const int ch) const {
    auto segment_id = (adc / 2) * 2 + ch / 4;

    if (segment_id < 0 || segment_id > s_num_seg_per_block - 1) {
      throw WibFrameRelatedIndexError(ERS_HERE, segment_id, 0, s_num_seg_per_block - 1);
    }
    return segment_id;
  }
};

inline std::ostream&
operator<<(std::ostream& o, const ColdataBlock& block)
{
  o << block.m_head;

  // Note that this is an ADC-centric view, whereas ColdataBlock uses a channel-centric view
  o << "\t\t0\t1\t2\t3\t4\t5\t6\t7\n";
  for (int adc = 0; adc < 8; adc++) {
    o << "Stream " << adc << ":\t";
    for (int ch = 0; ch < 8; ch++) {
      o << std::hex << block.get_channel(adc, ch) << '\t';
    }
    o << std::dec << '\n';
  }
  return o;
}

/**
 * @brief FELIX frame
 */
class WIBFrame
{
public:
  static constexpr int s_num_block_per_frame = 4;
  static constexpr int s_num_ch_per_frame = s_num_block_per_frame * ColdataBlock::s_num_ch_per_block;

  static constexpr int s_num_frame_hdr_words = sizeof(WIBHeader) / sizeof(word_t);
  static constexpr int s_num_COLDATA_hdr_words = sizeof(ColdataHeader) / sizeof(word_t);
  static constexpr int s_num_COLDATA_words = sizeof(ColdataBlock) / sizeof(word_t);
  static constexpr int s_num_frame_words = s_num_block_per_frame * s_num_COLDATA_words + s_num_frame_hdr_words;
  static constexpr int s_num_frame_bytes = s_num_frame_words * sizeof(word_t);

  const WIBHeader* get_wib_header() const { return &m_head; }
        WIBHeader* get_wib_header()       { return &m_head; }

  const ColdataHeader* get_coldata_header(const unsigned block_index) const
  {
    throw_if_invalid_block_index_(block_index);
    return &m_blocks[block_index].m_head;
  }
  const ColdataBlock& get_block(const uint8_t b) const { // NOLINT(build/unsigned)
    throw_if_invalid_block_index_(b);
    return m_blocks[b]; 
  }

  // WIBHeader mutators
  void set_wib_errors(const uint16_t new_wib_errors) { m_head.m_wib_errors = new_wib_errors; } // NOLINT(build/unsigned)
  void set_timestamp(const uint64_t new_timestamp) { m_head.set_timestamp(new_timestamp); }    // NOLINT(build/unsigned)

  // ColdataBlock channel accessors
  uint16_t get_channel(const uint8_t block_num, const uint8_t adc, const uint8_t ch) const // NOLINT(build/unsigned)
  {
    throw_if_invalid_block_index_(block_num);
    return m_blocks[block_num].get_channel(adc, ch);
  }
  uint16_t get_channel(const uint8_t block_num, const uint8_t ch) const // NOLINT(build/unsigned)
  {
    throw_if_invalid_block_index_(block_num);
    return get_channel(block_num, ch / ColdataBlock::s_num_adc_per_block, ch % ColdataBlock::s_num_adc_per_block);
  }
  uint16_t get_channel(const uint8_t ch) const // NOLINT(build/unsigned)
  {
    return get_channel(ch / ColdataBlock::s_num_ch_per_block, ch % ColdataBlock::s_num_ch_per_block);
  }

  // ColdataBlock channel mutators
  void set_channel(const uint8_t block_num,
                   const uint8_t adc,
                   const uint8_t ch,
                   const uint16_t new_val) // NOLINT(build/unsigned)
  {
    throw_if_invalid_block_index_(block_num);
    m_blocks[block_num].set_channel(adc, ch, new_val);
  }
  void set_channel(const uint8_t block_num, const uint8_t ch, const uint16_t new_val) // NOLINT(build/unsigned)
  {
    throw_if_invalid_block_index_(block_num);
    set_channel(block_num, ch / ColdataBlock::s_num_adc_per_block, ch % ColdataBlock::s_num_adc_per_block, new_val);
  }
  void set_channel(const uint8_t ch, const uint16_t new_val) // NOLINT(build/unsigned)
  {
    set_channel(ch / ColdataBlock::s_num_ch_per_block, ch % ColdataBlock::s_num_ch_per_block, new_val);
  }

  friend std::ostream& operator<<(std::ostream& o, WIBFrame const& frame);

private:

  void throw_if_invalid_block_index_(const int block_num) const {
    if (block_num < 0 || block_num > s_num_block_per_frame - 1) {
      throw WibFrameRelatedIndexError(ERS_HERE, block_num, 0, s_num_block_per_frame - 1);
    }
  }

  WIBHeader m_head;
  ColdataBlock m_blocks[s_num_block_per_frame]; // NOLINT
};

inline std::ostream&
operator<<(std::ostream& o, WIBFrame const& frame)
{
  o << "Printing frame:" << '\n';
  o << frame.m_head << '\n';
  for (auto b : frame.m_blocks) {
    o << b;
  }
  return o;
}

} // namespace dataformats
} // namespace dunedaq

#endif // DATAFORMATS_INCLUDE_DATAFORMATS_WIB_WIBFRAME_HPP_
