// This is the application info schema used by the data link handler module.
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.fakecardreaderinfo");

local info = {
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("packets", self.uint8, 0, doc="Application name"), 
       s.field("new_packets", self.uint8, 0, doc="State"), 
   ], doc="Data link handler information information")
};

moo.oschema.sort_select(info) 
