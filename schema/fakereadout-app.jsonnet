local moo = import "moo.jsonnet";
local cmd = import "appfwk-cmd-make.jsonnet";

local qdict = {
  fake_link: cmd.qspec("fakelink-0", "FollySPSCQueue",  100000),
};

local qspec_list = [
  qdict[xx]
  for xx in std.objectFields(qdict)
];

[
  cmd.init(qspec_list,
    [cmd.mspec("fake-source", "FakeCardReader",
      cmd.qinfo("output", qdict.fake_link.inst, cmd.qdir.output)),

      cmd.mspec("fake-handler", "DataLinkHandler",
        cmd.qinfo("input", qdict.fake_link.inst, cmd.qdir.input)),
    ]
  ) {},
  
  cmd.conf(
    [
      cmd.mcmd("fake-source", {
        "link_id": 0,
        "input_limit": 10485100,
        "rate_khz": 166,
        "raw_type": "wib",
        "data_filename": "/tmp/frames.bin",
        "queue_timeout_ms": 2000
      }
      ),
      cmd.mcmd("fake-handler", {
        "raw_type": "wib",
        "source_queue_timeout_ms": 2000,
        "latency_buffer_size": 100000,
        "pop_limit_pct": 0.8,
        "pop_size_pct": 0.3
      }
      )
    ]
  ) {},
    
  cmd.start(42) {},

  cmd.stop() {},
]
