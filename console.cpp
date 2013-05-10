#include <string>
#include <iostream>
#include "console.h"

Console::Console(FileSystem *fileSystem) : fs(fileSystem)
{
  
}

void	Console::displayPrompt()
{
  if (fs)
    {
      std::cout << fs->getCurrentPath();
    }
  std::cout << '>';
}

void	Console::split(std::vector<std::string> &tokens, const std::string &text, char sep)
{
  int start = 0, end = 0;
  while ((end = text.find(sep, start)) != (int)std::string::npos) {
    std::string tmp = text.substr(start, end - start);
    if (tmp.size())
      {
	tokens.push_back(tmp);
      }
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
}

void	Console::run()
{
  char input[256];

  while (true)
    {
      std::vector<std::string> elems;

      displayPrompt();
      std::cin.getline(input,256);
      std::string s(input);

      split(elems, s, ' ');
      
      if (!elems.size())
	continue;
      
      if (elems[0] == "ls" || elems[0] == "dir")
	fs->ls();
      else if (elems[0] == "cd")
	{
	  if (elems.size() > 1)
	    fs->cd(elems[1].c_str());
	  else
	    fs->cd();
	}
      else if (elems[0] == "exit" || elems[0] == "quit")
	break;
      else if (elems[0] == "fdisk")
	fs->fdisk();
      else if (elems[0] == "cp")
	{
	  if (elems.size() < 3)
	    std::cerr << "Missing arguments. Usage : cp [file] [dest_dir]" << std::endl;
	  else
	    fs->cp(elems[1].c_str(), elems[2].c_str());
	}
    }
}
