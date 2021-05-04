// This is the application info schema used by the DummyConsumer module.
// It describes the information object structure passed by the application
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.dummyconsumerinfo");

local info = {
    cl : s.string("class_s", moo.re.ident,
                  doc="A string field"),
    uint8  : s.number("uint8", "u8",
                  doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("class_name", self.cl, "dummyconsumerinfo", doc="Info class name"),
       s.field("packets_processed", self.uint8, 0, doc="Number of packets processed")
   ], doc="Dummy consumer information")
};

moo.oschema.sort_select(info)