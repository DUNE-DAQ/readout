/**
 * @file DefaultParserImpl.hpp
 */

#ifndef APPFWK_UDAQ_READOUT_DEFAULTPARSERIMPL_HPP_
#define APPFWK_UDAQ_READOUT_DEFAULTPARSERIMPL_HPP_

// 3rdparty, external
#include "packetformat/block_format.hpp"
#include "packetformat/block_parser.hpp"

// From Module
//#include "appfwk/DAQSource.hpp"

#include "ReadoutIssues.hpp"
//#include "ReadoutTypes.hpp"
#include "ReadoutStatistics.hpp"

// From STD
#include <iomanip>
#include <functional>

namespace dunedaq::readout {
/*
 * DefaultParserImpl
 * Author: Roland.Sipos@cern.ch
 * Description: Parser callbacks for FELIX chunks
 *   Implements ParserOperations from felix::packetformat
 * Date: May 2020
 * */
class DefaultParserImpl : public felix::packetformat::ParserOperations
{
public:
  explicit DefaultParserImpl();
  ~DefaultParserImpl();
  DefaultParserImpl(const DefaultParserImpl&) =
    delete; ///< DefaultParserImpl is not copy-constructible
  DefaultParserImpl& operator=(const DefaultParserImpl&) =
    delete; ///< DefaultParserImpl is not copy-assignable
  DefaultParserImpl(DefaultParserImpl&&) =
    delete; ///< DefaultParserImpl is not move-constructible
  DefaultParserImpl& operator=(DefaultParserImpl&&) =
    delete; ///< DefaultParserImpl is not move-assignable

  void set_ids(unsigned id, unsigned tag) { 
    link_id_ = id; 
    link_tag_ = tag; 
  }

  //void start() {}
  //void stop() {}
  void log_packet(bool isShort) {}

  std::function<void(const felix::packetformat::chunk& chunk)> post_process_chunk_func = nullptr;
  void post_process_chunk(const felix::packetformat::chunk& chunk);
  void chunk_processed(const felix::packetformat::chunk& chunk) { 
    post_process_chunk_func(chunk); 
  }

  std::function<void(const felix::packetformat::shortchunk& shortchunk)> post_process_shortchunk_func = nullptr;
  void post_process_shortchunk(const felix::packetformat::shortchunk& shortchunk);
  void shortchunk_processed(const felix::packetformat::shortchunk& shortchunk) { 
    post_process_shortchunk_func(shortchunk); 
  }

  std::function<void(const felix::packetformat::subchunk& subchunk)> post_process_subchunk_func = nullptr;
  void post_process_subchunk(const felix::packetformat::subchunk& subchunk);
  void subchunk_processed(const felix::packetformat::subchunk& subchunk) {
    post_process_subchunk_func(subchunk);
  }

  std::function<void(const felix::packetformat::block& block)> post_process_block_func = nullptr;
  void post_process_block(const felix::packetformat::block& block);
  void block_processed(const felix::packetformat::block& block) {
    post_process_block_func(block);
  }

  std::function<void(const felix::packetformat::chunk& chunk)> post_process_chunk_with_error_func = nullptr;
  void post_process_chunk_with_error(const felix::packetformat::chunk& chunk);
  void chunk_processed_with_error(const felix::packetformat::chunk& chunk) {
    post_process_chunk_with_error_func(chunk);
  }

  std::function<void(const felix::packetformat::subchunk& subchunk)> post_process_subchunk_with_error_func = nullptr;
  void post_process_subchunk_with_error(const felix::packetformat::subchunk& subchunk);
  void subchunk_processed_with_error(const felix::packetformat::subchunk& subchunk) {
    post_process_subchunk_with_error_func(subchunk);
  }

  std::function<void(const felix::packetformat::shortchunk& shortchunk)> post_process_shortchunk_with_error_func = nullptr;
  void post_process_shortchunk_with_error(const felix::packetformat::shortchunk& shortchunk);
  void shortchunk_process_with_error(const felix::packetformat::shortchunk& shortchunk) {
    post_process_shortchunk_with_error_func(shortchunk);
  }

  std::function<void(const felix::packetformat::block& block)> post_process_block_with_error_func = nullptr;
  void post_process_block_with_error(const felix::packetformat::block& block);
  void block_processed_with_error(const felix::packetformat::block& block) {
    post_process_block_with_error_func(block);
  }

private:
  unsigned link_id_ = 0;
  unsigned link_tag_ = 0;

};

} // namespace dunedaq::readout

#endif // APPFWK_UDAQ_READOUT_LINKPARSERIMPL_HPP_
