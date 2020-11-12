#! python3

import argparse
import os
import sys
import psutil
import json

parser = argparse.ArgumentParser(description='Alters CPU affinities of running processes based on a configuration file.')
parser.add_argument('--host', type=str, default='localhost', help='target host/endpoint')
parser.add_argument('-p', '--port', type=int, default=12345, help='target port')
parser.add_argument('-a', '--answer-port', type=int, default=12333, help='answer to service listening on this port')
parser.add_argument('-r', '--route', type=str, default='command', help='target route on endpoint')
parser.add_argument('-f', '--file', type=str, help='file that contains command to be posted')
parser.add_argument('-w', '--wait', type=int, default=2, help='seconds to wait between sending commands')
parser.add_argument('-i', '--interactive', dest='interactive', action='store_true', help='interactive mode')
parser.add_argument('--non-interactive', dest='interactive', action='store_false')
parser.set_defaults(interactive=False)

lcpu_count = psutil.cpu_count(0)
pcpu_count = psutil.cpu_count(logical=False)
vmem = psutil.virtual_memory()
print('Logical CPU count: ', lcpu_count)
print('Physical CPU count: ', lcpu_count)
print('Memory status: ', vmem) 

if len(sys.argv) != 2:  
  quit()

try:
  with open(sys.argv[1], 'r') as f:
    affinity_dict = json.load(f)
except Exception as e:
  print(e)
  quit()

print('#######################')
process_name = "daq_application"
pid = None

for proc in psutil.process_iter():
  if process_name in proc.name():

    pid = proc.pid
    print('------------------ Found daq_application pid: ', pid, ' ----------------------------')
    print('\n-> Command line info:')
    print("   -", proc.cmdline())

    memory = proc.memory_info()
    print("\n-> Memory info:")
    print("   -", memory)

    connections = proc.connections()
    print("\n-> Connections:")
    for conn in connections:
      print("   - " + str(conn))

    threads = proc.threads()
    print("\n-> Threads:")
    for t in threads:
      tp = psutil.Process(t.id)
      print('   - ' + str(tp))
      affinity = tp.cpu_affinity()
      new_aff = []
      notfound = None
      try:
        new_aff = affinity_dict[proc.cmdline()[1]][tp.name()]
      except Exception as e:
         notfound = True
      print('     affinity to set: ', new_aff, '\n')
      tp.cpu_affinity(new_aff)

    print('------------------------------------------------------------------------\n\n')
