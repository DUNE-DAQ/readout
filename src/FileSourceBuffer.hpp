/**
* @file FileSourceBuffer.hpp Reads in data from raw binary dump files
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_FILESOURCEBUFFER_HPP_
#define READOUT_SRC_FILESOURCEBUFFER_HPP_

#include "ReadoutIssues.hpp"
#include "readout/ReadoutLogging.hpp"

#include "logging/Logging.hpp"

#include <fstream>
#include <limits>
#include <string>

using dunedaq::readout::logging::TLVL_BOOKKEEPING;

namespace dunedaq {
namespace readout {

class FileSourceBuffer {
public:
  explicit FileSourceBuffer(int input_limit, int chunk_size)
  : m_input_limit(input_limit)
  , m_chunk_size(chunk_size)
  , m_element_count(0)
  , m_source_filename("")
  { }

  FileSourceBuffer(const FileSourceBuffer&) 
    = delete; ///< FileSourceBuffer is not copy-constructible
  FileSourceBuffer& operator=(const FileSourceBuffer&) 
    = delete; ///< FileSourceBuffer is not copy-assginable
  FileSourceBuffer(FileSourceBuffer&&) 
    = delete; ///< FileSourceBuffer is not move-constructible
  FileSourceBuffer& operator=(FileSourceBuffer&&) 
    = delete; ///< FileSourceBuffer is not move-assignable

  void read(const std::string& sourcefile)
  {
    m_source_filename = sourcefile;
    // Open file 
    m_rawdata_ifs.open(m_source_filename, std::ios::in | std::ios::binary);
    if (!m_rawdata_ifs) {
      throw CannotOpenFile(ERS_HERE, m_source_filename);
    }
    
    // Check file size 
    m_rawdata_ifs.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize filesize = m_rawdata_ifs.gcount();
    if (filesize > m_input_limit) { // bigger than configured limit
      ers::error(ConfigurationError(ERS_HERE, "File size limit exceeded."));
    }

    // Check for exact match
    int remainder = filesize % m_chunk_size;
    if (remainder > 0) {
      ers::error(ConfigurationError(ERS_HERE, "Binary file contains more data than expected."));
    }
    
    // Set usable element count
    m_element_count = filesize / m_chunk_size;
    
    // Read file into input buffer
    m_rawdata_ifs.seekg(0, std::ios::beg);
    m_input_buffer.reserve(filesize);
    m_input_buffer.insert(m_input_buffer.begin(), std::istreambuf_iterator<char>(m_rawdata_ifs), std::istreambuf_iterator<char>());
    TLOG_DEBUG(TLVL_BOOKKEEPING) << "Available elements: " << std::to_string(m_element_count) 
                                 << " | In bytes: " << std::to_string(m_input_buffer.size());
  }

  const int& num_elements() {
    return std::ref(m_element_count);
  }

  std::vector<std::uint8_t>& get() { // NOLINT
    return std::ref(m_input_buffer);
  }

private:
  // Configuration
  int m_input_limit;
  int m_chunk_size;
  int m_element_count;
  std::string m_source_filename;

  // Internals
  std::ifstream m_rawdata_ifs;
  std::vector<std::uint8_t> m_input_buffer; // NOLINT
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_FILESOURCEBUFFER_HPP_
