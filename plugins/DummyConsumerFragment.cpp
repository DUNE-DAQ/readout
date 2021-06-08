/**
 * @file DummyConsumerFragment.cpp Module that consumes fragments from a queue
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_
#define READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_

#include "DummyConsumer.cpp"
#include "DummyConsumer.hpp"
#include "dataformats/Fragment.hpp"
#include "dataformats/wib/WIBFrame.hpp"
#include "readout/ReadoutTypes.hpp"

#include <memory>
#include <string>

namespace dunedaq {
namespace readout {
class DummyConsumerFragment : public DummyConsumer<std::unique_ptr<dunedaq::dataformats::Fragment>>
{
public:
  explicit DummyConsumerFragment(const std::string name)
    : DummyConsumer<std::unique_ptr<dunedaq::dataformats::Fragment>>(name)
  {}

  void packet_callback(std::unique_ptr<dunedaq::dataformats::Fragment>& packet) override
  {
    dunedaq::dataformats::FragmentHeader header = packet->get_header();
    TLOG_DEBUG(TLVL_WORK_STEPS) << header;
    validate(*packet.get());
  }

  void validate(dunedaq::dataformats::Fragment& fragment) {

    if ((fragment.get_header().fragment_type == static_cast<dataformats::fragment_type_t>(dataformats::FragmentType::kTPCData)) ||
        (static_cast<dataformats::WIBFrame*>(fragment.get_data())->get_wib_header()->sof == 8)) {
      // Only do wib1 validation for now
      int num_frames = (fragment.get_size() - sizeof(dataformats::FragmentHeader)) / 464;
      int num_superchunks = num_frames / 12;
      auto window_begin = fragment.get_header().window_begin;
      auto window_end = fragment.get_header().window_end;
      
      types::WIB_SUPERCHUNK_STRUCT* first_superchunk = static_cast<types::WIB_SUPERCHUNK_STRUCT*>(fragment.get_data());
      types::WIB_SUPERCHUNK_STRUCT* last_superchunk = reinterpret_cast<types::WIB_SUPERCHUNK_STRUCT*>(static_cast<char*>(fragment.get_data()) + ((num_superchunks-1) * types::WIB_SUPERCHUNK_SIZE));

      //TLOG() << num_superchunks;
      //TLOG() << "window_begin: " << window_begin << ", window_end: " << window_end << ", first: " << first_superchunk->get_timestamp() << ", last: " << last_superchunk->get_timestamp();
      if (!((first_superchunk->get_timestamp() > window_begin - 25) && (first_superchunk->get_timestamp() <= window_begin))) {
        TLOG() << "First fragment not correctly aligned";
      }
      if (!((last_superchunk->get_timestamp() < window_end) && (last_superchunk->get_timestamp() >= window_end - 300))) {
        TLOG() << "Last fragment not correctly aligned";
      }

      for (int i = 0; i < num_superchunks; ++i) {
        types::WIB_SUPERCHUNK_STRUCT* superchunk = reinterpret_cast<types::WIB_SUPERCHUNK_STRUCT*>(static_cast<char*>(fragment.get_data()) + (i * types::WIB_SUPERCHUNK_SIZE));
        if (superchunk->get_timestamp() < fragment.get_header().window_begin || superchunk->get_timestamp() >= fragment.get_header().window_end) {
          TLOG() << "Fragment validation encountered fragment not fitting the requested window";
        }
      }
    } else if (fragment.get_header().fragment_type == static_cast<dataformats::fragment_type_t>(dataformats::FragmentType::kPDSData)) {
      int num_frames = (fragment.get_size() - sizeof(dataformats::FragmentHeader)) / 584;
      int num_superchunks = num_frames / 12;

      //TLOG() << num_superchunks;
      //TLOG() << "window_begin: " << window_begin << ", window_end: " << window_end << ", first: " << first_superchunk->get_timestamp() << ", last: " << last_superchunk->get_timestamp();

      for (int i = 0; i < num_superchunks; ++i) {
        types::PDS_SUPERCHUNK_STRUCT* superchunk = reinterpret_cast<types::PDS_SUPERCHUNK_STRUCT*>(static_cast<char*>(fragment.get_data()) + (i * types::PDS_SUPERCHUNK_SIZE));
        if (superchunk->get_timestamp() < fragment.get_header().window_begin || superchunk->get_timestamp() >= fragment.get_header().window_end) {
          TLOG() << "Fragment validation encountered fragment not fitting the requested window";
        }
      }
    }


  }

};
} // namespace readout
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::readout::DummyConsumerFragment)

#endif // READOUT_PLUGINS_DUMMYCONSUMERFRAGMENT_HPP_