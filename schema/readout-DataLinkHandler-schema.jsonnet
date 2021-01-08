// The schema used by classes in the appfwk code tests.
//
// It is an example of the lowest layer schema below that of the "cmd"
// and "app" and which defines the final command object structure as
// consumed by instances of specific DAQModule implementations (ie,
// the test/Fake* modules).

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.readout.datalinkhandler";
local s = moo.oschema.schema(ns);

// Object structure used by the test/fake producer module
local datalinkhandler = {
    size: s.number("Size", "u8",
                   doc="A count of very many things"),

    apa_number: s.number("APANumber", "u4",
                         doc="An APA number"),

    link_number: s.number("LinkNumber", "u4",
                        doc="A link number"),

    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    pct : s.number("Percent", "f4",
                   doc="Testing float number"),

    str : s.string("Str", "string",
                   doc="A string field"),

    conf: s.record("Conf", [
        s.field("raw_type", self.str, "wib",
                doc="Raw type"),
        s.field("source_queue_timeout_ms", self.count, 2000,
                doc="Timeout for source queue"),
        s.field("latency_buffer_size", self.size, 100000,
                doc="Size of latency buffer"),
        s.field("pop_limit_pct", self.pct, 0.5,
                doc="Latency buffer occupancy percentage to issue an auto-pop"),
        s.field("pop_size_pct", self.pct, 0.8,
                doc="Percentage of current occupancy to pop from the latency buffer"),
        s.field("apa_number", self.apa_number, 0,
                doc="The APA number of this link"),
        s.field("link_number", self.link_number, 0,
                doc="The link number of this link")
    ], doc="Generic readout element configuration"),

};

moo.oschema.sort_select(datalinkhandler, ns)

