// This is the application info schema used by the DataRecorder module.
// It describes the information object structure passed by the application
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.datarecorderinfo");

local info = {
   uint8  : s.number("uint8", "u8",
                  doc="An unsigned of 8 bytes"),
   float8 : s.number("float8", "f8",
                  doc="A float of 8 bytes"),

   info: s.record("Info", [
       s.field("packets_processed", self.uint8, 0, doc="Number of packets processed"),
       s.field("throughput_processed_packets", self.float8, 0, doc="Throughput of processed packets"),
   ], doc="Data link handler information information")
};

moo.oschema.sort_select(info)