# readout - Readout software and utilities 
Appfwk DAQModules, utilities, and scripts for DUNE Upstream DAQ Readout Software.

## Building

Clone the package into a work area as defined under the instructions of the appfwk:
https://github.com/DUNE-DAQ/appfwk/wiki/Compiling-and-running-under-v2.2.0


## Examples
Before running the application, please download a small binary files that contains 120 WIB Frames from the following [CERNBox link](https://cernbox.cern.ch/index.php/s/VAqNtn7bwuQtff3/download). Please update the path of the source file in the configuration that you will use below. 

After succesfully building the package, from another terminal go to your `workarea` directory and set up the runtime environment:

    dbt_setup_runtime_environment
    
After that, launch a readout emaulation via:

    daq_application -c stdin://sourcecode/readout/test/fakereadout-commands.json
    
Then start typing commands as instructed by the command facility.
