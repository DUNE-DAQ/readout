/**
* @file RawProcessorBase.hpp Glue between input receiver, payload processor, 
* latency buffer and request handler.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_RAWPROCESSORBASE_HPP_
#define UDAQ_READOUT_SRC_RAWPROCESSORBASE_HPP_

namespace dunedaq {
namespace readout {

class RawProcessorBase {
public:

  explicit RawProcessorBase(const std::string& rawtype)
  : raw_type_name_(rawtype)
  {}

  RawProcessorBase(const RawProcessorBase&) 
    = delete; ///< RawProcessorBase is not copy-constructible
  RawProcessorBase& operator=(const RawProcessorBase&) 
    = delete; ///< RawProcessorBase is not copy-assginable
  RawProcessorBase(RawProcessorBase&&) 
    = delete; ///< RawProcessorBase is not move-constructible
  RawProcessorBase& operator=(RawProcessorBase&&) 
    = delete; ///< RawProcessorBase is not move-assignable

  virtual void conf(const nlohmann::json& cfg) = 0;

private:
  std::string raw_type_name_;

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_RAWPROCESSORBASE_HPP_
