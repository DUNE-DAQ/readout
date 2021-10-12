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

using namespace dunedaq::dataformats;

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

  header.m_median = 0x1;
  header.m_accumulator = 0x2;
  header.m_nhits = 0x3;
  header.m_padding_1 = 0x4;
  header.m_padding_2 = 0x5;
  header.m_padding_3 = 0x6;

  BOOST_REQUIRE_EQUAL(header.m_median, 0x1);
  BOOST_REQUIRE_EQUAL(header.m_accumulator, 0x2);
  BOOST_REQUIRE_EQUAL(header.m_nhits, 0x3);
  BOOST_REQUIRE_EQUAL(header.m_padding_1, 0x4);
  BOOST_REQUIRE_EQUAL(header.m_padding_2, 0x5);
  BOOST_REQUIRE_EQUAL(header.m_padding_3, 0x6);
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

BOOST_AUTO_TEST_CASE(RawWIBTp_StreamMethods)
{
  RawWIBTp rwtp;
  //TpData data[2];
  rwtp.set_nhits(2);

  std::ostringstream ostr;
  ostr << rwtp;
  std::string output = ostr.str();
  TLOG() << "Stream operator: " << output << std::endl;
  BOOST_REQUIRE(!output.empty());
}

BOOST_AUTO_TEST_SUITE_END()
