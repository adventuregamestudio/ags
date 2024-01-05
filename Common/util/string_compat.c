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
#include "util/string_compat.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "core/platform.h"

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
