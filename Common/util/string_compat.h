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
#ifndef __AGS_CN_UTIL__STRINGCOMPAT_H
#define __AGS_CN_UTIL__STRINGCOMPAT_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

char *ags_strlwr(char *s);
char *ags_strupr(char *s);
int ags_stricmp(const char *, const char *);
int ags_strnicmp(const char *, const char *, size_t);
char *ags_strdup(const char *s);
char *ags_strstr(const char *haystack, const char *needle);
int ags_strncpy_s(char* strDest, size_t numberOfElements, const char* strSource, size_t count);

#ifdef __cplusplus
}
#endif

#endif // __AGS_CN_UTIL__STRINGCOMPAT_H
