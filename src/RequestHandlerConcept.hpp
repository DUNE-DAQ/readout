/**
* @file RequestHandlerConcept.hpp RequestHandler base class 
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_REQUESTHANDLERCONCEPT_HPP_
#define UDAQ_READOUT_SRC_REQUESTHANDLERCONCEPT_HPP_

#include "dfmessages/DataRequest.hpp"
#include "dataformats/Fragment.hpp"

namespace dunedaq {
namespace readout {

class RequestHandlerConcept {

public:
  explicit RequestHandlerConcept() {}
  RequestHandlerConcept(const RequestHandlerConcept&)
    = delete; ///< RequestHandlerConcept is not copy-constructible
  RequestHandlerConcept& operator=(const RequestHandlerConcept&)
    = delete; ///< RequestHandlerConcept is not copy-assginable
  RequestHandlerConcept(RequestHandlerConcept&&)
    = delete; ///< RequestHandlerConcept is not move-constructible
  RequestHandlerConcept& operator=(RequestHandlerConcept&&)
    = delete; ///< RequestHandlerConcept is not move-assignable

  virtual void conf(const nlohmann::json& args) = 0;
  virtual void start(const nlohmann::json& args) = 0;
  virtual void stop(const nlohmann::json& args) = 0;

  // requests
  virtual void auto_cleanup_check() = 0;
  virtual void issue_request(dfmessages::DataRequest /*dr*/) = 0;

protected:
  // Result code of requests
  enum ResultCode { kFound = 0, kNotFound, kTooOld, kNotYet, kPass, kUnknown };

  // Request Result
  struct RequestResult {
    RequestResult(ResultCode rc, dfmessages::DataRequest dr) 
    : result_code(rc)
    , data_request(dr)
    { }
    ResultCode result_code;
    dfmessages::DataRequest data_request;
  };

  // Bookkeeping of OOB requests
  std::map<dfmessages::DataRequest, int> request_counter_;

  virtual RequestResult cleanup_request(dfmessages::DataRequest /*dr*/) = 0;
  virtual RequestResult data_request(dfmessages::DataRequest /*dr*/) = 0;
  virtual void executor() = 0;

private:

};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_REQUESTHANDLERCONCEPT_HPP_
