# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes

moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')

moo.otypes.load_types('trigemu/triggerdecisionemulator.jsonnet')
moo.otypes.load_types('dfmodules/requestgenerator.jsonnet')
moo.otypes.load_types('dfmodules/fragmentreceiver.jsonnet')
moo.otypes.load_types('dfmodules/datawriter.jsonnet')
moo.otypes.load_types('dfmodules/hdf5datastore.jsonnet')
moo.otypes.load_types('readout/fakecardreader.jsonnet')
moo.otypes.load_types('readout/datalinkhandler.jsonnet')
moo.otypes.load_types('readout/datarecorder.jsonnet')


# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd,
import dunedaq.rcif.cmd as rccmd # AddressedCmd,
import dunedaq.appfwk.cmd as cmd # AddressedCmd,
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigemu.triggerdecisionemulator as tde
import dunedaq.dfmodules.requestgenerator as rqg
import dunedaq.dfmodules.fragmentreceiver as ffr
import dunedaq.dfmodules.datawriter as dw
import dunedaq.dfmodules.hdf5datastore as hdf5ds
import dunedaq.readout.fakecardreader as fcr
import dunedaq.readout.datalinkhandler as dlh
import dunedaq.readout.datarecorder as bfs


from appfwk.utils import mcmd, mrccmd, mspec

import json
import math
# Time to waait on pop()
QUEUE_POP_WAIT_MS=100;
# local clock speed Hz
CLOCK_SPEED_HZ = 50000000;

def generate(
        NUMBER_OF_DATA_PRODUCERS=2,
        EMULATOR_MODE=False,
        DATA_RATE_SLOWDOWN_FACTOR = 10,
        RUN_NUMBER = 333,
        TRIGGER_RATE_HZ = 1.0,
        DATA_FILE="./frames.bin",
        OUTPUT_PATH=".",
        DISABLE_OUTPUT=False,
        TOKEN_COUNT=10
    ):

    trigger_interval_ticks = math.floor((1/TRIGGER_RATE_HZ) * CLOCK_SPEED_HZ/DATA_RATE_SLOWDOWN_FACTOR)

    # Define modules and queues
    queue_bare_specs = [
            app.QueueSpec(inst="time_sync_q", kind='FollyMPMCQueue', capacity=100),
            app.QueueSpec(inst="token_q", kind='FollySPSCQueue', capacity=20),
            app.QueueSpec(inst="trigger_decision_q", kind='FollySPSCQueue', capacity=20),
            app.QueueSpec(inst="trigger_decision_copy_for_bookkeeping", kind='FollySPSCQueue', capacity=20),
            app.QueueSpec(inst="trigger_record_q", kind='FollySPSCQueue', capacity=20),
            app.QueueSpec(inst="data_fragments_q", kind='FollyMPMCQueue', capacity=100),
        ] + [
            app.QueueSpec(inst=f"data_requests_{idx}", kind='FollySPSCQueue', capacity=20)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [

            app.QueueSpec(inst=f"wib_fake_link_{idx}", kind='FollySPSCQueue', capacity=100000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
            app.QueueSpec(inst=f"snb_link_{idx}", kind='FollySPSCQueue', capacity=100000)
                            for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ]


    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [
        mspec("tde", "TriggerDecisionEmulator", [
                        app.QueueInfo(name="time_sync_source", inst="time_sync_q", dir="input"),
                        app.QueueInfo(name="token_source", inst="token_q", dir="input"),
                        app.QueueInfo(name="trigger_decision_sink", inst="trigger_decision_q", dir="output"),
                    ]),

        mspec("rqg", "RequestGenerator", [
                        app.QueueInfo(name="trigger_decision_input_queue", inst="trigger_decision_q", dir="input"),
                        app.QueueInfo(name="trigger_decision_for_event_building", inst="trigger_decision_copy_for_bookkeeping", dir="output"),
                    ] + [
                        app.QueueInfo(name=f"data_request_{idx}_output_queue", inst=f"data_requests_{idx}", dir="output")
                            for idx in range(NUMBER_OF_DATA_PRODUCERS)
                    ]),

        mspec("ffr", "FragmentReceiver", [
                        app.QueueInfo(name="trigger_decision_input_queue", inst="trigger_decision_copy_for_bookkeeping", dir="input"),
                        app.QueueInfo(name="trigger_record_output_queue", inst="trigger_record_q", dir="output"),
                        app.QueueInfo(name="data_fragment_input_queue", inst="data_fragments_q", dir="input"),
                    ]),

        mspec("datawriter", "DataWriter", [
                        app.QueueInfo(name="trigger_record_input_queue", inst="trigger_record_q", dir="input"),
                        app.QueueInfo(name="token_output_queue", inst="token_q", dir="output"),
                    ]),

        mspec("fake_source", "FakeCardReader", [

                        app.QueueInfo(name=f"output_{idx}", inst=f"wib_fake_link_{idx}", dir="output")
                            for idx in range(NUMBER_OF_DATA_PRODUCERS)
                        ]),

        ] + [
                mspec(f"datahandler_{idx}", "DataLinkHandler", [

                            app.QueueInfo(name="raw_input", inst=f"wib_fake_link_{idx}", dir="input"),
                            app.QueueInfo(name="timesync", inst="time_sync_q", dir="output"),
                            app.QueueInfo(name="requests", inst=f"data_requests_{idx}", dir="input"),
                            app.QueueInfo(name="fragments", inst="data_fragments_q", dir="output"),
                            app.QueueInfo(name="snb", inst=f"snb_link_{idx}", dir="output"),
                            ]) for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
                mspec(f"data_recorder_{idx}", "DataRecorder", [
                            app.QueueInfo(name="snb", inst=f"snb_link_{idx}", dir="input")
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

    if TOKEN_COUNT > 0:
        df_token_count = 0
        trigemu_token_count = TOKEN_COUNT
    else:
        df_token_count = -1 * TOKEN_COUNT
        trigemu_token_count = 0

    confcmd = mrccmd("conf", "INITIAL", "CONFIGURED",[
                ("tde", tde.ConfParams(
                        links=[idx for idx in range(NUMBER_OF_DATA_PRODUCERS)],
                        min_links_in_request=NUMBER_OF_DATA_PRODUCERS,
                        max_links_in_request=NUMBER_OF_DATA_PRODUCERS,
                        min_readout_window_ticks=1200,
                        max_readout_window_ticks=1200,
                        trigger_window_offset=1000,
                        # The delay is set to put the trigger well within the latency buff
                        trigger_delay_ticks=math.floor( 2* CLOCK_SPEED_HZ/DATA_RATE_SLOWDOWN_FACTOR),
                        # We divide the trigger interval by
                        # DATA_RATE_SLOWDOWN_FACTOR so the triggers are still
                        # emitted per (wall-clock) second, rather than being
                        # spaced out further
                        trigger_interval_ticks=trigger_interval_ticks,
                        clock_frequency_hz=CLOCK_SPEED_HZ/DATA_RATE_SLOWDOWN_FACTOR,
                        initial_token_count=trigemu_token_count
                        )),
                ("rqg", rqg.ConfParams(
                        map=rqg.mapgeoidqueue([
                                rqg.geoidinst(apa=0, link=idx, queueinstance=f"data_requests_{idx}") for idx in range(NUMBER_OF_DATA_PRODUCERS)
                            ])
                        )),
                ("ffr", ffr.ConfParams(
                            general_queue_timeout=QUEUE_POP_WAIT_MS
                        )),
                ("datawriter", dw.ConfParams(
                            initial_token_count=df_token_count,
                            data_store_parameters=hdf5ds.ConfParams(
                                name="data_store",
                                # type = "HDF5DataStore", # default
                                directory_path = OUTPUT_PATH, # default
                                # mode = "all-per-file", # default
                                max_file_size_bytes = 1073741824,
                                filename_parameters = hdf5ds.HDF5DataStoreFileNameParams(
                                    overall_prefix = "swtest",
                                    digits_for_run_number = 6,
                                    file_index_prefix = "",
                                    digits_for_file_index = 4,
                                ),
                                file_layout_parameters = hdf5ds.HDF5DataStoreFileLayoutParams(
                                    trigger_record_name_prefix= "TriggerRecord",
                                    digits_for_trigger_number = 5,
                                    digits_for_apa_number = 3,
                                    digits_for_link_number = 2,
                                )
                            )
                        )),
                ("fake_source",fcr.Conf(
                            link_ids=list(range(NUMBER_OF_DATA_PRODUCERS)),
                            # input_limit=10485100, # default
                            rate_khz = CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR*1000),
                            raw_type = "wib",
                            data_filename = DATA_FILE,
                            queue_timeout_ms = QUEUE_POP_WAIT_MS
                        )),
            ] + [
                (f"datahandler_{idx}", dlh.Conf(
                        raw_type = "wib",
                        emulator_mode = EMULATOR_MODE,
                        # fake_trigger_flag=0, # default
                        source_queue_timeout_ms= QUEUE_POP_WAIT_MS,
                        latency_buffer_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR),
                        pop_limit_pct = 0.8,
                        pop_size_pct = 0.1,
                        apa_number = 0,
                        link_number = idx
                        )) for idx in range(NUMBER_OF_DATA_PRODUCERS)
            ] + [
                (f"data_recorder_{idx}", bfs.Conf(
                        output_file = f"/mnt/micron1/output_{idx}.out",
                        compression_algorithm = "None",
                        stream_buffer_size = 8388608
                        )) for idx in range(NUMBER_OF_DATA_PRODUCERS)
            ])

    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = rccmd.StartParams(run=RUN_NUMBER, trigger_interval_ticks=trigger_interval_ticks, disable_data_storage=DISABLE_OUTPUT)
    startcmd = mrccmd("start", "CONFIGURED", "RUNNING", [
            ("datawriter", startpars),
            ("ffr", startpars),
            ("datahandler_.*", startpars),
            ("data_recorder_.*", startpars),
            ("fake_source", startpars),
            ("rqg", startpars),
            ("tde", startpars),
        ])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)


    stopcmd = mrccmd("stop", "RUNNING", "CONFIGURED", [
            ("tde", None),
            ("rqg", None),
            ("fake_source", None),
            ("datahandler_.*", None),
            ("ffr", None),
            ("datawriter", None),
            ("data_recorder_.*", None)
        ])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)

    pausecmd = mrccmd("pause", "RUNNING", "RUNNING", [
            ("", None)
        ])

    jstr = json.dumps(pausecmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPause\n\n", jstr)

    resumecmd = mrccmd("resume", "RUNNING", "RUNNING", [
            ("tde", tde.ResumeParams(
                            trigger_interval_ticks=trigger_interval_ticks
                        ))
        ])

    jstr = json.dumps(resumecmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nResume\n\n", jstr)

    scrapcmd = mrccmd("scrap", "CONFIGURED", "INITIAL", [
            ("", None)
        ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd, pausecmd, resumecmd, scrapcmd]

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr

if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-n', '--number-of-data-producers', default=2)
    @click.option('-e', '--emulator-mode', is_flag=True)
    @click.option('-s', '--data-rate-slowdown-factor', default=10)
    @click.option('-r', '--run-number', default=333)
    @click.option('-t', '--trigger-rate-hz', default=1.0)
    @click.option('-d', '--data-file', type=click.Path(), default='./frames.bin')
    @click.option('-o', '--output-path', type=click.Path(), default='.')
    @click.option('--disable-data-storage', is_flag=True)
    @click.option('-c', '--token-count', default=10)
    @click.argument('json_file', type=click.Path(), default='minidaq-app-fake-readout.json')
    def cli(number_of_data_producers, emulator_mode, data_rate_slowdown_factor, run_number, trigger_rate_hz, data_file, output_path, disable_data_storage, token_count, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    NUMBER_OF_DATA_PRODUCERS = number_of_data_producers,
                    EMULATOR_MODE = emulator_mode,
                    DATA_RATE_SLOWDOWN_FACTOR = data_rate_slowdown_factor,
                    RUN_NUMBER = run_number,
                    TRIGGER_RATE_HZ = trigger_rate_hz,
                    DATA_FILE = data_file,
                    OUTPUT_PATH = output_path,
                    DISABLE_OUTPUT = disable_data_storage,
                    TOKEN_COUNT = token_count
                ))

        print(f"'{json_file}' generation completed.")

    cli()

