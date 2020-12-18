/**
* @file WIBFrameProcessor.hpp Glue between input receiver, payload processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
#define UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_

#include "TaskRawDataProcessorModel.hpp"
#include "ReadoutStatistics.hpp"
#include "ReadoutIssues.hpp"
#include "Time.hpp"

#include "dataformats/wib/WIBFrame.hpp"

#include <functional>
#include <atomic>

#include "tbb/flow_graph.h"

namespace dunedaq {
namespace readout {

class WIBFrameProcessor : public TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT> {

public:
  using frameptr = types::WIB_SUPERCHUNK_STRUCT*;
  using wibframeptr = dunedaq::dataformats::WIBFrame*;
  using funcnode_t = tbb::flow::function_node<frameptr, frameptr>;

  explicit WIBFrameProcessor(const std::string& rawtype, std::function<void(frameptr)>& process_override)
  : TaskRawDataProcessorModel<types::WIB_SUPERCHUNK_STRUCT>(rawtype, process_override)
  {
    tasklist_.push_back( std::bind(&WIBFrameProcessor::timestamp_check, this, std::placeholders::_1) );
  } 

protected:
  
  time::timestamp_t previous_ts_ = 0;
  time::timestamp_t current_ts_ = 0;
  bool first_ts_missmatch_ = true;
  stats::counter_t ts_error_ctr_{0};

  void timestamp_check(frameptr fp) {
    auto* wfptr = reinterpret_cast<dunedaq::dataformats::WIBFrame*>(fp);
    current_ts_ = wfptr->timestamp();
    if (current_ts_ - previous_ts_ != 300) {
      ++ts_error_ctr_;
      if (first_ts_missmatch_) {
        wfptr->wib_header()->print();
        ERS_INFO("First TS mismatch is fine | previous: " << previous_ts_ << " next: " << current_ts_);
        first_ts_missmatch_ = false;
      }
      ERS_INFO("Timestamp MISSMATCH! -> | previous: " << previous_ts_ << " next: " << current_ts_);
    }
    previous_ts_ = current_ts_;
  }

private:

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_WIBFRAMEPROCESSOR_HPP_
