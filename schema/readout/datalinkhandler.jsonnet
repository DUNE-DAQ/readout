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

    choice : s.boolean("Choice"),

    file_name : s.string("FileName", moo.re.ident,
                      doc="A string field"),

    conf: s.record("Conf", [
        s.field("emulator_mode", self.choice, false,
                doc="If the input data is from an emulator."),
        s.field("source_queue_timeout_ms", self.count, 2000,
                doc="Timeout for source queue"),
        s.field("fake_trigger_flag", self.count, 0,
                doc="flag indicating whether to generate fake triggers: 1=true, 0=false "),
        s.field("latency_buffer_size", self.size, 100000,
                doc="Size of latency buffer"),
        s.field("pop_limit_pct", self.pct, 0.5,
                doc="Latency buffer occupancy percentage to issue an auto-pop"),
        s.field("pop_size_pct", self.pct, 0.8,
                doc="Percentage of current occupancy to pop from the latency buffer"),
        s.field("apa_number", self.apa_number, 0,
                doc="The APA number of this link"),
        s.field("link_number", self.link_number, 0,
                doc="The link number of this link"),
        s.field("num_request_handling_threads", self.count, 1,
                doc="Number of threads to use for data request handling"),
        s.field("postprocess_queue_sizes", self.size, 10000,
                doc="Size of the queues used for postprocessing"),
        s.field("tp_timeout", self.size, 100000,
                doc="Timeout after which ongoing TPs are discarded"),
        s.field("tpset_window_size", self.size, 10000,
                doc="Size of the TPSet windows"),
        s.field("channel_map_rce", self.file_name, "/tmp/protoDUNETPCChannelMap_RCE_v4.txt",
                doc="Channel map rce file for software TPG"),
        s.field("channel_map_felix", self.file_name, "/tmp/protoDUNETPCChannelMap_FELIX_v4.txt",
                        doc="Channel map felix file for software TPG")
    ], doc="Generic readout element configuration"),

    recording: s.record("RecordingParams", [
        s.field("duration", self.count, 1,
                doc="Number of seconds to record")
    ], doc="Recording parameters"),

};

moo.oschema.sort_select(datalinkhandler, ns)

