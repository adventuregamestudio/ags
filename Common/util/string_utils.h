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
//
// Various string helpers.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRINGUTILS_H
#define __AGS_CN_UTIL__STRINGUTILS_H

#include "util/string_types.h"

namespace AGS
{
namespace Common
{

class Stream;

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
    // Tries to convert whole string into float value;
    // returns def_val on failure
    float           StringToFloat(const String &s, float def_val = 0.f);

    // A simple unescape string implementation, unescapes "\\x" into '\x'.
    String          Unescape(const String &s);
    // Converts a classic wildcard search pattern into C++11 compatible regex pattern
    String          WildcardToRegex(const String &wildcard);

    // Serialize and unserialize unterminated string prefixed with 32-bit length;
    // length is presented as 32-bit integer integer
    String          ReadString(Stream *in);
    void            ReadString(char *cstr, Stream *in, size_t buf_limit);
    void            ReadString(char **cstr, Stream *in);
    void            ReadString(String &s, Stream *in);
    void            SkipString(Stream *in);
    void            WriteString(const String &s, Stream *out);
    void            WriteString(const char *cstr, Stream *out);
    void            WriteString(const char *cstr, size_t len, Stream *out);

    // Serialize and unserialize string as c-string (null-terminated sequence)
    //
    // Reads a null-terminated string until getting a null-terminator.
    // writes into the buffer up to the buf_limit.
    // Note that this will keep reading the stream out until 0 is read,
    // even if buffer is already full.
    // Guarantees that output buffer will contain a null-terminator.
    void            ReadCStr(char *buf, Stream *in, size_t buf_limit);
    // Reads N characters into the provided buffer.
    // Guarantees that output buffer will contain a null-terminator.
    void            ReadCStrCount(char *buf, Stream *in, size_t count);
    // Reads a null-terminated string and !! mallocs !! a char buffer for it;
    // returns nullptr if the read string is empty.
    // Buffer is hard-limited to 1024 bytes, including null-terminator.
    // Strictly for compatibility with the C lib code!
    char *          ReadMallocCStrOrNull(Stream *in);
    void            SkipCStr(Stream *in);
    void            WriteCStr(const char *cstr, Stream *out);
    void            WriteCStr(const String &s, Stream *out);

    // Serialize and unserialize a string map, both keys and values are read using ReadString
    void            ReadStringMap(StringMap &map, Stream *in);
    void            WriteStringMap(const StringMap &map, Stream *out);


    // Parses enum value by name, using provided C-string array,
    // where strings are compared as case-insensitive; returns def_val if failed;
    template<typename T, std::size_t SIZE>
    T ParseEnum(const String &option, const CstrArr<SIZE>& arr, const T def_val = static_cast<T>(-1))
    {
        for (auto it = arr.cbegin(); it < arr.cend(); ++it)
            if ((*it && *it[0] != 0) && (option.CompareNoCase(*it) == 0))
                return static_cast<T>(it - arr.begin());
        return def_val;
    }
    // Parses enum value either as a number, or searching withing the C-string array,
    // where strings are compared as case-insensitive; returns def_val if failed to do both
    template<typename T, std::size_t SIZE>
    T ParseEnumAllowNum(const String &option, const CstrArr<SIZE>& arr, const T def_val = static_cast<T>(-1))
    {
        int num = StrUtil::StringToInt(option, -1);
        if (num >= 0) return static_cast<T>(num);
        for (auto it = arr.cbegin(); it < arr.cend(); ++it)
            if ((*it && *it[0] != 0) && (option.CompareNoCase(*it) == 0))
                return static_cast<T>(it - arr.begin());
        return def_val;
    }

    // Convert utf-8 string to ascii/ansi representation;
    // writes into out_cstr buffer limited by out_sz bytes; returns bytes written.
    size_t ConvertUtf8ToAscii(const char *mbstr, const char *loc_name, char *out_cstr, size_t out_sz);
    // Convert utf-8 string to wide-string (16-bit char);
    // writes into out_wcstr buffer limited by out_sz *wchars*; returns *wchars* written.
    size_t ConvertUtf8ToWstr(const char *mbstr, wchar_t *out_wcstr, size_t out_sz);
    // Convert wide-string to utf-8 string;
    // writes into out_mbstr buffer limited by out_sz *bytes*; returns *bytes* written.
    size_t ConvertWstrToUtf8(const wchar_t *wcstr, char *out_mbstr, size_t out_sz);
}
} // namespace Common
} // namespace AGS


#endif // __AGS_CN_UTIL__STRINGUTILS_H
