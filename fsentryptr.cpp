#include <iomanip>
#include "my.h"
#include "fsentryptr.h"


FsEntryPtr::FsEntryPtr(FileSystem *fs, char *buffer, Uint32 len, FsEntry *parent)
{
  byte fileCharacteristics;
  Uint16 L_IU;
  Uint8 L_FI;
  tag descriptor_tag;
  

  is_hidden = false;

  memcpy(&descriptor_tag, buffer, sizeof(tag));
  if (descriptor_tag.TagIdentifier != FID_TAG_ID)
    {
      std::cerr << "Error : wrong FID tag (expected 257)" << std::endl;
    }

  (void) len;
  memcpy(&fileCharacteristics, buffer + 18, sizeof(fileCharacteristics));
  memcpy(&L_IU, buffer + 36, sizeof(L_IU));
  memcpy(&L_FI, buffer + 19, sizeof(L_FI));

  Uint32 padding = 4 * (Uint32)((L_FI + L_IU + 38 + 3)/4) - (L_FI + L_IU + 38);
  total_size = 38 + L_FI + L_IU + padding;
  
  identifier_length = L_FI;
  if (identifier_length == 0)
    {
      identifier = NULL;
    }
  else
    {
      //
      // bloody hack ! Gotta fix this
      //
      char *tmp = new char[identifier_length];
      memcpy(tmp, buffer + 38 + L_IU, identifier_length);

      int len = 0;
      for (int i = 0; i < identifier_length; i++)
	if (tmp[i] >= 32 && tmp[i] <= 126)
	  len++;

      identifier = new char[len + 1];
      if (identifier == NULL)
	{
	  std::cerr << "Memory allocation failed" << std::endl;
	  return;
	}
      int j = 0;
      for (int i = 0; i < identifier_length; i++)
	if (tmp[i] >= 32 && tmp[i] <= 126)
	  identifier[j++] = tmp[i];
      identifier_length = len;
      identifier[len] = '\0';
      delete tmp;
      //CompressUnicode(identifier_length / 2 + 1, 16, (unicode_t*)tmp, (byte*)identifier);
    }

  long_ad fe_ad;
  memcpy(&fe_ad, buffer + 20, sizeof(fe_ad));

  // Under UNIX and OS/400 these bits shall be processed the same as
  //   specified in 3.3.1.1.1, except for hidden files which will be processed as
  // normal non-hidden files
  //is_hidden = !((fileCharacteristics << 7) >> 7);
  is_hidden = false;

  fileCharacteristics >>= 1;
  fileCharacteristics <<= 7;
  fileCharacteristics >>= 7;
  if (fileCharacteristics)
    is_directory = true;
  else
    is_directory = false;

  entry = new FsEntry(fs, fe_ad, is_directory, parent);
}


FsEntryPtr::~FsEntryPtr()
{
  if (identifier)
    delete identifier;
}

void	FsEntryPtr::destroy()
{
  if (entry)
    {
      entry->destroy();
      delete entry;
      entry = NULL;
    }
}

bool		FsEntryPtr::isValid()
{
  return !(identifier_length == 0 || identifier == NULL);
}

Uint32		FsEntryPtr::getTotalLength() const
{
  return total_size;
}

FsEntry		*FsEntryPtr::getEntry()
{
  if (!entry)
    return NULL;
  entry->initialize();
  return entry;
}

bool		FsEntryPtr::matchName(const char *name)
{
  if (!identifier)
    return false;
  return !strcmp(identifier, name);
}

void		FsEntryPtr::print()
{
  if (!entry)
    return;

  if (is_hidden)
    std::cout << "<hide>";
  std::cout << '\t';

  std::cout << identifier;
  if (identifier_length < 40)
    std::cout << std::left << std::setw(40 - identifier_length);

  if (is_directory)
    std::cout << std::right << "<dir>";
  else
    {
      std::string tmp = entry->getFileSizeAsString();
      std::cout << std::right << tmp;//<< std::left << std::setw(15 - tmp.size());
    }

  entry->initialize();
  timestamp *ModificationTime = entry->getModificationTime();
  
  std::cout << "  ";
  std::cout << (int)ModificationTime->Year;
  std::cout << "-";
  std::cout << (int)ModificationTime->Month;
  std::cout << "-";
  std::cout << (int)ModificationTime->Day;

  std::cout << "\t";
  
  std::cout << (int)ModificationTime->Hour;
  std::cout << ":";
  std::cout << (int)ModificationTime->Minute;
  std::cout << ":";
  std::cout << (int)ModificationTime->Second;

  std::cout << std::endl;
}
