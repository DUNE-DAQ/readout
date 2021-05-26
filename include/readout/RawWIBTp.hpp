/**
 * @file RawWIBTp.hpp Raw Trigger Primitive bit fields and accessors
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_RAWWIBTP_HPP_
#define READOUT_INCLUDE_READOUT_RAWWIBTP_HPP_

#include <bitset>
#include <iostream>
#include <vector>

namespace dunedaq {
namespace dataformats {

using tp_word_t = uint32_t; // NOLINT(build/unsigned)

//===================
// TP header struct
//===================
struct TpHeader
{
  tp_word_t m_flags : 13, m_slot_no : 3, m_wire_no : 8, m_fiber_no : 3, m_crate_no : 5;
  tp_word_t m_timestamp_1;
  tp_word_t m_timestamp_2;

  uint64_t get_timestamp() const // NOLINT(build/unsigned)
  {
    uint64_t timestamp = m_timestamp_1 | (static_cast<int64_t>(m_timestamp_2) << 32); // NOLINT(build/unsigned)
    return timestamp;
  }

  void set_timestamp(const uint64_t new_timestamp) // NOLINT(build/unsigned)
  {
    m_timestamp_1 = new_timestamp;
    m_timestamp_2 = new_timestamp >> 32;
  }

  // Print functions for debugging.
  std::ostream& print(std::ostream& o) const
  {
    o << "Printing raw WIB TP header:\n";
    o << "flags:" << unsigned(m_flags) << " slot:" << unsigned(m_slot_no) << " wire:" << unsigned(m_wire_no)
      << " fiber:" << unsigned(m_fiber_no) << " crate:" << unsigned(m_crate_no) << " timestamp:" << get_timestamp();
    return o << '\n';
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    o << "Printing raw WIB TP header:\n";
    o << std::hex << "flags:" << m_flags << " slot:" << m_slot_no << " wire:" << m_wire_no << " fiber:" << m_fiber_no
      << " crate:" << m_crate_no << " timestamp:" << get_timestamp();
    return o << std::dec << '\n';
  }

  std::ostream& print_bits(std::ostream& o) const
  {
    o << "Printing raw WIB TP header:\n";
    o << "flags:" << std::bitset<13>(m_flags) << " slot:" << std::bitset<3>(m_slot_no)
      << " wire:" << std::bitset<8>(m_wire_no) << " fiber:" << std::bitset<3>(m_fiber_no)
      << " crate:" << std::bitset<5>(m_crate_no) << " timestamp:" << get_timestamp();
    return o << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpHeader const& h)
{
  o << "Printing raw WIB TP header:\n";
  o << "flags:" << unsigned(h.m_flags) << " slot:" << unsigned(h.m_slot_no) << " wire:" << unsigned(h.m_wire_no)
    << " fiber:" << unsigned(h.m_fiber_no) << " crate:" << unsigned(h.m_crate_no) << " timestamp:" << h.get_timestamp();
  return o << '\n';
}

//========================
// TP data struct
//========================
struct TpData
{
  // This struct contains three words of TP values that form the main repeating
  // pattern in the TP block.
  tp_word_t m_end_time : 16, m_start_time : 16;
  tp_word_t m_peak_time : 16, m_peak_adc : 16;
  tp_word_t m_hit_continue : 1, m_tp_flags : 15, m_sum_adc : 16;

  std::ostream& print(std::ostream& o) const
  {
    o << "Printing raw WIB TP:\n";
    o << "start_time:" << unsigned(m_start_time) << " end_time:" << unsigned(m_end_time)
      << " peak_adc:" << unsigned(m_peak_adc) << " peak_time:" << unsigned(m_peak_time)
      << " sum_adc:" << unsigned(m_sum_adc) << " flags:" << unsigned(m_tp_flags)
      << " hit_continue:" << unsigned(m_hit_continue);
    return o << '\n';
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    o << "Printing raw WIB TP:\n";
    o << std::hex << "start_time:" << m_start_time << " end_time:" << m_end_time << " peak_adc:" << m_peak_adc
      << " peak_time:" << m_peak_time << " sum_adc:" << m_sum_adc << " flags:" << m_tp_flags
      << " hit_continue:" << m_hit_continue;
    return o << std::dec << '\n';
  }

  std::ostream& print_bits(std::ostream& o) const
  {
    o << "Printing raw WIB TP:\n";
    o << "start_time:" << std::bitset<16>(m_start_time) << " end_time:" << std::bitset<16>(m_end_time)
      << " peak_adc:" << std::bitset<16>(m_peak_adc) << " peak_time:" << std::bitset<16>(m_peak_time)
      << " sum_adc:" << std::bitset<16>(m_sum_adc) << " flags:" << std::bitset<15>(m_tp_flags)
      << " hit_continue:" << std::bitset<1>(m_hit_continue);
    return o << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpData const& tp)
{
  o << "Printing raw WIB TP:\n";
  o << "start_time:" << unsigned(tp.m_start_time) << " end_time:" << unsigned(tp.m_end_time)
    << " peak_adc:" << unsigned(tp.m_peak_adc) << " peak_time:" << unsigned(tp.m_peak_time)
    << " sum_adc:" << unsigned(tp.m_sum_adc) << " flags:" << unsigned(tp.m_tp_flags)
    << " hit_continue:" << unsigned(tp.m_hit_continue);
  return o << '\n';
}

//========================
// TP pedestal information struct
//========================
struct TpPedinfo
{
  // This struct contains three words: one carrying median and accumulator and two padding words
  tp_word_t m_accumulator : 16, m_median : 16;
  tp_word_t m_padding_2 : 16, m_padding_1 : 16;
  tp_word_t m_padding_4 : 16, m_padding_3 : 16;

  std::ostream& print(std::ostream& o) const
  {
    o << "Printing raw WIB TP pedinfo:\n";
    o << "median:" << unsigned(m_median) << " accumulator:" << unsigned(m_accumulator)
      << " padding_1:" << unsigned(m_padding_1) << " padding_2:" << unsigned(m_padding_2)
      << " padding_3:" << unsigned(m_padding_3) << " padding_4:" << unsigned(m_padding_4);
    return o << '\n';
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    o << "Printing raw WIB TP pedinfo:\n";
    o << std::hex << "median:" << m_median << " accumulator:" << m_accumulator << " padding_1:" << m_padding_1
      << " padding_2:" << m_padding_2 << " padding_3:" << m_padding_3 << " padding_4:" << m_padding_4;
    return o << std::dec << '\n';
  }

  std::ostream& print_bits(std::ostream& o) const
  {
    o << "Printing raw WIB TP pedinfo:\n";
    o << "median:" << std::bitset<16>(m_accumulator) << " accumulator:" << std::bitset<16>(m_median)
      << " padding_1:" << std::bitset<16>(m_padding_1) << " padding_2:" << std::bitset<16>(m_padding_2)
      << " padding_3:" << std::bitset<16>(m_padding_3) << " padding_4:" << std::bitset<16>(m_padding_4);
    return o << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpPedinfo const& p)
{
  o << "Printing raw WIB TP pedinfo:\n";
  o << "median:" << unsigned(p.m_median) << " accumulator:" << unsigned(p.m_accumulator)
    << " padding_1:" << unsigned(p.m_padding_1) << " padding_2:" << unsigned(p.m_padding_2)
    << " padding_3:" << unsigned(p.m_padding_3) << " padding_4:" << unsigned(p.m_padding_4);
  return o << '\n';
}

//========================
// TpData block
//========================
struct TpDataBlock
{
  TpData tp;

  std::vector<TpData> block;

  void set_tp(const TpData& data) { block.push_back(data); }

  const TpData* get_tp(const int& tp_num) const { return &block[tp_num]; }

  size_t get_data_size() const { return block.size() * sizeof(TpData); }

  int get_num_tp_per_block() const { return block.size(); }

  std::ostream& print(std::ostream& o) const
  {
    int i = 0;
    o << "Printing raw WIB TP data block:\n";
    for (auto b : block) {
      o << i << ": ";
      b.print(o);
      ++i;
    }
    return o;
  }

  std::ostream& print_bits(std::ostream& o) const
  {
    int i = 0;
    o << "Printing raw WIB TP data block:\n";
    for (auto b : block) {
      o << i << ": ";
      b.print_bits(o);
      ++i;
    }
    return o;
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    int i = 0;
    o << "Printing raw WIB TP data block:\n";
    for (auto b : block) {
      o << i << ": ";
      b.print_hex(o);
      ++i;
    }
    return o;
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpDataBlock const& db)
{
  int i = 0;
  o << "Printing raw WIB TP data block:\n";
  for (auto b : db.block) {
    o << i << ": ";
    o << b;
    ++i;
  }
  return o;
}

//=============
// Raw WIB Trigger Primitive frame
//=============
class RawWIBTp
{
public:
  // Constant expressions
  static constexpr size_t m_num_frame_hdr_words = 3;
  static constexpr size_t m_num_tp_words = 3;
  static constexpr size_t m_num_pedinfo_words = 3;

  // TP header accessors
  uint8_t get_crate_no() const { return m_head.m_crate_no; }        // NOLINT(build/unsigned)
  uint8_t get_fiber_no() const { return m_head.m_fiber_no; }        // NOLINT(build/unsigned)
  uint8_t get_wire_no() const { return m_head.m_wire_no; }          // NOLINT(build/unsigned)
  uint8_t get_slot_no() const { return m_head.m_slot_no; }          // NOLINT(build/unsigned)
  uint16_t get_flags() const { return m_head.m_flags; }             // NOLINT(build/unsigned)
  uint64_t get_timestamp() const { return m_head.get_timestamp(); } // NOLINT(build/unsigned)
  // TP header mutators
  void set_crate_no(const uint8_t new_crate_no) { m_head.m_crate_no = new_crate_no; } // NOLINT(build/unsigned)
  void set_fiber_no(const uint8_t new_fiber_no) { m_head.m_fiber_no = new_fiber_no; } // NOLINT(build/unsigned)
  void set_flags(uint64_t new_flags) { m_head.m_flags = new_flags; }                  // NOLINT(build/unsigned)
  void set_slot_no(const uint8_t new_slot_no) { m_head.m_slot_no = new_slot_no; }     // NOLINT(build/unsigned)
  void set_wire_no(const uint8_t new_wire_no) { m_head.m_wire_no = new_wire_no; }     // NOLINT(build/unsigned)
  void set_timestamp(uint64_t new_timestamp) { m_head.set_timestamp(new_timestamp); } // NOLINT(build/unsigned)
  // TP data accessors
  int get_num_tp_per_block() const { return m_data.get_num_tp_per_block(); }
  uint16_t get_start_time(const TpData& tp) const { return tp.m_start_time; }    // NOLINT(build/unsigned)
  uint16_t get_end_time(const TpData& tp) const { return tp.m_end_time; }        // NOLINT(build/unsigned)
  uint16_t get_peak_adc(const TpData& tp) const { return tp.m_peak_adc; }        // NOLINT(build/unsigned)
  uint16_t get_peak_time(const TpData& tp) const { return tp.m_peak_time; }      // NOLINT(build/unsigned)
  uint16_t get_sum_adc(const TpData& tp) const { return tp.m_sum_adc; }          // NOLINT(build/unsigned)
  uint16_t get_tp_flags(const TpData& tp) const { return tp.m_tp_flags; }        // NOLINT(build/unsigned)
  uint8_t get_hit_continue(const TpData& tp) const { return tp.m_hit_continue; } // NOLINT(build/unsigned)
  // TP data mutators
  void set_start_time(TpData& tp, const uint16_t new_start_time)
  {
    tp.m_start_time = new_start_time;
  }                                                                                            // NOLINT(build/unsigned)
  void set_end_time(TpData& tp, const uint16_t new_end_time) { tp.m_end_time = new_end_time; } // NOLINT(build/unsigned)
  void set_peak_adc(TpData& tp, const uint16_t new_peak_adc) { tp.m_peak_adc = new_peak_adc; } // NOLINT(build/unsigned)
  void set_peak_time(TpData& tp, const uint16_t new_peak_time)
  {
    tp.m_peak_time = new_peak_time;
  }                                                                                            // NOLINT(build/unsigned)
  void set_sum_adc(TpData& tp, const uint16_t new_sum_adc) { tp.m_sum_adc = new_sum_adc; }     // NOLINT(build/unsigned)
  void set_tp_flags(TpData& tp, const uint16_t new_tp_flags) { tp.m_tp_flags = new_tp_flags; } // NOLINT(build/unsigned)
  void set_hit_continue(TpData& tp, const uint8_t new_hit_continue)
  {
    tp.m_hit_continue = new_hit_continue;
  } // NOLINT(build/unsigned)
  // Pedinfo accessors
  uint16_t get_accumulator() const { return m_pedinfo.m_accumulator; } // NOLINT(build/unsigned)
  uint16_t get_median() const { return m_pedinfo.m_median; }           // NOLINT(build/unsigned)
  uint16_t get_padding_1() const { return m_pedinfo.m_padding_1; }     // NOLINT(build/unsigned)
  uint16_t get_padding_2() const { return m_pedinfo.m_padding_2; }     // NOLINT(build/unsigned)
  uint16_t get_padding_3() const { return m_pedinfo.m_padding_3; }     // NOLINT(build/unsigned)
  uint16_t get_padding_4() const { return m_pedinfo.m_padding_4; }     // NOLINT(build/unsigned)
  // Pedinfo mutators
  void set_accumulator(const uint16_t new_accumulator)
  {
    m_pedinfo.m_accumulator = new_accumulator;
  }                                                                                           // NOLINT(build/unsigned)
  void set_median(const uint16_t new_median) { m_pedinfo.m_median = new_median; }             // NOLINT(build/unsigned)
  void set_padding_1(const uint16_t new_padding_1) { m_pedinfo.m_padding_1 = new_padding_1; } // NOLINT(build/unsigned)
  void set_padding_2(const uint16_t new_padding_2) { m_pedinfo.m_padding_2 = new_padding_2; } // NOLINT(build/unsigned)
  void set_padding_3(const uint16_t new_padding_3) { m_pedinfo.m_padding_3 = new_padding_3; } // NOLINT(build/unsigned)
  void set_padding_4(const uint16_t new_padding_4) { m_pedinfo.m_padding_4 = new_padding_4; } // NOLINT(build/unsigned)

  // Const struct accessors
  const TpHeader* get_header() const { return &m_head; }
  const TpData* get_tp(const int& tp_num) const { return m_data.get_tp(tp_num); }
  const TpDataBlock* get_data() const { return &m_data; }
  const TpPedinfo* get_pedinfo() const { return &m_pedinfo; }
  size_t get_data_size() const { return m_data.get_data_size(); }
  // Const struct mutators
  void set_header(const TpHeader& hdr) { m_head = hdr; }
  void set_tp(const TpData& tpdata) { m_data.set_tp(tpdata); }
  void set_data(const TpDataBlock& block) { m_data = block; }
  void set_pedinfo(const TpPedinfo& ped) { m_pedinfo = ped; }

  friend std::ostream& operator<<(std::ostream& o, RawWIBTp const& rwtp);

private:
  TpHeader m_head;
  TpDataBlock m_data;
  TpPedinfo m_pedinfo;
};

inline std::ostream&
operator<<(std::ostream& o, RawWIBTp const& rwtp)
{
  o << "Printing raw WIB TP frame:" << '\n';
  o << rwtp.m_head << '\n';
  o << rwtp.m_data << '\n';
  o << rwtp.m_pedinfo << '\n';
  return o;
}

} // namespace dataformats
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_RAWWIBTP_HPP_
