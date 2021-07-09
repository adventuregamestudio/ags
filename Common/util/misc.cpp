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

#include "core/platform.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if !AGS_PLATFORM_OS_WINDOWS
#include <dirent.h>
#include <unistd.h>
#endif

#include <allegro.h> // file path functions
#include "util/path.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/stream.h"


using namespace AGS::Common;
using namespace AGS::Common::Path;
using namespace AGS::Common::Directory;

//
// TODO: rewrite all this in a cleaner way perhaps, and move to our file or path utilities unit
//

#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
#include <string.h>
/* File Name Concatenator basically on Windows / DOS */
String ci_find_file(const String& dir_name, const String& file_name)
{
  String diamond = nullptr;

  if (dir_name.IsNullOrSpace() && file_name.IsNullOrSpace())
      return nullptr;

  if (dir_name.IsNullOrSpace()r) {
    diamond = file_name;
  } else {
    diamond = ConcatPaths(dir_name,file_name);
  }
  diamond.MakeUpper();
  FixupPath(diamond);
  return diamond;
}

#else

/* Case Sensitive File Find */
String ci_find_file(const String& dir_name, const String& file_name)
{
    struct stat   statbuf;
    struct dirent *entry     = nullptr;
    DIR           *rough     = nullptr;
    String prevdir = nullptr;
    String diamond = nullptr;
    String directory = nullptr;
    String filename = nullptr;

    if (dir_name.IsNullOrSpace() && file_name.IsNullOrSpace())
        return nullptr;

    if (!dir_name.IsNullOrSpace()) {
        directory = dir_name;
        FixupPath(directory);
    }

    if (!file_name.IsNullOrSpace()) {
        filename = file_name;
        FixupPath(filename);
    }

    if(!directory.IsNullOrSpace() && !filename.IsNullOrSpace() && filename.FindString("..") == -1) {
        String path = ConcatPaths(directory,filename);
        lstat(path.GetCStr(), &statbuf);
        if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
            return path;
        }
    }

    if (directory.IsNullOrSpace()) {
        String match = GetFilename(filename);
        if (match.IsNullOrSpace())
            return nullptr;

        if (match.Compare(filename) == 0) {
            directory = ".";
        } else {
            directory = GetDirectoryPath(filename);
        }

        filename = match;
    }

    if ((prevdir = GetCurrentDirectory()).IsNullOrSpace()) {
        fprintf(stderr, "ci_find_file: cannot open current working directory\n");
        return nullptr;
    }

    if (SetCurrentDirectory(directory).IsNullOrSpace()) {
        fprintf(stderr, "ci_find_file: cannot change to directory: %s\n", directory.GetCStr());
        return nullptr;
    }

    if ((rough = opendir(directory.GetCStr())) == nullptr) {
        fprintf(stderr, "ci_find_file: cannot open directory: %s\n", directory.GetCStr());
        return nullptr;
    }

    while ((entry = readdir(rough)) != nullptr) {
        lstat(entry->d_name, &statbuf);
        if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
            if (filename.Compare(entry->d_name) == 0) {
#if AGS_PLATFORM_DEBUG
                fprintf(stderr, "ci_find_file: Looked for %s in rough %s, found diamond %s.\n", filename.GetCStr(), directory.GetCStr(), entry->d_name);
#endif // AGS_PLATFORM_DEBUG
                diamond = ConcatPaths(directory,entry->d_name);
                break;
            }
        }
    }
    closedir(rough);

    SetCurrentDirectory(prevdir);

    return diamond;
}
#endif


/* Case Insensitive fopen */
Stream *ci_fopen(const char *file_name, FileOpenMode open_mode, FileWorkMode work_mode)
{
#if !defined (AGS_CASE_SENSITIVE_FILESYSTEM)
    return File::OpenFile(file_name, open_mode, work_mode);
#else
    Stream *fs = nullptr;
    String fullpath = ci_find_file(nullptr, file_name);

    /* If I didn't find a file, this could be writing a new file,
        so use whatever file_name they passed */
    if (fullpath == nullptr) {
        fs = File::OpenFile(file_name, open_mode, work_mode);
    } else {
        fs = File::OpenFile(fullpath.GetCStr(), open_mode, work_mode);
    }

    return fs;
#endif
}