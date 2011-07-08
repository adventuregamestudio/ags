/*
  Copyright (c) 2003, Shawn R. Walker
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  
      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of Shawn R. Walker nor names of contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "allegro.h"
#include "misc.h"

// Filenames are not case-sensitive on the PSP.

#include <string.h>
/* File Name Concatenator basically on Windows / DOS */
char *ci_find_file(char *dir_name, char *file_name)
{
  char  *diamond = NULL;

  if (dir_name == NULL && file_name == NULL)
      return NULL;

  if (dir_name == NULL) {
    diamond = (char *)malloc(strlen(file_name) + 3);
    strcpy(diamond, file_name);
  } else {
    diamond = (char *)malloc(strlen(dir_name) + strlen(file_name) + 2);
    append_filename(diamond, dir_name, file_name, strlen(dir_name) + strlen(file_name) + 2);
  }
  fix_filename_case(diamond);
  fix_filename_slashes(diamond);
  return diamond;
}


/* Case Insensitive fopen */
FILE *ci_fopen(char *file_name, const char *mode)
{
  // Don't pass a NULL pointer to newlib on the PSP.
  if (file_name == NULL)
  {
    return NULL;
  }
  else
  {
    return fopen(file_name, mode);
  }
}

