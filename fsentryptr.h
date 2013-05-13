#ifndef FS_ENTRY_PTR_H
#define FS_ENTRY_PTR_H

#include "fs.h"
#include "fsentry.h"

class FsEntry;
class FileSystem;
class FsEntryPtr
{
 private:
  char		*identifier;
  int		identifier_length;

  FsEntry	*entry;
  bool		is_directory;
  Uint32	total_size;

  bool		is_hidden;


 public :

  FsEntryPtr(FileSystem *fs, char *buffer, Uint32 len, FsEntry *parent);
  ~FsEntryPtr();

  Uint32	getTotalLength() const;
  bool		isValid();
  FsEntry	*getEntry();
  bool		matchName(const char *name);

  void		print();
  void		destroy();

};

#endif
