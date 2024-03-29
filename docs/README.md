# readout - Readout software and utilities 
Appfwk DAQModules, utilities, and scripts for DUNE Upstream DAQ Readout Software.

## Building and setting up the workarea

How to clone and build DUNE DAQ packages, including `readout`, is covered in [the daq-buildtools instructions](https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-buildtools/). You should follow these steps to set up your workarea that you can then use to run the following examples.

## Examples
Before running the application, please download a small binary file that contains WIB Frames from the following [CERNBox link](https://cernbox.cern.ch/index.php/s/7qNnuxD8igDOVJT), or from the commandline:

    curl https://cernbox.cern.ch/index.php/s/7qNnuxD8igDOVJT/download -o frames.bin

    
For WIB2 frames, download the following file that contains 120 WIB-2 Frames from the following [CERNBox link](https://cernbox.cern.ch/index.php/s/ocrHxSU8PucxphE), or like so:

    curl https://cernbox.cern.ch/index.php/s/ocrHxSU8PucxphE/download -o wib2-frames.bin

If you download it to a different destination, please update the path of the source file in the configuration that you will use below. 

To run a standalone readout app (instructions for the complete minidaqapp are included in the setup instructions above), you first create a config with:

    python -m readout.app_confgen -n 2 app.json
    
Here, we use a fake card emulator with two WIB links. More options can be viewed with `-h`. Then, start the application with

    daq_application -c stdin://app.json -n test
    
You can now issue commands by typing them and pressing enter. Issue the commands `init`, `conf` and then `start`. You will see some json output from the operational monitoring every 10 seconds.

## Enabling the Software TPG
To enable the SIMD accelerated software hit finding, one can use raw data recorded from ProtoDUNE-SP to get meaningful hits. A subset of these raw files can be found under:

    /eos/experiment/neutplatform/protodune/rawdata/np04/protodune-sp/raw/2020/detector/test/None/02/00/00/01/
    
For single link tests, a good link file can be:

    /eos/experiment/neutplatform/protodune/rawdata/np04/protodune-sp/raw/2020/detector/test/None/02/00/00/01/felix-2020-06-02-093338.0.0.0.bin

The produced hit rate should be around 100kHz.

## Enabling the fake TP source

The FakeCardReader module is capable of reading raw WIB TP data by enabling the corresponding link 
via configuration. Currently the fake TPs are read out from a binary file (with default location 
at /tmp/tp_frames.bin) and parsed using the "RawWibTp" format.

To get the "tp_frames.bin" TP data:

    curl https://cernbox.cern.ch/index.php/s/nd201XOcMCmHpIX/download -o /tmp/tp_frames.bin

_Instructions on how to test the fake raw WIB TP readout will be provided here_
    
## A Deeper Look Into Readout: Functional Elements

### Data-flow Diagram (DFD)
The functional elements are seen on the following DFD. Color codes indicate the ownership (or responsibility) of the DAQ subsystems: <span style="color:blue">Dataflow (=blue)</span> and <span style="color:red">Upstream DAQ (=red)</span>.
![functional-elements](https://cernbox.cern.ch/index.php/s/YmXmHC7LpsCjGjT/download)

### Domains
Individual domains represent a substantially different path of the raw data, including some sort of data interpretation, transformation, formatting, or buffering. Functional elements marked with _italicized_ text.

1. **Trigger Primitive (TP) handling domain** (**Input**: raw data, **Output**: TPs): The readout is responsible for _generating trigger primitives_ from raw data, and _format_ these TPs to the agreed data-format. After this, the TPs can be _buffered_ and _streamed_ for other subsystems (most importantly, to Dataselection).
2. **Raw Streaming domain** (**Input**: raw data, **Output**: raw/calib stream): The readout needs to interpret raw data and find possible problems and errors with and withing data (e.g.: timestamp continuity violation, data integrity, invalid headers, front-end specific error flags). Calibration flags in form of headers are also inside the front-end data frames. In case these flags are found, some data need to be _formatted_ (e.g.: expanded based on channels ) then _streamed_ to a configured destination (e.g.: local raw binary files or appfwk queues).
3. **Requested Data domain** (**Input**: raw data and data requests -[dfmessages::DataRequest](https://github.com/DUNE-DAQ/dfmessages/blob/develop/include/dfmessages/DataRequest.hpp)-, **Output**: special data requests to other functional elements and requested data -[daqdataformats::Fragment](https://github.com/DUNE-DAQ/daqdataformats/blob/develop/include/daqdataformats/Fragment.hpp)-): This domains contains the conventional "triggered" readout mode. Data from the front-end are _processed and routed_ to the appropriate destinations: either to the Raw Streaming domain (2) or to the raw data buffer (Latency Buffer). Incoming _requests are handled_ via accessing the data buffer then match and extract the requested data for the request. Special requests (e.g.: recording) may be routed to different domains' functional elements, and data leaving the buffer may be intercepted if needed (e.g.: stream to store).
4. **Recorded Data domain** (**Input**: raw data, "record" requests, **Output**: recorded data, metadata of data store, transfer acknowledgements and notifications): In case of a `record(O(seconds))` request, data leaving the latency buffer are streamed to a transient data store, which is usually local to the readout unit. The recorded data are transferred to other DAQ subsystems with the help of additional metadata, notifications, and acknowledgements.

### Definitions
1. Latency Buffer: A container that temporarily stores the raw data, and has certain attributes that ensures search-ability based on a lookup criteria. A notable example for this, is the lookup based on the timestamp, where the timestamp can be converted to an exact position in the buffer if the "timestamp continuity" attribute is ensured in the buffer.

### Class diagram

A zoomable visualization of [the readout code](https://github.com/DUNE-DAQ/readout/) for its `dunedaq-v2.8.0` release:

![class-diagram](https://cernbox.cern.ch/index.php/s/Hvyb41Ndj2MfKlw/download)

## Looking into the directories

At the top level, the readout package uses the same directory structure as other DUNE DAQ packages as described in [the daq-cmake documentation](https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/). However, due its large number of files, additional subdirectories have been added to organize them. This approach is covered [here](Directory-structure.md).

