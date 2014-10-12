
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
// String class with simple memory management and copy-on-write behavior.
//
// String objects do reference counting and share data buffer on assignment.
// The reallocation and copying is done only when the string is modified.
// This means that passing string object by value is about as effective, as
// passing by reference.
//
// The copying of memory inside buffer is reduced to minimum. If the string is
// truncated, it is not aligned to buffer head each time, instead the c-str
// pointer is advanced, or null-terminator is put on the new place. Similarly,
// when string is enlarged and new characters are prepended or appended, only
// c-str pointer and null-terminator's position are changed, if there's enough
// space before and after meaningful string data.
//
// The class provides means to reserve large amount of buffer space before
// making modifications, as well as compacting buffer to minimal size.
//
// For all methods that expect C-string as parameter - if the null pointer is
// passed in place of C-string it is treated in all aspects as a valid empty
// string.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRING_H
#define __AGS_CN_UTIL__STRING_H

#include "core/types.h"
#include "debug/assert.h"

namespace AGS
{
namespace Common
{

class Stream;

class String
{
public:
    // Standart constructor: intialize empty string
    String();
    // Copy constructor
    String(const String&);
    // Initialize with C-string
    String(const char *cstr);
    // Initialize by copying up to N chars from C-string
    String(const char *cstr, int length);
    // Initialize by filling N chars with certain value
    String(char c, int count);
    ~String();

    // Get underlying C-string for reading
    inline const char *GetCStr() const
    {
        return _meta ? _meta->CStr : "";
    }
    // Get character count
    inline int  GetLength() const
    {
        return _meta ? _meta->Length : 0;
    }
    // Know if the string is empty (has no meaningful characters)
    inline bool IsEmpty() const
    {
        return _meta ? _meta->Length == 0 : true;
    }

    // Those getters are for tests only, hence ifdef _DEBUG
#ifdef _DEBUG
    inline const char *GetData() const
    {
        return _data;
    }

    inline int GetCapacity() const
    {
        return _meta ? _meta->Capacity : 0;
    }

    inline int GetRefCount() const
    {
        return _meta ? _meta->RefCount : 0;
    }
#endif

    // Read() method implies that string length is initially unknown.
    // max_chars parameter determine the buffer size limit.
    // If stop_at_limit flag is set, it will read only up to the max_chars.
    // Otherwise (by default) hitting the limit won't stop stream reading;
    // the data will be read until null-terminator or EOS is met, and buffer
    // will contain only leftmost part of the longer string that fits in.
    // This method is better fit for reading from binary streams.
    void    Read(Stream *in, int max_chars = 5000000, bool stop_at_limit = false);
    // ReadCount() reads up to N characters from stream, ignoring null-
    // terminator. This method is better fit for reading from text
    // streams, or when the length of string is known beforehand.
    void    ReadCount(Stream *in, int count);
    // Write() puts the null-terminated string into the stream.
    void    Write(Stream *out) const;
    // WriteCount() writes N characters to stream, filling the remaining
    // space with null-terminators when needed.
    void    WriteCount(Stream *out, int count) const;

    static void WriteString(const char *cstr, Stream *out);

    //-------------------------------------------------------------------------
    // String analysis methods
    //-------------------------------------------------------------------------

    // Compares with given C-string
    int     Compare(const char *cstr) const;
    int     CompareNoCase(const char *cstr) const;
    // Compares the leftmost part of this string with given C-string
    int     CompareLeft(const char *cstr, int count = -1) const;
    int     CompareLeftNoCase(const char *cstr, int count = -1) const;
    // Compares any part of this string with given C-string
    int     CompareMid(const char *cstr, int from, int count = -1) const;
    int     CompareMidNoCase(const char *cstr, int from, int count = -1) const;
    // Compares the rightmost part of this string with given C-string
    int     CompareRight(const char *cstr, int count = -1) const;
    int     CompareRightNoCase(const char *cstr, int count = -1) const;

    int     FindChar(char c, int from = 0) const;
    int     FindCharReverse(char c, int from = -1) const;

    // Section methods treat string as a sequence of 'fields', separated by
    // special character. They search for a substring consisting of all such
    // 'fields' from the 'first' to the 'last', inclusive; the bounding
    // separators are optionally included too.
    // Section indexes are zero-based. The first (0th) section is always
    // located before the first separator and the last section is always
    // located after the last separator, meaning that if the outermost
    // character in string is separator char, there's still an empty trailing
    // field beyond that.
    // This also means that there's always at least one section in any string,
    // even if there are no separating chars.
    bool    FindSection(char separator, int first, int last, bool exclude_first_sep, bool exclude_last_sep,
                        int &from, int &to) const;

    // Get Nth character with bounds check (as opposed to subscript operator)
    inline char GetAt(int index) const
    {
        return (_meta && index >= 0 && index <= _meta->Length) ? _meta->CStr[index] : 0;
    }
    inline char GetLast() const
    {
        return (_meta && _meta->Length > 0) ? _meta->CStr[_meta->Length - 1] : 0;
    }

    //-------------------------------------------------------------------------
    // Value cast methods
    //-------------------------------------------------------------------------

    int     ToInt() const;

    //-------------------------------------------------------------------------
    // Factory methods
    //-------------------------------------------------------------------------

    static String FromFormat(const char *fcstr, ...);
    // Reads stream until null-terminator or EOS
    static String FromStream(Stream *in, int max_chars = 5000000, bool stop_at_limit = false);
    // Reads up to N chars from stream
    static String FromStreamCount(Stream *in, int count);

    // Creates a lowercased copy of the string
    String  Lower() const;
    // Creates an uppercased copy of the string
    String  Upper() const;

    // Extract N leftmost characters as a new string
    String  Left(int count) const;
    // Extract up to N characters starting from given index
    String  Mid(int from, int count = -1) const;
    // Extract N rightmost characters
    String  Right(int count) const;

    // Extract leftmost part, separated by the given char
    String  LeftSection(char separator, bool exclude_separator = true) const;
    // Extract rightmost part, separated by the given char
    String  RightSection(char separator, bool exclude_separator = true) const;
    // Extract the section between Xth and Yth appearance of the given character
    String  Section(char separator, int first, int last,
                              bool exclude_first_sep = true, bool exclude_last_sep = true) const;

    //-------------------------------------------------------------------------
    // String modification methods
    //-------------------------------------------------------------------------

    // Ensure string has at least space to store N chars;
    // this does not change string contents, nor length
    void    Reserve(int max_length);
    // Ensure string has at least space to store N additional chars
    void    ReserveMore(int more_length);
    // Make string's buffer as small as possible to hold current data
    void    Compact();

    // Append* methods add content at the string's end, increasing its length
    // Add C-string at string's end
    void    Append(const char *cstr);
    // Add single character at string's end
    void    AppendChar(char c);
    // Clip* methods decrease the string, removing defined part
    // Cuts off leftmost N characters
    void    ClipLeft(int count);
    // Cuts out N characters starting from given index
    void    ClipMid(int from, int count = -1);
    // Cuts off rightmost N characters
    void    ClipRight(int count);
    // Cuts off leftmost part, separated by the given char
    void    ClipLeftSection(char separator, bool include_separator = true);
    // Cuts off rightmost part, separated by the given char
    void    ClipRightSection(char separator, bool include_separator = true);
    // Cuts out the section between Xth and Yth appearance of the given character
    void    ClipSection(char separator, int first, int last,
                              bool include_first_sep = true, bool include_last_sep = true);
    // Sets string length to zero
    void    Empty();
    // Makes a new string by filling N chars with certain value
    void    FillString(char c, int count);
    // Makes a new string by putting in parameters according to format string
    void    Format(const char *fcstr, ...);
    // Decrement ref counter and deallocate data if must.
    // Free() should be called only when buffer is not needed anymore;
    // if string must be truncated to zero length, but retain the allocated
    // memory, call Empty() instead.
    void    Free();
    // Convert string to lowercase equivalent
    void    MakeLower();
    // Convert string to uppercase equivalent
    void    MakeUpper();
    // Prepend* methods add content before the string's head, increasing its length
    // Add C-string before string's head
    void    Prepend(const char *cstr);
    // Add single character before string's head
    void    PrependChar(char c);
    // Replaces all occurences of one character with another character
    void    Replace(char what, char with);
    // Overwrite the Nth character of the string; does not change string's length
    void    SetAt(int index, char c);
    // Makes a new string by copying up to N chars from C-string
    void    SetString(const char *cstr, int length = -1);
    // For all Trim functions, if given character value is 0, all whitespace
    // characters (space, tabs, CRLF) are removed.
    // Remove heading and trailing characters from the string
    void    Trim(char c = 0);
    // Remove heading characters from the string; 
    void    TrimLeft(char c = 0);
    // Remove trailing characters from the string
    void    TrimRight(char c = 0);
    // Truncate* methods decrease the string to the part of itself
    // Truncate the string to the leftmost N characters
    void    TruncateToLeft(int count);
    // Truncate the string to the middle N characters
    void    TruncateToMid(int from, int count = -1);
    // Truncate the string to the rightmost N characters
    void    TruncateToRight(int count);
    // Truncate the string to the leftmost part, separated by the given char
    void    TruncateToLeftSection(char separator, bool exclude_separator = true);
    // Truncate the string to the rightmost part, separated by the given char
    void    TruncateToRightSection(char separator, bool exclude_separator = true);
    // Truncate the string to the section between Xth and Yth appearance of
    // the given character
    void    TruncateToSection(char separator, int first, int last,
                              bool exclude_first_sep = true, bool exclude_last_sep = true);

    //-------------------------------------------------------------------------
    // Operators
    //-------------------------------------------------------------------------

    inline operator const char *() const
    {
        return GetCStr();
    }
    // Assign String by sharing data reference
    String &operator=(const String&);
    // Assign C-string by copying contents
    String &operator=(const char *cstr);
    inline char operator[](int index) const
    {
        assert(_meta && index >= 0 && index <= _meta->Length);
        return _meta->CStr[index];
    }
    inline bool operator==(const char *cstr) const
    {
        return Compare(cstr) == 0;
    }
    inline bool operator!=(const char *cstr) const
    {
        return Compare(cstr) != 0;
    }
    inline bool operator <(const char *cstr) const
    {
        return Compare(cstr) < 0;
    }

private:
    // Creates new empty string with buffer enough to fit given length
    void    Create(int buffer_length);
    // Release string and copy data to the new buffer
    void    Copy(int buffer_length, int offset = 0);
    // Aligns data at given offset
    void    Align(int offset);

    // Ensure this string is a compact independent copy, with ref counter = 1
    void    BecomeUnique();
    // Ensure this string is independent, and there's enough space before
    // or after the current string data
    void    ReserveAndShift(bool left, int more_length);

    struct Header
    {
        Header();

        int32_t RefCount;   // reference count
        // Capacity and Length do not include null-terminator
        int32_t Capacity;   // available space, in characters
        int32_t Length;     // used space
        char    *CStr;      // pointer to string data start
    };

    union
    {
        char    *_data;
        Header  *_meta;
    };

    static const int _internalBufferLength = 3000;
    static char _internalBuffer[3001];
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STRING_H
