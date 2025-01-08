//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "util/string_compat.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "core/platform.h"
#include "debug/assert.h"

char *ags_strlwr(char *s)
{
    char *p = s;
    for (; *p; p++)
        *p = (char)tolower(*p);
    return s;
}

char *ags_strupr(char *s)
{
    char *p = s;
    for (; *p; p++)
        *p = (char)toupper(*p);
    return s;
}

int ags_stricmp(const char *s1, const char *s2)
{
#if AGS_PLATFORM_OS_WINDOWS
    return stricmp(s1, s2);
#else
    return strcasecmp(s1, s2);
#endif
}

int ags_strnicmp(const char *s1, const char *s2, size_t n)
{
#if AGS_PLATFORM_OS_WINDOWS
    return strnicmp(s1, s2, n);
#else
    return strncasecmp(s1, s2, n);
#endif
}

char *ags_strdup(const char *s)
{
    size_t len = strlen(s);
    char *result = (char *)malloc(len + 1);
    memcpy(result, s, len + 1);
    return result;
}

char *ags_strstr(const char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}

int ags_strncpy_s(char *dest, size_t dest_sz, const char *src, size_t count)
{
    // NOTE: implementation approximately mimics explanation for "strncpy_s":
    // https://en.cppreference.com/w/c/string/byte/strncpy
    assert(dest && dest_sz > 0 && ((dest + dest_sz - 1 < src) || (dest > src + count)));
    if (!dest || dest_sz == 0 || ((dest <= src) && (dest + dest_sz - 1 >= src)) || ((src <= dest) && (src + count - 1 >= dest)))
        return EINVAL; // null buffer, or dest and src overlap
    if (!src)
    {
        dest[0] = 0; // ensure null terminator
        return EINVAL;
    }

    const size_t copy_len = (count < dest_sz - 1) ? count : dest_sz - 1; // reserve null-terminator
    const char *psrc = src;
    const char *src_end = src + copy_len;
    char *pdst = dest;
    for (; *psrc && (psrc != src_end); ++psrc, ++pdst)
        *pdst = *psrc;
    *pdst = 0; // ensure null terminator
    assert((*psrc == 0) || ((psrc - src) == count)); // assert that no *unintended* truncation occured
    if ((*psrc != 0) && ((psrc - src) < count))
        return ERANGE; // not enough dest buffer - error
    return 0; // success
}
