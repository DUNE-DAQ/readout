// The schema used by classes in the appfwk code tests.
//
// It is an example of the lowest layer schema below that of the "cmd"
// and "app" and which defines the final command object structure as
// consumed by instances of specific DAQModule implementations (ie,
// the test/Fake* modules).

local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.readout.sourceemulatorconfig";
local s = moo.oschema.schema(ns);

// Object structure used by the test/fake producer module
local sourceemulatorconfig = {
    uint4  : s.number("uint4", "u4",
                     doc="An unsigned of 4 bytes"),

    int8  : s.number("int8", "i8",
                     doc="integer of 8 bytes"),

    khz  : s.number("khz", "f8",
                     doc="A frequency in kHz"),

    double8 : s.number("double8", "f8",
                     doc="floating point of 8 bytes"),

    slowdown_t : s.number("slowdown_t", "f8",
                     doc="Slowdown factor"),
  
    region_id : s.number("region_id", "u2"),
    element_id : s.number("element_id", "u4"),
    system_type : s.string("system_type"),

    geoid : s.record("GeoID", [s.field("region", self.region_id, doc="" ),
        s.field("element", self.element_id, doc="" ),
        s.field("system", self.system_type, doc="" )],
        doc="GeoID"),

    linkvec : s.sequence("link_vec", self.geoid),

    string : s.string("FilePath", moo.re.hierpath,
                  doc="A string field"),

    choice : s.boolean("Choice"),

    tp_enabled: s.string("TpEnabled", moo.re.ident,
                  doc="A true or false flag for enabling raw WIB TP link"),

    link_conf : s.record("LinkConfiguration", [
        s.field("geoid", self.geoid, doc="GeoID of the link"),
        s.field("input_limit", self.uint4, 10485100,
            doc="Maximum allowed file size"),
        s.field("slowdown", self.slowdown_t, 1,
            doc="Slowdown factor"),
        s.field("data_filename", self.string, "/tmp/frames.bin",
            doc="Data file that contains user payloads"),
        s.field("queue_name", self.string,
            doc="Name of the output queue"),
        s.field("random_population_size", self.uint4, 10000,
            doc="Size of the random population"),
        s.field("emu_frame_error_rate", self.double8, 0.0,
            doc="Rate of emulated errors in frame header"),
        ], doc="Configuration for one link"),

    link_conf_list : s.sequence("link_conf_list", self.link_conf, doc="Link configuration list"),

    conf: s.record("Conf", [
        s.field("link_confs", self.link_conf_list,
                doc="Link configurations"),

        s.field("queue_timeout_ms", self.uint4, 2000,
                doc="Queue timeout in milliseconds"),
        
        s.field("set_t0_to", self.int8, -1,
                doc="The first DAQ timestamp. If -1, t0 from file is used.")

    ], doc="Fake Elink reader module configuration"),
};

moo.oschema.sort_select(sourceemulatorconfig, ns)
