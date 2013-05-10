#ifndef MY_H
#define MY_H

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <iostream>

#include "udf_types.h"

#if defined(_DEBUG) || defined(DEBUG)
# define LOG(msg) std::cout << msg << std::endl;
#else
# define LOG(msg) ;
#endif

# define raise(msg) std::cerr << msg << ": " <<  __FILE__ << ": " <<  __LINE__ << std::endl

typedef unsigned short unicode_t;

int CompressUnicode(
		    int numberOfChars,
		    /* (Input) number of unicode characters.
		     */
		    int compID,
		    /* (Input) compression ID to be used.
		     */
		    unicode_t *unicode,
		    /* (Input) unicode characters to compress. */
		    byte *UDFCompressed);

#endif
