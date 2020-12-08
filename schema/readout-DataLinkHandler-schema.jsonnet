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

    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    str : s.string("Str", "string",
                   doc="A string field"),

    conf: s.record("Conf", [
        s.field("raw_type", self.str, "wib",
                doc="Raw type"),
    ], doc="Generic readout element configuration"),

};

moo.oschema.sort_select(datalinkhandler, ns)

