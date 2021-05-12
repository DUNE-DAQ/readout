/**
 * @file test_ratelimiter_app.cxx Test application for 
 * ratelimiter implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "RateLimiter.hpp"
#include "RandomEngine.hpp"

#include "logging/Logging.hpp"

#include "readout/ReadoutTypes.hpp"

#include "folly/ConcurrentSkipList.h"

#include <atomic>
#include <string>
#include <chrono>
#include <memory>
#include <set>

using namespace dunedaq::readout;
using namespace folly;

int
main(int /*argc*/, char** /*argv[]*/)
{

  // ConcurrentSkipList from Folly
  typedef ConcurrentSkipList<types::WIB_SUPERCHUNK_STRUCT> SkipListT;
  typedef SkipListT::Accessor SkipListTAcc;
  //typedef SkipListT::Skipper SkipListTSkip; //Skipper accessor to test

  // Skiplist instance
  auto head_height = 2;
  std::shared_ptr<SkipListT> skl(SkipListT::createInstance(head_height));

  // Run for seconds
  int runsecs = 3600;

  // Run marker
  std::atomic<bool> marker{true};

  // RateLimiter
  TLOG() << "Creating ratelimiter with 10 KHz...";
  RateLimiter rl(10);

  // RateLimiter adjuster
  auto adjuster = std::thread([&]() {
    TLOG() << "Spawned adjuster thread...";
    RandomEngine re;
    auto rand_rates = re.get_random_population(runsecs, 10.0, 100.0);
    int idx = 0;
    while (marker) {
      TLOG() << "Adjusting rate to: " << rand_rates[idx] << " [kHz]";
      rl.adjust(rand_rates[idx]);
      if (idx > runsecs-1) {
        idx = 0;
      } else {
        ++idx;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });


  // Producer thread
  auto producer = std::thread([&]() {
    TLOG() << "SkipList Producer spawned... Creating accessor.";
    uint64_t ts = 0;
    while (marker) {
      types::WIB_SUPERCHUNK_STRUCT pl;
      auto plptr = const_cast<dunedaq::dataformats::WIBHeader*>(reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&pl));
      plptr->timestamp_1 = ts;
      plptr->timestamp_2 = ts >> 32;
      {
        SkipListTAcc prodacc(skl);
        prodacc.insert(std::move(pl));
      }
      ts += 25;
      rl.limit();
    }
  });


  // Cleanup thread
  auto cleaner = std::thread([&]() {
    std::string tname("Cleaner");
    TLOG() << "SkipList " << tname << " spawned... Creating accessor.";
    uint64_t max_time_diff = 100000; // accounts for few seconds in FE clock
    while (marker) {
      SkipListTAcc cleanacc(skl);
      TLOG() << tname << ": SkipList size: " << cleanacc.size();
      auto tail = cleanacc.last();
      auto head = cleanacc.first();
      if (tail && head) {
        auto tailptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(tail);
        auto headptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(head);
        auto tailts = tailptr->get_timestamp();
        auto headts = headptr->get_timestamp();
        if (headts - tailts > max_time_diff) { // ts differnce exceeds maximum
          uint64_t timediff = max_time_diff;
          auto removed_ctr = 0;
          while (timediff >= max_time_diff) {
            bool removed = cleanacc.remove(*tail);
            if (!removed) {
              TLOG() << tname << ": Unsuccessfull remove: " << removed;
            } else {
              ++removed_ctr;
            }
            tail = cleanacc.last();
            tailptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(tail);
            tailts = tailptr->get_timestamp();
            timediff = headts - tailts;
          }
          TLOG() << tname <<  ": Cleared " << removed_ctr << " elements.";
        }      
      } else {
        TLOG() << tname << ": Didn't manage to get SKL head and tail!";
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  });


  // Data extractor (Trigger Matcher, timestamp finder.)
  auto extractor = std::thread([&]() {
    std::string tname("TriggerMatcher");
    TLOG() << "SkipList " << tname << " spawned...";
    while (marker) {
      { // block enforce for Accessor
        SkipListTAcc exacc(skl);
        auto tail = exacc.last();
        auto head = exacc.first();
        if (tail && head) {
          auto tailptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(tail);
          auto headptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(head);
          auto tailts = tailptr->get_timestamp();
          auto headts = headptr->get_timestamp();

          // Adjust trigger right in between:
          auto trigts = (tailts + headts) / static_cast<uint64_t>(2); // NOLINT
          types::WIB_SUPERCHUNK_STRUCT trigger_pl;
          auto trigptr = const_cast<dunedaq::dataformats::WIBHeader*>(reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&trigger_pl));
          trigptr->timestamp_1 = trigts;
          trigptr->timestamp_2 = trigts >> 32;

          // Find closest to trigger payload
          auto close = exacc.lower_bound(trigger_pl);
          //if (close) {
            auto foundptr = reinterpret_cast<const dunedaq::dataformats::WIBHeader*>(&(*close));
            auto foundts = foundptr->get_timestamp();
            TLOG() << tname << ": Found element lower bound to " << trigts << " in skiplist with timestamp --> " << foundts;
          //} else {
            //TLOG() << tname << ": Timestamp doesn't seem to be in Skiplist...";
          //}
          //
        //} else {
          // TLOG() << tname << ": Didn't manage to get SKL head and tail!";
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });


  // Killswitch that flips the run marker
  auto killswitch = std::thread([&]() {
    TLOG() << "Application will terminate in 5s...";
    std::this_thread::sleep_for(std::chrono::seconds(runsecs));
    marker.store(false);
  });


  // Join local threads
  TLOG() << "Flipping killswitch in order to stop...";
  if (killswitch.joinable()) {
    killswitch.join();
  }

  // Exit
  TLOG() << "Exiting.";
  return 0;
}
