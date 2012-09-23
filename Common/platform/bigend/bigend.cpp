/*
 *  bigend.cpp
 *  AGSRunTime
 *
 *  Created by Steve McCrea on 6/16/05.
 *  Part of the Adventure Game Studio source code (c)1999-2005 Chris Jones.
 *
 *  Functions to deal with endianness issues of file access.
 *
 */

#include <allegro.h>

#ifdef ALLEGRO_BIG_ENDIAN

#include <stdio.h>
#include <stdlib.h>

// define this before including bigend.h in this file only
// allows the overridden functions to call into the originals
#define __BIGEND_ORIGINAL_FILE_FUNCTIONS
#include "bigend.h"

//-----------------------------------------------------------------

int __int_swap_endian(int i)
{
  unsigned int u = *((unsigned int *)(&i));
  u = (((u>>24)&0xff)|((u>>8)&0xff00)|((u&0xff00)<<8)|((u&0xff)<<24));
  return *((int *)(&u));
}

//-----------------------------------------------------------------

short __short_swap_endian(short i)
{
  unsigned short u = *((unsigned short *)(&i));
  u = (((u>>8)&0xff)|((u&0xff)<<8));
  return *((short *)(&u));
}

//-----------------------------------------------------------------

int __getw__bigendian(FILE *fp)
{
  int u;
  fread(&u, sizeof(u), 1, fp);
  // convert to big-endian
  u = __int_swap_endian(u);
  return u;
}

//-----------------------------------------------------------------

void __putw__lilendian(int u, FILE *fp)
{
  // convert to little-endian
  u = __int_swap_endian(u);
  putw(u, fp);
}

//-----------------------------------------------------------------

short int __getshort__bigendian(FILE *fp)
{
  short int u;
  fread(&u, sizeof(u), 1, fp);
  u = __short_swap_endian(u);
  
  return u;
}

//-----------------------------------------------------------------

void __putshort__lilendian(short int u, FILE *fp)
{
  u = __short_swap_endian(u);
  fwrite(&u, sizeof(u), 1, fp);
}

//-----------------------------------------------------------------
// override fread for arrays of shorts and ints

size_t __fread__bigendian(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
  size_t numread = fread(ptr, size, nmemb, fp);
  switch (size)
  {
    case 2:
    {
      short int *sptr = (short int *) ptr;
      for (size_t i = 0; i < numread; ++i)
      {
        sptr[i] = __short_swap_endian(sptr[i]);
      }
      break;
    }
    case 4:
    {
      int *iptr = (int *) ptr;
      for (size_t i = 0; i < numread; ++i)
      {
        iptr[i] = __int_swap_endian(iptr[i]);
      }
      break;
    }
    default:
      break;
  }
  
  return numread;
}

//-----------------------------------------------------------------
// override fwrite for arrays of ints and shorts

size_t __fwrite__lilendian(void const *ptr, size_t size, size_t nmemb, FILE *fp)
{
  size_t numwritten;
  switch (size)
  {
    case 2:
    {
      short const *sptr = (short const *) ptr;
      for (size_t i = 0; i < nmemb; ++i)
      {
        __putshort__lilendian(sptr[i], fp);
      }
      numwritten = nmemb;
      break;
    }
    case 4:
    {
      int const *iptr = (int const *) ptr;
      for (size_t i = 0; i < nmemb; ++i)
      {
        __putw__lilendian(iptr[i], fp);
      }
      numwritten = nmemb;
      break;
    }
    default:
    {
      numwritten = fwrite(ptr, size, nmemb, fp);
      break;
    }
  }
  
  return numwritten;
}

//-----------------------------------------------------------------
#undef __BIGEND_ORIGINAL_FILE_FUNCTIONS

#endif // ALLEGRO_BIG_ENDIAN
