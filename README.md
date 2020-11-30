# udaq-readout
Appfwk DAQModules, utilities, and scripts for DUNE Upstream DAQ FELIX Readout Software.

## Examples 

For the dependencies, you need the following external package that ships a build of a partial set of the ATLAS FELIX Software suite:

    udaq_readout_deps v0_0_1

Please modify your `.dunedaq_area` file in your work area, by appending the following item to your `dune_products_dir` set:

    "/cvmfs/dune.opensciencegrid.org/dunedaq/DUNE/products_dev"

And add the `udaq_readout_deps` package to your `dune_products` set:

    "udaq_readout_deps v0_0_1"

Then after building, one can run the application like:

    daq_application --commandFacility rest://localhost:12345
