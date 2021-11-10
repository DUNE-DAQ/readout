/**
 * @file RawWIBTp_test.cxx RawWIBTp class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "logging/Logging.hpp"
#include "readout/RawWIBTp.hpp"

/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE RawWIBTp_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::detdataformats;

BOOST_AUTO_TEST_SUITE(RawWIBTp_test)

BOOST_AUTO_TEST_CASE(TpHeader_TimestampMethods)
{
  TpHeader header;

  header.m_timestamp_1 = 0x11111111;
  header.m_timestamp_2 = 0x2222;
  BOOST_REQUIRE_EQUAL(header.get_timestamp(), 0x222211111111);
}
BOOST_AUTO_TEST_CASE(TpHeader_StreamMethods)
{
  TpHeader header;

  std::ostringstream ostr;
  header.print_hex(ostr);
  std::string output = ostr.str();
  TLOG() << "Print: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  header.print_bits(ostr);
  output = ostr.str();
  TLOG() << "Print hex: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  header.print_bits(ostr);
  output = ostr.str();
  TLOG() << "Print bits: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  ostr << header;
  output = ostr.str();
  TLOG() << "Stream operator: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
}
BOOST_AUTO_TEST_CASE(TpHeader_BitfieldMethods)
{
  TpHeader header;

  header.m_crate_no = 0x1;
  header.m_fiber_no = 0x2;
  header.m_wire_no = 0x3;
  header.m_slot_no = 0x4;
  header.m_flags = 0x5;
  BOOST_REQUIRE_EQUAL(header.m_crate_no, 0x1);
  BOOST_REQUIRE_EQUAL(header.m_fiber_no, 0x2);
  BOOST_REQUIRE_EQUAL(header.m_wire_no, 0x3);
  BOOST_REQUIRE_EQUAL(header.m_slot_no, 0x4);
  BOOST_REQUIRE_EQUAL(header.m_flags, 0x5);
}

BOOST_AUTO_TEST_CASE(TpData_StreamMethods)
{
  TpData data;

  std::ostringstream ostr;
  data.print(ostr);
  std::string output = ostr.str();
  TLOG() << "Print: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  data.print_hex(ostr);
  output = ostr.str();
  TLOG() << "Print hex: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  data.print_bits(ostr);
  output = ostr.str();
  TLOG() << "Print bits: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  ostr << data;
  output = ostr.str();
  TLOG() << "Stream operator: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
}
BOOST_AUTO_TEST_CASE(TpData_BitfieldMethods)
{
  TpData tp;

  tp.m_start_time = 0x1;
  tp.m_end_time = 0x2;
  tp.m_peak_adc = 0x3;
  tp.m_peak_time = 0x4;
  tp.m_sum_adc = 0x5;
  tp.m_tp_flags = 0x6;
  tp.m_hit_continue = 0x0;

  BOOST_REQUIRE_EQUAL(tp.m_start_time, 0x1);
  BOOST_REQUIRE_EQUAL(tp.m_end_time, 0x2);
  BOOST_REQUIRE_EQUAL(tp.m_peak_adc, 0x3);
  BOOST_REQUIRE_EQUAL(tp.m_peak_time, 0x4);
  BOOST_REQUIRE_EQUAL(tp.m_sum_adc, 0x5);
  BOOST_REQUIRE_EQUAL(tp.m_tp_flags, 0x6);
  BOOST_REQUIRE_EQUAL(tp.m_hit_continue, 0x0);
}

BOOST_AUTO_TEST_CASE(TpPedinfo_StreamMethods)
{
  TpPedinfo pedinfo = {};

  std::ostringstream ostr;
  pedinfo.print(ostr);
  std::string output = ostr.str();
  TLOG() << "Print: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  pedinfo.print_hex(ostr);
  output = ostr.str();
  TLOG() << "Print hex: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  pedinfo.print_bits(ostr);
  output = ostr.str();
  TLOG() << "Print bits: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  ostr << pedinfo;
  output = ostr.str();
  TLOG() << "Stream operator: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
}
BOOST_AUTO_TEST_CASE(TpPedinfo_BitfieldMethods)
{
  TpPedinfo ped;

  ped.m_median = 0x1;
  ped.m_accumulator = 0x2;
  ped.m_padding_1 = 0x3;
  ped.m_padding_2 = 0x4;
  ped.m_padding_3 = 0x5;
  ped.m_padding_4 = 0x6;

  BOOST_REQUIRE_EQUAL(ped.m_median, 0x1);
  BOOST_REQUIRE_EQUAL(ped.m_accumulator, 0x2);
  BOOST_REQUIRE_EQUAL(ped.m_padding_1, 0x3);
  BOOST_REQUIRE_EQUAL(ped.m_padding_2, 0x4);
  BOOST_REQUIRE_EQUAL(ped.m_padding_3, 0x5);
  BOOST_REQUIRE_EQUAL(ped.m_padding_4, 0x6);
}

BOOST_AUTO_TEST_CASE(TpDataBlock_StreamMethods)
{
  TpDataBlock block;

  TpData data_1;
  TpData data_2;
  block.set_tp(data_1);
  block.set_tp(data_2);

  std::ostringstream ostr;
  block.print(ostr);
  std::string output = ostr.str();
  TLOG() << "Print: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  block.print_hex(ostr);
  output = ostr.str();
  TLOG() << "Print hex: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  block.print_bits(ostr);
  output = ostr.str();
  TLOG() << "Print bits: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
  ostr.str("");
  ostr.clear();

  ostr << block;
  output = ostr.str();
  TLOG() << "Stream operator: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
}
BOOST_AUTO_TEST_CASE(TpDataBlock_BitfieldMethods)
{
  TpDataBlock block;
  TpData tp_1;
  TpData tp_2;
  tp_1.m_start_time = 0x1;
  tp_1.m_end_time = 0x2;
  tp_1.m_peak_adc = 0x3;
  tp_1.m_peak_time = 0x4;
  tp_1.m_sum_adc = 0x5;
  tp_1.m_tp_flags = 0x6;
  tp_1.m_hit_continue = 0x0;
  tp_2.m_start_time = 0x7;
  tp_2.m_end_time = 0x8;
  tp_2.m_peak_adc = 0x9;
  tp_2.m_peak_time = 0xa;
  tp_2.m_sum_adc = 0xb;
  tp_2.m_tp_flags = 0xc;
  tp_2.m_hit_continue = 0x1;
  block.set_tp(tp_1);
  block.set_tp(tp_2);
  const TpData* tp_1_ptr = block.get_tp(0);
  const TpData* tp_2_ptr = block.get_tp(1);
  BOOST_REQUIRE_EQUAL(block.get_num_tp_per_block(), 2);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_start_time, 0x1);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_end_time, 0x2);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_peak_adc, 0x3);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_peak_time, 0x4);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_sum_adc, 0x5);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_tp_flags, 0x6);
  BOOST_REQUIRE_EQUAL(tp_1_ptr->m_hit_continue, 0x0);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_start_time, 0x7);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_end_time, 0x8);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_peak_adc, 0x9);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_peak_time, 0xa);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_sum_adc, 0xb);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_tp_flags, 0xc);
  BOOST_REQUIRE_EQUAL(tp_2_ptr->m_hit_continue, 0x1);
}

BOOST_AUTO_TEST_CASE(RawWIBTp_StreamMethods)
{
  RawWIBTp rwtp;
  TpDataBlock block;
  TpData data_1;
  TpData data_2;
  rwtp.set_tp(data_1);
  rwtp.set_tp(data_2);

  std::ostringstream ostr;
  ostr << rwtp;
  std::string output = ostr.str();
  TLOG() << "Stream operator: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
}
BOOST_AUTO_TEST_CASE(RawWIBTp_HdrMethods)
{
  RawWIBTp rwtp;

  rwtp.set_crate_no(0x1);
  rwtp.set_fiber_no(0x2);
  rwtp.set_wire_no(0x3);
  rwtp.set_slot_no(0x4);
  rwtp.set_flags(0x5);
  BOOST_REQUIRE_EQUAL(rwtp.get_crate_no(), 0x1);
  BOOST_REQUIRE_EQUAL(rwtp.get_fiber_no(), 0x2);
  BOOST_REQUIRE_EQUAL(rwtp.get_wire_no(), 0x3);
  BOOST_REQUIRE_EQUAL(rwtp.get_slot_no(), 0x4);
  BOOST_REQUIRE_EQUAL(rwtp.get_flags(), 0x5);
}
BOOST_AUTO_TEST_CASE(RawWIBTp_PedinfoMethods)
{
  RawWIBTp rwtp;

  rwtp.set_accumulator(0x1);
  rwtp.set_median(0x2);
  rwtp.set_padding_1(0x3);
  rwtp.set_padding_2(0x4);
  rwtp.set_padding_3(0x5);
  rwtp.set_padding_4(0x6);
  BOOST_REQUIRE_EQUAL(rwtp.get_accumulator(), 0x1);
  BOOST_REQUIRE_EQUAL(rwtp.get_median(), 0x2);
  BOOST_REQUIRE_EQUAL(rwtp.get_padding_1(), 0x3);
  BOOST_REQUIRE_EQUAL(rwtp.get_padding_2(), 0x4);
  BOOST_REQUIRE_EQUAL(rwtp.get_padding_3(), 0x5);
  BOOST_REQUIRE_EQUAL(rwtp.get_padding_4(), 0x6);
}

BOOST_AUTO_TEST_CASE(RawWIBTp_TpMethods)
{
  RawWIBTp rwtp;

  TpData tp_1;
  TpData tp_2;
  tp_1.m_start_time = 0x1;
  tp_1.m_end_time = 0x2;
  tp_1.m_peak_adc = 0x3;
  tp_1.m_peak_time = 0x4;
  tp_1.m_sum_adc = 0x5;
  tp_1.m_tp_flags = 0x6;
  tp_1.m_hit_continue = 0x0;
  tp_2.m_start_time = 0x7;
  tp_2.m_end_time = 0x8;
  tp_2.m_peak_adc = 0x9;
  tp_2.m_peak_time = 0xa;
  tp_2.m_sum_adc = 0xb;
  tp_2.m_tp_flags = 0xc;
  tp_2.m_hit_continue = 0x1;

  rwtp.set_tp(tp_1);
  rwtp.set_tp(tp_2);

  BOOST_REQUIRE_EQUAL(rwtp.get_start_time(tp_1), 0x1);
  BOOST_REQUIRE_EQUAL(rwtp.get_end_time(tp_1), 0x2);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_adc(tp_1), 0x3);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_time(tp_1), 0x4);
  BOOST_REQUIRE_EQUAL(rwtp.get_sum_adc(tp_1), 0x5);
  BOOST_REQUIRE_EQUAL(rwtp.get_tp_flags(tp_1), 0x6);
  BOOST_REQUIRE_EQUAL(rwtp.get_hit_continue(tp_1), 0x0);
  BOOST_REQUIRE_EQUAL(rwtp.get_start_time(tp_2), 0x7);
  BOOST_REQUIRE_EQUAL(rwtp.get_end_time(tp_2), 0x8);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_adc(tp_2), 0x9);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_time(tp_2), 0xa);
  BOOST_REQUIRE_EQUAL(rwtp.get_sum_adc(tp_2), 0xb);
  BOOST_REQUIRE_EQUAL(rwtp.get_tp_flags(tp_2), 0xc);
  BOOST_REQUIRE_EQUAL(rwtp.get_hit_continue(tp_2), 0x1);

  TpData tp_3;
  TpData tp_4;

  rwtp.set_start_time(tp_3, 0x10);
  rwtp.set_end_time(tp_3, 0x20);
  rwtp.set_peak_adc(tp_3, 0x30);
  rwtp.set_peak_time(tp_3, 0x40);
  rwtp.set_sum_adc(tp_3, 0x50);
  rwtp.set_tp_flags(tp_3, 0x60);
  rwtp.set_hit_continue(tp_3, 0x1);
  rwtp.set_start_time(tp_4, 0x70);
  rwtp.set_end_time(tp_4, 0x80);
  rwtp.set_peak_adc(tp_4, 0x90);
  rwtp.set_peak_time(tp_4, 0xa0);
  rwtp.set_sum_adc(tp_4, 0xb0);
  rwtp.set_tp_flags(tp_4, 0xc0);
  rwtp.set_hit_continue(tp_4, 0x0);
  rwtp.set_tp(tp_3);
  rwtp.set_tp(tp_4);

  const TpData* tp_3_ptr = rwtp.get_tp(2);
  const TpData* tp_4_ptr = rwtp.get_tp(3);

  BOOST_REQUIRE_EQUAL(rwtp.get_num_tp_per_block(), 4);
  BOOST_REQUIRE_EQUAL(rwtp.get_start_time(*tp_3_ptr), 0x10);
  BOOST_REQUIRE_EQUAL(rwtp.get_end_time(*tp_3_ptr), 0x20);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_adc(*tp_3_ptr), 0x30);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_time(*tp_3_ptr), 0x40);
  BOOST_REQUIRE_EQUAL(rwtp.get_sum_adc(*tp_3_ptr), 0x50);
  BOOST_REQUIRE_EQUAL(rwtp.get_tp_flags(*tp_3_ptr), 0x60);
  BOOST_REQUIRE_EQUAL(rwtp.get_hit_continue(*tp_3_ptr), 0x1);
  BOOST_REQUIRE_EQUAL(rwtp.get_start_time(*tp_4_ptr), 0x70);
  BOOST_REQUIRE_EQUAL(rwtp.get_end_time(*tp_4_ptr), 0x80);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_adc(*tp_4_ptr), 0x90);
  BOOST_REQUIRE_EQUAL(rwtp.get_peak_time(*tp_4_ptr), 0xa0);
  BOOST_REQUIRE_EQUAL(rwtp.get_sum_adc(*tp_4_ptr), 0xb0);
  BOOST_REQUIRE_EQUAL(rwtp.get_tp_flags(*tp_4_ptr), 0xc0);
  BOOST_REQUIRE_EQUAL(rwtp.get_hit_continue(*tp_4_ptr), 0x0);
}

BOOST_AUTO_TEST_SUITE_END()
