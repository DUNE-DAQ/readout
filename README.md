# readout - Readout software and utilities 
Appfwk DAQModules, utilities, and scripts for DUNE Upstream DAQ FELIX Readout Software.

## TP Integration

TP is added to the FakeCardReader module (TP is currently not a standalone module). The fake TPs are read out from a binary file (/tmp/tp_frames.bin) 
and parsed using a structure "TPFrame" defined in  DUNE-DAQ/dataformats.

To get the "tp_frames.bin" TP data:

    curl https://cernbox.cern.ch/index.php/s/ENmgJ4DWom1Ixg2 -o tp_frames.bin


To test the TP readout, run

    daq_application -c stdin://sourcecode/readout/test/fakereadout-tp-commands-input.json

and start typing the commands:

    init, conf, start, stop


## Building

For the dependencies, you need the following external package that ships a build of a partial set of the ATLAS FELIX Software suite:

    udaq_readout_deps v0_0_1

Please modify your `.dunedaq_area` file in your work area, by appending the following item to your `dune_products_dir` set:

    "/cvmfs/dune.opensciencegrid.org/dunedaq/DUNE/products_dev"

And add the `udaq_readout_deps` package to your `dune_products` set:

    "udaq_readout_deps v0_0_1"


## Examples
Before running the application, please download a small binary files that contains 120 WIB Frames from the following [CERNBox link](https://cernbox.cern.ch/index.php/s/VAqNtn7bwuQtff3/download). Please update the path of the source file in the configuration that you will use below. 

After succesfully building the package, from another terminal go to your `workarea` directory and set up the runtime environment:

    setup_runtime_environment
    
After that, launch a readout emaulation via:

    daq_application -c stdin://sourcecode/readout/test/fakereadout-commands.json
    
Then start typing commands as follows:
