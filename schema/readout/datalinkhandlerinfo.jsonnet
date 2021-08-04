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
       s.field("packets", self.uint8, 0, doc="Application name"), 
       s.field("new_packets", self.uint8, 0, doc="State"), 
       s.field("requests", self.uint8, 0, doc="Busy flag"), 
       s.field("new_requests", self.uint8, 0, doc="Error flag"),
       s.field("found_requested", self.uint8, 0, doc="Found requested"),
       s.field("bad_requested", self.uint8, 0, doc="Did not find requested"),
       s.field("request_window_too_old", self.uint8, 0, doc="Request data is already gone"),
       s.field("retry_request", self.uint8, 0, doc="Data is not completely there yet and request can be retried"),
       s.field("uncategorized_request", self.uint8, 0, doc="Request is uncategorized"),
       s.field("requests_timed_out", self.uint8, 0, doc="Timed out requests"),
       s.field("cleanups", self.uint8, 0, doc="Cleanups issued on the latency buffer"),
       s.field("overwritten_packet_count", self.uint8, 0, doc="Overwritten packets due to the latency buffer being full"),
       s.field("num_waiting_requests", self.uint8, 0, doc="Number of requests that are waiting to be processed"),
       s.field("sent_tps", self.uint8, 0, doc="Number of sent TPs"),
       s.field("sent_tpsets", self.uint8, 0, doc="Number of sent TPSets"),
       s.field("dropped_tps", self.uint8, 0, doc="Number of dropped TPs (because they were too old)"),
       s.field("tp_hit_rate", self.float8, 0, doc="TP hit rate in kHz"),
       s.field("consumed_packet_rate", self.float8, 0, doc="Rate of consumed packets"),
       s.field("raw_queue_timeouts", self.uint8, 0, doc="Raw queue timeouts")
   ], doc="Data link handler information information")
};

moo.oschema.sort_select(info) 
