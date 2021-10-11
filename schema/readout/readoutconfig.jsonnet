// The schema used by classes in the appfwk code tests.
//
// It is an example of the lowest layer schema below that of the "cmd"
// and "app" and which defines the final command object structure as
// consumed by instances of specific DAQModule implementations (ie,
// the test/Fake* modules).

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.readout.readoutconfig";
local s = moo.oschema.schema(ns);

// Object structure used by the test/fake producer module
local readoutconfig = {
    size: s.number("Size", "u8",
                   doc="A count of very many things"),

    region_id: s.number("RegionID", "u4",
                         doc="A region id (as part of a GeoID)"),

    element_id: s.number("ElementID", "u4",
                        doc="An element id (as part of a GeoID)"),

    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    pct : s.number("Percent", "f4",
                   doc="Testing float number"),

    choice : s.boolean("Choice"),

    file_name : s.string("FileName",
                      doc="A string field"),

    string : s.string("String", moo.re.ident,
                      doc="A string field"),

    latencybufferconf : s.record("LatencyBufferConf", [
            s.field("latency_buffer_size", self.size, 100000,
                            doc="Size of latency buffer"),
            s.field("latency_buffer_numa_aware", self.choice, false,
                            doc="Use numa allocation for LB"),
            s.field("latency_buffer_numa_node", self.count, 0,
                            doc="NUMA node to use for allocation if latency_buffer_numa_aware is set to true"),
            s.field("latency_buffer_intrinsic_allocator", self.choice, false,
                            doc="Use intrinsic allocator for LB"),
            s.field("latency_buffer_alignment_size", self.count, 0,
                            doc="Alignment size of LB allocation"),
            s.field("latency_buffer_preallocation", self.choice, false,
                            doc="Preallocate memory for the latency buffer"),
            s.field("region_id", self.region_id, 0,
                            doc="The region id of this link"),
            s.field("element_id", self.element_id, 0,
                            doc="The element id of this link")],
            doc="Latency Buffer Config"),

    rawdataprocessorconf : s.record("RawDataProcessorConf", [
            s.field("postprocess_queue_sizes", self.size, 10000,
                            doc="Size of the queues used for postprocessing"),
            s.field("tp_timeout", self.size, 100000,
                            doc="Timeout after which ongoing TPs are discarded"),
            s.field("tpset_window_size", self.size, 10000,
                            doc="Size of the TPSet windows"),
            s.field("channel_map_rce", self.file_name, "",
                            doc="Channel map rce file for software TPG. If empty string, look in $READOUT_SHARE"),
            s.field("channel_map_felix", self.file_name, "",
                            doc="Channel map felix file for software TPG. If empty string, look in $READOUT_SHARE"),
            s.field("enable_software_tpg", self.choice, false,
                            doc="Enable software TPG"),
            s.field("emulator_mode", self.choice, false,
                            doc="If the input data is from an emulator."),
            s.field("region_id", self.region_id, 0,
                            doc="The region id of this link"),
            s.field("element_id", self.element_id, 0,
                            doc="The element id of this link"),
            s.field("max_queued_errored_frames", self.size, 100,
                            doc="Maximum number of frames queued per error type")],
            doc="RawDataProcessor Config"),

    requesthandlerconf : s.record("RequestHandlerConf", [
            s.field("num_request_handling_threads", self.count, 4,
                            doc="Number of threads to use for data request handling"),
            s.field("retry_count", self.count, 100,
                            doc="Number of times to recheck a request before sending an empty fragment"),
            s.field("output_file", self.file_name, "output.out",
                            doc="Name of the output file to write to"),
            s.field("stream_buffer_size", self.size, 8388608,
                            doc="Buffer size of the stream buffer"),
            s.field("compression_algorithm", self.string, "None",
                            doc="Compression algorithm to use before writing to file"),
            s.field("use_o_direct", self.choice, true,
                            doc="Whether to use O_DIRECT flag when opening files"),
            s.field("enable_raw_recording", self.choice, true,
                            doc="Enable raw recording"),
            s.field("fragment_queue_timeout_ms", self.count, 100,
                            doc="Timeout for pushing to the fragment queue"),
            s.field("pop_limit_pct", self.pct, 0.5,
                            doc="Latency buffer occupancy percentage to issue an auto-pop"),
            s.field("pop_size_pct", self.pct, 0.8,
                            doc="Percentage of current occupancy to pop from the latency buffer"),
            s.field("latency_buffer_size", self.size, 100000,
                            doc="Size of latency buffer"),
            s.field("region_id", self.region_id, 0,
                            doc="The region id of this link"),
            s.field("element_id", self.element_id, 0,
                            doc="The element id of this link")],
            doc="Request Handler Config"),

    readoutmodelconf : s.record("ReadoutModelConf", [
            s.field("fake_trigger_flag", self.count, 0,
                            doc="flag indicating whether to generate fake triggers: 1=true, 0=false "),
            s.field("source_queue_timeout_ms", self.count, 2000,
                            doc="Timeout for source queue"),
            s.field("region_id", self.region_id, 0,
                            doc="The APA number of this link"),
            s.field("element_id", self.element_id, 0,
                            doc="The link number of this link")
    ], doc="Readout Model Config"),

    conf: s.record("Conf", [
        s.field("latencybufferconf", self.latencybufferconf, doc="Latency Buffer config"),
        s.field("rawdataprocessorconf", self.rawdataprocessorconf, doc="RawDataProcessor config"),
        s.field("requesthandlerconf", self.requesthandlerconf, doc="Request Handler config"),
        s.field("readoutmodelconf", self.readoutmodelconf, doc="Readout Model config")

    ], doc="Generic readout element configuration"),

    recording: s.record("RecordingParams", [
        s.field("duration", self.count, 1,
                doc="Number of seconds to record")
    ], doc="Recording parameters"),

};

moo.oschema.sort_select(readoutconfig, ns)

