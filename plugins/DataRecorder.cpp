/**
 * @file DataRecorder.cpp DataRecorder implementation
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "RecorderImpl.hpp"
#include "readout/ReadoutLogging.hpp"
#include "readout/ReadoutTypes.hpp"
#include "readout/datarecorder/Nljs.hpp"
#include "readout/datarecorder/Structs.hpp"
#include "readout/datarecorderinfo/Nljs.hpp"

#include "DataRecorder.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "ReadoutIssues.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "logging/Logging.hpp"
#include <string>

using namespace dunedaq::readout::logging;

namespace dunedaq {
namespace readout {

DataRecorder::DataRecorder(const std::string& name)
  : DAQModule(name)
{
  register_command("conf", &DataRecorder::do_conf);
  register_command("start", &DataRecorder::do_start);
  register_command("stop", &DataRecorder::do_stop);
}

void
DataRecorder::init(const data_t& args)
{
  try {
    auto qi = appfwk::queue_index(args, { "raw_recording" });
    auto inst = qi["raw_recording"].inst;

    // IF WIB2
    if (inst.find("wib2") != std::string::npos) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating recorder for wib2";
      recorder.reset(new RecorderImpl<types::WIB2_SUPERCHUNK_STRUCT>(get_name()));
      recorder->init(args);
      return;
    }

    // IF WIB
    if (inst.find("wib") != std::string::npos) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating recorder for wib";
      recorder.reset(new RecorderImpl<types::WIB_SUPERCHUNK_STRUCT>(get_name()));
      recorder->init(args);
      return;
    }

    // IF PDS
    if (inst.find("pds") != std::string::npos) {
      TLOG_DEBUG(TLVL_WORK_STEPS) << "Creating recorder for pds";
      recorder.reset(new RecorderImpl<types::PDS_SUPERCHUNK_STRUCT>(get_name()));
      recorder->init(args);
      return;
    }

    throw ConfigurationError(ERS_HERE, "Could not create DataRecorder of type " + inst);

  } catch (const ers::Issue& excpt) {
    throw ResourceQueueError(ERS_HERE, "Could not initialize queue", "raw_recording", "");
  }
}

void
DataRecorder::get_info(opmonlib::InfoCollector& ci, int level)
{
  recorder->get_info(ci, level);
  }

void
DataRecorder::do_conf(const data_t& args)
{
  recorder->do_conf(args);
}

void
DataRecorder::do_start(const data_t& args)
{
  recorder->do_start(args);
}

void
DataRecorder::do_stop(const data_t& args)
{
  recorder->do_stop(args);
}

} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DataRecorder)
