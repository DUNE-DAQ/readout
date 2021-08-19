/**
 * @file test_lb_allocation_app.cxx Test application for LB allocations.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "readout/models/IterableQueueModel.hpp"
#include "readout/models/SkipListLatencyBufferModel.hpp"

#include "logging/Logging.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <numaif.h>

//#define REGISTER (*(volatile unsigned char*)0x1234)

using namespace dunedaq::readout;

namespace {
struct kBlock {
  kBlock(){};
  char data[1024];
};
}

int
main(int /*argc*/, char** /*argv[]*/)
{
  int runsecs = 5;

  // Run marker
  std::atomic<bool> marker{ true };

  // Counter for ops/s
  std::atomic<int> newops = 0;

  TLOG() << "Creating IQM with intrinsic allocator aligned to 64 bytes.";

  IterableQueueModel<kBlock> numa0IQM(1000, true, 0, false, 0);
  IterableQueueModel<kBlock> numa1IQM(1000, true, 1, false, 0);
  
  numa0IQM.write( kBlock() );
  numa1IQM.write( kBlock() );
 
  int numa_node = -1;
  get_mempolicy(&numa_node, NULL, 0, (void*)numa0IQM.front(), MPOL_F_NODE | MPOL_F_ADDR);
  TLOG() << "Numa 0 IQM frontAddr : " << std::hex << (void*)numa0IQM.front() << " is on: " << numa_node << std::dec;
  get_mempolicy(&numa_node, NULL, 0, (void*)numa0IQM.back(), MPOL_F_NODE | MPOL_F_ADDR);
  TLOG() << "Numa 0 IQM backAddr : " << std::hex << (void*)numa0IQM.back() << " is on: " << numa_node << std::dec;

  get_mempolicy(&numa_node, NULL, 0, (void*)numa1IQM.front(), MPOL_F_NODE | MPOL_F_ADDR);
  TLOG() << "Numa 1 IQM frontAddr: " << std::hex << (void*)numa1IQM.front() << " is on: " << numa_node << std::dec;
  get_mempolicy(&numa_node, NULL, 0, (void*)numa1IQM.back(), MPOL_F_NODE | MPOL_F_ADDR);
  TLOG() << "Numa 1 IQM backAddr: " << std::hex << (void*)numa1IQM.back() << " is on: " << numa_node << std::dec;

  return 0;

  // Stats
  auto stats = std::thread([&]() {
    TLOG() << "Spawned stats thread...";
    while (marker) {
      TLOG() << "ops/s ->  " << newops.exchange(0);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  // Producer
  auto producer = std::thread([&]() {
    TLOG() << "Spawned producer thread...";
    while (marker) { 
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  // Consumer
  auto consumer = std::thread([&]() {
    TLOG() << "Spawned consumer thread...";
    while (marker) { 
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    TLOG() << "Application will terminate in 5s...";
    std::this_thread::sleep_for(std::chrono::seconds(runsecs));
    marker.store(false);
  });

  // Join local threads
  TLOG() << "Flipping killswitch in order to stop...";
  if (killswitch.joinable()) {
    killswitch.join();
  }

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
