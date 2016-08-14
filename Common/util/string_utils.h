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
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRINGUTILS_H
#define __AGS_CN_UTIL__STRINGUTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#if !defined (WINDOWS_VERSION)

#if !defined (strlwr)
extern "C" char *strlwr(char *s);
#endif

#if !defined (strupr)
extern "C" char *strupr(char *s);
#endif

#if !defined (stricmp)
#define stricmp strcasecmp
#endif

#if !defined (strnicmp)
#define strnicmp strncasecmp
#endif

#else

#if !defined (snprintf)
#define snprintf _snprintf
#endif

#endif // !WINDOWS_VERSION

void unescape(char *buffer);
// Break up the text into lines
void split_lines(const char *todis, int wii, int fonnt);

//=============================================================================

// TODO: remove later when arrays of chars are replaced by string class
// fputstring writes c-string with null terminator
void fputstring(const char *sss, Common::Stream *out);
// fgetstring_limit limits number of bytes stored in the buffer, but it does NOT limit
// number of bytes being read; the string is continued to be read until null terminator
// is reached
void fgetstring_limit(char *sss, Common::Stream *in, int bufsize);
// fgestring is similar to fgetstring_limit, but it assumes very large arbitrary buffer limit
// TODO: replace this function, because it should not exist >:[
void fgetstring(char *sss, Common::Stream *in);

#include "util/string.h"

namespace AGS
{
namespace Common
{
namespace StrUtil
{
    enum ConversionError
    {
        kNoError,   // conversion successful
        kFailed,    // conversion failed (e.g. wrong format)
        kOutOfRange // the resulting value is out of range
    };

    // Convert integer to string, by printing its value
    String          IntToString(int val);
    // Tries to convert whole string into integer value;
    // returns def_val on failure
    int             StringToInt(const String &s, int def_val = 0);
    // Tries to convert whole string into integer value;
    // Returns error code if any non-digit character was met or if value is out
    // of range; the 'val' variable will be set with resulting integer, or
    // def_val on failure
    ConversionError StringToInt(const String &s, int &val, int def_val);

    // Serializes and unserializes unterminated string prefixed with length;
    // length is presented as int32 integer
    String          ReadString(Stream *in);
    void            WriteString(const String &s, Stream *out);

    // Serializes string as c-string (null-terminated sequence)
    void            WriteCStr(const String &s, Stream *out);
}
} // namespace Common
} // namespace AGS


#endif // __AGS_CN_UTIL__STRINGUTILS_H
