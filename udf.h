#ifndef UDF_H
#define UDF_H

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string>
#include <iostream>
#include <string.h>
#include "udf_types.h"

#define DEFAULT_COPY_SIZE 2048
#define SECTOR_SIZE 2048
#define OFFSET(sector) (Uint64)(sector * SECTOR_SIZE)
#define OFFSET_LONG_AD(start_sector, l_ad) \
  (start_sector + l_ad.ExtentLocation.logicalBlockNumber) * SECTOR_SIZE

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

#define VRS_CHECK_BEA(bea) ((strncmp((char*)bea.identifier, "BEA01", 5) == 0) ? true : false)
#define VRS_CHECK_TEA(tea) ((strncmp((char*)tea.identifier, "TEA01", 5) == 0) ? true : false)
#define VRS_CHECK_NSR(vsd) (((strncmp((char*)vsd.identifier, "NSR02", 5) == 0 || strncmp((char*)vsd.identifier, "NSR03", 5) == 0) ? true : false))

#define VRS_IS_VALID_SEQUENCE(vrs) ((VRS_CHECK_BEA(vrs.bea) && VRS_CHECK_TEA(vrs.tea) && VRS_CHECK_NSR(vrs.vsd)) ? true : false)

#define VRS_SECTOR 16
#define VRS_OFFSET VRS_SECTOR * SECTOR_SIZE

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

struct VRS
{
  struct VolumeStructureDescriptor bea;
  struct VolumeStructureDescriptor vsd;
  struct VolumeStructureDescriptor tea;
};


////////////////////////////////////////////////////////////////////////
//		AVDP - Anchor Volume Descriptor Pointer
////////////////////////////////////////////////////////////////////////

/**
 * Contains start address & size of Volume Descriptor Sequence (VDS) and reserve VDS
 */


#define AVDP_SECTOR 256
#define AVDP_OFFSET OFFSET(AVDP_SECTOR)

#define AVDP_GET_MVDS_SECTOR(avdp) avdp.MainVolumeDescriptorSequenceExtent.location
#define AVDP_GET_MVDS_LENGTH(avdp) avdp.MainVolumeDescriptorSequenceExtent.length
#define AVDP_GET_RVDS_SECTOR(avdp) avdp.ReserveVolumeDescriptorSequenceExtent.location
#define AVDP_GET_RVDS_LENGTH(avdp) avdp.ReserveVolumeDescriptorSequenceExtent.length

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

#define VDS_PD_TAG_IDENTIFIER 5
#define VDS_LVD_TAG_IDENTIFIER 6

/* MVDS */
/* ----------------------------------------------------- */
/* | PVD | IUVD | PD | LVD | USD | TD | ...   | LVID | | */
/* ----------------------------------------------------- */

// MVDS_Location through MVDS_Location + (MVDS_Length - 1) / SectorSize ?

//
// PD  --> TAG IDENTIFIER : 5
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


//
// PVD --> TAG IDENTIFIER : 1
//

#define VDS_PVD_TAG_IDENTIFIER 1

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


////////////////////////////////////////////////////////////////////////
//		FSD - FILE SET DESCRIPTOR
////////////////////////////////////////////////////////////////////////

#define FSD_TAG_ID 256
#define FSD_CHECK_TAG(fsd) ((fsd.DescriptorTag.TagIdentifier == FSD_TAG_ID) ? true : false)

#define FE_TAG_ID 261
#define FE_CHECK_TAG(fe)  (((fe).DescriptorTag.TagIdentifier == FE_TAG_ID) ? true : false)

#define FID_TAG_ID 257
#define FID_CHECK_TAG(fid)  (((fid).DescriptorTag.TagIdentifier == FID_TAG_ID) ? true : false)

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


////////////////////////////////////////////////////////////////////////
//		FE and FID
////////////////////////////////////////////////////////////////////////

#define FE_L_EA(fe) (fe).LengthofExtendedAttributes
#define FE_L_AD(fe) (fe).LengthofAllocationDescriptors

struct FileEntry { /* ECMA 167 4/14.9 */
  struct tag		DescriptorTag;
  struct icbtag		ICBTag;
  Uint32		Uid;
  Uint32		Gid;
  Uint32		Permissions;
  Uint16		FileLinkCount;
  Uint8			RecordFormat;
  Uint8			RecordDisplayAttributes;
  Uint32		RecordLength;
  Uint64		InformationLength;
  Uint64		LogicalBlocksRecorded;
  struct timestamp	AccessTime;
  struct timestamp	ModificationTime;
  struct timestamp	AttributeTime;
  Uint32		Checkpoint;
  struct long_ad	ExtendedAttributeICB;
  struct EntityID	ImplementationIdentifier;
  Uint64		UniqueID;
  Uint32		LengthofExtendedAttributes;
  Uint32		LengthofAllocationDescriptors;
  byte			ExtendedAttributes[];
  byte			AllocationDescriptors[];
};

struct FileIdentifierDescriptor {
  /* ECMA 167 4/14.4 */
  struct tag  DescriptorTag;
  Uint16  FileVersionNumber;
  Uint8  FileCharacteristics;
  /** FileCharacteristics
   * bit 1 : 0 -> existence of the file shall be made known to the user
   * bit 2 : 1 -> is a directory
   * bit 3 : ...
   */
  Uint8  LengthofFileIdentifier;
  struct long_ad  ICB;
  Uint16  LengthofImplementationUse; // L_IU (4)
  byte  ImplementationUse[]; //
  char  FileIdentifier[];
  byte Padding[];
};

#endif
