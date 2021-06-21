/**
 * @file RequestHandlerConcept.hpp RequestHandler interface class
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_CONCEPTS_REQUESTHANDLERCONCEPT_HPP_
#define READOUT_INCLUDE_READOUT_CONCEPTS_REQUESTHANDLERCONCEPT_HPP_

#include "dataformats/Fragment.hpp"
#include "dfmessages/DataRequest.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>

namespace dunedaq {
namespace readout {

template<class RawType, class LatencyBufferType>
class RequestHandlerConcept
{

public:
  RequestHandlerConcept() {}
  RequestHandlerConcept(const RequestHandlerConcept&) = delete; ///< RequestHandlerConcept is not copy-constructible
  RequestHandlerConcept& operator=(const RequestHandlerConcept&) =
    delete;                                                ///< RequestHandlerConcept is not copy-assginable
  RequestHandlerConcept(RequestHandlerConcept&&) = delete; ///< RequestHandlerConcept is not move-constructible
  RequestHandlerConcept& operator=(RequestHandlerConcept&&) = delete; ///< RequestHandlerConcept is not move-assignable

  virtual void conf(const nlohmann::json& args) = 0;
  virtual void start(const nlohmann::json& args) = 0;
  virtual void stop(const nlohmann::json& args) = 0;
  virtual void record(const nlohmann::json& args) = 0;
  virtual void get_info(datalinkhandlerinfo::Info&) = 0;

  // requests
  virtual void cleanup_check() = 0;
  virtual void issue_request(dfmessages::DataRequest /*dr*/) = 0;

protected:
  // Result code of requests
  enum ResultCode
  {
    kFound = 0,
    kNotFound,
    kTooOld,
    kNotYet,
    kPass,
    kCleanup,
    kUnknown
  };
  std::map<ResultCode, std::string> ResultCodeStrings{
    { ResultCode::kFound, "FOUND" },    { ResultCode::kNotFound, "NOT_FOUND" },
    { ResultCode::kTooOld, "TOO_OLD" }, { ResultCode::kNotYet, "NOT_YET_PRESENT" },
    { ResultCode::kPass, "PASSED" },    { ResultCode::kCleanup, "CLEANUP" },
    { ResultCode::kUnknown, "UNKNOWN" }
  };

  inline const std::string& resultCodeAsString(ResultCode rc) { return ResultCodeStrings[rc]; }

  // Request Result
  struct RequestResult
  {
    RequestResult(ResultCode rc, dfmessages::DataRequest dr)
      : result_code(rc)
      , data_request(dr)
      , fragment()
    {}
    RequestResult(ResultCode rc, dfmessages::DataRequest dr, dataformats::Fragment&& frag)
      : result_code(rc)
      , data_request(dr)
      , fragment(std::move(frag))
    {}
    ResultCode result_code;
    dfmessages::DataRequest data_request;
    std::unique_ptr<dataformats::Fragment> fragment;
  };

  // Bookkeeping of OOB requests
  std::map<dfmessages::DataRequest, int> m_request_counter;

  virtual void cleanup() = 0;
  virtual RequestResult data_request(dfmessages::DataRequest /*dr*/) = 0;

private:
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_CONCEPTS_REQUESTHANDLERCONCEPT_HPP_
