#include <iomanip>
#include "fs.h"

////////////////////////////////////////////////////////////////////////
//		CONSTRUCTION
////////////////////////////////////////////////////////////////////////

FileSystem::FileSystem() : stream(), is_loaded(false)
{
  pvd_found = false;
  vds_length = 0;
  vds_sector = 0;
  root_file_entry = NULL;
  memset(udf_version, 0, 5);
}

FileSystem::FileSystem(const char *device) : stream(device), is_loaded(false)
{
  pvd_found = false;
  vds_length = 0;
  vds_sector = 0;
  root_file_entry = NULL;
  memset(udf_version, 0, 5);
}

FileSystem::~FileSystem()
{
  if (volumeName)
    delete volumeName;
  if (root_file_entry)
    {
      root_file_entry->destroy();
      delete root_file_entry;
    }
}


////////////////////////////////////////////////////////////////////////
//		STEP1 - VOLUME DESCRIPTOR SEQUENCE
////////////////////////////////////////////////////////////////////////

// private
bool	FileSystem::checkVolumeRecognitionSequence()
{
  int sector = VRS_SECTOR;
  bool is_valid_format = false;

  LOG("=== Checking VRS ==");
  while (!is_valid_format && (sector <= VRS_SECTOR + 16))
    {
      if (stream.read(OFFSET(sector), sizeof(vrs), &vrs) == false)
	{
	  return false;
	}

      if (VRS_IS_VALID_SEQUENCE(vrs))
	{
	  if (!strcmp((char*)vrs.vsd.identifier, "NSR02"))
	    memcpy(udf_version, "1.02", 4);
	  else
	    memcpy(udf_version, "2.01", 4);
	  is_valid_format = true;
	}
      sector++;
    }

  if (!is_valid_format)
    {
      std::cerr << "Recognition sequence did not match. Wrong format" << std::endl;
      return false;
    }
  LOG("Success");
  return true;
}

////////////////////////////////////////////////////////////////////////
//		STEP 2 - READING THE AVPD
////////////////////////////////////////////////////////////////////////

// private
bool	FileSystem::loadAvdp()
{
  if (!stream.read(AVDP_OFFSET, sizeof(avdp), &avdp))
    return false;

  LOG("=== Reading AVPD for VDS location ===");
  if (AVDP_GET_MVDS_LENGTH(avdp) != 0 && AVDP_GET_MVDS_SECTOR(avdp) != 0)
    {
      vds_length = AVDP_GET_MVDS_LENGTH(avdp);
      vds_sector = AVDP_GET_MVDS_SECTOR(avdp);
      return true;
    }

  LOG("MVDP is null, checking RVDP instead");
  if (AVDP_GET_RVDS_LENGTH(avdp) != 0 && AVDP_GET_RVDS_SECTOR(avdp) != 0)
    {
      vds_length = AVDP_GET_RVDS_LENGTH(avdp);
      vds_sector = AVDP_GET_RVDS_SECTOR(avdp);
      return true;
    }


  std::cerr << "AVDP corrupted, unable to find VDS location" << std::endl;

  return false;
}

////////////////////////////////////////////////////////////////////////
//		STEP 3 - VDS
////////////////////////////////////////////////////////////////////////

// private
bool	FileSystem::loadVds()
{
  int sector = vds_sector;
  int end_sector = sector + (vds_length - 1) / SECTOR_SIZE;
  
  tag tmp_tag;
  bool pd_found = false;
  bool lvd_found = false;
  // pvd is not mandatory

  LOG("=== Reading Volume Descriptor Sequence ===");
  while (sector != end_sector)
    {
      if (!stream.read(OFFSET(sector), sizeof(tmp_tag), &tmp_tag))
	{
	  return false;
	}
      
      if (tmp_tag.TagIdentifier == VDS_PD_TAG_IDENTIFIER)
	{
	  LOG("PD FOUND");
	  pd_found = true;
	  if (!stream.read(OFFSET(sector), sizeof(pd), &pd))
	    return false;
	}
      else if (tmp_tag.TagIdentifier == VDS_LVD_TAG_IDENTIFIER)
	{
	  LOG("LVD FOUND");
	  lvd_found = true;
	  if (!stream.read(OFFSET(sector), sizeof(lvd), &lvd))
	    return false;

	  LOG("Loading LVID");
	  if (!stream.read(lvd.IntegritySequenceExtent.location * SECTOR_SIZE,
			   SECTOR_SIZE,
			   lvid_buffer))
	    return false;
	}
      else if (tmp_tag.TagIdentifier == VDS_PVD_TAG_IDENTIFIER)
	{
	  char	buffer[SECTOR_SIZE];

	  LOG("PVD FOUND");
	  if (!stream.read(OFFSET(sector), SECTOR_SIZE, buffer))
	    return false;
	  pvd_found = true;
	  setVolumeName(buffer + 24, 32);
	  memcpy(&recordingTime, buffer + 376, sizeof(recordingTime));
	}
      ++sector;
    }
  
  if (!lvd_found)
    std::cerr << "error : LVD was not found." << std::endl;
  if (!pd_found)
    std::cerr << "error : PD was not found." << std::endl;

  if (!lvd_found || !pd_found)
    return false;

  partition_sector = pd.PartitionStartingLocation;

  return true;
}

////////////////////////////////////////////////////////////////////////
//		STEP 4 - ROOT DIRECTORY
////////////////////////////////////////////////////////////////////////

// @TODO FREE STUFF

bool	FileSystem::loadRootDirectory()
{

  LOG("=== Reading File Set Descriptor ===");  
  LOG("Partition starting location "  << partition_sector);  

  char *fsd_buffer = new char[SECTOR_SIZE];
  long_ad root_dir_ad;
  tag fsd_tag;

  memcpy(&fsd_ad, lvd.LogicalVolumeContentsUse, sizeof(fsd_ad)); // address
  Uint64 fsd_offset = OFFSET(partition_sector) +
    (fsd_ad.ExtentLocation.logicalBlockNumber) * lvd.LogicalBlockSize;
  
  if (!stream.read(fsd_offset, SECTOR_SIZE, fsd_buffer))
    return false;

  memcpy(&fsd_tag, fsd_buffer, sizeof(fsd_tag));
  if (fsd_tag.TagIdentifier != FSD_TAG_ID)
    {
      std::cerr << "error : Wrong FSD tag (expecting " << FSD_TAG_ID << ")" << std::endl;
      return false;
    }

  memcpy(&root_dir_ad, fsd_buffer + 400, sizeof(root_dir_ad)); // Get root FE address

  delete fsd_buffer;

  root_file_entry = new FsEntry(this, root_dir_ad, true, NULL);
  root_file_entry->initialize();
  root_file_entry->populate();

  current_entry = root_file_entry;

  return true;
}


////////////////////////////////////////////////////////////////////////
//		PUBLIC API
////////////////////////////////////////////////////////////////////////

bool	FileSystem::load()
{
  if (is_loaded)
    return false;

  if (!checkVolumeRecognitionSequence())
    return false;

  if (!loadAvdp())
    return false;

  if (!loadVds())
    return false;

  if (!loadRootDirectory())
    return false;

  current_path = "\033[32mROOT\e[0m:/";
  
  is_loaded = true;
  return is_loaded;
}

void	FileSystem::ls()
{
  if (current_entry)
    {
      current_entry->populate();
      current_entry->print();
    }
}

void	FileSystem::cd(const char *name)
{
  if (name[0] == '/' && name[1] == 0)
    return cd();

  if (!strcmp(name, ".."))
    {
      int slash_count = 0;
      int i = current_path.size() - 1;
      while (i >= 0)
	{
	  if (current_path[i] == '/')
	    slash_count++;
	  if (slash_count == 2)
	    break;
	  --i;
	}

      current_path = current_path.substr(0, i);

      current_entry = current_entry->getParentEntry();
      return;
    }
  
  FsEntry *tmp = current_entry->getSubEntry(name);
  if (tmp)
    {
      if (tmp->isDirectory())
	{
	  current_entry = tmp;
	  current_path += name;
	  current_path += "/";
	}
      else
	std::cout << name << " is not a directory" << std::endl;
    }
  else
    std::cout << name << ": no such directory" << std::endl;
}

void	FileSystem::cd()
{
  current_path = "\033[32mROOT\e[0m:/";
  current_entry = root_file_entry;
}

std::string	&FileSystem::getCurrentPath()
{
  return current_path;
}

FsEntry		*FileSystem::getEntryFromPath(const char *src, std::string &file_name_out)
{
  std::string	path = src;
  std::vector<std::string>	tokens;
  
  std::cout << path << std::endl;
  Console::split(tokens, path, '/');

  FsEntry *entry;
  if (src[0] == '/')
    {
      entry = root_file_entry;
      file_name_out = "ROOT";
    }
  else
    entry = current_entry;
    

  while (tokens.size())
    {
      if (tokens[0] != ".")
	{
	  if (tokens[0] == "..")
	    {
	      if (entry->getParentEntry())
		entry = entry->getParentEntry();
	      file_name_out = "ROOT";
	    }
	  else
	    {
	      entry = entry->getSubEntry(tokens[0].c_str());
	      if (entry == NULL)
		return NULL;
	      file_name_out = tokens[0];
	    }
	}
      tokens.erase(tokens.begin());
    }
  return entry;
}

void		FileSystem::cp(const char *src, const char *dest)
{
  std::string	name;
  FsEntry	*e = getEntryFromPath(src, name);

  if (!e)
    {
      std::cerr << name << ": no such file" << std::endl;
      return;
    }
  if (e->isDirectory())
    {
      std::cerr << "Cannot copy a directory (@TODO)" << std::endl;
      return;
    }
  if (!e->writeDataToFile(name.c_str(), dest))
    {
      std::cerr << "Unable to copy file :(" << std::endl;
    }
}

void		FileSystem::fdisk()
{
  if (volumeName)
    std::cout << volumeName << std::endl;
  if (pvd_found)
    {
      std::cout << "Record Time:\t\t" <<
	(int)recordingTime.Year << '-' <<
	(int)recordingTime.Month << '-' <<
	(int)recordingTime.Day << "  " <<
	(int)recordingTime.Hour << ':' <<
	(int)recordingTime.Minute << ':' <<
	(int)recordingTime.Second << std::endl;
    }
  tag lvidtag;
  memcpy(&lvidtag, lvid_buffer, sizeof(lvidtag));

  if (lvidtag.TagIdentifier == 9)
    {
      float	total_size = 0;
      Uint32	*sizeArray = (Uint32*)(lvid_buffer + 80);
      Uint32	partition_number;

      memcpy(&partition_number, lvid_buffer + 72, sizeof(partition_number));
      std::cout << "Number of partitions:\t" << partition_number << std::endl;
      // FREE SPACE
      for (unsigned int i = 0; i < partition_number; i++)
	total_size += sizeArray[i] * SECTOR_SIZE;
      total_size = ((total_size / 1024) / 1024) /1024;
      std::cout << "Disk Free Size:\t\t" << std::setprecision(2) << total_size << "GB" << std::endl;
      // TOTAL SPACE
      total_size = 0;
      sizeArray = (Uint32*)(lvid_buffer + 4 * partition_number + 80);
      for (unsigned int i = 0; i < partition_number; i++)
	total_size += sizeArray[i] * SECTOR_SIZE;
      total_size = ((total_size / 1024) / 1024 ) / 1024;
      std::cout << "Disk Size:\t\t" << std::setprecision(2) << total_size << "GB" << std::endl;
    }
  std::cout << "Udf version:\t\t" << udf_version << std::endl;
}

void		FileSystem::setVolumeName(const char *name, Uint32 len)
{
  // bloody hack again >,<'
  int charcount = 0;

  for (unsigned int i = 0; i < len; i++)
    if (name[i] >= 32 && name[i] <= 126)
	charcount++;
  
  if (volumeName)
    delete volumeName;
  volumeName = new char[charcount + 1];
  
  int j = 0;
  for (unsigned int i = 0; i < len; i++)
    if (name[i] >= 32 && name[i] <= 126)
      volumeName[j++] = name[i];
  volumeName[charcount] = '\0';
}
