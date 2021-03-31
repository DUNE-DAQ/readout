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

template<class RawType>
void test_read_write(BufferedWriter<RawType>& writer, BufferedReader<RawType>& reader, uint numbers_to_write) {
  std::vector<int> numbers(numbers_to_write/sizeof(int));

  bool write_successful = false;
  for (uint i = 0; i < numbers.size(); ++i) {
    numbers[i] = i;
    write_successful = writer.write(i);
    BOOST_REQUIRE(write_successful);
  }

  writer.close();

  int read_value;
  bool read_successful = false;
  for (uint i = 0; i < numbers.size(); ++i) {
    read_successful = reader.read(read_value);
    if (!read_successful) std::cout << i << std::endl;
    BOOST_REQUIRE(read_successful);
    BOOST_REQUIRE_EQUAL(read_value, numbers[i]);
  }

  read_successful = reader.read(read_value);
  BOOST_REQUIRE(!read_successful);

  reader.close();

  remove("test.out");
}

BOOST_AUTO_TEST_CASE(BufferedReadWrite)
{
    remove("test.out");
    BufferedWriter<int> writer;
    writer.open("test.out", 4096);
    BufferedReader<int> reader;
    reader.open("test.out", 4096);
    uint numbers_to_write = 1;

    test_read_write(writer, reader, numbers_to_write);
}

BOOST_AUTO_TEST_CASE(BufferedReadWrite_extended)
{
    remove("test.out");
    BufferedWriter<int> writer;
    writer.open("test.out", 4096);
    BufferedReader<int> reader;
    reader.open("test.out", 4096);
    uint numbers_to_write = 4096 * 4096;

    test_read_write(writer, reader, numbers_to_write);
}

BOOST_AUTO_TEST_CASE(BufferedReadWrite_zstd)
{
    remove("test.out");
    BufferedWriter<int> writer;
    writer.open("test.out", 4096, "zstd");
    BufferedReader<int> reader;
    reader.open("test.out", 4096, "zstd");
    uint numbers_to_write = 4096 * 4096;

    test_read_write(writer, reader, numbers_to_write);
}

BOOST_AUTO_TEST_CASE(BufferedReadWrite_lzma)
{
  remove("test.out");
  BufferedWriter<int> writer;
  writer.open("test.out", 4096, "lzma");
  BufferedReader<int> reader;
  reader.open("test.out", 4096, "lzma");
  uint numbers_to_write = 4096 * 4096;

  test_read_write(writer, reader, numbers_to_write);
}

BOOST_AUTO_TEST_CASE(BufferedReadWrite_zlib)
{
  remove("test.out");
  BufferedWriter<int> writer;
  writer.open("test.out", 4096, "zlib");
  BufferedReader<int> reader;
  reader.open("test.out", 4096, "zlib");
  uint numbers_to_write = 4096 * 4096;

  test_read_write(writer, reader, numbers_to_write);
}



BOOST_AUTO_TEST_SUITE_END()

