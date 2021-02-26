// This is the application info schema used by the data link handler module.
// It describes the information object structure passed by the application 
// for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.readout.fakecardreaderinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("class_name", self.cl, "fakecardreaderinfo", doc="Info class name"),
       s.field("packets", self.uint8, 0, doc="Application name"), 
       s.field("new_packets", self.uint8, 0, doc="State"), 
   ], doc="Data link handler information information")
};

moo.oschema.sort_select(info) 
