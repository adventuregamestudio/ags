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

#include <memory>

#include "util/string.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

extern "C" char *ags_strlwr(char *s);
extern "C" char *ags_strupr(char *s);
extern "C" int ags_stricmp(const char *, const char *);
extern "C" int ags_strnicmp(const char *, const char *, size_t);
extern "C" char *ags_strdup(const char *s);

void unescape(char *buffer);
// Break up the text into lines
void split_lines(const char *todis, int wii, int fonnt);

//=============================================================================

// Converts char* to string and frees original malloc-ed array;
// This is used when we get a malloc'd char array from some utility function.
Common::String cbuf_to_string_and_free(char *char_buf);

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

    // Serialize and unserialize unterminated string prefixed with 32-bit length;
    // length is presented as 32-bit integer integer
    String          ReadString(std::shared_ptr<AGS::Common::Stream> in);
    void            ReadString(char *cstr, std::shared_ptr<AGS::Common::Stream> in, size_t buf_limit);
    void            ReadString(char **cstr, std::shared_ptr<AGS::Common::Stream> in);
    void            ReadString(String &s, std::shared_ptr<AGS::Common::Stream> in);
    void            SkipString(std::shared_ptr<AGS::Common::Stream> in);
    void            WriteString(const String &s, std::shared_ptr<AGS::Common::Stream> out);
    void            WriteString(const char *cstr, std::shared_ptr<AGS::Common::Stream> out);

    // Serialize and unserialize string as c-string (null-terminated sequence)
    void            ReadCStr(char *buf, std::shared_ptr<AGS::Common::Stream> in, size_t buf_limit);
    void            SkipCStr(std::shared_ptr<AGS::Common::Stream> in);
    void            WriteCStr(const char *cstr, std::shared_ptr<AGS::Common::Stream> out);
    void            WriteCStr(const String &s, std::shared_ptr<AGS::Common::Stream> out);
}
} // namespace Common
} // namespace AGS


#endif // __AGS_CN_UTIL__STRINGUTILS_H
