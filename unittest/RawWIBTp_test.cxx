/**
 * @file RawWIBTp_test.cxx RawWIBTp class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

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

BOOST_AUTO_TEST_CASE(RawWIBTp_TimestampMethods)
{
  TpHeader header;
  BOOST_REQUIRE_EQUAL(header.get_timestamp(), 0x7FFF222211111111);
}

BOOST_AUTO_TEST_SUITE_END()
