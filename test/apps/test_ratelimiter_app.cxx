/**
 * @file test_ratelimiter_app.cxx Test application for
 * ratelimiter implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "readout/utils/RateLimiter.hpp"

#include "logging/Logging.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <random>
#include <vector>

using namespace dunedaq::readout;

int
main(int /*argc*/, char** /*argv[]*/)
{
  int runsecs = 15;

  // Run marker
  std::atomic<bool> marker{ true };

  // RateLimiter
  TLOG() << "Creating ratelimiter with 1MHz...";
  RateLimiter rl(1000);

  // Counter for ops/s
  std::atomic<int> newops = 0;

  // Stats
  auto stats = std::thread([&]() {
    TLOG() << "Spawned stats thread...";
    while (marker) {
      TLOG() << "ops/s ->  " << newops.exchange(0);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  std::random_device rd;
  std::mt19937 mt(rd());

  // Adjuster
  auto adjuster = std::thread([&]() {
    TLOG() << "Spawned adjuster thread...";
    std::uniform_int_distribution<> dist(1, 1000);
    std::vector<int> rand_rates;
    for (int i = 0; i < runsecs; ++i) {
      rand_rates.push_back(dist(mt));
    }
    int idx = 0;
    while (marker) {
      TLOG() << "Adjusting rate to: " << rand_rates[idx] << "[kHz]";
      rl.adjust(rand_rates[idx]);
      if (idx > runsecs - 1) {
        idx = 0;
      } else {
        ++idx;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    TLOG() << "Application will terminate in 5s...";
    std::this_thread::sleep_for(std::chrono::seconds(runsecs));
    marker.store(false);
  });

  // Limit task
  TLOG() << "Launching task to count...";
  int sumops = 0;
  rl.init();
  while (marker) {
    // just  count...
    sumops++;
    newops++;
    rl.limit();
  }

  // Join local threads
  TLOG() << "Flipping killswitch in order to stop...";
  if (killswitch.joinable()) {
    killswitch.join();
  }

  // Check
  // TLOG() << "Operations in 5 seconds (should be really close to 5 million:): " << sumops;

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
