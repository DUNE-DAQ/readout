/**
 * @file ErrorBitGenerator.hpp Utility to deliver error bits as integers to the source emulator
 * This is part of the DUNE DAQ , copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_INCLUDE_READOUT_UTILS_ERRORBITGENERATOR_HPP
#define READOUT_INCLUDE_READOUT_UTILS_ERRORBITGENERATOR_HPP

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
  uint16_t next()
  {
    if (m_occurrence_count >= m_current_occurrence) {
      if (m_set_error_bits) {
        m_error_bits_index = (m_error_bits_index + 1) % 1000;
        m_error_occurrences_index = (m_error_occurrences_index + 1) % 1000;
        m_set_error_bits = false;
        m_current_occurrence = m_no_error_occurrences[m_no_error_occurrences_index];
        m_occurrence_count = 0;
      }
      else {
        m_no_error_occurrences_index = (m_no_error_occurrences_index + 1) % 1000;
        m_set_error_bits = true;
        m_current_occurrence = m_error_occurrences[m_error_occurrences_index];
        m_occurrence_count = 0;
      }
    }
    m_occurrence_count++;
    return m_set_error_bits ? m_error_bits[m_error_bits_index] : 0;
  }

  void generate()
  {
//    TLOG() << "M_ERROR_RATE: " << m_error_rate;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint16_t> err_bit_dis(0, 65535);
    std::uniform_int_distribution<int> duration_dis(1, 1000000);

    for (int i = 0; i < 1000; ++i) {
      m_error_bits[i] = err_bit_dis(mt);
    }
    for (int i = 0; i < 1000; ++i) {
      m_error_occurrences[i] = duration_dis(mt) * m_error_rate;
//      TLOG() << "M_ERROR_OCCURRENCES_" << i << ": " << m_error_occurrences[i];
      m_no_error_occurrences[i] = duration_dis(mt) * (1 - m_error_rate);
//      TLOG() << "M_NO_ERROR_OCCURRENCES_" << i << ": " << m_no_error_occurrences;
    }
  }

private:
  double m_error_rate;
  uint16_t m_error_bits[1000];
  int m_error_bits_index;
  int m_error_occurrences[1000];
  int m_error_occurrences_index;
  int m_no_error_occurrences[1000];
  int m_no_error_occurrences_index;
  int m_current_occurrence;
  int m_occurrence_count;
  bool m_set_error_bits;
};
}
}

#endif // READOUT_INCLUDE_READOUT_UTILS_ERRORBITGENERATOR_HPP_