/**
 * @file BufferedFileReader.hpp Code to read data from a file. The same compression algorithms as for the
 * BufferedFileWriter are supported.
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_BUFFEREDFILEREADER_HPP_
#define READOUT_SRC_BUFFEREDFILEREADER_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include <boost/align/aligned_allocator.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/zstd.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <unistd.h>

using dunedaq::readout::logging::TLVL_WORK_STEPS;

namespace dunedaq {
namespace readout {
/**
 * Class to read data of a specified type in a buffered manner.
 * @tparam RawType Type of the data that is read from the file.
 * @tparam Alignment The alignment size of internal buffers.
 */
template<class RawType, size_t Alignment = 4096>
class BufferedFileReader
{
  using io_source_t = boost::iostreams::file_descriptor_source;
  using aligned_allocator_t = boost::alignment::aligned_allocator<io_source_t::char_type, Alignment>;
  using filtering_istream_t =
    boost::iostreams::filtering_stream<boost::iostreams::input, char, std::char_traits<char>, aligned_allocator_t>;

public:
  /**
   * Constructor to construct and initalize an instance. The file will be open after initialization.
   * @param filename The file to be used.
   * @param buffer_size The size of the buffer to be used.
   * @param compression_algorithm The compression algorithm to use. Can be one of: None, zstd, lzma or zlib
   * @throw CannotOpenFile If the file can not be opened.
   * @throw ConfigurationError If the compression algorithm parameter is not recognized.
   */
  BufferedFileReader(std::string filename, size_t buffer_size, std::string compression_algorithm = "None")
  {
    open(filename, buffer_size, compression_algorithm);
  }

  /**
   * Constructor to construct and instance without opening a file.
   */
  BufferedFileReader() {}

  BufferedFileReader(const BufferedFileReader&) = delete;            ///< BufferedFileReader is not copy-constructible
  BufferedFileReader& operator=(const BufferedFileReader&) = delete; ///< BufferedFileReader is not copy-assginable
  BufferedFileReader(BufferedFileReader&&) = delete;                 ///< BufferedFileReader is not move-constructible
  BufferedFileReader& operator=(BufferedFileReader&&) = delete;      ///< BufferedFileReader is not move-assignable

  /**
   * Open a file.
   * @param filename The file to be used.
   * @param buffer_size The size of the buffer to be used.
   * @param compression_algorithm The compression algorithm to use. Can be one of: None, zstd, lzma or zlib
   * @throw CannotOpenFile If the file can not be opened.
   * @throw ConfigurationError If the compression algorithm parameter is not recognized.
   */
  void open(std::string filename, size_t buffer_size, std::string compression_algorithm = "None")
  {
    m_filename = filename;
    m_buffer_size = buffer_size;
    m_compression_algorithm = compression_algorithm;

    int fd = ::open(m_filename.c_str(), O_RDONLY);
    if (fd == -1) {
      throw CannotOpenFile(ERS_HERE, m_filename);
    }

    io_source_t io_source(fd, boost::iostreams::file_descriptor_flags::close_handle);
    if (m_compression_algorithm == "zstd") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Using zstd compression" << std::endl;
      m_input_stream.push(boost::iostreams::zstd_decompressor());
    } else if (m_compression_algorithm == "lzma") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Using lzma compression" << std::endl;
      m_input_stream.push(boost::iostreams::lzma_decompressor());
    } else if (m_compression_algorithm == "zlib") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Using zlib compression" << std::endl;
      m_input_stream.push(boost::iostreams::zlib_decompressor());
    } else if (m_compression_algorithm == "None") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Running without compression" << std::endl;
    } else {
      throw ConfigurationError(ERS_HERE, "Non-recognized compression algorithm: " + m_compression_algorithm);
    }

    m_input_stream.push(io_source, m_buffer_size);
    m_is_open = true;
  }

  /**
   * Check if the file is open.
   * @return true if the file is open, false otherwise.
   * @throw CannotOpenFile If the file can not be opened.
   * @throw ConfigurationError If the compression algorithm parameter is not recognized.
   */
  bool is_open() const { return m_is_open; }

  /**
   * Read one element from the file.
   * @param element Reference to the element that will contain the value that was read.
   * @return true if the read was successful, false if the reader is not open or the read was not successful.
   */
  bool read(RawType& element)
  {
    if (!m_is_open)
      return false;
    m_input_stream.read(reinterpret_cast<char*>(&element), sizeof(element)); // NOLINT
    return (m_input_stream.gcount() == sizeof(element));
  }

  /**
   * Close the reader.
   */
  void close()
  {
    m_input_stream.reset();
    m_is_open = false;
  }

private:
  // Config parameters
  std::string m_filename;
  size_t m_buffer_size;
  std::string m_compression_algorithm;

  // Internals
  filtering_istream_t m_input_stream;
  bool m_is_open = false;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_BUFFEREDFILEREADER_HPP_
