// The schema used by classes in the appfwk code tests.
//
// It is an example of the lowest layer schema below that of the "cmd"
// and "app" and which defines the final command object structure as
// consumed by instances of specific DAQModule implementations (ie,
// the test/Fake* modules).

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.readout.fakecardreader";
local s = moo.oschema.schema(ns);

// Object structure used by the test/fake producer module
local fakecardreader = {
    uint4  : s.number("uint4", "u4",
                     doc="An unsigned of 4 bytes"),

    khz  : s.number("khz", "f8",
                     doc="A frequency in kHz"),
  
    linkid : s.number("linkid", "i4",
                     doc="A count of not too many things"),

    linkvec : s.sequence("linkvec", self.linkid,
                     doc="A sequence of links"),

    raw_type : s.string("RawType", moo.re.ident,
                  doc="A string field"),

    filepath : s.string("FilePath", moo.re.hierpath,
                  doc="A string field"),

    conf: s.record("Conf", [
        s.field("link_ids", self.linkvec,
                doc="Link IDs"),

        s.field("input_limit", self.uint4, 10485100,
                doc="Maximum allowed file size"),

        s.field("rate_khz", self.khz, 166,
                doc="Rate of ratelimiter"),

        s.field("raw_type", self.raw_type, "wib",
                doc="User payload type"),

        s.field("data_filename", self.filepath, "/tmp/frames.bin",
                doc="Data file that contains user payloads"),

        s.field("queue_timeout_ms", self.uint4, 2000,
                doc="Queue timeout in milliseconds"),

    ], doc="Fake Elink reader module configuration"),

};

moo.oschema.sort_select(fakecardreader, ns)
