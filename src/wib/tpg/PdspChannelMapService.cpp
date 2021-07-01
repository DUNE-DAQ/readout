///////////////////////////////////////////////////////////////////////////////////////////////////
// Class:       PdspChannelMapService
// Module type: service
// File:        PdspChannelMapService.h
// Author:      Jingbo Wang (jiowang@ucdavis.edu), February 2018
//
// Implementation of hardware-offline channel mapping reading from a file.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PdspChannelMapService.h"
#include <stdexcept>
#include <sstream>

// Bad channel value
unsigned int bad() {
  unsigned int val = std::numeric_limits<unsigned int>::max();
  return val;
}

PdspChannelMapService::PdspChannelMapService(std::string rcename, std::string felixname) {

  fBadCrateNumberWarningsIssued = 0;
  fBadSlotNumberWarningsIssued = 0;
  fBadFiberNumberWarningsIssued = 0;
  fSSPBadChannelNumberWarningsIssued = 0;
  fASICWarningsIssued = 0;
  fASICChanWarningsIssued = 0;

  std::ifstream inFile(rcename, std::ios::in);
  if(inFile.bad() || inFile.fail() || !inFile.is_open()){
      throw std::runtime_error(std::string("Bad file ")+std::string(rcename));
  }
  std::string line;

  while (std::getline(inFile,line)) {
    unsigned int crateNo, slotNo, fiberNo, FEMBChannel, StreamChannel, slotID, fiberID, chipNo, chipChannel, asicNo, asicChannel, planeType, offlineChannel;
    std::stringstream linestream(line);
    linestream >> crateNo >> slotNo >> fiberNo>> FEMBChannel >> StreamChannel >> slotID >> fiberID >> chipNo >> chipChannel >> asicNo >> asicChannel >> planeType >> offlineChannel;

    // fill lookup tables.  Throw an exception if any number is out of expected bounds.
    // checking for negative values produces compiler warnings as these are unsigned ints

    if (offlineChannel >= fNChans)
    {
        throw std::logic_error("Ununderstood Offline Channel");
    }
    if (crateNo >= fNCrates)
    {
        throw std::logic_error("Ununderstood Crate Number");
    }
    if (slotNo >= fNSlots)
    {
        throw std::logic_error("Ununderstood Slot Number");
    }
    if (fiberNo >= fNFibers)
    {
        throw std::logic_error("Ununderstood Fiber Number");
    }
    if (StreamChannel >= fNFEMBChans)
    {
	throw std::logic_error("Ununderstood FEMB (Stream) Channel Number");
    }

    farrayCsfcToOffline[crateNo][slotNo][fiberNo][StreamChannel] = offlineChannel;
    fvAPAMap[offlineChannel] = crateNo; 
    fvWIBMap[offlineChannel] = slotNo; 
    fvFEMBMap[offlineChannel] = fiberNo; 
    fvFEMBChannelMap[offlineChannel] = FEMBChannel; 
    fvStreamChannelMap[offlineChannel] = StreamChannel;
    fvSlotIdMap[offlineChannel] = slotID; 
    fvFiberIdMap[offlineChannel] = fiberID; 
    fvChipMap[offlineChannel] = chipNo; 
    fvChipChannelMap[offlineChannel] = chipChannel;
    fvASICMap[offlineChannel] = asicNo;
    fvASICChannelMap[offlineChannel] = asicChannel;
    fvPlaneMap[offlineChannel] = planeType;

  }
  inFile.close();

  std::ifstream FELIXinFile(felixname, std::ios::in);
  if(FELIXinFile.bad() || FELIXinFile.fail() || !FELIXinFile.is_open()){
      throw std::runtime_error(std::string("Bad file ")+std::string(felixname));
  }

  while (std::getline(FELIXinFile,line)) {
    unsigned int crateNo, slotNo, fiberNo, FEMBChannel, StreamChannel, slotID, fiberID, chipNo, chipChannel, asicNo, asicChannel, planeType, offlineChannel;
    std::stringstream linestream(line);
    linestream >> crateNo >> slotNo >> fiberNo>> FEMBChannel >> StreamChannel >> slotID >> fiberID >> chipNo >> chipChannel >> asicNo >> asicChannel >> planeType >> offlineChannel;

    // fill lookup tables.  Throw an exception if any number is out of expected bounds.
    // checking for negative values produces compiler warnings as these are unsigned ints

    if (offlineChannel >= fNChans)
    {
        throw std::logic_error("Ununderstood Offline Channel");
    }
    if (crateNo >= fNCrates)
    {
        throw std::logic_error("Ununderstood Crate Number");
    }
    if (slotNo >= fNSlots)
    {
        throw std::logic_error("Ununderstood Slot Number");
    }
    if (fiberNo >= fNFibers)
    {
        throw std::logic_error("Ununderstood Fiber Number");
    }
    if (StreamChannel >= fNFEMBChans)
    {
	throw std::logic_error("Ununderstood FEMB (Stream) Channel Number");
    }

    fFELIXarrayCsfcToOffline[crateNo][slotNo][fiberNo][StreamChannel] = offlineChannel;
    fFELIXvAPAMap[offlineChannel] = crateNo; 
    fFELIXvWIBMap[offlineChannel] = slotNo; 
    fFELIXvFEMBMap[offlineChannel] = fiberNo; 
    fFELIXvFEMBChannelMap[offlineChannel] = FEMBChannel; 
    fFELIXvStreamChannelMap[offlineChannel] = StreamChannel;
    fFELIXvSlotIdMap[offlineChannel] = slotID; 
    fFELIXvFiberIdMap[offlineChannel] = fiberID; 
    fFELIXvChipMap[offlineChannel] = chipNo; 
    fFELIXvChipChannelMap[offlineChannel] = chipChannel;
    fFELIXvASICMap[offlineChannel] = asicNo;
    fFELIXvASICChannelMap[offlineChannel] = asicChannel;
    fFELIXvPlaneMap[offlineChannel] = planeType;

  }
  inFile.close();

  // APA numbering -- hardcoded here.
  // Installation numbering:
  //             APA5  APA6  APA4
  //  beam -->
  //             APA3  APA2  APA1
  //
  //  The Offline numbering:
  //             APA1  APA3  APA5
  //  beam -->
  //             APA0  APA2  APA4
  //
  fvInstalledAPA[0] = 3;
  fvInstalledAPA[1] = 5;
  fvInstalledAPA[2] = 2;
  fvInstalledAPA[3] = 6;
  fvInstalledAPA[4] = 1;
  fvInstalledAPA[5] = 4;

  // and the inverse map -- shifted by 1 -- the above list must start counting at 1.

  for (size_t i=0; i<6; ++i)
    {
      fvTPCSet_VsInstalledAPA[fvInstalledAPA[i]-1] = i;
    }
}

// assumes crate goes from 1-6, in "installed crate ordering"
// assumes slot goes from 0-5.
// assumes fiber goes from 1-4.
// These conventions are observed in Run 2973, a cryo commissioning run.

unsigned int PdspChannelMapService::GetOfflineNumberFromDetectorElements(unsigned int crate, unsigned int slot, unsigned int fiber, unsigned int streamchannel, FelixOrRCE frswitch) {

  unsigned int offlineChannel=0;
  unsigned int lcrate = crate;
  unsigned int lslot = slot;
  unsigned int lfiber = fiber;

  if (crate > fNCrates || crate == 0)
    {
      if ( count_bits(fBadCrateNumberWarningsIssued) == 1)
	{
            std::cout << "PdspChannelMapService: Bad Crate Number, expecting a number between 1 and 6.  Falling back to 1.  Ununderstood crate number=" << crate << std::endl;
	}
      fBadCrateNumberWarningsIssued++;
      lcrate = 1;
    }

  if (slot >= fNSlots)
    {
      if (count_bits(fBadSlotNumberWarningsIssued) == 1)
	{
            std::cout << "PdspChannelMapService: Bad slot number, using slot number zero as a fallback.  Ununderstood slot number: " << slot << std::endl;
	}
      fBadSlotNumberWarningsIssued++;
      lslot = 0;
    }

  if (fiber > fNFibers || fiber == 0)
    {
      if (count_bits(fBadFiberNumberWarningsIssued)==1)
	{
            std::cout << "PdspChannelMapService: Bad fiber number, falling back to 1.  Ununderstood fiber number: " << fiber;
	}
      fBadFiberNumberWarningsIssued++;
      lfiber = 1;
    }

  if (streamchannel >= fNFEMBChans)
  {
      std::cout << streamchannel << " >= " << fNFEMBChans << std::endl;
        throw std::logic_error("Ununderstood Stream (FEMB) chan"); 
    }

  if (frswitch == kRCE)
    {
      offlineChannel = farrayCsfcToOffline[fvTPCSet_VsInstalledAPA[lcrate-1]][lslot][lfiber-1][streamchannel];
    }
  else
    {
      offlineChannel = fFELIXarrayCsfcToOffline[fvTPCSet_VsInstalledAPA[lcrate-1]][lslot][lfiber-1][streamchannel];
    }

  return offlineChannel;
}

// does not depend on FELIX or RCE -- offline channels should always map to the same APA/crate regardless of RCE or FELIX

unsigned int PdspChannelMapService::APAFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvAPAMap[offlineChannel];
  // return fFELIXvAPAMap[offlineChannel];   // -- FELIX one -- should be the same
}

unsigned int PdspChannelMapService::InstalledAPAFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  unsigned int offlineAPA = fvAPAMap[offlineChannel];
  if (offlineAPA > 5)
    {
        throw std::logic_error("Offline APA Number out of range");
    }
  return fvInstalledAPA[fvAPAMap[offlineChannel]];
}

// does not depend on FELIX or RCE 

unsigned int PdspChannelMapService::WIBFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvWIBMap[offlineChannel];       
  // return fFELIXvWIBMap[offlineChannel];    // -- FELIX one -- should be the same
}

// does not depend on FELIX or RCE 

unsigned int PdspChannelMapService::FEMBFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvFEMBMap[offlineChannel]+1; 
  //return fFELIXvFEMBMap[offlineChannel];   
}

// does not depend on FELIX or RCE 

unsigned int PdspChannelMapService::FEMBChannelFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvFEMBChannelMap[offlineChannel];       
  // return fFELIXvFEMBChannelMap[offlineChannel];    // -- FELIX one -- should be the same
}

// this one does depend on FELIX or RCE

unsigned int PdspChannelMapService::StreamChannelFromOfflineChannel(unsigned int offlineChannel, FelixOrRCE frswitch) const {
  check_offline_channel(offlineChannel);
  if (frswitch == kRCE)
    {
      return fvStreamChannelMap[offlineChannel];       
    }
  else
    {
      return fFELIXvStreamChannelMap[offlineChannel];       
    }
}

// does not depend on FELIX or RCE 

unsigned int PdspChannelMapService::SlotIdFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvSlotIdMap[offlineChannel];  
  //  return fFELIXvSlotIdMap[offlineChannel];    // -- FELIX one -- should be the same
}

// may potentially depend on FELIX or RCE, but if fibers are switched between the WIB and the FELIX or RCE,
// we can fix this in the channel map but report with this method the fiber coming out of the WIB, not the
// one going in to the FELIX or RCE

unsigned int PdspChannelMapService::FiberIdFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvFiberIdMap[offlineChannel];       
}

// does not depend on FELIX or RCE 

unsigned int PdspChannelMapService::ChipFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvChipMap[offlineChannel];       
}

unsigned int PdspChannelMapService::AsicFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvChipMap[offlineChannel];       
}

unsigned int PdspChannelMapService::ChipChannelFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvChipChannelMap[offlineChannel];
}

// from David Adams -- use the chip channel instead of the asic channel

unsigned int PdspChannelMapService::AsicChannelFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvChipChannelMap[offlineChannel];
}

// really shouldn't be using this as it doesn't mean asic

unsigned int PdspChannelMapService::ASICFromOfflineChannel(unsigned int offlineChannel) {
  if (count_bits(fASICWarningsIssued) == 1)
    {
        std::cout << "PdspChannelMapService: Deprecated call to ASICFromOfflineChannel.  Use AsicLinkFromOfflineChannel" << std::endl;
    }
  fASICWarningsIssued++;
  check_offline_channel(offlineChannel);
  return fvASICMap[offlineChannel];       
}

unsigned int PdspChannelMapService::AsicLinkFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvASICMap[offlineChannel];       
}

unsigned int PdspChannelMapService::ASICChannelFromOfflineChannel(unsigned int offlineChannel) {
  if (count_bits(fASICChanWarningsIssued) == 1)
    {
        std::cout << "PdspChannelMapService: Deprecated call to ASICChannelFromOfflineChannel.  Not a meaningful number -- channels are grouped by 16's not 8's" << std::endl;
    }
  fASICChanWarningsIssued++;
  check_offline_channel(offlineChannel);
  return fvASICChannelMap[offlineChannel];
}

unsigned int PdspChannelMapService::PlaneFromOfflineChannel(unsigned int offlineChannel) const {
  check_offline_channel(offlineChannel);
  return fvPlaneMap[offlineChannel];       
}

size_t PdspChannelMapService::count_bits(size_t i)
{
  size_t result=0;
  size_t s = sizeof(size_t)*8;
  for (size_t j=0; j<s; ++j)
    {
      if (i & 1) ++result;
      i >>= 1;
    }
  return result;
}

unsigned int PdspChannelMapService::SSPOfflineChannelFromOnlineChannel(unsigned int onlineChannel) 
{
  unsigned int lchannel = onlineChannel;

  if (onlineChannel > fNSSPChans)
    {
      if (count_bits(fSSPBadChannelNumberWarningsIssued)==1)
	{
            std::cout << "PdspChannelMapService: Online Channel Number too high, using zero as a fallback: " << onlineChannel << std::endl;
	}
      fSSPBadChannelNumberWarningsIssued++;
      lchannel = 0;
    }
  return farraySSPOnlineToOffline[lchannel];
}

unsigned int PdspChannelMapService::SSPOnlineChannelFromOfflineChannel(unsigned int offlineChannel) const
{
  SSP_check_offline_channel(offlineChannel);
  return farraySSPOfflineToOnline[offlineChannel];
}

unsigned int PdspChannelMapService::SSPAPAFromOfflineChannel(unsigned int offlineChannel) const
{
  SSP_check_offline_channel(offlineChannel);
  return fvSSPAPAMap[offlineChannel];
}

unsigned int PdspChannelMapService::SSPWithinAPAFromOfflineChannel(unsigned int offlineChannel) const
{
  SSP_check_offline_channel(offlineChannel);
  return fvSSPWithinAPAMap[offlineChannel];
}

unsigned int PdspChannelMapService::SSPGlobalFromOfflineChannel(unsigned int offlineChannel) const
{
  SSP_check_offline_channel(offlineChannel);
  return fvSSPGlobalMap[offlineChannel];
}

unsigned int PdspChannelMapService::SSPChanWithinSSPFromOfflineChannel(unsigned int offlineChannel) const
{
  SSP_check_offline_channel(offlineChannel);
  return fvSSPChanWithinSSPMap[offlineChannel];
}

unsigned int PdspChannelMapService::OpDetNoFromOfflineChannel(unsigned int offlineChannel) const
{
  SSP_check_offline_channel(offlineChannel);
  return fvOpDetNoMap[offlineChannel];
}
