///////////////////////////////////////////////////////////////////////////////////////////////////
// Class:       PdspChannelMapService
// Module type: service
// File:        PdspChannelMapService.h
// Author:      Jingbo Wang (jiowang@ucdavis.edu), February 2018
//
// Implementation of hardware-offline channel mapping reading from a file.  
// Separate files for TPC wires and SSP modules
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PdspChannelMapService_H
#define PdspChannelMapService_H

#include <map>
#include <vector>
#include <iostream>
#include <limits>
#include <fstream>

class PdspChannelMapService {

public:

    PdspChannelMapService(std::string, std::string);

  typedef enum _FelixOrRCE
  {
    kRCE,
    kFELIX
  } FelixOrRCE;

  /////////////////////////\ ProtoDUNE-SP channel map fundtions //////////////////////////////

  // TPC channel map accessors

  // Map instrumentation numbers (crate:slot:fiber:FEMBchannel) to offline channel number
  // FEMB channel number is really the stream index number, but for FELIX, it is restricted to be in the range 0:127, so really it's more like an FEMB channel number

  unsigned int GetOfflineNumberFromDetectorElements(unsigned int crate, unsigned int slot, unsigned int fiber, unsigned int fembchannel, FelixOrRCE frswitch);
  	  
  /// Returns APA/crate
  unsigned int APAFromOfflineChannel(unsigned int offlineChannel) const;

  /// Returns APA/crate in installation notation
  unsigned int InstalledAPAFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns WIB/slot
  unsigned int WIBFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns FEMB/fiber
  unsigned int FEMBFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns FEMB channel
  unsigned int FEMBChannelFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns RCE(FELIX) stream(frame) channel
  unsigned int StreamChannelFromOfflineChannel(unsigned int offlineChannel, FelixOrRCE frswitch) const;
  
  /// Returns global slot ID
  unsigned int SlotIdFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns global fiber ID
  unsigned int FiberIdFromOfflineChannel(unsigned int offlineChannel) const;

  /// Returns chip number
  unsigned int ChipFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns chip channel number
  unsigned int ChipChannelFromOfflineChannel(unsigned int offlineChannel) const;
  
  /// Returns ASIC number -- to be deprecated
  unsigned int ASICFromOfflineChannel(unsigned int offlineChannel);
  
  /// Returns ASIC channel number -- to be deprecated
  unsigned int ASICChannelFromOfflineChannel(unsigned int offlineChannel);

  // replaced by these
  
  unsigned int AsicFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int AsicChannelFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int AsicLinkFromOfflineChannel(unsigned int offlineChannel) const;

  /// Returns plane
  unsigned int PlaneFromOfflineChannel(unsigned int offlineChannel) const;
  

  // SSP channel map accessors

  unsigned int SSPOfflineChannelFromOnlineChannel(unsigned int onlineChannel);

  unsigned int SSPOnlineChannelFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int SSPAPAFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int SSPWithinAPAFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int SSPGlobalFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int SSPChanWithinSSPFromOfflineChannel(unsigned int offlineChannel) const;

  unsigned int OpDetNoFromOfflineChannel(unsigned int offlineChannel) const;

private:

  // hardcoded TPC channel map sizes
  // Note -- we are assuming that FELIX with its two fibers per fragment knows the fiber numbers and that we aren't
  // encoding double-size channel lists for FELIX with single fiber numbers.

  const size_t fNChans = 15360;
  const size_t fNCrates = 6;     
  const size_t fNSlots = 5;
  const size_t fNFibers = 4;
  const size_t fNFEMBChans = 128; 

  // hardcoded SSP channel map sizes

  const size_t fNSSPChans = 288;
  //const size_t fNSSPs = 24;
  //const size_t fNSSPsPerAPA = 4;
  const size_t fNChansPerSSP = 12;
  const size_t fNAPAs = 6;

  // control behavior in case we need to fall back to default behavior

  size_t fBadCrateNumberWarningsIssued;
  size_t fBadSlotNumberWarningsIssued;
  size_t fBadFiberNumberWarningsIssued;
  size_t fSSPBadChannelNumberWarningsIssued;

  size_t fASICWarningsIssued;
  size_t fASICChanWarningsIssued;

  // TPC Maps
  unsigned int farrayCsfcToOffline[6][5][4][128];  // implement as an array.  Do our own bounds checking

  // lookup tables as functions of offline channel number

  unsigned int fvAPAMap[15360]; // APA(=crate)
  unsigned int fvWIBMap[15360];	// WIB(slot)
  unsigned int fvFEMBMap[15360];	// FEMB(fiber)
  unsigned int fvFEMBChannelMap[15360];	// FEMB internal channel
  unsigned int fvStreamChannelMap[15360];	// RCE(FELIX) internal channel
  unsigned int fvSlotIdMap[15360]; // global WIB(slot) ID
  unsigned int fvFiberIdMap[15360]; // global FEMB(fiber) ID
  unsigned int fvChipMap[15360]; // Chip
  unsigned int fvChipChannelMap[15360]; //Chip internal channel 
  unsigned int fvASICMap[15360]; // ASIC
  unsigned int fvASICChannelMap[15360]; // ASIC internal channel
  unsigned int fvPlaneMap[15360]; // Plane type

  unsigned int fFELIXarrayCsfcToOffline[6][5][4][128];  // implement as an array.  Do our own bounds checking
  unsigned int fFELIXvAPAMap[15360]; // APA(=crate)
  unsigned int fFELIXvWIBMap[15360];	// WIB(slot)
  unsigned int fFELIXvFEMBMap[15360];	// FEMB(fiber)
  unsigned int fFELIXvFEMBChannelMap[15360];	// FEMB internal channel
  unsigned int fFELIXvStreamChannelMap[15360];	// RCE(FELIX) internal channel
  unsigned int fFELIXvSlotIdMap[15360]; // global WIB(slot) ID
  unsigned int fFELIXvFiberIdMap[15360]; // global FEMB(fiber) ID
  unsigned int fFELIXvChipMap[15360]; // Chip
  unsigned int fFELIXvChipChannelMap[15360]; //Chip internal channel 
  unsigned int fFELIXvASICMap[15360]; // ASIC
  unsigned int fFELIXvASICChannelMap[15360]; // ASIC internal channel
  unsigned int fFELIXvPlaneMap[15360]; // Plane type

  unsigned int fvInstalledAPA[6];  // APA as installed.  This array maps the two conventions.  Argument = offline, value = installed
  unsigned int fvTPCSet_VsInstalledAPA[6];  // inverse map

  // SSP Maps

  unsigned int farraySSPOnlineToOffline[288];  // all accesses to this array need to be bounds-checked first.
  unsigned int farraySSPOfflineToOnline[288];  
  unsigned int fvSSPAPAMap[288];
  unsigned int fvSSPWithinAPAMap[288];  // Global SSP number 11, 12, 21, etc.
  unsigned int fvSSPGlobalMap[288];     // Also global SSP number 11, 12, 21, etc. 
  unsigned int fvSSPChanWithinSSPMap[288];
  unsigned int fvOpDetNoMap[288];   // PDS op det number

  size_t count_bits(size_t i);  // returns the number of bits set, for use in determing whether to print a warning out
 
  //-----------------------------------------------

  void check_offline_channel(unsigned int offlineChannel) const
  {
  if (offlineChannel >= fNChans)
    {      
        throw std::logic_error("Offline TPC Channel Number out of range");
    }
  };

  void SSP_check_offline_channel(unsigned int offlineChannel) const
  {
  if (offlineChannel >= fNSSPChans)
    {      
        throw std::logic_error("Offline SSP Channel Number out of range");
    }
  };
};

#endif
