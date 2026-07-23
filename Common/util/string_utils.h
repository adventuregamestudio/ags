//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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

#include <utility>
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

    // Note: this is used when parsing arbitrary text or script.
    // Tells if this character may be a part of a script symbol
    inline bool IsScriptWordChar(char c)
    {
        return ((c >= 'A') && (c <= 'Z')) ||
               ((c >= 'a') && (c <= 'z')) ||
               ((c >= '0') && (c <= '9')) ||
               (c == '_');
    }

    // Compares two strings lexographically, by the meaning of their characters
    // rather than their code values. For example, 'À' follows 'A' and 'Č'
    // follows 'C', as opposed to common char code-based comparison, where 'À'
    // is positioned after 'Z'.
    int             LexographicalCompare(const String &s1, const String &s2, const char *locale_name = "");
    inline int      LexographicalCompare(const char *cstr1, const char *cstr2, const char *locale_name = "")
                        { return LexographicalCompare(String::Wrapper(cstr1), String::Wrapper(cstr2), locale_name); }
    // Compares two strings lexographically and case-insensitively.
    // For example, 'À' follows 'A' and 'Č' follows 'C', as opposed to common
    // char code-based comparison, where 'À' is positioned after 'Z'.
    int             LexographicalCompareNoCase(const String &s1, const String &s2, const char *locale_name = "");
    inline int      LexographicalCompareNoCase(const char* cstr1, const char* cstr2, const char* locale_name = "")
                        { return LexographicalCompareNoCase(String::Wrapper(cstr1), String::Wrapper(cstr2), locale_name); }

    // Constructs a string 'compare' predicate purposed for the certain case described by parameters
    std::unique_ptr<IStrCmp> GetStrCmpImplFor(bool unicode, bool nocase, const char *locale_name = nullptr);
    // Constructs a string 'equal' predicate purposed for the certain case described by parameters
    inline StrEqAuto GetStrEqAutoFor(bool unicode, bool nocase, const char *locale_name = nullptr)
    {
        return StrEqAuto(GetStrCmpImplFor(unicode, nocase, locale_name));
    }
    // Constructs a string 'less' predicate purposed for the certain case described by parameters
    inline StrLessAuto GetStrLessAutoFor(bool unicode, bool nocase, const char *locale_name = nullptr)
    {
        return StrLessAuto(GetStrCmpImplFor(unicode, nocase, locale_name));
    }

    // Convert integer to string, by printing its value
    String          IntToString(int val);
    // Tries to convert whole string into integer value;
    // returns def_val on failure
    int             StringToInt(const String &s, int def_val = 0);
    int             StringToIntHex(const String &s, int def_val = 0);
    int64_t         StringToInt64(const String &s, int64_t def_val = 0);
    uint64_t        StringToUInt64(const String &s, uint64_t def_val = 0);
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

    // Remove double quotes from the string
    inline String   Undoublequote(const String &s) { String und = s; und.Trim('\"'); return und; }

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
    // Writes a fixed-size field for binary serialization: pads/truncates by raw
    // byte length, not Unicode codepoint boundaries.
    void            WriteFixedString(const String &s, size_t len, Stream *out);
    void            WriteFixedString(const char *cstr, size_t len, Stream *out);

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
    // Variant of above, that allocates std::string.
    // Buffer is hard-limited to 1024 bytes, including null-terminator.
    std::string     ReadCStrAsStdString(Stream *in);
    void            SkipCStr(Stream *in);
    void            WriteCStr(const char *cstr, Stream *out);
    void            WriteCStr(const String &s, Stream *out);

    // Serialize and unserialize a string map, both keys and values are read using ReadString
    void            ReadStringMap(StringMap &map, Stream *in);
    void            SkipStringMap(Stream *in);
    void            WriteStringMap(const StringMap &map, Stream *out);


    // Parses enum value by name, using provided C-string array,
    // where strings are compared as case-insensitive; returns def_val if failed
    template<typename T, std::size_t SIZE>
    T ParseEnum(const String &option, const CstrArr<SIZE> &arr, const T &def_val)
    {
        for (auto it = arr.cbegin(); it < arr.cend(); ++it)
            if ((*it && *it[0] != 0) && (option.CompareNoCase(*it) == 0))
                return static_cast<T>(it - arr.begin());
        return def_val;
    }
    // Parses enum value by name, using provided C-string array and a base value (e.g. 0, -1, +1, etc),
    // where strings are compared as case-insensitive; returns def_val if failed
    template<typename T, std::size_t SIZE>
    T ParseEnumWithBase(const String &option, const CstrArr<SIZE> &arr, const T &base_val, const T &def_val)
    {
        for (auto it = arr.cbegin(); it < arr.cend(); ++it)
            if ((*it && *it[0] != 0) && (option.CompareNoCase(*it) == 0))
                return static_cast<T>(it - arr.begin() + base_val);
        return def_val;
    }
    // Parses enum value either as a number, or searching withing the C-string array,
    // where strings are compared as case-insensitive; defines a base value (e.g. 0, -1, +1, etc);
    // returns def_val if failed to do both
    template<typename T, std::size_t SIZE>
    T ParseEnumAllowNum(const String &option, const CstrArr<SIZE> &arr, const T &base_val, const T &def_val)
    {
        // Try parse as a number, detect failure by returning an out-of-range "default value"
        int num = StrUtil::StringToInt(option, base_val - 1);
        if (num >= base_val)
            return num < static_cast<int>(SIZE + base_val) ? static_cast<T>(num) : def_val;
        // Try parse as an option from array
        for (auto it = arr.cbegin(); it < arr.cend(); ++it)
            if ((*it && *it[0] != 0) && (option.CompareNoCase(*it) == 0))
                return static_cast<T>(it - arr.begin() + base_val);
        return def_val;
    }
    // Parses enum value by name, using provided map of correspondence between
    // C-strings and enum constants, where strings are compared as case-insensitive;
    // returns def_val if failed
    template<typename T, std::size_t SIZE>
    T ParseEnumOptions(const String &option, const std::array<std::pair<const char*, T>, SIZE> &arr,
        const T &def_val)
    {
        for (auto it = arr.cbegin(); it < arr.cend(); ++it)
            if ((it->first && it->first[0] != 0) && (option.CompareNoCase(it->first) == 0))
                return static_cast<T>(it->second);
        return def_val;
    }

    // Safely selects a c-string out of array using an arbitrary option index;
    // returns def_val if index is out of bounds.
    // TODO: make this a generic <T> function and move to algorithm utils?
    template<std::size_t SIZE>
    const char *SelectCStr(const std::array<const char *, SIZE> &arr, int option, const char *def_val = "")
    {
        return (option >= 0 && static_cast<size_t>(option) < SIZE) ? arr[option] : def_val;
    }

    // Reads the string and splits it by key and value, separated by the first found separator char;
    // if no separator is found, then returns the key as the source string, with empty value.
    std::pair<String, String> GetKeyValue(const String &s, char key_val_separator = '=');

    // Convert utf-8 string to ascii/ansi representation;
    // writes into out_cstr buffer limited by out_sz bytes; returns bytes written.
    size_t ConvertUtf8ToAscii(const char *mbstr, const char *loc_name, char *out_cstr, size_t out_sz);
    // Convert utf-8 string to wide-string (16-bit char);
    // writes into out_wcstr buffer limited by out_sz *wchars*; returns *wchars* written.
    size_t ConvertUtf8ToWstr(const char *mbstr, wchar_t *out_wcstr, size_t out_sz);
    // Convert wide-string to utf-8 string;
    // writes into out_mbstr buffer limited by out_sz *bytes*; returns *bytes* written.
    size_t ConvertWstrToUtf8(const wchar_t *wcstr, char *out_mbstr, size_t out_sz);

    // Applies text directon using LTR and RTL instructions. Allows to have LTR and RTL
    // sequences in any order, using respective Unicode control characters.
    // Control characters are not copied, but removed from the final string.
    // Returns a new resulting string.
    // IMPORTANT: Assumes that the input string contains UTF-8 data.
    String ApplyTextDirection(const String &text, bool default_rtl = false);

    // Tries to find a UTF8 locale name for the given language name,
    // suitable for the current runtime backend. Returns the locale name found,
    // or empty string if none was found.
    String FindCompatibleUTF8LocaleName(const String &lang_name);
}
} // namespace Common
} // namespace AGS


#endif // __AGS_CN_UTIL__STRINGUTILS_H
