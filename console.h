#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>
#include "fs.h"

class FileSystem;
class Console
{
private:

FileSystem *fs;

void	displayPrompt();

 public:

Console(FileSystem *fileSystem);

void		run();
static void	split(std::vector<std::string> &tokens, const std::string &text, char sep);

};


#endif 
