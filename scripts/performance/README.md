# Performance scripts 
Performance related utilities.

## Explanation

The balancer can change the CPU affinities of already running processes and their threads.
One criteria is, that the thread's pthread handle need to be named. `psutil` is a dependency.

The json configuration file holds the thread names and desired CPU affinity.
One can use the script as:

    python3.6 balancer.py --process daq_application --pinfile cpupin-n1-orig.json
