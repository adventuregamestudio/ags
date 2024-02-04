//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STDIOCOMPAT_H
#define __AGS_CN_UTIL__STDIOCOMPAT_H

#include <stdio.h>
#include <stdint.h>

typedef int64_t file_off_t;

// Size of the buffer enough to accomodate a UTF-8 path
#ifdef __cplusplus
const size_t MAX_PATH_SZ = 1024u;
#else
#define MAX_PATH_SZ (1024u)
#endif

#ifdef __cplusplus
extern "C" {
#endif

FILE *ags_fopen(const char *path, const char *mode);
int	 ags_fseek(FILE * stream, file_off_t offset, int whence);
file_off_t	 ags_ftell(FILE * stream);

int ags_file_exists(const char *path);
int ags_directory_exists(const char *path);
int ags_path_exists(const char *path);
file_off_t ags_file_size(const char *path);

int ags_file_remove(const char *path);
int ags_file_rename(const char *src, const char *dst);
int ags_file_copy(const char *src, const char *dst, int overwrite);

#ifdef __cplusplus
}
#endif

#endif // __AGS_CN_UTIL__STDIOCOMPAT_H
