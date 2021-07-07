/**
 * @file RawDataProcessorConcept.hpp Concept of raw data processing
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_RAWDATAPROCESSORCONCEPT_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_RAWDATAPROCESSORCONCEPT_HPP_

#include <string>

namespace dunedaq {
namespace readout {

template<class ReadoutType>
class RawDataProcessorConcept
{
public:
  RawDataProcessorConcept() {}

  RawDataProcessorConcept(const RawDataProcessorConcept&) =
    delete; ///< RawDataProcessorConcept is not copy-constructible
  RawDataProcessorConcept& operator=(const RawDataProcessorConcept&) =
    delete;                                                    ///< RawDataProcessorConcept is not copy-assginable
  RawDataProcessorConcept(RawDataProcessorConcept&&) = delete; ///< RawDataProcessorConcept is not move-constructible
  RawDataProcessorConcept& operator=(RawDataProcessorConcept&&) =
    delete; ///< RawDataProcessorConcept is not move-assignable

  //! Set the emulator mode, if active, timestamps of processed packets are overwritten with new ones
  virtual void conf(const nlohmann::json& cfg) { set_emulator_mode(cfg.get<datalinkhandler::Conf>().emulator_mode); }
  //! Get newest timestamp of last seen packet
  std::uint64_t get_last_daq_time() { return m_last_processed_daq_ts.load(); } // NOLINT(build/unsigned)
  //! Reset timestamp (useful in emulator mode)
  void reset_last_daq_time() { m_last_processed_daq_ts.store(0); }
  //! Set the emulator mode
  void set_emulator_mode(bool do_emu) { m_emulator_mode = do_emu; }
  //! Get the emulator mode
  bool get_emulator_mode() { return m_emulator_mode; }
  //! Process one element
  virtual void process_item(ReadoutType* item) = 0;

protected:
  bool m_emulator_mode{ false };
  std::atomic<std::uint64_t> m_last_processed_daq_ts{ 0 }; // NOLINT(build/unsigned)
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_RAWDATAPROCESSORCONCEPT_HPP_
