/**
 * @file BufferedFileWriter.hpp Code to buffer and write data to a file. For better performance, the O_DIRECT flag is
 * used to circumvent additional kernel buffering. The buffer size has to be tuned according to the system. The writer
 * also supports several compression algorithms that are applied before data is written to the file. This can be useful
 * when writing is slow and enough cpu resources are available for compression.
 *
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_UTILS_BUFFEREDFILEWRITER_HPP_
#define READOUT_INCLUDE_READOUT_UTILS_BUFFEREDFILEWRITER_HPP_

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
 * Class to buffer and write data of a specified type to a file using O_DIRECT. In addition, data can be compressed.
 * before being written.
 * @tparam RawType Type of the data that will be written to file
 * @tparam Alignment The alignment used for allocations of buffers. It has to fulfil certain system specific
 * requirements such that file writing with O_DIRECT can work.
 */
template<class RawType, size_t Alignment = 4096>
class BufferedFileWriter
{
  using io_sink_t = boost::iostreams::file_descriptor_sink;
  using aligned_allocator_t = boost::alignment::aligned_allocator<io_sink_t::char_type, Alignment>;
  using filtering_ostream_t =
    boost::iostreams::filtering_stream<boost::iostreams::output, char, std::char_traits<char>, aligned_allocator_t>;

public:
  /**
   * Constructor to construct and initialize an instance. The file will be open after initialization.
   * @param filename The file to be used. Existing data will be overwritten, if the file does not exist it will be
   * created.
   * @param buffer_size The size of the buffer that is used before data is written to the file. Make sure that this
   * size fulfils size requirements of O_DIRECT, otherwise writes will fail.
   * @param compression_algorithm The compression algorithm to use. Can be one of: None, zstd, lzma or zlib
   * @throw CannotOpenFile If the file can not be opened.
   * @throw ConfigurationError If the compression algorithm parameter is not recognized.
   */
  BufferedFileWriter(std::string filename, size_t buffer_size, std::string compression_algorithm = "None")
  {
    open(filename, buffer_size, compression_algorithm);
  }

  /**
   * Constructor to construct an instance without opening a file.
   */
  BufferedFileWriter() {}

  /**
   * Destructor that closes the file before destruction to make sure all data in any buffers is written to file.
   */
  ~BufferedFileWriter()
  {
    if (m_is_open)
      close();
  }

  BufferedFileWriter(const BufferedFileWriter&) = delete;            ///< BufferedFileWriter is not copy-constructible
  BufferedFileWriter& operator=(const BufferedFileWriter&) = delete; ///< BufferedFileWriter is not copy-assginable
  BufferedFileWriter(BufferedFileWriter&&) = delete;                 ///< BufferedFileWriter is not move-constructible
  BufferedFileWriter& operator=(BufferedFileWriter&&) = delete;      ///< BufferedFileWriter is not move-assignable

  /**
   * Open a file.
   * @param filename The file to be used. Existing data will be overwritten, if the file does not exist it will be
   * created.
   * @param buffer_size The size of the buffer that is used before data is written to the file. Make sure that this
   * size fulfils size requirements of O_DIRECT, otherwise writes will fail.
   * @param compression_algorithm The compression algorithm to use. Can be one of: None, zstd, lzma or zlib
   * @throw CannotOpenFile If the file can not be opened.
   * @throw ConfigurationError If the compression algorithm parameter is not recognized.
   */
  void open(std::string filename,
            size_t buffer_size,
            std::string compression_algorithm = "None",
            bool use_o_direct = true)
  {
    m_use_o_direct = use_o_direct;
    if (m_is_open) {
      close();
    }

    m_filename = filename;
    m_buffer_size = buffer_size;
    m_compression_algorithm = compression_algorithm;
    auto oflag = O_CREAT | O_WRONLY;
    if (m_use_o_direct) {
      oflag = oflag | O_DIRECT;
    }

    m_fd = ::open(m_filename.c_str(), oflag, 0644);
    if (m_fd == -1) {
      throw CannotOpenFile(ERS_HERE, m_filename);
    }

    m_sink = io_sink_t(m_fd, boost::iostreams::file_descriptor_flags::close_handle);
    if (m_compression_algorithm == "zstd") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Using zstd compression" << std::endl;
      m_output_stream.push(boost::iostreams::zstd_compressor(boost::iostreams::zstd::best_speed));
    } else if (m_compression_algorithm == "lzma") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Using lzma compression" << std::endl;
      m_output_stream.push(boost::iostreams::lzma_compressor(boost::iostreams::lzma::best_speed));
    } else if (m_compression_algorithm == "zlib") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Using zlib compression" << std::endl;
      m_output_stream.push(boost::iostreams::zlib_compressor(boost::iostreams::zlib::best_speed));
    } else if (m_compression_algorithm == "None") {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Running without compression" << std::endl;
    } else {
      throw ConfigurationError(ERS_HERE, "Non-recognized compression algorithm: " + m_compression_algorithm);
    }

    m_output_stream.push(m_sink, m_buffer_size);
    m_is_open = true;
  }

  /**
   * Check if the file is open.
   * @return true if the file is open, false otherwise.
   */
  bool is_open() const { return m_is_open; }

  /**
   * Write an element to the buffer. If the buffer is full, all data from it will be written to file.
   * @param element The element to write.
   * @return true if the write was successful, false if the writer is not open or the write was not successful.
   */
  bool write(const RawType& element)
  {
    if (!m_is_open)
      return false;
    m_output_stream.write(reinterpret_cast<const char*>(&element), sizeof(element)); // NOLINT
    return !m_output_stream.bad();
  }

  /**
   * Close the writer. All data from any buffers will be written to file.
   */
  void close()
  {
    // Set the file descriptor to not use O_DIRECT. This is necessary because the write size has to be aligned for
    // O_DIRECT to succeed. This is not guaranteed for the data that remains in the buffer.
    fcntl(m_fd, F_SETFL, O_CREAT | O_WRONLY);
    m_output_stream.reset();
    m_is_open = false;
  }

  /**
   * If no compression is used, this writes all data from buffers to the file. In case that compression is used,
   * this is not guaranteed.
   */
  void flush()
  {
    // Set the file descriptor to not use O_DIRECT. This is necessary because the write size has to be aligned for
    // O_DIRECT to succeed. This is not guaranteed for the data that remains in the buffer.
    fcntl(m_fd, F_SETFL, O_CREAT | O_WRONLY);
    // This does not flush the compressor as it is not flushable
    m_output_stream.flush();
    // Activate O_DIRECT again
    auto oflag = O_CREAT | O_WRONLY;
    if (m_use_o_direct) {
      oflag = oflag | O_DIRECT;
    }
    fcntl(m_fd, F_SETFL, oflag);
  }

private:
  // Config parameters
  std::string m_filename;
  size_t m_buffer_size;
  std::string m_compression_algorithm;

  // Internals
  int m_fd;
  io_sink_t m_sink;
  filtering_ostream_t m_output_stream;
  bool m_is_open = false;
  bool m_use_o_direct = true;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_UTILS_BUFFEREDFILEWRITER_HPP_
