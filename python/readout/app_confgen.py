# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('readout/FakeCardReader.jsonnet')
moo.otypes.load_types('readout/DataLinkHandler.jsonnet')

# Import new types
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.readout.fakecardreader as fcr
import dunedaq.readout.datalinkhandler as dlh

from appfwk.utils import mcmd, mspec

import json
import math
# Time to waait on pop()
QUEUE_POP_WAIT_MS=100;
# local clock speed Hz
CLOCK_SPEED_HZ = 50000000;

def generate(
        NUMBER_OF_DATA_PRODUCERS=1,          
        NUMBER_OF_TP_PRODUCERS=1,          
        DATA_RATE_SLOWDOWN_FACTOR = 1,
        RUN_NUMBER = 333, 
        DATA_FILE="./frames.bin"
    ):
    

    # Define modules and queues
    queue_bare_specs = [
            cmd.QueueSpec(inst="time_sync_q", kind='FollyMPMCQueue', capacity=100),
            cmd.QueueSpec(inst="data_fragments_q", kind='FollyMPMCQueue', capacity=100),
        ] + [
            cmd.QueueSpec(inst=f"data_requests_{idx}", kind='FollySPSCQueue', capacity=1000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
            cmd.QueueSpec(inst=f"wib_fake_link_{idx}", kind='FollySPSCQueue', capacity=100000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ] + [
            cmd.QueueSpec(inst=f"tp_fake_link_{idx}", kind='FollySPSCQueue', capacity=100000)
                for idx in range(NUMBER_OF_DATA_PRODUCERS, NUMBER_OF_DATA_PRODUCERS+NUMBER_OF_TP_PRODUCERS) 
        ]
    

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = cmd.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [
        mspec("fake_source", "FakeCardReader", [
                        cmd.QueueInfo(name=f"output_{idx}", inst=f"wib_fake_link_{idx}", dir="output")
                            for idx in range(NUMBER_OF_DATA_PRODUCERS)
                        ] + [
                        cmd.QueueInfo(name=f"tp_output_{idx}", inst=f"fake_link_{idx}", dir="output")
			    for idx in range(NUMBER_OF_DATA_PRODUCERS, NUMBER_OF_DATA_PRODUCERS+NUMBER_OF_TP_PRODUCERS)
                        ]),

        ] + [
                mspec(f"datahandler_{idx}", "DataLinkHandler", [
                            cmd.QueueInfo(name="raw_input", inst=f"wib_fake_link_{idx}", dir="input"),
                            cmd.QueueInfo(name="timesync", inst="time_sync_q", dir="output"),
                            cmd.QueueInfo(name="requests", inst=f"data_requests_{idx}", dir="input"),
                            cmd.QueueInfo(name="fragments", inst="data_fragments_q", dir="output"),
                            ]) for idx in range(NUMBER_OF_DATA_PRODUCERS)
        ]

    init_specs = cmd.Init(queues=queue_specs, modules=mod_specs)

    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)

    initcmd = cmd.Command(
        id=cmd.CmdId("init"),
        data=init_specs
    )


    confcmd = mcmd("conf", [
                ("fake_source",fcr.Conf(
                            link_ids=list(range(NUMBER_OF_DATA_PRODUCERS+NUMBER_OF_TP_PRODUCERS)),
                            # input_limit=10485100, # default
                            rate_khz = CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR*1000),
                            raw_type = "wib",
                            data_filename = DATA_FILE,
                            queue_timeout_ms = QUEUE_POP_WAIT_MS,
			    set_t0_to = 0,
                            tp_enabled = "false",
                            tp_rate_khz = 0,
                            tp_data_filename = "./tp_frames.bin"
                        )),
            ] + [
                (f"datahandler_{idx}", dlh.Conf(
                        raw_type = "wib",
                        source_queue_timeout_ms= QUEUE_POP_WAIT_MS,
                        fake_trigger_flag=1,
                        latency_buffer_size = 3*CLOCK_SPEED_HZ/(25*12*DATA_RATE_SLOWDOWN_FACTOR),
                        pop_limit_pct = 0.8,
                        pop_size_pct = 0.1,
                        apa_number = 0,
                        link_number = idx
                        )) for idx in range(NUMBER_OF_DATA_PRODUCERS)
            ])
    
    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = cmd.StartParams(run=RUN_NUMBER)
    startcmd = mcmd("start", [
            ("datahandler_.*", startpars),
            ("fake_source", startpars),
        ])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)

    emptypars = cmd.EmptyParams()

    stopcmd = mcmd("stop", [
            ("fake_source", emptypars),
            ("datahandler_.*", emptypars),
        ])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)

    scrapcmd = mcmd("scrap", [
            ("", emptypars)
        ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd, scrapcmd]

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr
        
if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-n', '--number-of-data-producers', default=1)
    @click.option('-t', '--number-of-tp-producers', default=0)
    @click.option('-s', '--data-rate-slowdown-factor', default=10)
    @click.option('-r', '--run-number', default=333)
    @click.option('-d', '--data-file', type=click.Path(), default='./frames.bin')
    @click.argument('json_file', type=click.Path(), default='fake_readout.json')
    def cli(number_of_data_producers, number_of_tp_producers, data_rate_slowdown_factor, run_number, data_file, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    NUMBER_OF_DATA_PRODUCERS = number_of_data_producers,
                    NUMBER_OF_TP_PRODUCERS = number_of_tp_producers,
                    DATA_RATE_SLOWDOWN_FACTOR = data_rate_slowdown_factor,
                    RUN_NUMBER = run_number, 
                    DATA_FILE = data_file,
                ))

        print(f"'{json_file}' generation completed.")

    cli()
    
