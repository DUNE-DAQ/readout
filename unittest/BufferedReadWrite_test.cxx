/**
 * @file RawWIBTp_test.cxx RawWIBTp class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */


/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE BufferedReadWrite_test // NOLINT

#include "boost/test/unit_test.hpp"

#include "readout/ReadoutTypes.hpp"
#include "BufferedWriter.hpp"
#include "BufferedReader.hpp"

#include <string>
#include <stdio.h>

using namespace dunedaq::readout;

BOOST_AUTO_TEST_SUITE(BufferedReadWrite_test)

BOOST_AUTO_TEST_CASE(BufferedReadWrite)
{
    BufferedWriter<int> m_buffered_writer;
    BufferedReader<int> m_buffered_reader;
    int number = 42;

    m_buffered_writer.open("test.out", 2048);
    bool write_successful = m_buffered_writer.write(number);
    BOOST_REQUIRE(write_successful);
    m_buffered_writer.close();

    m_buffered_reader.open("test.out", 2048);
    int read_value;
    bool read_successful = m_buffered_reader.read(read_value);
    std::cout << read_value << std::endl;
    BOOST_REQUIRE(read_successful);
    m_buffered_reader.close();

    BOOST_REQUIRE_EQUAL(read_value, number);
    remove("test.out");
}



BOOST_AUTO_TEST_SUITE_END()

