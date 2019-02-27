
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
// String class with that wraps std::string. Because we require C++11 as
// minimum, there should be _no_ Copy-On-Write (COW) behaviour that caused
// havoc with multi threading.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRING_H
#define __AGS_CN_UTIL__STRING_H

#include <vector>
#include <string>
#include <stdarg.h>
#include "core/types.h"
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

class Stream;

class String final
{
public:
    // Standard constructor: intialize empty string
    inline String() : __data() {}
    // Copy constructor
    inline String(const String &str): __data(str.__data) {}
    // Initialize with C-string
    inline String(const char *cstr) : __data(cstr ? cstr : "") {}
    // Initialise with std::string
    explicit inline String(const std::string& cppstr): __data(cppstr) {}
    inline ~String() {}

    // Get underlying C-string for reading
    inline const char *GetCStr() const { return __data.c_str(); }
    // Get character count
    inline size_t GetLength() const { return __data.length(); }
    // Know if the string is empty (has no meaningful characters)
    inline bool IsEmpty() const { return __data.empty(); }

    // Read() method implies that string length is initially unknown.
    // max_chars parameter determine the buffer size limit.
    // If stop_at_limit flag is set, it will read only up to the max_chars.
    // Otherwise (by default) hitting the limit won't stop stream reading;
    // the data will be read until null-terminator or EOS is met, and buffer
    // will contain only leftmost part of the longer string that fits in.
    // This method is better fit for reading from binary streams.
    void    Read(Stream *in, size_t max_chars = 5000000, bool stop_at_limit = false);
    // ReadCount() reads up to N characters from stream, ignoring null-
    // terminator. This method is better fit for reading from text
    // streams, or when the length of string is known beforehand.
    void    ReadCount(Stream *in, size_t count);
    // Write() puts the null-terminated string into the stream.
    void    Write(Stream *out) const;

    //-------------------------------------------------------------------------
    // String analysis methods
    //-------------------------------------------------------------------------

    // Compares with given C-string
    int     Compare(const String& str) const;
    int     CompareNoCase(const String& str) const;
    // Compares the leftmost part of this string with given string
    bool     StartsWith(const String& str) const;
    bool     StartsWithNoCase(const String& str) const;
    // Compares the rightmost part of this string with given string
    bool     EndsWithNoCase(const String& str) const;

    // These functions search for character or substring inside this string
    // and return the index of the (first) character, or -1 if nothing found.
    size_t  FindChar(char c, size_t from = 0) const;
    size_t  FindCharReverse(char c, size_t from = -1) const;

    std::vector<String> Split(const String& delims, int max_splits = -1) const;

    // Get Nth character with bounds check (as opposed to subscript operator)
    inline char GetAt(size_t index) const
    {
        return (index <= __data.length()) ? __data[index] : 0;
    }
    inline char GetLast() const
    {
        auto len = __data.length();
        return (len > 0) ? __data[len - 1] : 0;
    }

    //-------------------------------------------------------------------------
    // Value cast methods
    //-------------------------------------------------------------------------

    int     ToInt() const;

    //-------------------------------------------------------------------------
    // Factory methods
    //-------------------------------------------------------------------------

    static String FromFormat(const char *fcstr, ...);
    static String FromFormatV(const char *fcstr, va_list argptr);
    // Reads stream until null-terminator or EOS
    static String FromStream(Stream *in, size_t max_chars = 5000000, bool stop_at_limit = false);
    // Reads up to N chars from stream
    static String FromStreamCount(Stream *in, size_t count);

    // Creates a lowercased copy of the string
    String  Lower() const;

    // Extract N leftmost characters as a new string
    String  Left(size_t count) const;
    // Extract up to N characters starting from given index
    String  Mid(size_t from, size_t count = -1) const;

    //-------------------------------------------------------------------------
    // String modification methods
    //-------------------------------------------------------------------------

    // Append* methods add content at the string's end, increasing its length
    // Add C-string at string's end
    inline void    Append(const String& str) { __data += str.__data; }
    // Add single character at string's end
    inline void    AppendChar(char c) { if (c) { __data += c; } }
    // Clip* methods decrease the string, removing defined part
    // Cuts off leftmost N characters
    void    ClipLeft(size_t count);
    // Cuts out N characters starting from given index
    void    ClipMid(size_t from, size_t count = -1);
    // Cuts off rightmost N characters
    void    ClipRight(size_t count);
    // Sets string length to zero
    inline void    Empty() { __data.clear(); }
    // Makes a new string by putting in parameters according to format string
    void    Format(const char *fcstr, ...);
    // Decrement ref counter and deallocate data if must.
    // Free() should be called only when buffer is not needed anymore;
    // if string must be truncated to zero length, but retain the allocated
    // memory, call Empty() instead.
    void    Free();
    // Convert string to lowercase equivalent
    void    MakeLower();
    // Prepend* methods add content before the string's head, increasing its length
    // Add C-string before string's head
    void    Prepend(const String& str);
    // Add single character before string's head
    void    PrependChar(char c);
    // Replaces all occurences of one character with another character
    void    Replace(char what, char with);
    // Replaces particular substring with another substring; new substring
    // may have different length
    void    ReplaceMid(size_t from, size_t count, const String& str);
    // Overwrite the Nth character of the string; does not change string's length
    void    SetAt(size_t index, char c);
    // Makes a new string by copying up to N chars from C-string
    void    SetString(const String& str, size_t length = -1);
    // For all Trim functions, if given character value is 0, all whitespace
    // characters (space, tabs, CRLF) are removed.
    // Remove trailing characters from the string
    void    TrimRight(char c = 0);
    // Truncate* methods decrease the string to the part of itself
    // Truncate the string to the leftmost N characters
    void    TruncateToLeft(size_t count);

    //-------------------------------------------------------------------------
    // Operators
    //-------------------------------------------------------------------------

    inline char operator[](size_t index) const { return __data[index]; }
    inline bool operator==(const String& rhs) const { return this->__data.compare(rhs.__data) == 0; }
    inline bool operator!=(const String& rhs) const { return this->__data.compare(rhs.__data) != 0; }
    inline bool operator<(const String& rhs) const { return this->__data.compare(rhs.__data) < 0; }


private:
    std::string __data {};
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STRING_H
