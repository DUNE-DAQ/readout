/**
* @file RawDataProcessorConcept.hpp Concept of raw data processing
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_RAWDATAPROCESSORCONCEPT_HPP_
#define READOUT_SRC_RAWDATAPROCESSORCONCEPT_HPP_

#include "Time.hpp"

#include <string>

namespace dunedaq {
namespace readout {

  template <class RawType>
class RawDataProcessorConcept {
public:

  explicit RawDataProcessorConcept()
  {}

  RawDataProcessorConcept(const RawDataProcessorConcept&) 
    = delete; ///< RawDataProcessorConcept is not copy-constructible
  RawDataProcessorConcept& operator=(const RawDataProcessorConcept&) 
    = delete; ///< RawDataProcessorConcept is not copy-assginable
  RawDataProcessorConcept(RawDataProcessorConcept&&) 
    = delete; ///< RawDataProcessorConcept is not move-constructible
  RawDataProcessorConcept& operator=(RawDataProcessorConcept&&) 
    = delete; ///< RawDataProcessorConcept is not move-assignable

  virtual void conf(const nlohmann::json& cfg) {
    set_emulator_mode(cfg.get<datalinkhandler::Conf>().emulator_mode);
  }
  time::timestamp_t get_last_daq_time() { return m_last_processed_daq_ts.load(); }
  void reset_last_daq_time() { m_last_processed_daq_ts.store(0); }
  void set_emulator_mode(bool do_emu) { m_emulator_mode = do_emu; }
  bool get_emulator_mode() { return m_emulator_mode; }
  virtual void process_item(RawType* item) = 0;

protected:
  bool m_emulator_mode{false};
  std::atomic<time::timestamp_t> m_last_processed_daq_ts{0};

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_RAWDATAPROCESSORCONCEPT_HPP_
