#ifndef FS_ENTRY_H
#define FS_ENTRY_H

#include <list>
#include "fs.h"
#include "fsentryptr.h"

class FsEntryPtr;
class FileSystem;
class FsEntry
{

 private:

  std::list<FsEntryPtr*> sub_entries;
  FsEntry		*parent_entry;
  long_ad		 fe_ad;
  FileSystem		*fs;

  bool			is_valid;
  bool			is_initialized;
  bool			is_directory;

  tag			descriptor_tag;
  Uint32		l_ea;
  Uint32		l_ad;

  timestamp		AccessTime;
  timestamp		ModificationTime;
  timestamp		AttributeTime;

  // DATA ON DISK
  char			*fe_buffer;


  bool			loadBuffer();

 public :

  FsEntry(FileSystem *filesystem, long_ad fe_addr, bool is_dir, FsEntry *parent); 
  ~FsEntry();

  bool			isDirectory() const;
  bool			initialize();
  bool			clearBuffer();
  bool			populate();
  void			setDirectory(bool d);

  FsEntry		*getSubEntry(const char *name);
  FsEntry		*getParentEntry();
  timestamp		*getModificationTime() { return &ModificationTime; }
  void			print();
  void			destroy();


  bool			writeDataToFile(const char *name, const char *dest_dir);
  std::string		getFileSizeAsString();
};



#endif
