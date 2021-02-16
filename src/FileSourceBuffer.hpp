/**
* @file FileSourceBuffer.hpp Reads in data from raw binary dump files
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_FILESOURCEBUFFER_HPP_
#define UDAQ_READOUT_SRC_FILESOURCEBUFFER_HPP_

#include "ReadoutIssues.hpp"

#include <fstream>

namespace dunedaq {
namespace readout {

class FileSourceBuffer {
public:
  explicit FileSourceBuffer(int input_limit, int chunk_size)
  : input_limit_(input_limit)
  , chunk_size_(chunk_size)
  , element_count_(0)
  , source_filename_("")
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
    source_filename_ = sourcefile;
    // Open file 
    rawdata_ifs_.open(source_filename_, std::ios::in | std::ios::binary);
    if (!rawdata_ifs_) {
      throw CannotOpenFile(ERS_HERE, source_filename_);
    }
    
    // Check file size 
    rawdata_ifs_.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize filesize = rawdata_ifs_.gcount();
    if (filesize > input_limit_) { // bigger than configured limit
      ers::error(ConfigurationError(ERS_HERE, "File size limit exceeded."));
    }

    // Check for exact match
    int remainder = filesize % chunk_size_;
    if (remainder > 0) {
      ers::error(ConfigurationError(ERS_HERE, "Binary file contains more data than expected."));
    }
    
    // Set usable element count
    element_count_ = filesize / chunk_size_;
    
    // Read file into input buffer
    rawdata_ifs_.seekg(0, std::ios::beg);
    input_buffer_.reserve(filesize);
    input_buffer_.insert(input_buffer_.begin(), std::istreambuf_iterator<char>(rawdata_ifs_), std::istreambuf_iterator<char>());
    ERS_INFO("Available elements: " << element_count_ << " | In bytes: " << input_buffer_.size());
  }

  const int& num_elements() {
    return std::ref(element_count_);
  }

  std::vector<std::uint8_t>& get() {
    return std::ref(input_buffer_);
  }

private:
  // Configuration
  int input_limit_;
  int chunk_size_;
  int element_count_;
  std::string source_filename_;

  // Internals
  std::ifstream rawdata_ifs_;
  std::vector<std::uint8_t> input_buffer_;
};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_FILESOURCEBUFFER_HPP_
