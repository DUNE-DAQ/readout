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
#include <chrono>
#include <memory>
#include <sstream>
#include <string>

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

  std::ostringstream oss;
  oss << "Population sizes: ";
  for (unsigned i = 0; i < population_sizes.size(); ++i) { // NOLINT
    oss << population_sizes[i] << ' ';
  }
  TLOG() << oss.str();
  oss.str("");

  oss << "Population rates: ";
  for (unsigned i = 0; i < population_rates.size(); ++i) { // NOLINT
    oss << population_rates[i] << ' ';
  }
  TLOG() << oss.str();

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
