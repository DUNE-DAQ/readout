#! python3

import argparse
import os
import sys
import psutil
import json

parser = argparse.ArgumentParser(description='Alters CPU affinities of running processes based on a configuration file.')
parser.add_argument('--process', '-p', type=str, required=True, help='target processes\' name')
parser.add_argument('--pinfile', '-f', type=str, required=True, help='file with thread name to CPU mask list')

try:
  args = parser.parse_args()
except:
  parser.print_help()
  sys.exit(0)

print('Basic information...')
lcpu_count = psutil.cpu_count(0)
pcpu_count = psutil.cpu_count(logical=False)
vmem = psutil.virtual_memory()
print('  -> Logical CPU count: ', lcpu_count)
print('  -> Physical CPU count: ', lcpu_count)
print('  -> Memory status: ', vmem) 

process_name = args.process
pinfile = args.pinfile 

try:
  with open(pinfile, 'r') as f:
    affinity_dict = json.load(f)
except Exception as e:
  print(e)
  sys.exit(0)

print('\nAttempt to find processes and apply cpu mask...')
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
