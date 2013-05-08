#ifndef MY_H
#define MY_H

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <iostream>

#if defined(_DEBUG) || defined(DEBUG)
# define LOG(msg) std::cout << msg << std::endl;
#else
# define LOG(msg) ;
#endif

# define raise(msg) std::cerr << msg << ": " <<  __FILE__ << ": " <<  __LINE__ << std::endl

#endif
