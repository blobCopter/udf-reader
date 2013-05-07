#ifndef FS_H
#define FS_H

#include "my.h"
#include "udf.h"
#include "datastream.h"

class	FileSystem
{

 private:

  DataStream	stream;
  bool		is_loaded;

  // VRS
  VRS		vrs;
  //AVDP
  AnchorVolumeDescriptorPointer avdp;
  // VDS
  int vds_length;
  int vds_sector;
  PartitionDescriptor pd;
  LogicalVolumeDescriptor lvd;
  // FILE SET DESCRIPTOR
  FileSetDescriptor fsd;
  // ROOT
  FileEntry root_dir;


  bool checkVolumeRecognitionSequence();
  bool loadVds();
  bool loadAvdp();
  bool loadFSD();



 public:

  FileSystem();
  FileSystem(const char *dev);

  bool	load();

  
};


#endif
