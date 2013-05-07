#ifndef DATA_STREAM_H_
#define DATA_STREAM_H_

#include "my.h"

#define DEFAULT_DEVICE "/dev/dvd"

class DataStream
{
 private:

  int	fd; // file descriptor
  bool	is_open;
  const char *device;

 public:

  DataStream();
  DataStream(const char * device);
  ~DataStream();

  bool	isOpen() const;
  bool	read(int seek, unsigned int len, void *data);
  void	close();
};

#endif // DATA_STREAM
