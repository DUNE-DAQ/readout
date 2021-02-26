/**
 * @file DataLinkHandler.cpp DataLinkHandler class implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/
#include "readout/datalinkhandler/Nljs.hpp"
#include "DataLinkHandler.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"

#include <sstream>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Name used by TRACE TLOG calls from this source file
*/
#define TRACE_NAME "DataLinkHandler" // NOLINT

/**
 * @brief TRACE debug levels used in this source file
 */
enum
{
  TLVL_ENTER_EXIT_METHODS = 5,
  TLVL_WORK_STEPS = 10,
  TLVL_BOOKKEEPING = 15
};

namespace dunedaq {
namespace readout { 

DataLinkHandler::DataLinkHandler(const std::string& name)
  : DAQModule(name)
  , m_configured(false)
  , m_readout_impl(nullptr)
  , m_run_marker{false}
{
  register_command("conf", &DataLinkHandler::do_conf);
  register_command("scrap", &DataLinkHandler::do_scrap);
  register_command("start", &DataLinkHandler::do_start);
  register_command("stop", &DataLinkHandler::do_stop);
}

void
DataLinkHandler::init(const data_t& args)
{
  TLOG() << get_name() << "Initialize readout implementation...";
  m_readout_impl = createReadout(args, m_run_marker);
  if (m_readout_impl == nullptr) {
    TLOG() << get_name() << "Initialize readout implementation FAILED...";
    throw FailedReadoutInitialization(ERS_HERE, get_name(), args.dump(4)); // 4 json ident
  }
}

void DataLinkHandler::get_info(opmonlib::InfoCollector& ci, int level) {
  m_readout_impl->get_info(ci, level);
}

void
DataLinkHandler::do_conf(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << "Configure readout implementation...";
  m_readout_impl->conf(args);
  m_configured = true;
}

void
DataLinkHandler::do_scrap(const data_t& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << "Scrap readout implementation...";
  m_configured = false;
}
void 
DataLinkHandler::do_start(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << "Start readout implementeation...";
  m_run_marker.store(true);
  m_readout_impl->start(args);
}

void 
DataLinkHandler::do_stop(const data_t& args)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << "Stop readout implementation...";
  m_run_marker.store(false);
  m_readout_impl->stop(args);
}

} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DataLinkHandler)
