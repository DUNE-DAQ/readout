#ifndef POWERTWOHIST_HH
#define POWERTWOHIST_HH

#include "stdint.h"
#include <cstring> // For memset

// A simple histogram class that can only store unsigned integers in
// bins of logarithmic size. Bin 0's limits are [0,0], bin 1 [1,1],
// bin 2 [2,3], bin 3 [4, 7] and so on, up to bin N-1, which is an
// overflow with limits [2**(N-1), UINT64_MAX]
template<size_t Nbins>
class PowerTwoHist
{
public:
    PowerTwoHist()
    {
        // Initialize all bin contents to zero
        memset(bins, 0, Nbins*sizeof(uint64_t));
    }

    // Fill the bin corresponding to `x`. Return the filled bin number
    // (might be overflow)
    size_t fill(unsigned long x)
    {
        int bin=64-__builtin_clzl(x);
        if((size_t)bin>=Nbins) bin=Nbins-1;
        ++bins[bin];
        return bin;
    }

    size_t nbins() const { return Nbins; }

    // Get the bin content in bin `ibin`
    uint64_t bin(size_t ibin) const { return bins[ibin]; }

    // The minimum value that will be stored in bin `ibin`
    uint64_t binLo(size_t ibin) const { return ibin==0 ? 0 : (1<<(ibin-1)); }
    // The maximum value that will be stored in bin `ibin`
    uint64_t binHi(size_t ibin) const
    {
        if(ibin==0) return 0;
        else if(ibin==Nbins-1) return UINT64_MAX;
        else return (1<<ibin)-1;
    }
    
private:
    uint64_t bins[Nbins];
};

#endif
