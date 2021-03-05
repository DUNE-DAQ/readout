/**
* @file ReadoutLogging.hpp Common logging declarations in udaq-readout
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_INCLUDE_READOUT_READOUTLOGGING_HPP_
#define READOUT_INCLUDE_READOUT_READOUTLOGGING_HPP_

namespace dunedaq {
namespace readout {
namespace logging {

/**
 * @brief Common name used by TRACE TLOG calls from this package
*/
enum
{
  TLVL_HOUSEKEEPING = 1,
  TLVL_TAKE_NOTE = 2,
  TLVL_ENTER_EXIT_METHODS = 5,
  TLVL_WORK_STEPS = 10,
  TLVL_BOOKKEEPING = 15
};

} // namespace types
} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_READOUTLOGGING_HPP_
