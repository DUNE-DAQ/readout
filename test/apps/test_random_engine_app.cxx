/**
 * @file test_ratelimiter_app.cxx Test application for 
 * ratelimiter implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "RandomEngine.hpp"

#include "logging/Logging.hpp"

#include <atomic>
#include <string>
#include <chrono>
#include <memory>

using namespace dunedaq::readout;

int
main(int /*argc*/, char** /*argv[]*/)
{
  // RandomEngine
  TLOG() << "Creating RandomEngine...";
  RandomEngine re;

  TLOG() << "Getting a random population of 100 elements from 123 to 7568...";
  auto population_sizes = re.get_random_population(100, 123, 7568);

  auto population_rates = re.get_random_population(100, 1.2f, 7.5f);

  TLOG() << "Population sizes: ";
  for (unsigned i=0; i<population_sizes.size(); ++i) { // NOLINT
    std::cout << population_sizes[i] << ' ';
  }
  std::cout << '\n';

  TLOG() << "Population rates: ";
  for (unsigned i=0; i<population_rates.size(); ++i) { // NOLINT
    std::cout << population_rates[i] << ' ';
  }
  std::cout << '\n';

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
