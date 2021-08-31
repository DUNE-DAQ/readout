/**
 * @file ReadoutIssues.hpp Readout system related ERS issues
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_READOUTISSUES_HPP_
#define READOUT_INCLUDE_READOUT_READOUTISSUES_HPP_

#include "dataformats/GeoID.hpp"

#include <ers/Issue.hpp>

#include <string>

namespace dunedaq {
ERS_DECLARE_ISSUE(readout,
                  InternalError,
                  "GeoID[" << geoid << "] Internal Error: " << error,
                  ((dataformats::GeoID)geoid)((std::string)error))

ERS_DECLARE_ISSUE(readout,
                  CommandError,
                  "GeoID[" << geoid << "] Command Error: " << commanderror,
                  ((dataformats::GeoID)geoid)((std::string)commanderror))

ERS_DECLARE_ISSUE(readout, InitializationError, "Readout Initialization Error: " << initerror, ((std::string)initerror))

ERS_DECLARE_ISSUE(readout,
                  ConfigurationError,
                  "GeoID[" << geoid << "] Readout Configuration Error: " << conferror,
                  ((dataformats::GeoID)geoid)((std::string)conferror))

ERS_DECLARE_ISSUE(readout,
                  BufferedReaderWriterConfigurationError,
                  "Configuration Error: " << conferror,
                  ((std::string)conferror))

ERS_DECLARE_ISSUE(readout,
                  DataRecorderConfigurationError,
                  "Configuration Error: " << conferror,
                  ((std::string)conferror))

                  ERS_DECLARE_ISSUE(readout,
                                    ProducerNotKeepingUp,
                                    "GeoID[" << geoid << "] Producer could not keep up with rate: " << error,
                                    ((dataformats::GeoID)geoid)((std::string)error))

ERS_DECLARE_ISSUE(readout, GenericConfigurationError, "Configuration Error: " << conferror, ((std::string)conferror))

ERS_DECLARE_ISSUE(readout, CannotOpenFile, "Couldn't open binary file: " << filename, ((std::string)filename))

ERS_DECLARE_ISSUE(readout,
                  BufferedReaderWriterCannotOpenFile,
                  "Couldn't open file: " << filename,
                  ((std::string)filename))

ERS_DECLARE_ISSUE_BASE(readout,
                       CannotReadFile,
                       readout::ConfigurationError,
                       " Couldn't read properly the binary file: " << filename << " Cause: " << errorstr,
                       ((dataformats::GeoID)geoid)((std::string)filename),
                       ((std::string)errorstr))

ERS_DECLARE_ISSUE(readout, CannotWriteToFile, "Could not write to file: " << filename, ((std::string)filename))

ERS_DECLARE_ISSUE(readout,
                  PostprocessingNotKeepingUp,
                  "GeoID[" << geoid << "] Postprocessing has too much backlog, thread: " << i,
                  ((dataformats::GeoID)geoid)((size_t)i))

ERS_DECLARE_ISSUE(readout,
                  EmptySourceBuffer,
                  "GeoID[" << geoid << "] Source Buffer is empty, check file: " << filename,
                  ((dataformats::GeoID)geoid)((std::string)filename))

ERS_DECLARE_ISSUE(readout,
                  CannotReadFromQueue,
                  "GeoID[" << geoid << "] Failed attempt to read from the queue: " << queuename,
                  ((dataformats::GeoID)geoid)((std::string)queuename))

ERS_DECLARE_ISSUE(readout,
                  CannotWriteToQueue,
                  "GeoID[" << geoid << "] Failed attempt to write to the queue: " << queuename
                           << ". Data will be lost!",
                  ((dataformats::GeoID)geoid)((std::string)queuename))

ERS_DECLARE_ISSUE(readout,
                  TrmWithEmptyFragment,
                  "GeoID[" << geoid << "] Trigger Matching result with empty fragment: " << trmdetails,
                  ((dataformats::GeoID)geoid)((std::string)trmdetails))

ERS_DECLARE_ISSUE(readout,
                  RequestOnEmptyBuffer,
                  "GeoID[" << geoid << "] Request on empty buffer: " << trmdetails,
                  ((dataformats::GeoID)geoid)((std::string)trmdetails))

ERS_DECLARE_ISSUE_BASE(readout,
                       FailedReadoutInitialization,
                       readout::InitializationError,
                       " Couldnt initialize Readout with current Init arguments " << initparams << ' ',
                       ((std::string)name),
                       ((std::string)initparams))

ERS_DECLARE_ISSUE(readout, FailedFakeCardInitialization, "Could not initialize fake card " << name, ((std::string)name))

ERS_DECLARE_ISSUE_BASE(readout,
                       NoImplementationAvailableError,
                       readout::ConfigurationError,
                       " No " << impl << " implementation available for raw type: " << rawt << ' ',
                       ((dataformats::GeoID)geoid)((std::string)impl),
                       ((std::string)rawt))

ERS_DECLARE_ISSUE(readout,
                  ResourceQueueError,
                  " The " << queueType << " queue was not successfully created for " << moduleName,
                  ((std::string)queueType)((std::string)moduleName))

ERS_DECLARE_ISSUE_BASE(readout,
                       DataRecorderResourceQueueError,
                       readout::DataRecorderConfigurationError,
                       " The " << queueType << " queue was not successfully created. ",
                       ((std::string)name),
                       ((std::string)queueType))

ERS_DECLARE_ISSUE(readout,
                  GenericResourceQueueError,
                  "The " << queueType << " queue was not successfully created for " << moduleName,
                  ((std::string)queueType)((std::string)moduleName))

ERS_DECLARE_ISSUE(readout, ConfigurationNote, "ConfigurationNote: " << text, ((std::string)name)((std::string)text))

ERS_DECLARE_ISSUE(readout,
                  ConfigurationProblem,
                  "GeoID[" << geoid << "] Configuration problem: " << text,
                  ((dataformats::GeoID)geoid)((std::string)text))

ERS_DECLARE_ISSUE(readout, RequestTimedOut, "GeoID[" << geoid << "] Request timed out", ((dataformats::GeoID)geoid))

ERS_DECLARE_ISSUE(readout,
                  EndOfRunEmptyFragment,
                  "GeoID[" << geoid << "] Empty fragment at the end of the run",
                  ((dataformats::GeoID)geoid))

} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_READOUTISSUES_HPP_
