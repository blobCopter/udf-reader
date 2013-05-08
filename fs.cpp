#include "fs.h"

////////////////////////////////////////////////////////////////////////
//		CONSTRUCTION
////////////////////////////////////////////////////////////////////////

FileSystem::FileSystem() : stream(), is_loaded(false)
{
  vds_length = 0;
  vds_sector = 0;
  root_file_entry = NULL;
}

FileSystem::FileSystem(const char *device) : stream(device), is_loaded(false)
{
  vds_length = 0;
  vds_sector = 0;
  root_file_entry = NULL;
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
  while (!is_valid_format && (sector <= AVDP_SECTOR - 3))
    {
      if (stream.read(OFFSET(sector), sizeof(vrs), &vrs) == false)
	{
	  break;
	}
      
      if (VRS_IS_VALID_SEQUENCE(vrs))
	is_valid_format = true;
      
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
      LOG("MVDS LENGTH/LOCATION " << vds_length << "/" << vds_sector);
      return true;
    }

  LOG("MVDP is null, checking RVDP instead");
  if (AVDP_GET_RVDS_LENGTH(avdp) != 0 && AVDP_GET_RVDS_SECTOR(avdp) != 0)
    {
      vds_length = AVDP_GET_RVDS_LENGTH(avdp);
      vds_sector = AVDP_GET_RVDS_SECTOR(avdp);
      LOG("RVDS LENGTH/LOCATION " << vds_length << "/" << vds_sector);
      return true;
    }

  LOG("VDS LENGTH/LOCATION " << vds_length << "/" << vds_sector);

  std::cerr << "AVDP corrupted, unable to find VDS location" << std::endl;

  return false;
}

////////////////////////////////////////////////////////////////////////
//		STEP 3 - VDS
////////////////////////////////////////////////////////////////////////

bool	FileSystem::loadVds()
{
  int sector = vds_sector;
  int end_sector = sector + (vds_length - 1) / SECTOR_SIZE;
  
  tag tmp_tag;
  bool pd_found = false;
  bool lvd_found = false;

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
	}
      //else
      //  LOG(tmp_tag.TagIdentifier);
      ++sector;
    }
  
  if (!lvd_found)
    std::cerr << "error : LVD was not found." << std::endl;
  if (!pd_found)
    std::cerr << "error : PD was not found." << std::endl;

  if (!lvd_found || !pd_found)
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////
//		STEP 4 - FILE STEP DESCRIPTOR
////////////////////////////////////////////////////////////////////////

bool	FileSystem::loadFSD()
{
  int partition_sector = pd.PartitionStartingLocation;
  long_ad fsd_ad;

  LOG("=== Reading File Set Descriptor ===");
  LOG("Logical Block size : " << lvd.LogicalBlockSize);
  LOG("Partition starting location "  << partition_sector);  

  ////// FILE SET DESCRIPTOR ////////


  //process fsd_offset and fsd_length
  memcpy(&fsd_ad, lvd.LogicalVolumeContentsUse, sizeof(fsd_ad));
  //int fsd_length = fsd_ad.ExtentLength;
  Uint64 fsd_offset = OFFSET(partition_sector) + (fsd_ad.ExtentLocation.logicalBlockNumber)
    * lvd.LogicalBlockSize;
  
  if (!stream.read(fsd_offset, sizeof(fsd), &fsd))
    return false;

  LOG("FSB BLOCK NUMBER : " << fsd_ad.ExtentLocation.logicalBlockNumber);
  LOG("Checking FSD TAG : " << FSD_CHECK_TAG(fsd));
  if (!FSD_CHECK_TAG(fsd))
    {
      std::cerr << "error : Wrong FST tag (expecting " << FSD_TAG_ID << ")" << std::endl;
      return false;
    }

  LOG(fsd.FileSetIdentifier);
  LOG((int)fsd.RecordingDateandTime.Month);


  //////// ROOT FILE ENTRY //////////


  LOG("Locating ROOT file entry : at " << fsd.RootDirectoryICB.ExtentLocation.logicalBlockNumber);

  int root_offset = (partition_sector +
		     fsd.RootDirectoryICB.ExtentLocation.logicalBlockNumber) * lvd.LogicalBlockSize;
  int root_length = fsd.RootDirectoryICB.ExtentLength;

  LOG("ROOT FILE ENTRY LEN : " << root_length);
  LOG("ROOT FILE ENTRY SECTOR : " << root_offset / SECTOR_SIZE);

  // malloc is probably a bad idea
  root_file_entry = static_cast<FileEntry*>(malloc(root_length));

  if (!stream.read(root_offset, root_length, root_file_entry))
    return false;

  LOG("<=== ROOT FILE ENTRY DUMP ===> ");
  LOG("icbtag.FileType " << (int)root_file_entry->ICBTag.FileType);
  LOG("UID 0x" << std::hex << root_file_entry->Uid);
  LOG("GID 0x" << std::hex << root_file_entry->Gid);
  LOG("PERMISSIONS 0x" << std::hex << root_file_entry->Permissions);
  LOG("ACCESS TIME (MONTH) " << (int)root_file_entry->AccessTime.Month);
  LOG("<============================> ");
  LOG("Checking FileEntry tag : " << FE_CHECK_TAG(*root_file_entry));
  if (!FE_CHECK_TAG(*root_file_entry))
    {
      std::cerr << "error : Wrong FE tag (expecting " << FE_TAG_ID << ")" << std::endl;
      return false;
    }

  LOG(root_file_entry->LengthofAllocationDescriptors);

  ////////////// ROOT FID /////////////
  /*
  LOG("==================================");

   FileIdentifierDescriptor fidtest;
   if (!stream.read(root_offset + 176 + FE_L_EA(*root_file_entry),
		    sizeof(fidtest), &fidtest))
     return false;


  LOG("LENGTH : " << root_length << " " << 176 + FE_L_EA(*root_file_entry));

  //  FileIdentifierDescriptor *fid2 = mall;
  LOG("Checking FID tag : " << FID_CHECK_TAG(fidtest));
  if (!FID_CHECK_TAG(fidtest))
    {
      std::cerr << "error : Wrong FID tag (expecting " << FID_TAG_ID << ")" << std::endl;
      return false;
    }
  else
    {
      LOG("GOOD FID");
      LOG((char*)(&fidtest ));
      }*/
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

  if (!loadFSD())
    return false;
  
  is_loaded = true;
  return is_loaded;
}
