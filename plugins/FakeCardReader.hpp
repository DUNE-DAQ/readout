/**
 * @file FakeCardReader.hpp FELIX FULLMODE Fake Links
 * Generates user payloads at a given rate, from WIB to FELIX binary captures
 * This implementation is purely software based, no FELIX card and tools
 * are needed to use this module.
 *
 * TODO: Make it generic, to consumer other types of user payloads.
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_PLUGINS_FAKECARDREADER_HPP_
#define READOUT_PLUGINS_FAKECARDREADER_HPP_

// package
#include "readout/ReadoutTypes.hpp"
#include "readout/concepts/SourceEmulatorConcept.hpp"
#include "readout/sourceemulatorconfig/Structs.hpp"
#include "readout/utils/ReusableThread.hpp"
//#include "CreateSourceEmulator.hpp"
#include "readout/utils/FileSourceBuffer.hpp"
#include "readout/utils/RateLimiter.hpp"

// appfwk
#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"

// std
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace readout {

class FakeCardReader : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief FakeCardReader Constructor
   * @param name Instance name for this FakeCardReader instance
   */
  explicit FakeCardReader(const std::string& name);

  FakeCardReader(const FakeCardReader&) = delete;            ///< FakeCardReader is not copy-constructible
  FakeCardReader& operator=(const FakeCardReader&) = delete; ///< FakeCardReader is not copy-assignable
  FakeCardReader(FakeCardReader&&) = delete;                 ///< FakeCardReader is not move-constructible
  FakeCardReader& operator=(FakeCardReader&&) = delete;      ///< FakeCardReader is not move-assignable

  void init(const data_t&) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  using sink_t = appfwk::DAQSink<types::WIB_SUPERCHUNK_STRUCT>;
  // Commands
  void do_conf(const data_t& /*args*/);
  void do_scrap(const data_t& /*args*/);
  void do_start(const data_t& /*args*/);
  void do_stop(const data_t& /*args*/);

  void generate_data(sink_t* queue, int link_id);

  // Configuration
  bool m_configured;
  using module_conf_t = sourceemulatorconfig::Conf;
  module_conf_t m_cfg;

  std::map<std::string, std::unique_ptr<SourceEmulatorConcept>> m_source_emus;

  // appfwk Queues
  std::chrono::milliseconds m_queue_timeout_ms;
  std::vector<sink_t*> m_output_queues;

  // Internals
  std::unique_ptr<FileSourceBuffer> m_source_buffer;

  // Threading
  std::atomic<bool> m_run_marker;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_PLUGINS_FAKECARDREADER_HPP_
