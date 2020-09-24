/**
 * @file CardReaderDAQModule.hpp
 *
 * CardReaderDAQModule creates vectors of integers of a given length, starting with the given start integer and
 * counting up to the given ending integer. Its current position is persisted between generated vectors, so if the
 * parameters are chosen correctly, the generated vectors will "walk" through the valid range.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef APPFWK_UDAQ_READOUT_CARDREADERDAQMODULE_HPP_
#define APPFWK_UDAQ_READOUT_CARDREADERDAQMODULE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <future>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace readout {
/**
 * @brief CardReaderDAQModule creates vectors of ints and sends them
 * downstream
 */
class CardReaderDAQModule : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief CardReaderDAQModule Constructor
   * @param name Instance name for this CardReaderDAQModule instance
   */
  explicit CardReaderDAQModule(const std::string& name);

  CardReaderDAQModule(const CardReaderDAQModule&) =
    delete; ///< CardReaderDAQModule is not copy-constructible
  CardReaderDAQModule& operator=(const CardReaderDAQModule&) =
    delete; ///< CardReaderDAQModule is not copy-assignable
  CardReaderDAQModule(CardReaderDAQModule&&) =
    delete; ///< CardReaderDAQModule is not move-constructible
  CardReaderDAQModule& operator=(CardReaderDAQModule&&) =
    delete; ///< CardReaderDAQModule is not move-assignable

  void init() override;

private:
  // Commands
  void do_configure(const std::vector<std::string>& args);
  void do_start(const std::vector<std::string>& args);
  void do_stop(const std::vector<std::string>& args);

  // Threading
  ThreadHelper thread_;
  void do_work(std::atomic<bool>& running_flag);

  // Configuration
  std::unique_ptr<DAQSink<std::vector<int>>> outputQueue_;
  std::chrono::milliseconds queueTimeout_;
  size_t nIntsPerVector_ = 999;
  int starting_int_ = -999;
  int ending_int_ = -999;

  size_t wait_between_sends_ms_ = 999;

  // Card control
  felix::types::UniqueFlxCard& m_flx_card;
  std::mutex& m_card_mutex;


};
} // namespace appfwk

ERS_DECLARE_ISSUE_BASE(appfwk,
                       ProducerProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))
} // namespace dunedaq

#endif // APPFWK_UDAQ_READOUT_CARDREADERDAQMODULE_HPP_
