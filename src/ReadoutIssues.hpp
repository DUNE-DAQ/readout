/**
 * @file ReadoutIssues.hpp Readout system related ERS issues
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef UDAQ_READOUT_SRC_READOUTISSUES_HPP_
#define UDAQ_READOUT_SRC_READOUTISSUES_HPP_

#include <ers/Issue.h>

#include <string>

namespace dunedaq {

    ERS_DECLARE_ISSUE(readout, FelixError,
                      " FELIX Error: " << flxerror,
                      ((std::string)flxerror))

    ERS_DECLARE_ISSUE(readout, ConfigurationError,
                      " Readout Configuration Error: " << conferror,
                      ((std::string)conferror)) 

    ERS_DECLARE_ISSUE(readout, QueueTimeoutError,
                      " Readout queue timed out: " << queuename,
                      ((std::string)queuename))

    ERS_DECLARE_ISSUE_BASE(readout,
                           NoImplementationAvailableError,
                           readout::ConfigurationError,
                           " No " << impl << " implementation available for raw type: " << rawt << ' ',
                           ((std::string)impl),
                           ((std::string)rawt))

    ERS_DECLARE_ISSUE_BASE(readout,
                           ResourceQueueError,
                           readout::ConfigurationError,
                           " The " << queueType << " queue was not successfully created. ",
                           ((std::string)name),
                           ((std::string)queueType))

} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTISSUES_HPP_
