#include "udf.h"
#include "fs.h"

int		main(int argc, char **argv)
{
  (void) argc;
  (void) argv;


  FileSystem fs;

  fs.load();

  return EXIT_SUCCESS;
}
