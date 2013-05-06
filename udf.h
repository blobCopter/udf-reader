#ifndef UDF_H
#define UDF_H

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>
#include "udf_types.h"

#define SECTOR_SIZE 2048

struct PrimaryVolumeDescriptor {
    /* ECMA 167 3/10.1 */
  struct tag DescriptorTag;
  Uint32 VolumeDescriptorSequenceNumber;
  Uint32 PrimaryVolumeDescriptorNumber;
  dstring VolumeIdentifier[32];
  Uint16 VolumeSequenceNumber;
  Uint16 MaximumVolumeSequenceNumber;
  Uint16 InterchangeLevel;
  Uint16 MaximumInterchangeLevel;
  Uint32 CharacterSetList;
  Uint32 MaximumCharacterSetList;
  dstring VolumeSetIdentifier[128];
  struct charspec DescriptorCharacterSet;
  struct charspec ExplanatoryCharacterSet;
  struct extent_ad VolumeAbstract;   
  struct extent_ad VolumeCopyrightNotice;
  struct EntityID ApplicationIdentifier;
  struct timestamp RecordingDateandTime;
  struct EntityID ImplementationIdentifier;
  byte ImplementationUse[64];
  Uint32 PredecessorVolumeDescriptorSequenceLocation;
  Uint16 Flags;
  byte Reserved[22];
};

struct UnallocatedSpaceDesc {
  /* ECMA 167 3/10.8 */
  struct tag DescriptorTag;
  Uint32 VolumeDescriptorSequenceNumber;
  Uint32 NumberofAllocationDescriptors;
  extent_ad AllocationDescriptors[];
};


struct LogicalVolumeIntegrityDesc {
  /* ECMA 167 3/10.10 */
  struct tag DescriptorTag;
  timestamp RecordingDateAndTime;
  Uint32 IntegrityType;
  struct extent_ad NextIntegrityExtent;
  byte LogicalVolumeContentsUse[32];
  Uint32 NumberOfPartitions;
  Uint32 LengthOfImplementationUse; /* = L_IU */
  Uint32 FreeSpaceTable[];
  Uint32 SizeTable[];
  byte ImplementationUse[];
};

////////////////////////////////////////////////////////////////////////
//		VRS - Volume Recognition Sequence
////////////////////////////////////////////////////////////////////////

/**
 * A volume recognition sequence shall consist of a consecutively recorded sequence of
 * one or more Volume Structure Descriptors
 *
 */

#define VRS_OFFSET 16*SECTOR_SIZE

struct VolumeStructureDescriptor
{
  Uint8 type;
  byte identifier[5]; // NSR02 || NSR03
  Uint8 version;
  byte data[2041];
};

typedef VolumeStructureDescriptor BeginningExtendedAreaDescriptor;
/* type == 0
 * identifier == "BEA01"
 * version == 1
 * data #00
 */

typedef VolumeStructureDescriptor TerminatingExtendedAreaDescriptor;
/* type == 0
 * identifier == "TEA01"
 * version == 1
 * data #00
 */


////////////////////////////////////////////////////////////////////////
//		AVDP - Anchor Volume Descriptor Pointer
////////////////////////////////////////////////////////////////////////

/**
 * Contains start address & size of Volume Descriptor Sequence (VDS) and reserve VDS
 */


#define AVDP_OFFSET 256*SECTOR_SIZE

struct AnchorVolumeDescriptorPointer {
  /* ECMA 167 3/10.2 */
  struct tag DescriptorTag;
  struct extent_ad MainVolumeDescriptorSequenceExtent; // MVDS
  struct extent_ad ReserveVolumeDescriptorSequenceExtent; // RVDS
  byte Reserved[480];
};


////////////////////////////////////////////////////////////////////////
//		VDS - Volume descriptor sequence
////////////////////////////////////////////////////////////////////////

/**
 * The offset is defined by the AVDP
 * Contains many descriptors
 */

/* MVDS */
/* ----------------------------------------------------- */
/* | PVD | IUVD | PD | LVD | USD | TD | ...   | LVID | | */
/* ----------------------------------------------------- */

// MVDS_Location through MVDS_Location + (MVDS_Length - 1) / SectorSize ?

//
// PD  --> TAG IDENTIFIER : 5
// Normally you have 1 PD, but you can have 2 PD (ex: read-only and overwritable)
//
struct PartitionDescriptor {
  /* ECMA 167 3/10.5 */
  struct tag   DescriptorTag;
  Uint32 VolumeDescriptorSequenceNumber;
  Uint16 PartitionFlags;
  Uint16 PartitionNumber;
  struct EntityID PartitionContents;
  byte PartitionContentsUse[128];
  Uint32 AccessType;
  Uint32 PartitionStartingLocation; // START
  Uint32 PartitionLength; // SIZE
  struct EntityID ImplementationIdentifier;
  byte ImplementationUse[128];
  byte Reserved[156];
};

//
// LVD --> TAG IDENTIFIER : 6
//

/**
 * LVD records the logical volume name, and the start address and length of FSD
 * And FSD records the root directory of current mounted disc.
 * There is only one FSD inside a partition.
 * FSD contains the start address and length of root directory FE
 * FE is consider as the entry of the file or file folder.
 * When you get the start address of FSD, it is the offset relative to the start position of Partition.
 */

struct LogicalVolumeDescriptor {
  /* ECMA 167 3/10.6 */
  struct tag DescriptorTag;
  Uint32 VolumeDescriptorSequenceNumber;
  struct charspec DescriptorCharacterSet;
  dstring LogicalVolumeIdentifier[128];
  Uint32 LogicalBlockSize;
  struct EntityID DomainIdentifier;
  byte LogicalVolumeContentsUse[16]; // File Set Descriptor 
  Uint32 MapTableLength;
  Uint32 NumberofPartitionMaps;
  struct EntityID ImplementationIdentifier;
  byte ImplementationUse[128];
  extent_ad IntegritySequenceExtent;
  byte PartitionMaps[];
};

////////////////////////////////////////////////////////////////////////
//		FSD - FILE SET DESCRIPTOR
////////////////////////////////////////////////////////////////////////

struct FileSetDescriptor { /* ECMA 167 4/14.1 */
  struct tag DescriptorTag;
  struct timestamp RecordingDateandTime;
  Uint16 InterchangeLevel;
  Uint16 MaximumInterchangeLevel;
  Uint32 CharacterSetList;
  Uint32 MaximumCharacterSetList;
  Uint32 FileSetNumber;
  Uint32 FileSetDescriptorNumber;
  struct charspec LogicalVolumeIdentifierCharacterSet;
  dstring LogicalVolumeIdentifier[128];
  struct charspec FileSetCharacterSet;
  dstring FileSetIdentifier[32];
  dstring CopyrightFileIdentifier[32];
  dstring AbstractFileIdentifier[32];
  struct long_ad RootDirectoryICB;
  struct EntityID DomainIdentifier;
  struct long_ad NextExtent;
  struct long_ad SystemStreamDirectoryICB;
  byte Reserved[32];
};



#endif
