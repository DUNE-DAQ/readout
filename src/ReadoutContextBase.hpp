/**
* @file ReadoutContextBase.hpp ReadoutContext base for constructors and
* forwarding command args.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTCONTEXTBASE_HPP_
#define UDAQ_READOUT_SRC_READOUTCONTEXTBASE_HPP_

namespace dunedaq {
namespace readout {

class ReadoutContextBase {
public:
  explicit ReadoutContextBase() {}
  ReadoutContextBase(const ReadoutContextBase&)
    = delete; ///< ReadoutContextBase is not copy-constructible
  ReadoutContextBase& operator=(const ReadoutContextBase&)
    = delete; ///< ReadoutContextBase is not copy-assginable
  ReadoutContextBase(ReadoutContextBase&&)
    = delete; ///< ReadoutContextBase is not move-constructible
  ReadoutContextBase& operator=(ReadoutContextBase&&)
    = delete; ///< ReadoutContextBase is not move-assignable

  virtual void conf(const nlohmann::json& cfg) = 0;
  virtual void start(const nlohmann::json& args) = 0;
  virtual void stop(const nlohmann::json& args) = 0;

private:

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTCONTEXTBASE_HPP_
