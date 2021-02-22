local moo = import "moo.jsonnet";
local cmd = import "appfwk-cmd-make.jsonnet";

local qdict = {
  fake_link: cmd.qspec("fakelink-0", "FollySPSCQueue",  100000),
  tp_fake_link: cmd.qspec("fakelink-5", "FollySPSCQueue",  100000),
  time_sync: cmd.qspec("ts-sync-out", "FollyMPMCQueue",  1000),
  requests_in: cmd.qspec("requests-in", "FollyMPMCQueue",  1000),
  frags_out: cmd.qspec("frags-out", "FollyMPMCQueue",  1000),
};

local qspec_list = [
  qdict[xx]
  for xx in std.objectFields(qdict)
];

[
  cmd.init(qspec_list,
    [cmd.mspec("fake-source", "FakeCardReader", [
      cmd.qinfo("output", qdict.fake_link.inst, cmd.qdir.output),
      cmd.qinfo("tp_output", qdict.tp_fake_link.inst, cmd.qdir.output)
      ]),

      cmd.mspec("fake-handler", "DataLinkHandler", [
        cmd.qinfo("raw-input", qdict.fake_link.inst,   cmd.qdir.input),
        cmd.qinfo("timesync",  qdict.time_sync.inst,   cmd.qdir.output),
        cmd.qinfo("requests",  qdict.requests_in.inst, cmd.qdir.input),
        cmd.qinfo("fragments", qdict.frags_out.inst,   cmd.qdir.output)
        ]),
    ]
  ) {},
  
  cmd.conf(
    [
      cmd.mcmd("fake-source", {
        "link_ids": [0],
        "input_limit": 10485100,
        "rate_khz": 166,
        "raw_type": "wib",
        "data_filename": "/tmp/frames.bin",
        "queue_timeout_ms": 2000,
        "tp_enabled": "true",
        "tp_rate_khz": 66,
        "tp_data_filename": "/tmp/tp_frames.bin"
      }
      ),
      cmd.mcmd("fake-handler", {
        "raw_type": "wib",
        "fake_trigger_flag": 1,
        "source_queue_timeout_ms": 2000,
        "latency_buffer_size": 100000,
        "pop_limit_pct": 0.8,
        "pop_size_pct": 0.3,
        "apa_number": 1,
        "link_number": 2
      }
      )
    ]
  ) {},
    
  cmd.start(42) {},

  cmd.stop() {},
]
