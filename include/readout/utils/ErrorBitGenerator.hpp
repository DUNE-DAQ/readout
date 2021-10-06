/**
 * @file ErrorBitGenerator.hpp Utility to deliver error bits as integers to the source emulator
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_INCLUDE_READOUT_UTILS_ERRORBITGENERATOR_HPP_
#define READOUT_INCLUDE_READOUT_UTILS_ERRORBITGENERATOR_HPP_

#include <random>
#include <unistd.h>

namespace dunedaq {
namespace readout {

/** ErrorBitGenerator usage:
 *
 * auto ebg = ErrorBitGenerator(1.0)
 * ebg.generate();
 * uint16_t errs = ebg.next();
 *
 */

class ErrorBitGenerator
{
public:
  explicit ErrorBitGenerator(double rate = 0)
    : m_error_rate(rate)
    , m_error_bits_index(0)
    , m_error_occurrences_index(0)
    , m_no_error_occurrences_index(0)
    , m_current_occurrence(0)
    , m_occurrence_count(0)
    , m_set_error_bits(true)
  {}
  uint16_t next() // NOLINT(build/unsigned)
  {
    if (m_occurrence_count >= m_current_occurrence) {
      if (m_set_error_bits) {
        m_error_bits_index = (m_error_bits_index + 1) % m_size;
        m_error_occurrences_index = (m_error_occurrences_index + 1) % m_size;
        m_set_error_bits = false;
        m_current_occurrence = m_no_error_occurrences[m_no_error_occurrences_index];
        m_occurrence_count = 0;
      } else {
        m_no_error_occurrences_index = (m_no_error_occurrences_index + 1) % m_size;
        m_set_error_bits = true;
        m_current_occurrence = m_error_occurrences[m_error_occurrences_index];
        m_occurrence_count = 0;
      }
    }
    m_occurrence_count++;
    return (m_set_error_bits && m_current_occurrence) ? m_error_bits[m_error_bits_index] : 0;
  }

  void generate()
  {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint16_t> err_bit_dis(0, 65535);  // NOLINT(build/unsigned)
    std::uniform_int_distribution<int> duration_dis(1, 100000);

    for (int i = 0; i < m_size; ++i) {
      m_error_bits[i] = err_bit_dis(mt);
    }
    for (int i = 0; i < m_size; ++i) {
      m_error_occurrences[i] = duration_dis(mt) * m_error_rate;
      m_no_error_occurrences[i] = duration_dis(mt) * (1 - m_error_rate);
    }
  }

private:
  int m_size = 1000;
  double m_error_rate;
  uint16_t m_error_bits[1000]; // NOLINT(build/unsigned)
  int m_error_bits_index;
  int m_error_occurrences[1000];
  int m_error_occurrences_index;
  int m_no_error_occurrences[1000];
  int m_no_error_occurrences_index;
  int m_current_occurrence;
  int m_occurrence_count;
  bool m_set_error_bits;
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_UTILS_ERRORBITGENERATOR_HPP_