/**
 * @file RawWibTp.hpp Raw Trigger Primitive bit fields and accessors
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef DATAFORMATS_INCLUDE_DATAFORMATS_WIB_RAWWIBTP_HPP_
#define DATAFORMATS_INCLUDE_DATAFORMATS_WIB_RAWWIBTP_HPP_

#include <bitset>
#include <iostream>
#include <vector>

namespace dunedaq {
namespace dataformats {

typedef uint32_t word_t;

//===================
// TP header struct
//===================
struct TpHeader
{
  word_t flags: 13, slot_no: 3, wire_no : 8, fiber_no : 3, crate_no : 5;
  word_t timestamp_1;
  word_t timestamp_2;

  uint64_t timestamp() const
  {
    uint64_t timestamp = timestamp_1 | (timestamp_2 << 32);
    return timestamp;
  }

  void set_timestamp(const uint64_t new_timestamp)
  {
    timestamp_1 = new_timestamp;
    timestamp_2 = new_timestamp >> 32;
  }

  // Print functions for debugging.
  void print() const
  {
    std::cout << "flags:" << unsigned(flags) << " slot:" << unsigned(slot_no) << " wire:" << unsigned(wire_no) 
              << " fiber:" << unsigned(fiber_no) << " crate:" << unsigned(crate_no)
              << " timestamp:" << timestamp()
              << '\n';
  }

  void printHex() const
  {
    std::cout << "Printing raw WIB TP header:\n";
    std::cout << std::hex << "flags:" << flags << " slot:" << slot_no << " wire:" << wire_no
              << " fiber:" << fiber_no << " crate:" << crate_no << " timestamp:" << timestamp() << std::dec << '\n';
  }

  void printBits() const
  {
    std::cout << "flags:" << std::bitset<13>(flags)
              << " slot:" << std::bitset<3>(slot_no) << " wire:" << std::bitset<8>(wire_no)
              << " fiber:" << std::bitset<3>(fiber_no) << " crate:" << std::bitset<5>(crate_no) 
              << " timestamp: " << timestamp() << '\n';
  }
};

//========================
// TP data struct
//========================
struct TpData
{
  // This struct contains three words of TP values that form the main repeating
  // pattern in the TP block.
  word_t end_time: 16, start_time: 16;
  word_t peak_time: 16, peak_adc: 16;
  word_t hit_continue: 1, flags: 15, sum_adc: 16;

  void print() const
  {
    std::cout << "end_time:" << unsigned(end_time) << " start_time:" << unsigned(start_time)
              << " peak_time:" << unsigned(peak_time) << " peak_adc:" << unsigned(peak_adc)
              << " hit_continue:" << unsigned(hit_continue) << " flags:" << unsigned(flags) 
              << " sum_adc:" << unsigned(sum_adc) 
              << '\n';
  }

  void printHex(const int& i) const
  {
    std::cout << "Printing raw WIB TP " << i << ":\n";
    std::cout << std::hex << "end_time:" << end_time << " start_time:" << start_time 
              << " peak_time:" << peak_time << " peak_adc:" << peak_adc 
              << " hit_continue:" << hit_continue << " flags:" << flags 
              << " sum_adc:" << sum_adc << std::dec << '\n';
  }
};

//========================
// TP pedestal information struct
//========================
struct TpPedinfo
{
  // This struct contains three words: one carrying median and accumulator and two padding words
  word_t accumulator: 16, median: 16;
  word_t padding_2: 16, padding_1: 16;
  word_t padding_4: 16, padding_3: 16;

  void print() const 
  {
    std::cout << "median:" << unsigned(median) << " accumulator:" << unsigned(accumulator)
              << '\n';
  }

  void printHex() const
  {
    std::cout << "Printing raw WIB TP pedinfo:\n";
    std::cout << std::hex << "median:" << median << " accumulator:" << accumulator 
              << " padding_1:" << padding_1 << " padding_2:" << padding_2 
              << " padding_3: "<< padding_3 << " padding_4:" << padding_4 << std::dec << '\n';
  }
};

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

  void print() const 
  {
    for (auto b : block) {
      b.print();
    }
  }

  void printHex() const
  {
    int i=0;
    std::cout << "Printing raw WIB TP data block:\n";
    for (auto b: block) {
      b.printHex(i+1);
      ++i;
    }
  }
};

//=============
// Raw WIB Trigger Primitive frame
//=============
class RawWibTp
{
public:
  // Constant expressions
  static constexpr size_t m_num_frame_hdr_words = 3;
  static constexpr size_t m_num_tp_words = 3;
  static constexpr size_t m_num_pedinfo_words = 3;

private:
  TpHeader head;
  TpData tp;
  TpDataBlock data;
  TpPedinfo pedinfo;

public:
  // TP header accessors
  uint8_t slot_no() const { return head.slot_no; }
  uint8_t wire_no() const { return head.wire_no; }
  uint8_t fiber_no() const { return head.fiber_no; }
  uint8_t crate_no() const { return head.crate_no; }
  uint64_t timestamp() const { return head.timestamp(); }
  // TP header mutators
  void set_slot_no(const uint8_t new_slot_no) { head.slot_no = new_slot_no; }
  void set_wire_no(const uint8_t new_wire_no) { head.wire_no = new_wire_no; }
  void set_fiber_no(const uint8_t new_fiber_no) { head.fiber_no = new_fiber_no; }
  void set_crate_no(const uint8_t new_crate_no) { head.crate_no = new_crate_no; }
  void set_timestamp(uint64_t new_timestamp) { head.set_timestamp(new_timestamp); }

  // TP data accessors
  int num_tp_per_block() const { return data.num_tp_per_block(); }
  // TP data mutators

  // Const struct accessors
  const TpHeader*     get_header() const { return &head; }
  const TpData*       get_tp(const int& tp_num) const { return data.get_tp(tp_num); }
  const TpDataBlock*  get_data() const { return &data; }
  const TpPedinfo*    get_pedinfo() const { return &pedinfo; }
  size_t get_data_size() const { return data.get_data_size(); }
  // Const struct mutators
  void set_header(const TpHeader& hdr) { head = hdr; }
  void set_tp(const TpData& tpdata) { data.set_tp(tpdata); }
  void set_data(const TpDataBlock& block) {data = block; }
  void set_pedinfo(const TpPedinfo& ped) { pedinfo = ped; }

  // Utility functions
  void print() const
  {
    std::cout << "Printing raw WIB TP frame:\n";
    head.print();
    data.print();
    pedinfo.print();  
  }

  void printHex() const 
  {
    std::cout << "Printing raw WIB TP frame:\n";
    head.printHex();
    data.printHex();
    pedinfo.printHex();
  }
};


} // namespace dataformats
} // namespace dunedaq

#endif // DATAFORMATS_INCLUDE_DATAFORMATS_WIB_WIBFRAME_HPP_
