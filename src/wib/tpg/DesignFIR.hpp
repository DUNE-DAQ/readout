/**
 * @file design_fir.h Functions to design a lowpass filter with the
 * given number of taps and cutoff, as a fraction of the
 * Nyquist frequency. Copied from the corresponding functions
 * in scipy and converted to C++
 * @author Philip Rodrigues (rodriges@fnal.gov)
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_TPG_DESIGNFIR_HPP_
#define READOUT_SRC_WIB_TPG_DESIGNFIR_HPP_

#include <cmath>
#include <cstdint>
#include <vector>

namespace swtpg {

// constexpr double pi() { return 3.1416; }
constexpr double
pi()
{
  return std::atan(1) * 4;
}

std::vector<double>
hamming(int M);

double
sinc(double x);

std::vector<double>
firwin(int N, double cutoff);

std::vector<int16_t>
firwin_int(int N, double cutoff, const int multiplier);

} // namespace swtpg

#endif // READOUT_SRC_WIB_TPG_DESIGNFIR_HPP_
