/**
* @file BufferedWriter.hpp Writes data to a file
*
* This is part of the DUNE DAQ , copyright 2021.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_BUFFEREDWRITER_HPP_
#define READOUT_SRC_BUFFEREDWRITER_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include <boost/align/aligned_allocator.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zstd.hpp>
#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <fcntl.h>
#include <unistd.h>

using namespace dunedaq::readout::logging;

namespace dunedaq {
  namespace readout {
    using io_sink_t = boost::iostreams::file_descriptor_sink;
    using aligned_allocator_t = boost::alignment::aligned_allocator<io_sink_t::char_type, 4096>;
    using filtering_stream_t = boost::iostreams::filtering_stream<boost::iostreams::output, char, std::char_traits<char>, aligned_allocator_t>;

    template<class RawType>
    class BufferedWriter {
    public:
      explicit BufferedWriter(std::string filename, size_t buffer_size, std::string compression_algorithm = "None")
      {
        open(filename, buffer_size, compression_algorithm);
      }

      explicit BufferedWriter() {

      }

      BufferedWriter(const BufferedWriter&)
      = delete; ///< BufferedWriter is not copy-constructible
      BufferedWriter& operator=(const BufferedWriter&)
      = delete; ///< BufferedWriter is not copy-assginable
      BufferedWriter(BufferedWriter&&)
      = delete; ///< BufferedWriter is not move-constructible
      BufferedWriter& operator=(BufferedWriter&&)
      = delete; ///< BufferedWriter is not move-assignable

      void open(std::string filename, size_t buffer_size, std::string compression_algorithm = "None") {
        if (m_is_open) flush();

        m_filename = filename;
        m_buffer_size = buffer_size;
        m_compression_algorithm = compression_algorithm;

        int fd = ::open(m_filename.c_str(), O_CREAT | O_RDWR | O_DIRECT);
        if (fd == -1) {
          throw CannotOpenFile(ERS_HERE, m_filename);
        }

        io_sink_t io_sink(fd, boost::iostreams::file_descriptor_flags::close_handle);
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

        m_output_stream.push(io_sink, m_buffer_size);
        m_is_open = true;
      }

      bool is_open() const {
        return m_is_open;
      }

      bool write(RawType element) {
        m_output_stream.write((char*)&element, sizeof(element));
        return !m_output_stream.bad();
      }

      void close() {
        flush();
        m_output_stream = filtering_stream_t();
        m_is_open = false;
      }

      void flush() {
        m_output_stream.flush();
      }

    private:
      std::string m_filename;
      size_t m_buffer_size;
      std::string m_compression_algorithm;

      filtering_stream_t m_output_stream;
      bool m_is_open = false;
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_BUFFEREDWRITER_HPP_
