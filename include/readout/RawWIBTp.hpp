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
  tp_word_t m_flags: 13, m_slot_no: 3, m_wire_no : 8, m_fiber_no : 3, m_crate_no : 5;
  tp_word_t m_timestamp_1;
  tp_word_t m_timestamp_2;

  uint64_t get_timestamp() const  // NOLINT(build/unsigned)
  {
    uint64_t timestamp = static_cast<uint64_t>(m_timestamp_1) | (static_cast<int64_t>(m_timestamp_2) << 32);  // NOLINT(build/unsigned)
    return timestamp;
  }

  void set_timestamp(const uint64_t new_timestamp)  // NOLINT(build/unsigned)
  {
    m_timestamp_1 = new_timestamp;
    m_timestamp_2 = new_timestamp >> 32;
  }

  // Print functions for debugging.
  std::ostream& print(std::ostream& o) const
  {
    o << "flags:" << unsigned(m_flags) << " slot:" << unsigned(m_slot_no) << " wire:" << unsigned(m_wire_no) 
      << " fiber:" << unsigned(m_fiber_no) << " crate:" << unsigned(m_crate_no)
      << " timestamp:" << get_timestamp();
    return o  << '\n';
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    o << "Printing raw WIB TP header:\n";
    o << std::hex << "flags:" << m_flags << " slot:" << m_slot_no << " wire:" << m_wire_no
      << " fiber:" << m_fiber_no << " crate:" << m_crate_no << " timestamp:" << get_timestamp(); 
    return o << std::dec << '\n';
  }

  std::ostream& print_bits(std::ostream& o) const
  {
    return o << "flags:" << std::bitset<13>(m_flags)
             << " slot:" << std::bitset<3>(m_slot_no) << " wire:" << std::bitset<8>(m_wire_no)
             << " fiber:" << std::bitset<3>(m_fiber_no) << " crate:" << std::bitset<5>(m_crate_no) 
             << " timestamp: " << get_timestamp() << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpHeader const& h)
{
  return o << "flags:" << std::bitset<13>(h.m_flags)
           << " slot:" << std::bitset<3>(h.m_slot_no) << " wire:" << std::bitset<8>(h.m_wire_no)
           << " fiber:" << std::bitset<3>(h.m_fiber_no) << " crate:" << std::bitset<5>(h.m_crate_no) 
           << " timestamp: " << h.get_timestamp() << '\n';
}


//========================
// TP data struct
//========================
struct TpData
{
  // This struct contains three words of TP values that form the main repeating
  // pattern in the TP block.
  tp_word_t m_end_time: 16, m_start_time: 16;
  tp_word_t m_peak_time: 16, m_peak_adc: 16;
  tp_word_t m_hit_continue: 1, m_flags: 15, m_sum_adc: 16;

  std::ostream& print(std::ostream& o) const
  {
    return o << "end_time:" << unsigned(m_end_time) << " start_time:" << unsigned(m_start_time)
           << " peak_time:" << unsigned(m_peak_time) << " peak_adc:" << unsigned(m_peak_adc)
           << " hit_continue:" << unsigned(m_hit_continue) << " flags:" << unsigned(m_flags) 
           << " sum_adc:" << unsigned(m_sum_adc) 
           << '\n';
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    return o << std::hex << "end_time:" << m_end_time << " start_time:" << m_start_time 
              << " peak_time:" << m_peak_time << " peak_adc:" << m_peak_adc 
              << " hit_continue:" << m_hit_continue << " flags:" << m_flags 
              << " sum_adc:" << m_sum_adc << std::dec << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpData const& tp)
{
  return o << "end_time:" << unsigned(tp.m_end_time) << " start_time:" << unsigned(tp.m_start_time)
           << " peak_time:" << unsigned(tp.m_peak_time) << " peak_adc:" << unsigned(tp.m_peak_adc)
           << " hit_continue:" << unsigned(tp.m_hit_continue) << " flags:" << unsigned(tp.m_flags)
           << " sum_adc:" << unsigned(tp.m_sum_adc)
           << '\n';
}


//========================
// TP pedestal information struct
//========================
struct TpPedinfo
{
  // This struct contains three words: one carrying median and accumulator and two padding words
  tp_word_t m_accumulator: 16, m_median: 16;
  tp_word_t m_padding_2: 16, m_padding_1: 16;
  tp_word_t m_padding_4: 16, m_padding_3: 16;

  std::ostream& print(std::ostream& o) const 
  {
    return o << "median:" << unsigned(m_median) << " accumulator:" << unsigned(m_accumulator)
             << " padding_1:" << m_padding_1 << " padding_2:" << m_padding_2
             << " padding_3: "<< m_padding_3 << " padding_4:" << m_padding_4
             << '\n';
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    o << "Printing raw WIB TP pedinfo:\n";
    o << std::hex << "median:" << m_median << " accumulator:" << m_accumulator 
      << " padding_1:" << m_padding_1 << " padding_2:" << m_padding_2 
      << " padding_3: "<< m_padding_3 << " padding_4:" << m_padding_4;
    return o << std::dec << '\n';
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpPedinfo const& p)
{
  return o << "median:" << unsigned(p.m_median) << " accumulator:" << unsigned(p.m_accumulator)
           << " padding_1:" << p.m_padding_1 << " padding_2:" << p.m_padding_2
           << " padding_3: "<< p.m_padding_3 << " padding_4:" << p.m_padding_4
           << '\n';
}


//========================
// TpData block
//========================
struct TpDataBlock
{
  TpData tp;

  std::vector<TpData> block;

  void set_tp(const TpData& data) 
  {
    block.push_back(data);
  }

  const TpData* get_tp(const int& tp_num) const {
    return &block[tp_num];
  }

  size_t get_data_size() const {
    return block.size()*sizeof(TpData);
  }

  int num_tp_per_block() const
  {
    return block.size();
  }

  std::ostream& print(std::ostream& o) const
  {
    for (auto b : block) {
      o << b;
    }
    return o;
  }

  std::ostream& print_hex(std::ostream& o) const
  {
    int i=0;
    o << "Printing raw WIB TP data block:\n";
    for (auto b: block) {
      o << "Printing raw WIB TP " << i << ":\n";
      o << b;
      ++i;
    }
    return o;
  }
};

inline std::ostream&
operator<<(std::ostream& o, TpDataBlock const& db)
{
   int i=0;
    o << "Printing raw WIB TP data block:\n";
    for (auto b: db.block) {
      o << "Printing raw WIB TP " << i << ":\n";
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
  uint8_t slot_no() const { return m_head.m_slot_no; }           // NOLINT(build/unsigned)
  uint8_t wire_no() const { return m_head.m_wire_no; }           // NOLINT(build/unsigned)
  uint8_t fiber_no() const { return m_head.m_fiber_no; }         // NOLINT(build/unsigned)
  uint8_t crate_no() const { return m_head.m_crate_no; }         // NOLINT(build/unsigned)
  uint64_t timestamp() const { return m_head.get_timestamp(); }  // NOLINT(build/unsigned)
  // TP header mutators
  void set_slot_no(const uint8_t new_slot_no) { m_head.m_slot_no = new_slot_no; }      // NOLINT(build/unsigned)
  void set_wire_no(const uint8_t new_wire_no) { m_head.m_wire_no = new_wire_no; }      // NOLINT(build/unsigned)
  void set_fiber_no(const uint8_t new_fiber_no) { m_head.m_fiber_no = new_fiber_no; }  // NOLINT(build/unsigned)
  void set_crate_no(const uint8_t new_crate_no) { m_head.m_crate_no = new_crate_no; }  // NOLINT(build/unsigned)
  void set_timestamp(uint64_t new_timestamp) { m_head.set_timestamp(new_timestamp); }  // NOLINT(build/unsigned)

  // TP data accessors
  int num_tp_per_block() const { return m_data.num_tp_per_block(); }
  // TP data mutators

  // Const struct accessors
  const TpHeader*     get_header() const { return &m_head; }
  const TpData*       get_tp(const int& tp_num) const { return m_data.get_tp(tp_num); }
  const TpDataBlock*  get_data() const { return &m_data; }
  const TpPedinfo*    get_pedinfo() const { return &m_pedinfo; }
  size_t get_data_size() const { return m_data.get_data_size(); }
  // Const struct mutators
  void set_header(const TpHeader& hdr) { m_head = hdr; }
  void set_tp(const TpData& tpdata) { m_data.set_tp(tpdata); }
  void set_data(const TpDataBlock& block) {m_data = block; }
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
  o << "Printing raw WIB TP:" << '\n';
  o << rwtp.m_head << '\n';
  o << rwtp.m_data << '\n';
  o << rwtp.m_pedinfo << '\n';
  return o;
}


} // namespace dataformats
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_RAWWIBTP_HPP_
