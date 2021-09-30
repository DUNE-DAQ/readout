# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('cmdlib/cmd.jsonnet')
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')
moo.otypes.load_types('readout/fakecardreader.jsonnet')
moo.otypes.load_types('readout/readoutconfig.jsonnet')
moo.otypes.load_types('readout/datarecorder.jsonnet')
moo.otypes.load_types('nwqueueadapters/queuetonetwork.jsonnet')
moo.otypes.load_types('nwqueueadapters/networkobjectsender.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.readout.fakecardreader as fcr
import dunedaq.readout.readoutconfig as rconf
import dunedaq.readout.datarecorder as bfs
import dunedaq.nwqueueadapters.queuetonetwork as qton
import dunedaq.nwqueueadapters.networkobjectsender as nos

from appfwk.utils import mcmd, mrccmd, mspec

import json
import math
# Time to waait on pop()
QUEUE_POP_WAIT_MS=100;
# local clock speed Hz
CLOCK_SPEED_HZ = 50000000;

def generate(
        FRONTEND_TYPE='wib',
        NUMBER_OF_DATA_PRODUCERS=1,          
        NUMBER_OF_TP_PRODUCERS=1,          
        DATA_RATE_SLOWDOWN_FACTOR = 1,
        ENABLE_SOFTWARE_TPG=False,
        RUN_NUMBER = 333, 
        DATA_FILE="./frames.bin"
    ):

    # Define modules and queues
    queue_bare_specs = [
            app.QueueSpec(inst="time_sync_q", kind='FollyMPMCQueue', capacity=100),
            app.QueueSpec(inst="data_fragments_q", kind='FollyMPMCQueue', capacity=100),
        ] + [
            app.QueueSpec(inst=f"data_requests_{idx}", kind='FollySPSCQueue', capacity=1000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
            app.QueueSpec(inst=f"{FRONTEND_TYPE}_link_{idx}", kind='FollySPSCQueue', capacity=100000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
            app.QueueSpec(inst=f"tp_link_{idx}", kind='FollySPSCQueue', capacity=100000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS, NUMBER_OF_DATA_PRODUCERS+NUMBER_OF_TP_PRODUCERS)
        ] + [
            app.QueueSpec(inst=f"tp_queue_{idx}", kind='FollySPSCQueue', capacity=100000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
            app.QueueSpec(inst=f"tp_data_requests", kind='FollySPSCQueue', capacity=1000)
        ] + [
            app.QueueSpec(inst="tp_recording_link", kind='FollySPSCQueue', capacity=1000)
        ] + [
            app.QueueSpec(inst=f"tpset_link_{idx}", kind='FollySPSCQueue', capacity=10000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ]
    

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [
        mspec("fake_source", "FakeCardReader", [
                        app.QueueInfo(name=f"output_{idx}", inst=f"{FRONTEND_TYPE}_link_{idx}", dir="output")
                            for idx in range(NUMBER_OF_DATA_PRODUCERS)
                        ] + [
                        app.QueueInfo(name=f"output_{idx}", inst=f"tp_link_{idx}", dir="output")
                            for idx in range(NUMBER_OF_DATA_PRODUCERS, NUMBER_OF_DATA_PRODUCERS+NUMBER_OF_TP_PRODUCERS)
        ]),
        ] + [
                mspec(f"datahandler_{idx}", "DataLinkHandler", [
                            app.QueueInfo(name="raw_input", inst=f"{FRONTEND_TYPE}_link_{idx}", dir="input"),
                            app.QueueInfo(name="timesync", inst="time_sync_q", dir="output"),
                            app.QueueInfo(name="data_requests_0", inst=f"data_requests_{idx}", dir="input"),
                            app.QueueInfo(name="data_response_0", inst="data_fragments_q", dir="output"),
                            app.QueueInfo(name="raw_recording", inst=f"{FRONTEND_TYPE}_recording_link_{idx}", dir="output"),
                            app.QueueInfo(name="tp_out", inst=f"tp_queue_{idx}", dir="output"),
                            app.QueueInfo(name="tpset_out", inst=f"tpset_link_{idx}", dir="output")
                ]) for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
                mspec(f"timesync_consumer", "TimeSyncConsumer", [
                                            app.QueueInfo(name="input_queue", inst=f"time_sync_q", dir="input")
                                            ])
        ] + [
                mspec(f"fragment_consumer", "FragmentConsumer", [
                                            app.QueueInfo(name="input_queue", inst=f"data_fragments_q", dir="input")
                                            ])
        ] + [
                mspec(f"tp_handler_{idx}", "DataLinkHandler", [
                                            app.QueueInfo(name="raw_input", inst=f"tp_queue_{idx}", dir="input"),
                                            app.QueueInfo(name="timesync", inst="time_sync_q", dir="output"),
                                            app.QueueInfo(name="requests", inst="tp_data_requests", dir="input"),
                                            app.QueueInfo(name="fragments", inst="data_fragments_q", dir="output"),
                                            app.QueueInfo(name="raw_recording", inst="tp_recording_link", dir="output")
                                            ]) for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
                mspec(f"tpset_publisher_{idx}", "QueueToNetwork", [
                                            app.QueueInfo(name="input", inst=f"tpset_link_{idx}", dir="input")
                                            ]) for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ]

    init_specs = app.Init(queues=queue_specs, modules=mod_specs)

    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)

    initcmd = rccmd.RCCommand(
        id=basecmd.CmdId("init"),
        entry_state="NONE",
        exit_state="INITIAL",
        data=init_specs
    )


    confcmd = mrccmd("conf", "INITIAL", "CONFIGURED", [
                ("fake_source",fcr.Conf(
                            link_confs=[fcr.LinkConfiguration(
                                geoid=fcr.GeoID(system="TPC", region=0, element=idx),
                                slowdown=DATA_RATE_SLOWDOWN_FACTOR,
                                queue_name=f"output_{idx}",
                                data_filename=DATA_FILE
                            ) for idx in range(NUMBER_OF_DATA_PRODUCERS)]
                            + [fcr.LinkConfiguration(
                                geoid=fcr.GeoID(system="TPC", region=0, element=idx),
                                slowdown=DATA_RATE_SLOWDOWN_FACTOR,
                                queue_name=f"output_{idx}",
                            ) for idx in range(NUMBER_OF_DATA_PRODUCERS, NUMBER_OF_DATA_PRODUCERS+NUMBER_OF_TP_PRODUCERS)],
                            # input_limit=10485100, # default
                            queue_timeout_ms = QUEUE_POP_WAIT_MS,
			                set_t0_to = 0
                        )),
            ] + [
                (f"datahandler_{idx}", rconf.Conf(
                        readoutmodelconf= rconf.ReadoutModelConf(
                            source_queue_timeout_ms= QUEUE_POP_WAIT_MS,
                            fake_trigger_flag=1,
                            apa_number = 0,
                            link_number = idx,
                        ),
                        latencybufferconf= rconf.LatencyBufferConf(
                            latency_buffer_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR),
                            apa_number = 0,
                            link_number = idx,
                        ),
                        rawdataprocessorconf= rconf.RawDataProcessorConf(
                            apa_number = 0,
                            link_number = idx,
                            enable_software_tpg = ENABLE_SOFTWARE_TPG,
                        ),
                        requesthandlerconf= rconf.RequestHandlerConf(
                            latency_buffer_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR),
                            pop_limit_pct = 0.8,
                            pop_size_pct = 0.1,
                            apa_number = 0,
                            link_number = idx,
                            output_file = f"/mnt/md0/output_{idx}.out",
                            stream_buffer_size = 8388608,
                            enable_raw_recording = True
                        )
                        )) for idx in range(NUMBER_OF_DATA_PRODUCERS)
            ] + [
                (f"tp_handler_{idx}", rconf.Conf(
                    readoutmodelconf= rconf.ReadoutModelConf(
                        source_queue_timeout_ms= QUEUE_POP_WAIT_MS,
                        fake_trigger_flag=1,
                        apa_number = 0,
                        link_number = idx,
                    ),
                    latencybufferconf= rconf.LatencyBufferConf(
                        latency_buffer_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR),
                        apa_number = 0,
                        link_number = idx,
                    ),
                    rawdataprocessorconf= rconf.RawDataProcessorConf(
                        apa_number = 0,
                        link_number = idx,
                        enable_software_tpg = ENABLE_SOFTWARE_TPG,
                    ),
                    requesthandlerconf= rconf.RequestHandlerConf(
                        latency_buffer_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR),
                        pop_limit_pct = 0.8,
                        pop_size_pct = 0.1,
                        apa_number = 0,
                        link_number = idx,
                        output_file = f"/mnt/md0/output_{idx}.out",
                        stream_buffer_size = 8388608,
                        enable_raw_recording = False
                    )
                )) for idx in range(NUMBER_OF_DATA_PRODUCERS)
            ] + [
                (f"tpset_publisher_{idx}", qton.Conf(msg_type="dunedaq::trigger::TPSet",
                           msg_module_name="TPSetNQ",
                           sender_config=nos.Conf(ipm_plugin_type="ZmqPublisher",
                                                  address='tcp://127.0.0.1:' + str(5000 + idx),
                                                  topic="foo",
                                                  stype="msgpack")
                           )
         ) for idx in range(NUMBER_OF_DATA_PRODUCERS)
    ])
    
    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = rccmd.StartParams(run=RUN_NUMBER)
    startcmd = mrccmd("start", "CONFIGURED", "RUNNING", [
            ("datahandler_.*", startpars),
            ("fake_source", startpars),
            ("data_recorder_.*", startpars),
            ("timesync_consumer", startpars),
            ("fragment_consumer", startpars),
            ("tp_handler_.*", startpars),
            ("tpset_publisher_.*", startpars)
        ])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)


    stopcmd = mrccmd("stop", "RUNNING", "CONFIGURED", [
            ("fake_source", None),
            ("datahandler_.*", None),
            ("data_recorder_.*", None),
            ("timesync_consumer", None),
            ("fragment_consumer", None),
            ("tp_handler_.*", None),
            ("tpset_publisher_.*", None)
        ])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)

    scrapcmd = mrccmd("scrap", "CONFIGURED", "INITIAL", [
            ("", None)
        ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd, scrapcmd]

    record_cmd = mrccmd("record", "RUNNING", "RUNNING", [
        ("datahandler_.*", rconf.RecordingParams(
            duration=10
        ))
    ])

    jstr = json.dumps(record_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nRecord\n\n", jstr)

    cmd_seq.append(record_cmd)

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr
        
if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-f', '--frontend-type', type=click.Choice(['wib', 'wib2', 'pds_queue', 'pds_list'], case_sensitive=True), default='wib')
    @click.option('-n', '--number-of-data-producers', default=1)
    @click.option('-t', '--number-of-tp-producers', default=0)
    @click.option('-s', '--data-rate-slowdown-factor', default=10)
    @click.option('-g', '--enable-software-tpg', is_flag=True)
    @click.option('-r', '--run-number', default=333)
    @click.option('-d', '--data-file', type=click.Path(), default='./frames.bin')
    # @click.option('--tp-data-file') TBA
    @click.argument('json_file', type=click.Path(), default='fake_readout.json')
    def cli(frontend_type, number_of_data_producers, number_of_tp_producers, data_rate_slowdown_factor, enable_software_tpg, run_number, data_file, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    FRONTEND_TYPE = frontend_type,
                    NUMBER_OF_DATA_PRODUCERS = number_of_data_producers,
                    NUMBER_OF_TP_PRODUCERS = number_of_tp_producers,
                    DATA_RATE_SLOWDOWN_FACTOR = data_rate_slowdown_factor,
                    ENABLE_SOFTWARE_TPG = enable_software_tpg,
                    RUN_NUMBER = run_number, 
                    DATA_FILE = data_file,
                ))

        print(f"'{json_file}' generation completed.")

    cli()
    
