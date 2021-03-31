/**
* @file BufferedReader.hpp Reads data to a file
*
* This is part of the DUNE DAQ , copyright 2021.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_BUFFEREDREADER_HPP_
#define READOUT_SRC_BUFFEREDREADER_HPP_

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
    using io_source_t = boost::iostreams::file_descriptor_source;
    using aligned_allocator_t = boost::alignment::aligned_allocator<io_sink_t::char_type, 4096>;
    using filtering_istream_t = boost::iostreams::filtering_stream<boost::iostreams::input, char, std::char_traits<char>, aligned_allocator_t>;

    template<class RawType>
    class BufferedReader {
    public:
      explicit BufferedReader(std::string filename, size_t buffer_size, std::string compression_algorithm = "None")
      {
        open(filename, buffer_size, compression_algorithm);
      }

      explicit BufferedReader() {

      }

      BufferedReader(const BufferedReader&)
      = delete; ///< BufferedReader is not copy-constructible
      BufferedReader& operator=(const BufferedReader&)
      = delete; ///< BufferedReader is not copy-assginable
      BufferedReader(BufferedReader&&)
      = delete; ///< BufferedReader is not move-constructible
      BufferedReader& operator=(BufferedReader&&)
      = delete; ///< BufferedReader is not move-assignable

      void open(std::string filename, size_t buffer_size, std::string compression_algorithm = "None") {
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

      bool is_open() const {
        return m_is_open;
      }

      bool read(RawType& element) {
        m_input_stream.read((char*)&element, sizeof(element));
        return (m_input_stream.gcount() == sizeof(element));
      }

      void close() {
        m_input_stream.reset();
        m_is_open = false;
      }

    private:
      std::string m_filename;
      size_t m_buffer_size;
      std::string m_compression_algorithm;

      filtering_istream_t m_input_stream;
      bool m_is_open = false;
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_BUFFEREDREADER_HPP_
