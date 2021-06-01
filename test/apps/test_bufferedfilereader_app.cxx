/**
 * @file test_bufferedfilereader_app.cxx Test application for
 * BufferedFileReader implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "readout/utils/BufferedFileReader.hpp"

#include "logging/Logging.hpp"
#include "readout/types/ReadoutTypes.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

using namespace dunedaq::readout;

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    TLOG() << "usage: readout_test_bufferedfilereader filename" << std::endl;
    exit(1);
  }
  std::string filename(argv[1]);
  BufferedFileReader<types::WIB_SUPERCHUNK_STRUCT> reader(filename, 8388608);
  types::WIB_SUPERCHUNK_STRUCT chunk;
  for (uint i = 0; i < sizeof(chunk); ++i) {
    (reinterpret_cast<char*>(&chunk))[i] = static_cast<char>(i); // NOLINT
  }

  std::atomic<int64_t> bytes_read_total = 0;
  std::atomic<int64_t> bytes_read_since_last_statistics = 0;
  std::chrono::steady_clock::time_point time_point_last_statistics = std::chrono::steady_clock::now();

  auto statistics_thread = std::thread([&]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      double time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() -
                                                                                   time_point_last_statistics)
                           .count();
      TLOG() << "Bytes read: " << bytes_read_total << ", Throughput: "
             << static_cast<double>(bytes_read_since_last_statistics) / ((int64_t)1 << 20) / time_diff << " MiB/s"
             << std::endl;
      time_point_last_statistics = std::chrono::steady_clock::now();
      bytes_read_since_last_statistics = 0;
    }
  });

  while (true) {
    if (!reader.read(chunk)) {
      TLOG() << "Finished reading from file" << std::endl;
      exit(0);
    }
    bytes_read_total += sizeof(chunk);
    bytes_read_since_last_statistics += sizeof(chunk);
  }
}
