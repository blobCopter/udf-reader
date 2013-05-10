#ifndef FS_H
#define FS_H

#include "console.h"
#include "my.h"
#include "udf.h"
#include "datastream.h"
#include "fsentry.h"

class	FsEntry;
class	FileSystem
{
 private:

  DataStream	stream;
  bool		is_loaded;

  // DISK INFO
  char	*volumeName;


  // VRS
  VRS				vrs;
  //AVDP
  AnchorVolumeDescriptorPointer avdp;
  // VDS
  int				vds_length;
  int				vds_sector;
  PartitionDescriptor		pd;
  LogicalVolumeDescriptor	lvd;
  Uint32			partition_sector;
  // FSD
  long_ad			fsd_ad;
  // ROOT
  Uint64			root_offset;
  FsEntry			*root_file_entry;
  FsEntry			*current_entry;
  // ROOT FID
  short_ad			root_fid_ad;

  std::string			current_path;

  bool checkVolumeRecognitionSequence();
  bool loadVds();
  bool loadAvdp();
  bool loadRootDirectory();

  void	setVolumeName(const char *name, Uint32 len);

 public:

  FileSystem();
  FileSystem(const char *dev);

  bool	load();
  void	ls();
  void	cd(const char *name);
  void	cd();
  void	cp(const char *src, const char *dest);
  void	fdisk();

  std::string	&getCurrentPath();
  FsEntry	*getEntryFromPath(const char *src, std::string &file_name_out);

  Uint32 getPartitionSectorNumber() {  return partition_sector; }
  DataStream & getStream() { return stream; }
  
  
};


#endif
