/**
* @file RawDataProcessorConcept.hpp Concept of raw data processing
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_RAWDATAPROCESSORCONCEPT_HPP_
#define UDAQ_READOUT_SRC_RAWDATAPROCESSORCONCEPT_HPP_

#include "Time.hpp"

namespace dunedaq {
namespace readout {

class RawDataProcessorConcept {
public:

  explicit RawDataProcessorConcept(const std::string& rawtype)
  : raw_type_name_(rawtype)
  {}

  RawDataProcessorConcept(const RawDataProcessorConcept&) 
    = delete; ///< RawDataProcessorConcept is not copy-constructible
  RawDataProcessorConcept& operator=(const RawDataProcessorConcept&) 
    = delete; ///< RawDataProcessorConcept is not copy-assginable
  RawDataProcessorConcept(RawDataProcessorConcept&&) 
    = delete; ///< RawDataProcessorConcept is not move-constructible
  RawDataProcessorConcept& operator=(RawDataProcessorConcept&&) 
    = delete; ///< RawDataProcessorConcept is not move-assignable

  virtual void conf(const nlohmann::json& cfg) = 0;
  uint64_t get_last_daq_time() { return last_processed_daq_ts_.load(); }
  void reset_last_daq_time() { last_processed_daq_ts_.store(0); }

protected:
  std::atomic<time::timestamp_t> last_processed_daq_ts_{0};

private:
  std::string raw_type_name_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_RAWDATAPROCESSORCONCEPT_HPP_
