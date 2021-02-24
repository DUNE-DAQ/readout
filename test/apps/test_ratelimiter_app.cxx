/**
 * @file test_ratelimiter_app.cxx Test application for 
 * ratelimiter implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "RateLimiter.hpp"

#include "logging/Logging.hpp"

#include <atomic>
#include <string>
#include <chrono>
#include <memory>

using namespace dunedaq::readout;

int
main(int /*argc*/, char** /*argv[]*/)
{
  // Run marker
  std::atomic<bool> marker{true};

  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    TLOG() << "Application will terminate in 5s...";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    marker.store(false);
  });

  // RateLimiter
  TLOG() << "Creating ratelimiter with 1MHz...";
  RateLimiter rl(1000);

  // Limit task
  TLOG() << "Launching task to count...";
  int ops = 0;
  rl.init();
  while (marker) {
    // just  count...
    ops++;
    rl.limit();
  }

  // Join local threads
  TLOG() << "Flipping killswitch in order to stop...";
  if (killswitch.joinable()) {
    killswitch.join();
  }

  // Check
  TLOG() << "Operations in 5 seconds (should be really close to 5 million:): " << ops;

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
