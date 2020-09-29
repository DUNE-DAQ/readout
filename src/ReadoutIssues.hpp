#ifndef APPFWK_UDAQ_READOUT_READOUTISSUES_HPP_
#define APPFWK_UDAQ_READOUT_READOUTISSUES_HPP_

#include "ers/ers.h"
#include <string>

namespace dunedaq {
namespace appfwk {

/**
 * @brief Readout unit specific issues
 * @author Roland Sipos
 * @date   2020-2021
 *
 * */

    ERS_DECLARE_ISSUE(readout, FelixError,
                      "FELIX Error: " << flxerror,
                      ((std::string)flxerror))

    ERS_DECLARE_ISSUE(readout, ConfigurationError,
                      "Readout Configuration Error: " << conferror,
                      ((std::string)conferror)) 

}
}

#endif
