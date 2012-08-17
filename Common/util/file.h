
//=============================================================================
//
// Platform-independent FILE interface
//
// TODO: abstract interface wrapper around file handle
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILE_H
#define __AGS_CN_UTIL__FILE_H

#include <stdio.h>
// TODO: other platforms?

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
long int filelength(int fhandle);
#endif

#if defined ALLEGRO_BIG_ENDIAN
#include "platform/bigend/bigend.h"
#else !ALLEGRO_BIG_ENDIAN
// Two functions to match those defined by bigend version by McCrea
short int   getshort(FILE *);
void        putshort(short int, FILE *);
#endif // !ALLEGRO_BIG_ENDIAN

// Get required padding length when reading/writing a structure
// same way as if it was written as an object in whole;
// result could be from 0 to 3 inclusive.
inline size_t get_padding(int previous_data_length)
{
    return 4 - (previous_data_length % 4);
}

#endif // __AGS_CN_UTIL__FILE_H
