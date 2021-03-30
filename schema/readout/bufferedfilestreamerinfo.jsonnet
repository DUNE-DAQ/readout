// This is the application info schema used by the buffered file streamer module.
// It describes the information object structure passed by the application
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.bufferedfilestreamerinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"),
   uint8  : s.number("uint8", "u8",
                  doc="An unsigned of 8 bytes"),
   float8 : s.number("float8", "f8",
                  doc="A float of 8 bytes"),

   info: s.record("Info", [
       s.field("class_name", self.cl, "bufferedfilestreamerinfo", doc="Info class name"),
       s.field("packets_processed", self.uint8, 0, doc="Number of packets processed"),
       s.field("bytes_written", self.uint8, 0, doc="Number of bytes written to disk"),
       s.field("throughput_processed_packets", self.float8, 0, doc="Throughput of processed packets"),
       s.field("throughput_to_file", self.float8, 0, doc="Throughput of bytes written to file")
   ], doc="Data link handler information information")
};

moo.oschema.sort_select(info)