/**
 * @file ReadoutIssues.hpp Readout system related ERS issues
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_READOUTISSUES_HPP_
#define READOUT_INCLUDE_READOUT_READOUTISSUES_HPP_

#include <ers/Issue.hpp>

#include <string>

namespace dunedaq {

ERS_DECLARE_ISSUE(readout, InternalError, " Readout Internal Error: " << intererror, ((std::string)intererror))

ERS_DECLARE_ISSUE(readout, CommandError, " Command Error: " << commanderror, ((std::string)commanderror))

ERS_DECLARE_ISSUE(readout,
                  InitializationError,
                  " Readout Initialization Error: " << initerror,
                  ((std::string)initerror))

ERS_DECLARE_ISSUE(readout, ConfigurationError, " Readout Configuration Error: " << conferror, ((std::string)conferror))

ERS_DECLARE_ISSUE(readout, CannotOpenFile, " Couldn't open binary file: " << filename, ((std::string)filename))

ERS_DECLARE_ISSUE_BASE(readout,
                       CannotReadFile,
                       readout::ConfigurationError,
                       " Couldn't read properly the binary file: " << filename << " Cause: " << errorstr,
                       ((std::string)filename),
                       ((std::string)errorstr))

ERS_DECLARE_ISSUE(readout, CannotWriteToFile, " Could not write to file: " << filename, ((std::string)filename))

ERS_DECLARE_ISSUE(readout,
                  PostprocessingNotKeepingUp,
                  "Postprocessing has too much backlog, thread: " << i,
                  ((size_t)i))

ERS_DECLARE_ISSUE(readout,
                  EmptySourceBuffer,
                  " Source Buffer is empty, check file: " << filename,
                  ((std::string)filename))

ERS_DECLARE_ISSUE(readout,
                  CannotReadFromQueue,
                  " Failed attempt to read from the queue: " << queuename,
                  ((std::string)queuename))

ERS_DECLARE_ISSUE(readout,
                  CannotWriteToQueue,
                  " Failed attempt to write to the queue: " << queuename << ". Data will be lost!",
                  ((std::string)queuename))

ERS_DECLARE_ISSUE(readout,
                  TrmWithEmptyFragment,
                  " Trigger Matching result with empty fragment: " << trmdetails,
                  ((std::string)trmdetails))

ERS_DECLARE_ISSUE_BASE(readout,
                       FailedReadoutInitialization,
                       readout::InitializationError,
                       " Couldnt initialize Readout with current Init arguments " << initparams << ' ',
                       ((std::string)name),
                       ((std::string)initparams))

ERS_DECLARE_ISSUE_BASE(readout,
                       NoImplementationAvailableError,
                       readout::ConfigurationError,
                       " No " << impl << " implementation available for raw type: " << rawt << ' ',
                       ((std::string)impl),
                       ((std::string)rawt))

ERS_DECLARE_ISSUE_BASE(readout,
                       DefaultImplementationCalled,
                       readout::InternalError,
                       " Default " << impl << " implementation called! Function: " << func << ' ',
                       ((std::string)impl),
                       ((std::string)func))

ERS_DECLARE_ISSUE_BASE(readout,
                       ResourceQueueError,
                       readout::ConfigurationError,
                       " The " << queueType << " queue was not successfully created. ",
                       ((std::string)name),
                       ((std::string)queueType))

ERS_DECLARE_ISSUE(readout, ConfigurationNote, name << ": " << text, ((std::string)name)((std::string)text))

ERS_DECLARE_ISSUE(readout,
                  ConfigurationProblem,
                  name << " configuration problem: " << text,
                  ((std::string)name)((std::string)text))

} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_READOUTISSUES_HPP_
