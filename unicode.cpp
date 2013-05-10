#include "my.h"

/***********************************************************************
 * DESCRIPTION:
 * Takes a string of unicode wide characters and returns an OSTA CS0
 * compressed unicode string. The unicode MUST be in the byte order of
 * the compiler in order to obtain correct results. Returns an error
 * if the compression ID is invalid.
 *
 * NOTE: This routine assumes the implementation already knows, by
 * the local environment, how many bits are appropriate and
 * therefore does no checking to test if the input characters fit
 * into that number of bits or not.
 *
 * RETURN VALUE
 *
 *
The total number of bytes in the compressed OSTA CS0 string,
*
including the compression ID.
*
A -1 is returned if the compression ID is invalid.
*/
int CompressUnicode(
		    int numberOfChars,
		    /* (Input) number of unicode characters.
		     */
		    int compID,
		    /* (Input) compression ID to be used.
		     */
		    unicode_t *unicode,
		    /* (Input) unicode characters to compress. */
		    byte *UDFCompressed) /* (Output) compressed string, as bytes.
					  */
{
  int byteIndex, unicodeIndex;
  if (compID != 8 && compID != 16)
    {
      byteIndex = -1; /* Unsupported compression ID ! */
    }
  else
    {
      /* Place compression code in first byte. */
      //UDFCompressed[0] = compID;
      byteIndex = 0;
      unicodeIndex = 0;
      while (unicodeIndex < numberOfChars)
	{
	  if (compID == 16)
	    {
	      /* First, place the high bits of the char
	       * into the byte stream.
	       */
	      UDFCompressed[byteIndex++] =
		(unicode[unicodeIndex] & 0xFF00) >> 8;
	    }
	  /*Then place the low bits into the stream. */
	  UDFCompressed[byteIndex++] = unicode[unicodeIndex] & 0x00FF;
	  unicodeIndex++;
	}
    }
  return(byteIndex);
}
