#include "console.h"
#include "udf.h"
#include "fs.h"

int		main(int argc, char **argv)
{

  FileSystem *fs;

  if (argc != 1)
    fs = new FileSystem(argv[1]);
  else
    fs = new FileSystem();
  

  if (!fs->load())
    {
      std::cerr << "Unable to load file correcly" << std::endl;
      delete fs;
      return EXIT_FAILURE;
    }

  /*  fs->ls();
  fs->cd("VIDEO_TS");
  fs->ls();
  fs->cd();
  fs->ls();*/

  Console console(fs);

  console.run();

  delete fs;

  return EXIT_SUCCESS;
}
