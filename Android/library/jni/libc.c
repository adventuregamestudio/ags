// Functions missing in the Android Standard C library.

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
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

// Only a dummy. It is used in a function of alfont, but never called in AGS.
size_t malloc_usable_size(void* allocation)
{
  return 0;
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
