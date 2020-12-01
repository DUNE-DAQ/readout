// The schema used by classes in the appfwk code tests.
//
// It is an example of the lowest layer schema below that of the "cmd"
// and "app" and which defines the final command object structure as
// consumed by instances of specific DAQModule implementations (ie,
// the test/Fake* modules).

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.readout.fakeelinkreader";
local s = moo.oschema.schema(ns);

// Object structure used by the test/fake producer module
local fakeelinkreader = {
    size  : s.number("Size", "u8",
                     doc="A count of very many things"),

    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    str : s.string("Str", "string",
                  doc="A string field"),

    conf: s.record("Conf", [
        s.field("link_id", self.count, 0,
                doc="Link ID"),

        s.field("input_limit", self.size, 10485100,
                doc="Maximum allowed file size"),

        s.field("rate_khz", self.count, 166,
                doc="Rate of ratelimiter"),

        s.field("raw_type", self.str, "wib",
                doc="User payload type"),

        s.field("data_filename", self.str, "/tmp/frames.bin",
                doc="Data file that contains user payloads"),

        s.field("queue_timeout_ms", self.count, 2000,
                doc="Queue timeout in milliseconds"),

    ], doc="Fake Elink reader module configuration"),

};

moo.oschema.sort_select(fakeelinkreader, ns)
