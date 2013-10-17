//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Implementations for missing libc functions
//
//=============================================================================

#include <string.h>

// Only a dummy. It is used in a function of alfont, but never called in AGS.
size_t malloc_usable_size(const void* allocation)
{
  return 0;
}


#if defined (LINUX_VERSION) || defined (ANDROID_VERSION) || defined (IOS_VERSION) || defined (MAC_VERSION)

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <ctype.h>

size_t mbstowcs(wchar_t *wcstr, const char *mbstr, size_t max)
{
  int count = 0;
  
  while ((count < max) && (*mbstr != 0))
  {
    *wcstr++ = *mbstr++;
    count++;
  }
  return count;

}

size_t wcstombs(char* mbstr, const wchar_t *wcstr, size_t max)
{
  int count = 0;

  while ((count < max) && (*wcstr != 0))
  {
    *mbstr++ = *wcstr++;
    count++;
  }
  return count;
}

int getw(FILE *fp)
{
  int result;
  
  if (fread((char*)&result, 4, 1, fp) != 1)
    return EOF;
  
  return result;
}

char* strupr(char* s)
{
  char *original = s;
  
  while (*s)
  {
    *s = toupper(*s);
    s++;
  }

  return original;
}

char* strlwr(char* s)
{
  char *original = s;
  
  while (*s)
  {
    *s = tolower(*s);
    s++;
  }
  
  return original;
}

#endif // defined (LINUX_VERSION) || defined (ANDROID_VERSION) || defined (IOS_VERSION) || defined (MAC_VERSION)
