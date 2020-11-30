/**
* @file ReadoutElement.hpp Glue between raw processor, latency buffer 
* and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTELEMENT_HPP_
#define UDAQ_READOUT_SRC_READOUTELEMENT_HPP_

#include "ReadoutIssues.hpp"
#include "ReadoutElementMaker.hpp"
#include "ReusableThread.hpp"
//#include "RequestHandler.hpp"


namespace dunedaq {
namespace readout {

class ReadoutElement {
public:

  explicit ReadoutElement(std::atomic<bool>& run_marker)
  : run_marker_(run_marker)
  //, parser_impl_()
  //, producer_(parser_impl_)
  { 
  }

  ReadoutElement(const ReadoutElement&) 
    = delete; ///< ReadoutElement is not copy-constructible
  ReadoutElement& operator=(const ReadoutElement&) 
    = delete; ///< ReadoutElement is not copy-assginable
  ReadoutElement(ReadoutElement&&) 
    = delete; ///< ReadoutElement is not move-constructible
  ReadoutElement& operator=(ReadoutElement&&) 
    = delete; ///< ReadoutElement is not move-assignable

  void do_conf(const std::string& rawtype, int size=0) {
    latency_buffer_impl_ = LatencyBufferBaseMaker(rawtype, size);
    if(latency_buffer_impl_.get() == nullptr) {
      throw std::runtime_error("No Implementation available for raw type");
    }
  }

  void start() {

  }

  void stop() {

  }

private:
  std::atomic<bool>& run_marker_;

  // Producer: Mutabe parser operation and the producer itself
  //DefaultParserImpl parser_impl_; // this can be exposed anywhere to add things to preproc pipeline.
  //felix::packetformat::BlockParser<DefaultParserImpl> producer_;

  // Latency buffer: Continous and lookup tables
  std::unique_ptr<LatencyBufferBase> latency_buffer_impl_;

  // Consumers: Request handlers
  //std::unique_ptr<RequestHandler> request_handler_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTELEMENT_HPP_
