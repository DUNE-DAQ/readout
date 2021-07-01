#ifndef DESIGN_FIR_H
#define DESIGN_FIR_H

#include <vector>
#include <cmath>
#include <stdint.h>

// Functions to design a lowpass filter with the given number of taps
// and cutoff, as a fraction of the Nyquist frequency. Copied from the
// corresponding functions in scipy and converted to C++

std::vector<double> hamming(int M);

double sinc(double x);

std::vector<double> firwin(int N, double cutoff);

std::vector<int16_t> firwin_int(int N, double cutoff, const int multiplier);

#endif
