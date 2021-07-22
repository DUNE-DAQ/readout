#ifndef PROCESSINGTASKS_H
#define PROCESSINGTASKS_H

//#include "zmq.h"

//#include "NetioWIBRecords.hh"

#include "frame_expand.h"

#include <chrono>

// ProcessingTasks.h
// Author: Philip Rodrigues
//
// Some classes to facilitate telling the hit finding processing
// thread about new netio messages that are ready for processing. The
// main thread needs to return as soon as possible, so it just sends a
// message to the processing thread with the timestamp of and pointer
// to the netio message that's ready for processing. We use zeromq to
// communicate between threads, which has the nice effect that it does
// all the queueing for us

namespace ProcessingTasks {

// The details of a netio message to be processed
struct ItemToProcess
{
  uint64_t timestamp;         // The timestamp of the first frame in the
                              // netio message
  SUPERCHUNK_CHAR_STRUCT scs; // The raw message to be
                              // processed
  uint64_t timeQueued;        // The time this item was queued
                              // so receivers can detect
                              // whether they're getting behind
};

constexpr uint64_t END_OF_MESSAGES = 0xffffffffffffffff;

// Return the current steady clock in microseconds
inline uint64_t
now_us()
{
  // std::chrono is the worst
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
    .count();
}

} // end namespace ProcessingTasks

#endif // include guard

/* Local Variables:  */
/* mode: c++         */
/* c-basic-offset: 4 */
/* End:              */
