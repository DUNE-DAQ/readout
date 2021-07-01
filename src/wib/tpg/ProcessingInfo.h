#ifndef PROCESSINGINFO_H
#define PROCESSINGINFO_H

#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>

#include "frame_expand.h"

// The state variables for each channel in the link, saved from the last time
template<size_t NREGISTERS>
struct ChanState
{
    ChanState()
    {
        for(size_t i=0; i<NREGISTERS*SAMPLES_PER_REGISTER; ++i){
            pedestals[i]=0;
            accum[i]=0;
            accum25[i]=0;
            accum75[i]=0;
            prev_was_over[i]=0;
            hit_charge[i]=0;
            hit_tover[i]=0;
            for(size_t j=0; j<NTAPS; ++j) prev_samp[i*NTAPS+j]=0;
        }
    }

    // TODO: DRY
    static const int NTAPS=8;

    alignas(32) int16_t __restrict__ pedestals[NREGISTERS*SAMPLES_PER_REGISTER];
    alignas(32) int16_t __restrict__ quantile25[NREGISTERS*SAMPLES_PER_REGISTER];
    alignas(32) int16_t __restrict__ quantile75[NREGISTERS*SAMPLES_PER_REGISTER];

    alignas(32) int16_t __restrict__ accum[NREGISTERS*SAMPLES_PER_REGISTER];
    alignas(32) int16_t __restrict__ accum25[NREGISTERS*SAMPLES_PER_REGISTER];
    alignas(32) int16_t __restrict__ accum75[NREGISTERS*SAMPLES_PER_REGISTER];

    // Variables for filtering
    alignas(32) int16_t __restrict__ prev_samp[NREGISTERS*SAMPLES_PER_REGISTER*NTAPS];

    // Variables for hit finding
    alignas(32) int16_t __restrict__ prev_was_over[NREGISTERS*SAMPLES_PER_REGISTER]; // was the previous sample over threshold?
    alignas(32) int16_t __restrict__ hit_charge[NREGISTERS*SAMPLES_PER_REGISTER];
    alignas(32) int16_t __restrict__ hit_tover[NREGISTERS*SAMPLES_PER_REGISTER]; // time over threshold
};

template<size_t NREGISTERS>
struct ProcessingInfo
{
    ProcessingInfo(const RegisterArray<NREGISTERS*FRAMES_PER_MSG>* __restrict__ input_,
                   size_t timeWindowNumFrames_,
                   uint8_t first_register_,
                   uint8_t last_register_,
                   uint16_t* __restrict__ output_,
                   const int16_t* __restrict__ taps_,
                   int16_t ntaps_,
                   const uint8_t tap_exponent_,
                   uint16_t threshold_,
                   size_t nhits_,
                   uint16_t absTimeModNTAPS_)
        : input(input_),
          timeWindowNumFrames(timeWindowNumFrames_),
          first_register(first_register_),
          last_register(last_register_),
          output(output_),
          taps(taps_),
          ntaps(ntaps_),
          tap_exponent(tap_exponent_),
          threshold(threshold_),
          multiplier(1 << tap_exponent),
          adcMax(INT16_MAX/multiplier),
          nhits(nhits_),
          absTimeModNTAPS(absTimeModNTAPS_)
    {
    }

    // Set the initial state from the window starting at first_msg_p
    template<size_t N>
    void setState(const RegisterArray<N>& first_tick_registers)
    {
        static_assert(N>=NREGISTERS, "Wrong array size");
        // Set the pedestals and the 25/75-percentiles: everything else is just the same as in default values
        for(size_t j=0; j<NREGISTERS*SAMPLES_PER_REGISTER; ++j){
            const int16_t ped=first_tick_registers.uint16(j);
            chanState.pedestals[j]=ped;
            chanState.quantile25[j]=ped-3;
            chanState.quantile75[j]=ped+3;
        }
    }

    const RegisterArray<NREGISTERS*FRAMES_PER_MSG>* __restrict__ input;
    size_t timeWindowNumFrames;
    uint8_t first_register;
    uint8_t last_register;
    uint16_t* __restrict__ output;
    const int16_t* __restrict__ taps;
    int16_t ntaps;
    uint8_t tap_exponent;
    uint16_t threshold;
    int16_t multiplier;
    int16_t adcMax;
    size_t nhits;
    uint16_t absTimeModNTAPS;
    ChanState<NREGISTERS> chanState;
};


#endif

/* Local Variables:   */
/* mode: c++          */
/* c-basic-offset: 4  */
/* End:               */
