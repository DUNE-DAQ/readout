// This is the application info schema used by the data link handler module.
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.datalinkhandlerinfo");

local info = {
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),
    float8 : s.number("float8", "f8",
                      doc="A float of 8 bytes"),

   info: s.record("Info", [
       s.field("sum_payloads",                  self.uint8,     0, doc="Total number of received payloads"),
       s.field("num_payloads",                  self.uint8,     0, doc="Number of received payloads"),
       s.field("sum_requests",                  self.uint8,     0, doc="Total number of received requests"),
       s.field("num_requests",                  self.uint8,     0, doc="Number of received requests"),
       s.field("num_requests_found",            self.uint8,     0, doc="Number of found requests"),
       s.field("num_requests_bad",              self.uint8,     0, doc="Number of bad requests"),
       s.field("num_requests_old_window",       self.uint8,     0, doc="Number of requests with data that is too old"),
       s.field("num_requests_delayed",          self.uint8,     0, doc="Number of delayed requests (data not there yet)"),
       s.field("num_requests_uncategorized",    self.uint8,     0, doc="Number of uncategorized requests"),
       s.field("num_requests_timed_out",        self.uint8,     0, doc="Number of timed out requests"),
       s.field("num_requests_waiting",          self.uint8,     0, doc="Number of waiting requests"),
       s.field("num_buffer_cleanups",           self.uint8,     0, doc="Number of latency buffer cleanups"),
       s.field("num_payloads_overwritten",      self.uint8,     0, doc="Number of overwritten payloads because the LB is full"),
       s.field("num_tps_sent",                  self.uint8,     0, doc="Number of sent TPs"),
       s.field("num_tpsets_sent",               self.uint8,     0, doc="Number of sent TPSets"),
       s.field("num_tps_dropped",               self.uint8,     0, doc="Number of dropped TPs (because they were too old)"),
       s.field("rate_tp_hits",                  self.float8,    0, doc="TP hit rate in kHz"),
       s.field("rate_payloads_consumed",        self.float8,    0, doc="Rate of consumed packets"),
       s.field("num_raw_queue_timeouts",        self.uint8,     0, doc="Raw queue timeouts"),
       s.field("avg_request_response_time",     self.uint8,     0, doc="Average response time in us")
   ], doc="Data link handler information information")
};

moo.oschema.sort_select(info) 
