/**
 * @file ProcessAVX2.hpp Process frames with AVX2 registers and instructions
 * @author Philip Rodrigues (rodriges@fnal.gov)
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_SRC_WIB_TPG_PROCESSAVX2_HPP_
#define READOUT_SRC_WIB_TPG_PROCESSAVX2_HPP_

#include "FrameExpand.hpp"
#include "ProcessingInfo.hpp"
#include "TPGConstants.hpp"

#include <immintrin.h>

namespace swtpg {

inline void
frugal_accum_update_avx2(__m256i& __restrict__ median,
                         const __m256i s,
                         __m256i& __restrict__ accum,
                         const int16_t acclimit,
                         const __m256i mask) __attribute__((always_inline));

inline void
frugal_accum_update_avx2(__m256i& __restrict__ median,
                         const __m256i s,
                         __m256i& __restrict__ accum,
                         const int16_t acclimit,
                         const __m256i mask)
{
  // if the sample is greater than the median, add one to the accumulator
  // if the sample is less than the median, subtract one from the accumulator.

  // For reasons that I don't understand, there's no cmplt
  // for "compare less-than", so we have to compare greater,
  // compare equal, and take everything else to be compared
  // less-then
  __m256i is_gt = _mm256_cmpgt_epi16(s, median);
  __m256i is_eq = _mm256_cmpeq_epi16(s, median);

  __m256i to_add = _mm256_set1_epi16(-1);
  // Really want an epi16 version of this, but the cmpgt and
  // cmplt functions set their epi16 parts to 0xff or 0x0,
  // so treating everything as epi8 works the same
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(0), is_eq);

  // Don't add anything to the channels which are masked out
  to_add = _mm256_and_si256(to_add, mask);

  accum = _mm256_add_epi16(accum, to_add);

  // if the accumulator is >10, add one to the median and
  // set the accumulator to zero. if the accumulator is
  // <-10, subtract one from the median and set the
  // accumulator to zero
  is_gt = _mm256_cmpgt_epi16(accum, _mm256_set1_epi16(acclimit));
  __m256i is_lt =
    _mm256_cmpgt_epi16(_mm256_sign_epi16(accum, _mm256_set1_epi16(-1 * acclimit)), _mm256_set1_epi16(acclimit));

  to_add = _mm256_setzero_si256();
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(1), is_gt);
  to_add = _mm256_blendv_epi8(to_add, _mm256_set1_epi16(-1), is_lt);

  // Don't add anything to the channels which are masked out
  to_add = _mm256_and_si256(to_add, mask);

  median = _mm256_adds_epi16(median, to_add);

  // Reset the unmasked channels that were >10 or <-10 to zero, leaving the others unchanged
  __m256i need_reset = _mm256_or_si256(is_lt, is_gt);
  need_reset = _mm256_and_si256(need_reset, mask);
  accum = _mm256_blendv_epi8(accum, _mm256_setzero_si256(), need_reset);
}

template<size_t NREGISTERS>
inline void
process_window_avx2(ProcessingInfo<NREGISTERS>& info)
{
  // Start with taps as floats that add to 1. Multiply by some
  // power of two (2**N) and round to int. Before filtering, cap the
  // value of the input to INT16_MAX/(2**N)
  const size_t NTAPS = 8;
  // int16_t taps[NTAPS] = {0};
  // for (size_t i = 0; i < std::min(NTAPS, info.tapsv.size()); ++i) {
  //  taps[i] = info.tapsv[i];
  //}

  const __m256i adcMax = _mm256_set1_epi16(info.adcMax);
  // The maximum value that sigma can have before the threshold overflows a 16-bit signed integer
  const __m256i sigmaMax = _mm256_set1_epi16((1 << 15) / (info.multiplier * 5));

  __m256i tap_256[NTAPS];
  for (size_t i = 0; i < NTAPS; ++i) {
    tap_256[i] = _mm256_set1_epi16(info.taps[i]);
  }
  // Pointer to keep track of where we'll write the next output hit
  __m256i* output_loc = (__m256i*)(info.output); // NOLINT(readability/casting)

  const __m256i iota = _mm256_set_epi16(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);

  int nhits = 0;

  for (uint16_t ireg = info.first_register; ireg < info.last_register; ++ireg) { // NOLINT(build/unsigned)

    uint16_t absTimeModNTAPS = info.absTimeModNTAPS; // NOLINT(build/unsigned)

    // ------------------------------------
    // Variables for pedestal subtraction

    // The current estimate of the pedestal in each channel: get
    // from the previous go-around.

    ChanState<NREGISTERS>& state = info.chanState;
    __m256i median = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.pedestals) + ireg);      // NOLINT
    __m256i quantile25 = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.quantile25) + ireg); // NOLINT
    __m256i quantile75 = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.quantile75) + ireg); // NOLINT

    // The accumulator that we increase/decrease when the current
    // sample is greater/less than the median
    __m256i accum = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.accum) + ireg);     // NOLINT
    __m256i accum25 = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.accum25) + ireg); // NOLINT
    __m256i accum75 = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.accum75) + ireg); // NOLINT

    // ------------------------------------
    // Variables for filtering

    // The (unfiltered) samples `n` places before the current one
    __m256i prev_samp[NTAPS];
    for (size_t j = 0; j < NTAPS; ++j) {
      prev_samp[j] = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.prev_samp) + NTAPS * ireg + j); // NOLINT
    }

    // ------------------------------------
    // Variables for hit finding

    // Was the previous step over threshold?
    __m256i prev_was_over = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.prev_was_over) + ireg); // NOLINT
    ;
    // The integrated charge (so far) of the current hit
    __m256i hit_charge = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.hit_charge) + ireg); // NOLINT
    ;
    // The time-over-threshold (so far) of the current hit
    __m256i hit_tover = _mm256_lddqu_si256(reinterpret_cast<__m256i*>(state.hit_tover) + ireg); // NOLINT
    ;

    // The channel numbers in each of the slots in the register
    __m256i channel_base = _mm256_set1_epi16(ireg * SAMPLES_PER_REGISTER);
    __m256i channels = _mm256_add_epi16(channel_base, iota);

    for (size_t itime = 0; itime < info.timeWindowNumFrames; ++itime) {
      const size_t msg_index = itime / 12;
      const size_t msg_time_offset = itime % 12;
      const size_t index = msg_index * NREGISTERS * FRAMES_PER_MSG + FRAMES_PER_MSG * ireg + msg_time_offset;
      // const __m256i* rawp=reinterpret_cast<const __m256i*>(info.input)+index; // NOLINT

      // The current sample
      __m256i s = info.input->ymm(index);

      // First, find which channels are above/below the median,
      // since we need these as masks in the call to
      // frugal_accum_update_avx2
      __m256i is_gt = _mm256_cmpgt_epi16(s, median);
      __m256i is_eq = _mm256_cmpeq_epi16(s, median);
      // Would like a "not", but there isn't one. Emulate it
      // with xor against a register of all ones
      __m256i gt_or_eq = _mm256_or_si256(is_gt, is_eq);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
      __m256i is_lt = _mm256_xor_si256(gt_or_eq, _mm256_set1_epi16(0xffff));
#pragma GCC diagnostic pop
      // Update the 25th percentile in the channels that are below the median
      frugal_accum_update_avx2(quantile25, s, accum25, 10, is_lt);
      // Update the 75th percentile in the channels that are above the median
      frugal_accum_update_avx2(quantile75, s, accum75, 10, is_gt);
      // Update the median itself in all channels
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
      frugal_accum_update_avx2(median, s, accum, 10, _mm256_set1_epi16(0xffff));
#pragma GCC diagnostic pop
      // Actually subtract the pedestal
      s = _mm256_sub_epi16(s, median);

      // Find the interquartile range
      __m256i sigma = _mm256_sub_epi16(quantile75, quantile25);
      // Clamp sigma to a range where it won't overflow when
      // multiplied by info.multiplier*5
      sigma = _mm256_min_epi16(sigma, sigmaMax);

      // __m256i sigma = _mm256_set1_epi16(2000); // 20 ADC
      // --------------------------------------------------------------
      // Filtering
      // --------------------------------------------------------------

      // Don't let the sample exceed adcMax, which is the value
      // at which its filtered version might overflow
      s = _mm256_min_epi16(s, adcMax);

      // NB we're doing integer multiplication of
      // (short)*(short) here, so the result can be larger than
      // will fit in a short. `mullo` gives us back the low half
      // of the result, so it will be bogus if the result didn't
      // fit in a short. I'm relying on the fact that we started
      // with a 12-bit number which included the pedestal, and
      // we've subtracted the pedestal and put the result in a
      // 16-bit space to get us never to overflow. Another
      // approach might be to make the filter coeffs so large
      // that we _always_ overflow, and use `mulhi` instead

      // Try pipelining this with multiple accumulators, but it doesn't really make any difference
      //
      // TODO: Do the multiplication then right-shift before | July-22-2021 Philip Rodrigues (rodriges@fnal.gov)
      // adding the items together, to try to save us from
      // overflow
      __m256i filt0 = _mm256_setzero_si256();
      __m256i filt1 = _mm256_setzero_si256();
      __m256i filt2 = _mm256_setzero_si256();
      __m256i filt3 = _mm256_setzero_si256();

      // % would be slow, but we're making sure that NTAPS is a power of two so the optimizer ought to save us
      //#define ADDTAP0(x)
      //  filt0 = _mm256_add_epi16(filt0, _mm256_mullo_epi16(tap_256[x], prev_samp[(x + absTimeModNTAPS) % NTAPS]));
      //#define ADDTAP1(x)
      //  filt1 = _mm256_add_epi16(filt1, _mm256_mullo_epi16(tap_256[x], prev_samp[(x + absTimeModNTAPS) % NTAPS]));
      //#define ADDTAP2(x)
      //  filt2 = _mm256_add_epi16(filt2, _mm256_mullo_epi16(tap_256[x], prev_samp[(x + absTimeModNTAPS) % NTAPS]));
      //#define ADDTAP3(x)
      //  filt3 = _mm256_add_epi16(filt3, _mm256_mullo_epi16(tap_256[x], prev_samp[(x + absTimeModNTAPS) % NTAPS]));

      // ADDTAP0(0);
      filt0 = _mm256_add_epi16(filt0, _mm256_mullo_epi16(tap_256[0], prev_samp[(0 + absTimeModNTAPS) % NTAPS]));

      // ADDTAP1(1);
      filt1 = _mm256_add_epi16(filt1, _mm256_mullo_epi16(tap_256[1], prev_samp[(1 + absTimeModNTAPS) % NTAPS]));

      // ADDTAP2(2);
      filt2 = _mm256_add_epi16(filt2, _mm256_mullo_epi16(tap_256[2], prev_samp[(2 + absTimeModNTAPS) % NTAPS]));

      // ADDTAP3(3);
      filt3 = _mm256_add_epi16(filt3, _mm256_mullo_epi16(tap_256[3], prev_samp[(3 + absTimeModNTAPS) % NTAPS]));

      // ADDTAP0(4);
      filt0 = _mm256_add_epi16(filt0, _mm256_mullo_epi16(tap_256[4], prev_samp[(4 + absTimeModNTAPS) % NTAPS]));

      // ADDTAP1(5);
      filt1 = _mm256_add_epi16(filt1, _mm256_mullo_epi16(tap_256[5], prev_samp[(5 + absTimeModNTAPS) % NTAPS]));

      // ADDTAP2(6);
      filt2 = _mm256_add_epi16(filt2, _mm256_mullo_epi16(tap_256[6], prev_samp[(6 + absTimeModNTAPS) % NTAPS]));

      __m256i filt = _mm256_add_epi16(_mm256_add_epi16(filt0, filt1), _mm256_add_epi16(filt2, filt3));
      prev_samp[absTimeModNTAPS] = s;
      // This is a reference to the value in the ProcessingInfo,
      // so this line has the effect of directly modifying the
      // `info` object
      absTimeModNTAPS = (absTimeModNTAPS + 1) % NTAPS;

      // --------------------------------------------------------------
      // Hit finding
      // --------------------------------------------------------------
      // Mask for channels that are over the threshold in this step
      // const uint16_t threshold=2000; // NOLINT(build/unsigned)
      __m256i is_over = _mm256_cmpgt_epi16(filt, sigma * info.multiplier * info.threshold);
      // Mask for channels that left "over threshold" state this step
      __m256i left = _mm256_andnot_si256(is_over, prev_was_over);

      //-----------------------------------------
      // Update hit start times for the channels where a hit started
      const __m256i timenow = _mm256_set1_epi16(itime);
      //-----------------------------------------
      // Accumulate charge and time-over-threshold in the is_over channels

      // Really want an epi16 version of this, but the cmpgt and
      // cmplt functions set their epi16 parts to 0xff or 0x0,
      // so treating everything as epi8 works the same
      __m256i to_add_charge = _mm256_blendv_epi8(_mm256_set1_epi16(0), filt, is_over);
      // Divide by the multiplier before adding (implemented as a shift-right)
      hit_charge = _mm256_adds_epi16(hit_charge, _mm256_srai_epi16(to_add_charge, info.tap_exponent));

      // if(ireg==2){
      //     printf("itime=%ld\n", itime);
      //     printf("s:             "); print256_as16_dec(s);             printf("\n");
      //     printf("median:        "); print256_as16_dec(median);        printf("\n");
      //     printf("sigma:         "); print256_as16_dec(sigma);         printf("\n");
      //     printf("filt:          "); print256_as16_dec(filt);          printf("\n");
      //     printf("to_add_charge: "); print256_as16_dec(to_add_charge); printf("\n");
      //     printf("hit_charge:    "); print256_as16_dec(hit_charge);    printf("\n");
      //     printf("left:          "); print256_as16_dec(left);          printf("\n");
      // }

      __m256i to_add_tover = _mm256_blendv_epi8(_mm256_set1_epi16(0), _mm256_set1_epi16(1), is_over);
      hit_tover = _mm256_adds_epi16(hit_tover, to_add_tover);

      // Only store the values if there are >0 hits ending on
      // this sample. We have to save the entire 16-channel
      // register, which is inefficient, but whatever

      // Testing whether a whole register is zeroes turns out to be tricky. Here's a way:
      //
      // https://stackoverflow.com/questions/22674205/is-there-an-or-equivalent-to-ptest-in-x64-assembly
      //
      // In x64 assembly, PTEST %XMM0 -> %XMM1 sets the
      // zero-flag if none of the same bits are set in %XMM0 and
      // %XMM1, and sets the carry-flag if everything that is
      // set in %XMM0 is also set in %XMM1:
      const int no_hits_to_store = _mm256_testc_si256(_mm256_setzero_si256(), left);

      if (!no_hits_to_store) {

        ++nhits;
        // We have to save the whole register, including the
        // lanes that don't have anything interesting, but
        // we'll mask them to zero so they're easy to remove
        // in a later processing step.
        //
        // (TODO: Maybe we should do that processing step in this function?)
        //#define STORE_MASK(x) _mm256_storeu_si256(output_loc++, _mm256_blendv_epi8(_mm256_set1_epi16(0), x, left));

        _mm256_storeu_si256(output_loc++, channels); // NOLINT(runtime/increment_decrement)
        // Store the end time of the hit, not the start
        // time. Since we also have the time-over-threshold,
        // we can calculate the absolute 64-bit start time in
        // the caller. This saves faffing with hits that span
        // a message boundary, hopefully

        _mm256_storeu_si256(output_loc++, timenow); // NOLINT(runtime/increment_decrement)
        // STORE_MASK(hit_charge);
        _mm256_storeu_si256(
          output_loc++, // NOLINT(runtime/increment_decrement)
          _mm256_blendv_epi8(_mm256_set1_epi16(0), hit_charge, left));
        _mm256_storeu_si256(output_loc++, hit_tover);                  // NOLINT(runtime/increment_decrement)

        // reset hit_start, hit_charge and hit_tover in the channels we saved
        const __m256i zero = _mm256_setzero_si256();
        hit_charge = _mm256_blendv_epi8(hit_charge, zero, left);
        hit_tover = _mm256_blendv_epi8(hit_tover, zero, left);
      } // end if(!no_hits_to_store)

      prev_was_over = is_over;

    } // end loop over itime (times for this register)

    // Store the state, ready for the next time round
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.pedestals) + ireg, median);      // NOLINT
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.quantile25) + ireg, quantile25); // NOLINT
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.quantile75) + ireg, quantile75); // NOLINT

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.accum) + ireg, accum);     // NOLINT
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.accum25) + ireg, accum25); // NOLINT
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.accum75) + ireg, accum75); // NOLINT

    for (size_t j = 0; j < NTAPS; ++j) {
      _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.prev_samp) + NTAPS * ireg + j, prev_samp[j]); // NOLINT
    }

    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.prev_was_over) + ireg, prev_was_over); // NOLINT
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.hit_charge) + ireg, hit_charge);       // NOLINT
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(state.hit_tover) + ireg, hit_tover);         // NOLINT

  } // end loop over ireg (the 8 registers in this frame)

  info.absTimeModNTAPS = (info.absTimeModNTAPS + info.timeWindowNumFrames) % NTAPS;
  // Store the output
  for (int i = 0; i < 4; ++i) {
    _mm256_storeu_si256(output_loc++, _mm256_set1_epi16(swtpg::MAGIC)); // NOLINT(runtime/increment_decrement)
  }

  info.nhits = nhits;

} // NOLINT(readability/fn_size)

} // namespace swtpg

#endif // READOUT_SRC_WIB_TPG_PROCESSAVX2_HPP_
