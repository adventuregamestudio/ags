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
#include "util/misc.h"
#include "util/filestream.h"

using AGS::Common::DataStream;

#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
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

#else
/* Case Insensitive File Find */
char *ci_find_file(char *dir_name, char *file_name)
{
  struct stat   statbuf;
  struct dirent *entry = NULL;
  DIR           *rough = NULL;
  DIR           *prevdir = NULL;
  char          *diamond = NULL;
  char          *directory = NULL;
  char          *filename = NULL;

  if (dir_name == NULL && file_name == NULL)
      return NULL;

  if (!(dir_name == NULL)) {
    fix_filename_case(dir_name);
    fix_filename_slashes(dir_name);
  }

  fix_filename_case(file_name);
  fix_filename_slashes(file_name);

  if (dir_name == NULL) {
    char  *match = NULL;
    int   match_len = 0;
    int   dir_len = 0;

    match = get_filename(file_name);
    if (match == NULL)
      return NULL;

    match_len = strlen(match);
    dir_len = (match - file_name);

    if (dir_len == 0) {
      directory = (char *)malloc(2);
      strcpy(directory,".");
    } else {
      directory = (char *)malloc(dir_len + 1);
      strncpy(directory, file_name, dir_len);
      directory[dir_len] = '\0';
    }

    filename = (char *)malloc(match_len + 1);
    strncpy(filename, match, match_len);
    filename[match_len] = '\0';
  } else {
    directory = (char *)malloc(strlen(dir_name) + 1);
    strcpy(directory, dir_name);
    
    filename = (char *)malloc(strlen(file_name) + 1);
    strcpy(filename, file_name);
  }

  if ((prevdir = opendir(".")) == NULL) {
    fprintf(stderr, "ci_find_file: cannot open current working directory\n");
    return NULL;
  }

  if (chdir(directory) == -1) {
    fprintf(stderr, "ci_find_file: cannot change to directory: %s\n", directory);
    return NULL;
  }
  
  if ((rough = opendir(directory)) == NULL) {
    fprintf(stderr, "ci_find_file: cannot open directory: %s\n", directory);
    return NULL;
  }

  while ((entry = readdir(rough)) != NULL) {
    lstat(entry->d_name, &statbuf);
    if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
      if (strcasecmp(filename, entry->d_name) == 0) {
#ifdef _DEBUG
        fprintf(stderr, "ci_find_file: Looked for %s in rough %s, found diamond %s.\n", filename, directory, entry->d_name);
#endif // _DEBUG
        diamond = (char *)malloc(strlen(directory) + strlen(entry->d_name) + 2);
        append_filename(diamond, directory, entry->d_name, strlen(directory) + strlen(entry->d_name) + 2);
        break;
      }
    }
  }
  closedir(rough);

  fchdir(dirfd(prevdir));
  closedir(prevdir);

  free(directory);
  free(filename);
  return diamond;
}
#endif


/* Case Insensitive fopen */
DataStream *ci_fopen(const char *file_name, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode)
{
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
  return Common::File::OpenFile(file_name, open_mode, work_mode);
#else
  DataStream *fs = NULL;
  char *fullpath = ci_find_file(NULL, (char*)file_name);

  /* If I didn't find a file, this could be writing a new file,
      so use whatever file_name they passed */
  if (fullpath == NULL) {
    fs = Common::File::OpenFile(file_name, open_mode, work_mode);
  } else {
    fs = Common::File::OpenFile(fullpath, open_mode, work_mode);
    free(fullpath);
  }

  return fs;
#endif
}
