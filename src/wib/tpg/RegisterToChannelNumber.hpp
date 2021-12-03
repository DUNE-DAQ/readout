/**
 * @file RegisterToChannelNumber.hpp Convert from WIB1 data AVX register position to offline channel numbers
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef READOUT_SRC_WIB_TPG_REGISTERTOCHANNELNUMBER_HPP_
#define READOUT_SRC_WIB_TPG_REGISTERTOCHANNELNUMBER_HPP_

#include "detchannelmaps/TPCChannelMap.hpp"
#include "detdataformats/wib/WIBFrame.hpp"
#include "readout/ReadoutTypes.hpp"
#include "wib/tpg/FrameExpand.hpp"
#include "wib/tpg/TPGConstants.hpp"

#include <sys/types.h>
#include <vector>

namespace swtpg {

struct RegisterChannelMap
{
  // TODO: Make these the right size
  uint collection[256];
  uint induction[256];
};

RegisterChannelMap
get_register_to_offline_channel_map(const dunedaq::detdataformats::wib::WIBFrame* frame, std::string channel_map_name)
{
  auto ch_map = dunedaq::detchannelmaps::make_map(channel_map_name);
  // Find the lowest offline channel number of all the channels in the input frame
  uint min_ch = UINT_MAX;
  for (size_t ich = 0; ich < dunedaq::detdataformats::wib::WIBFrame::s_num_ch_per_frame; ++ich) {
    auto offline_ch = ch_map->get_offline_channel_from_crate_slot_fiber_chan(
      frame->get_wib_header()->crate_no, frame->get_wib_header()->slot_no, frame->get_wib_header()->fiber_no, ich);
    min_ch = std::min(min_ch, offline_ch);
  }
  TLOG_DEBUG(0) << "get_register_to_offline_channel_map for crate " << frame->get_wib_header()->crate_no << " slot " << frame->get_wib_header()->slot_no << " fiber " << frame->get_wib_header()->fiber_no << ". min_ch is " << min_ch;
  // Now set each of the channels in our test frame to their
  // corresponding offline channel number, minus the minimum channel
  // number we just calculated (so we don't overflow the 12 bits we
  // have available)
  dunedaq::readout::types::WIB_SUPERCHUNK_STRUCT superchunk;
  memset(superchunk.data, 0, sizeof(dunedaq::readout::types::WIB_SUPERCHUNK_STRUCT));

  dunedaq::detdataformats::wib::WIBFrame* test_frame =
    reinterpret_cast<dunedaq::detdataformats::wib::WIBFrame*>(&superchunk);
  for (size_t ich = 0; ich < dunedaq::detdataformats::wib::WIBFrame::s_num_ch_per_frame; ++ich) {
    auto offline_ch = ch_map->get_offline_channel_from_crate_slot_fiber_chan(
      frame->get_wib_header()->crate_no, frame->get_wib_header()->slot_no, frame->get_wib_header()->fiber_no, ich);
    test_frame->set_channel(ich, offline_ch - min_ch);
  }
  // Expand the test frame, so the offline channel numbers are now in the relevant places in the output registers
  swtpg::MessageRegistersCollection collection_registers;
  swtpg::MessageRegistersInduction induction_registers;
  expand_message_adcs_inplace(&superchunk, &collection_registers, &induction_registers);

  RegisterChannelMap ret;
  for (size_t i = 0; i < 6 * SAMPLES_PER_REGISTER; ++i) {
    ret.collection[i] = collection_registers.uint16(i) + min_ch;
  }
  for (size_t i = 0; i < 10 * SAMPLES_PER_REGISTER; ++i) {
    ret.induction[i] = induction_registers.uint16(i) + min_ch;
  }

  return ret;
}

} // namespace swtpg

#endif // READOUT_SRC_WIB_TPG_REGISTERTOCHANNELNUMBER_HPP_
