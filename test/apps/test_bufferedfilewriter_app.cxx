/**
 * @file test_bufferedfilewriter_app.cxx Test application for
 * BufferedFileWriter implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "BufferedFileWriter.hpp"

#include "logging/Logging.hpp"
#include "readout/ReadoutTypes.hpp"

#include <atomic>
#include <string>
#include <chrono>
#include <memory>

using namespace dunedaq::readout;

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    std::cout << "usage: readout_test_bufferedfilewriter filename" << std::endl;
    exit(1);
  }
  remove(argv[1]); // NOLINT
  std::string filename(argv[1]);
  BufferedFileWriter<types::WIB_SUPERCHUNK_STRUCT> writer(filename, 8388608);
  types::WIB_SUPERCHUNK_STRUCT chunk;
  for (uint i = 0; i < sizeof(chunk); ++i) {
    (reinterpret_cast<char*>(&chunk))[i] = static_cast<char>(i); // NOLINT
  }

  std::atomic<int64_t> bytes_written_total = 0;
  std::atomic<int64_t> bytes_written_since_last_statistics = 0;
  std::chrono::steady_clock::time_point time_point_last_statistics = std::chrono::steady_clock::now();

  auto statistics_thread = std::thread([&]() {
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      double time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now()
                                                                                   - time_point_last_statistics).count();
      std::cout << "Bytes written: " << bytes_written_total << ", Throughput: "
                << static_cast<double>(bytes_written_since_last_statistics) / ((int64_t) 1 << 20) / time_diff << " MiB/s" << std::endl;
      time_point_last_statistics = std::chrono::steady_clock::now();
      bytes_written_since_last_statistics = 0;
    }
  });

  while (true) {
    if (!writer.write(chunk)) {
      std::cout << "Could not write to file" << std::endl;
      exit(1);
    }
    bytes_written_total += sizeof(chunk);
    bytes_written_since_last_statistics += sizeof(chunk);
  }
}
