#ifndef UDF_TYPES_H
#define UDF_TYPES_H

#include <stdint.h>

typedef uint8_t byte;
typedef uint8_t dstring;
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef int16_t Int16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;

struct charspec {
  /* ECMA 167 1/7.2.1 */
  Uint8 CharacterSetType;
  byte CharacterSetInfo[63];
};

struct timestamp {
  /* ECMA 167 1/7.3 */
  Uint16 TypeAndTimezone;
  Int16 Year;
  Uint8 Month;
  Uint8 Day;
  Uint8 Hour;
  Uint8 Minute;
  Uint8 Second;
  Uint8 Centiseconds;
  Uint8 HundredsofMicroseconds;
  Uint8 Microseconds;
};

struct EntityID {
    /* ECMA 167 1/7.4 */
  Uint8 Flags;
  char  Identifier[23];
  char  IdentifierSuffix[8];
};

struct extent_ad
{
  Uint32 length;
  Uint32 location;
};

struct Lb_addr {

  Uint32 logicalBlockNumber; // logical block size in LogicalVolumeDescriptor
  Uint16 partitionReferenceNumber;

};

struct short_ad {
  Uint32 ExtentLength;
  Uint32 ExtentPosition;
};

// ALLOCATION DESCRIPTOR
struct long_ad {
  /* ECMA 167 4/14.14.2 */
  Uint32  ExtentLength;
  Lb_addr ExtentLocation;
  byte ImplementationUse[6];
};

struct tag {
  /* ECMA 167 4/7.2 */
  Uint16 TagIdentifier;
  Uint16 DescriptorVersion;
  Uint8 TagChecksum;
  byte Reserved;
  Uint16 TagSerialNumber;
  Uint16 DescriptorCRC;
  Uint16 DescriptorCRCLength;
  Uint32 TagLocation;
};

struct icbtag { /* ECMA 167 4/14.6 */
Uint32  PriorRecordedNumberofDirectEntries;
Uint16 StrategyType;
byte StrategyParameter[2];
Uint16 MaximumNumberofEntries;
byte Reserved;
Uint8 FileType;
Lb_addr ParentICBLocation;
Uint16 Flags;
};

#endif
