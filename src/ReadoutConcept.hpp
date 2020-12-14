/**
* @file ReadoutConcept.hpp ReadoutConcept for constructors and
* forwarding command args.
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_READOUTCONCEPT_HPP_
#define UDAQ_READOUT_SRC_READOUTCONCEPT_HPP_

namespace dunedaq {
namespace readout {

class ReadoutConcept {
public:
  explicit ReadoutConcept() {}
  ReadoutConcept(const ReadoutConcept&)
    = delete; ///< ReadoutConcept is not copy-constructible
  ReadoutConcept& operator=(const ReadoutConcept&)
    = delete; ///< ReadoutConcept is not copy-assginable
  ReadoutConcept(ReadoutConcept&&)
    = delete; ///< ReadoutConcept is not move-constructible
  ReadoutConcept& operator=(ReadoutConcept&&)
    = delete; ///< ReadoutConcept is not move-assignable

  virtual void conf(const nlohmann::json& args) = 0;
  virtual void start(const nlohmann::json& args) = 0;
  virtual void stop(const nlohmann::json& args) = 0;

  virtual void consume() = 0;

private:

};

} // namespace readout
} // namespace dunedaq

#endif // UDAQ_READOUT_SRC_READOUTCONCEPT_HPP_
