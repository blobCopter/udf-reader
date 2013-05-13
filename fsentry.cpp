#include <iomanip>
#include <sstream>
#include "fsentry.h"


FsEntry::FsEntry(FileSystem *filesystem, long_ad fe_addr, bool is_dir, FsEntry *parent) :
  sub_entries(),
  parent_entry(parent),
  fe_ad(fe_addr),
  fs(filesystem),
  is_valid(false),
  is_initialized(false),
  is_directory(is_dir)
{
  fe_buffer = NULL;
}

FsEntry::~FsEntry()
{
  clearBuffer();
}

bool		FsEntry::loadBuffer()
{
  if (fe_buffer != NULL)
    return true;

  /**
   * LOAD BUFFER FROM DATA STREAM
   */
  Uint64 offset = (fs->getPartitionSectorNumber() + fe_ad.ExtentLocation.logicalBlockNumber)
    * SECTOR_SIZE;
  
  Uint32 length = fe_ad.ExtentLength;

  fe_buffer = new char[length];
  if (!fe_buffer)
    return false;

  if (!fs->getStream().read(offset, length, fe_buffer))
    {
      delete fe_buffer;
      fe_buffer = NULL;
      return false;
    }
  return true;
}

bool		FsEntry::clearBuffer()
{
  if (fe_buffer)
    {
      delete fe_buffer;
      fe_buffer = NULL;
    }
  return true;
}

bool		FsEntry::initialize()
{
  if (is_initialized)
    return true;

  if (!loadBuffer())
    return false;

  /**
   * INITIALIZE 
   */
  memcpy(&descriptor_tag, fe_buffer, sizeof(descriptor_tag));
  if (descriptor_tag.TagIdentifier != FE_TAG_ID)
    {
      std::cerr << "error : Wrong FE tag (expecting " << FE_TAG_ID << ")" << std::endl;
      return false;
    }
  is_valid = true;
  memcpy(&l_ea, fe_buffer + 168, sizeof(l_ea));
  memcpy(&l_ad, fe_buffer + 172, sizeof(l_ad));
  memcpy(&AccessTime, fe_buffer + 72, sizeof(AccessTime));
  memcpy(&ModificationTime, fe_buffer + 84, sizeof(ModificationTime));
  memcpy(&AttributeTime, fe_buffer + 96, sizeof(AttributeTime));
  is_initialized = true;
  return true;
}

bool		FsEntry::populate()
{
  if (!is_directory)
    return false;

  if (!is_initialized)
    if (!initialize())
      return false;

  if (sub_entries.size()) // already populated
    return true;

  short_ad fid_ad;
  memcpy(&fid_ad, fe_buffer + l_ea + 176, sizeof(fid_ad));

  char *fid_buffer = new char[fid_ad.ExtentLength];
  if (!fid_buffer)
    {
      std::cerr << "Memory allocation failed" << std::endl;
      return false;
    }

  tag	fid_tag;
  Uint32 completion = 0;

  if (!fs->getStream().read((fs->getPartitionSectorNumber() + fid_ad.ExtentPosition) * SECTOR_SIZE,
			   fid_ad.ExtentLength, fid_buffer))
    return false;


  while (completion < fid_ad.ExtentLength)
    {


      memcpy(&fid_tag, fid_buffer + completion, sizeof(fid_tag));
      if (fid_tag.TagIdentifier != FID_TAG_ID) {
	std::cerr << "Error : invalid FID tag " << std::endl;
	return false;
      }

      FsEntryPtr *fsp = new FsEntryPtr(fs, fid_buffer + completion,
				       fid_ad.ExtentLength - completion, this);

      if (fsp->isValid())
	{
	  sub_entries.push_back(fsp);
	}
      else
	{
	  delete fsp;
	}
      
      completion += fsp->getTotalLength();      
    }
  
  delete fid_buffer;
  clearBuffer();
  return true;
}

bool		FsEntry::isDirectory() const
{
  return is_directory;
}

void		FsEntry::setDirectory(bool d)
{
  is_directory = d;
}

FsEntry		*FsEntry::getSubEntry(const char *name)
{
  if (!populate())
    return NULL;

  std::list<FsEntryPtr*>::iterator it = sub_entries.begin();

  while (it != sub_entries.end())
    {
      if ((*it)->matchName(name))
	return (*it)->getEntry();
      ++it;
    }
  return NULL;
}

FsEntry		*FsEntry::getParentEntry()
{
  if (!parent_entry)
    return this;
  return parent_entry;
}

void		FsEntry::print()
{
  if (is_directory && parent_entry)
    {
      std::cout << "\t.." << std::setw(40) << "<dir>  ";
      timestamp *ts = parent_entry->getModificationTime();
      std::cout << (int)ts->Year;
      std::cout << "-";
      std::cout << (int)ts->Month;
      std::cout << "-";
      std::cout << (int)ts->Day;
      std::cout << "\t";
      std::cout << (int)ts->Hour;
      std::cout << ":";
      std::cout << (int)ts->Minute;
      std::cout << ":";
      std::cout << (int)ts->Second;
      std::cout << std::endl;
    }
  std::list<FsEntryPtr*>::iterator it = sub_entries.begin();
  while (it != sub_entries.end())
    {
      (*it)->print();
      ++it;
    }
}

bool		FsEntry::writeDataToFile(const char *name, const char *dest_dir)
{
  std::string	dest = dest_dir;

  if (isDirectory())
    return false;

  if (!initialize())
    return false;

  if (dest[dest.size() - 1] != '/')
    dest += '/';
  dest += name;

  LOG("Creating file : " << dest);
  int fd = open(dest.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd < 0)
    {
      perror("open");
      return false;
    }

  loadBuffer();
  
  short_ad file_ad;
  memcpy(&file_ad, fe_buffer + l_ea + 176, sizeof(file_ad));

  Uint32 cp_offset = 0;
  std::cout << "File size : " << file_ad.ExtentLength << std::endl;
  std::cout << "Copying file " << name << "\033[32m 0%\e[0m" << std::flush;
  char *buffer = new char[DEFAULT_COPY_SIZE];
  Uint64 file_position = (fs->getPartitionSectorNumber() +
			  file_ad.ExtentPosition) * SECTOR_SIZE;
  while (cp_offset < file_ad.ExtentLength)
    {
      Uint32 to_copy = DEFAULT_COPY_SIZE;
      if (to_copy > file_ad.ExtentLength - cp_offset)
	to_copy = file_ad.ExtentLength - cp_offset;
      
      if (!fs->getStream().read(file_position + cp_offset, to_copy,
		       buffer))
	{
	  close(fd);
	  return false;
	}

      if (write(fd, buffer, to_copy) < 0)
	{
	  perror("write");
	  close(fd);
	  return false;
	}

      if (cp_offset % 10000 == 0)
	std::cout << "\r" << "Copying file " << name << " \033[35m" <<
	  (int)(((float)cp_offset / (float)file_ad.ExtentLength) * 100)
		  << "%\e[0m" << std::flush; 

      cp_offset += to_copy;
    }

  std::cout << "\rCopying file " << name << "\033[32m 100%\e[0m" << std::endl;

  close(fd);

  return true;
}

std::string	FsEntry::getFileSizeAsString()
{
  if (!initialize())
    return "unknown";

  if (isDirectory())
    return "<dir>";

  if (!loadBuffer())
    return "unknown";
  
  short_ad file_ad;
  memcpy(&file_ad, fe_buffer + l_ea + 176, sizeof(file_ad));
  clearBuffer();
  
  Uint32 size = file_ad.ExtentLength;
  std::string ext = "B";

  if (size > 5000)
    {
      size /= 1024;
      ext = "KB";
    }
  if (size > 5000)
    {
      size /= 1024;
      ext = "MB";
    }

  std::ostringstream sstream;
  sstream << size << ext;
  return sstream.str();
}

void	FsEntry::destroy()
{
  while (sub_entries.size())
    {
      FsEntryPtr *first = sub_entries.front();

      if (first)
	{
	  first->destroy();
	  delete first;
	}
      sub_entries.pop_front();
    }
}
