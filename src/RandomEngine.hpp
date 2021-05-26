/**
* @file RandomEngine.hpp Reusable thread wrapper
* The same thread instance can be used with different tasks to be executed
* Inspired by: 
* https://codereview.stackexchange.com/questions/134214/reuseable-c11-thread
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_SRC_RANDOMENGINE_HPP_
#define READOUT_SRC_RANDOMENGINE_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <string>

#include <random>
#include <vector>

namespace dunedaq {
namespace readout {

class RandomEngine {
public:
  RandomEngine()
    : m_mt(m_rd())
  { }

  ~RandomEngine() {}

  RandomEngine(const RandomEngine&) 
    = delete; ///< RandomEngine is not copy-constructible
  RandomEngine& operator=(const RandomEngine&) 
    = delete; ///< RandomEngine is not copy-assginable
  RandomEngine(RandomEngine&&) 
    = delete; ///< RandomEngine is not move-constructible
  RandomEngine& operator=(RandomEngine&&) 
    = delete; ///< RandomEngine is not move-assignable

  std::vector<int> get_random_population(unsigned size, int low, int high) { // NOLINT
    std::uniform_int_distribution<> dist(low, high);
    std::vector<int> population;
    for (unsigned i=0; i<size; ++i) { // NOLINT
      population.push_back(dist(m_mt));
    }
    return population;
  }

  std::vector<float> get_random_population(unsigned size, float low, float high) { // NOLINT
    std::uniform_real_distribution<> dist(low, high);
    std::vector<float> population;
    for (unsigned i=0; i<size; ++i) { // NOLINT
      population.push_back(dist(m_mt));
    }
    return population;
  }

  std::vector<double> get_random_population(unsigned size, double low, double high) { // NOLINT
    std::uniform_real_distribution<> dist(low, high);
    std::vector<double> population;
    for (unsigned i=0; i<size; ++i) { // NOLINT
      population.push_back(dist(m_mt));
    }
    return population;
  }

private:
  std::random_device m_rd;
  std::mt19937 m_mt;

};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_SRC_RANDOMENGINE_HPP_
