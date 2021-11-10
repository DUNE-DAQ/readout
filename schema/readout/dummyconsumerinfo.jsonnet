// This is the application info schema used by the DummyConsumer module.
// It describes the information object structure passed by the application
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.dummyconsumerinfo");

local info = {
    uint8  : s.number("uint8", "u8",
                  doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("packets_processed", self.uint8, 0, doc="Number of packets processed")
   ], doc="Dummy consumer information")
};

moo.oschema.sort_select(info)