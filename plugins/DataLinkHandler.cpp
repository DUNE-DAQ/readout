/**
 * @file DataLinkHandler.cpp DataLinkHandler class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "readout/ReadoutLogging.hpp"

#include "DataLinkHandler.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

DataLinkHandler::DataLinkHandler(const std::string& name)
  : DAQModule(name)
  , m_configured(false)
  , m_readout_impl(nullptr)
  , m_run_marker{ false }
{
  register_command("conf", &DataLinkHandler::do_conf);
  register_command("scrap", &DataLinkHandler::do_scrap);
  register_command("start", &DataLinkHandler::do_start);
  register_command("stop", &DataLinkHandler::do_stop);
  register_command("record", &DataLinkHandler::do_record);
}

void
DataLinkHandler::init(const data_t& args)
{

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  m_readout_impl = createReadout(args, m_run_marker);
  if (m_readout_impl == nullptr) {
    TLOG() << get_name() << "Initialize readout implementation FAILED! "
           << "Failed to find specialization for given queue setup!";
    throw FailedReadoutInitialization(ERS_HERE, get_name(), args.dump()); // 4 json ident
  }
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
DataLinkHandler::get_info(opmonlib::InfoCollector& ci, int level)
{
  m_readout_impl->get_info(ci, level);
}

void
DataLinkHandler::do_conf(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_conf() method";
  m_readout_impl->conf(args);
  m_configured = true;
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_conf() method";
}

void
DataLinkHandler::do_scrap(const data_t& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  m_configured = false;
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}
void
DataLinkHandler::do_start(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_run_marker.store(true);
  m_readout_impl->start(args);

  rcif::cmd::StartParams start_params = args.get<rcif::cmd::StartParams>();
  m_run_number = start_params.run;
  TLOG() << get_name() << " successfully started for run number " << m_run_number;

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
DataLinkHandler::do_stop(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_run_marker.store(false);
  m_readout_impl->stop(args);
  TLOG() << get_name() << " successfully stopped for run number " << m_run_number;
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
DataLinkHandler::do_record(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_issue_recording() method";
  m_readout_impl->record(args);
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_issue_recording() method";
}

} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DataLinkHandler)
