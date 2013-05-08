#include "datastream.h"

DataStream::DataStream()
{
  fd = -1;
  is_open = false;
  device = DEFAULT_DEVICE;
}

DataStream::DataStream(const char * dev)
{
  fd = -1;
  is_open = false;
  device = dev;
}

DataStream::~DataStream()
{
  close();
}

bool	DataStream::isOpen() const
{
  return is_open;
}

void	DataStream::close()
{
  if (is_open)
    {
      is_open = false;
      ::close(fd);
      fd = -1;
    }
}

bool	DataStream::read(Uint64 seek, unsigned int len, void *data_out)
{
  if (device == NULL)
    {
      raise("device is null");
      return false;
    }

  if (data_out == NULL)
    {
      raise("output buffer is null");
      return false;
    }


  /**
   * OPEN
   */
  if (!is_open)
    {
      std::cout << "Opening device " << device << std::endl;
      if ((fd = open(device, O_RDONLY | O_LARGEFILE)) == -1) {
	perror("open");
	return false;
      }
      is_open = true;
    }

  /**
   * SEEK
   */
  if (lseek(fd, seek, SEEK_SET) == -1)
    {
      perror("seek");
      return false;
    }


  /**
   * READ
   */
  int ret = ::read(fd, data_out, len);
  if (ret < 0)
    {
      perror("read");
      return false;
    }
  if (ret < (int)len)
    {
      raise("Not enough data to read");
      return false;
    }

  return true;
}
