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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug/assert.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_utils.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

/* static */ char String::_internalBuffer[3001];

String::Header::Header()
    : RefCount(0)
    , Capacity(0)
    , Length(0)
{
}

String::String()
    : _data(NULL)
{
}

String::String(const String &str)
    : _data(NULL)
{
    *this = str;
}

String::String(const char *cstr)
    :_data(NULL)
{
    *this = cstr;
}

String::String(const char *cstr, int length)
    : _data(NULL)
{
    SetString(cstr, length);
}

String::String(char c, int count)
    : _data(NULL)
{
    FillString(c, count);
}

String::~String()
{
    Free();
}

void String::Read(Stream *in, int max_chars, bool stop_at_limit)
{
    Empty();
    if (!in)
    {
        return;
    }
    max_chars = max_chars >= 0 ? max_chars : 0;
    if (!max_chars && stop_at_limit)
    {
        return;
    }

    char *read_ptr = _internalBuffer;
    int read_size = 0;
    int ichar;
    do
    {
        ichar = in->ReadByte();
        read_size++;
        if (read_size > max_chars)
        {
            continue;
        }
        *read_ptr = (char)(ichar >= 0 ? ichar : 0);
        if (!*read_ptr || read_ptr - _internalBuffer == _internalBufferLength - 1)
        {
            _internalBuffer[_internalBufferLength] = 0;
            Append(_internalBuffer);
            read_ptr = _internalBuffer;
        }
        else
        {
            read_ptr++;
        }
    }
    while(ichar > 0 && !(stop_at_limit && read_size == max_chars));
}

void String::ReadCount(Stream *in, int count)
{
    Empty();
    if (in && count > 0)
    {
        ReserveAndShift(false, count);
        count = in->Read(_meta->CStr, count);
        _meta->CStr[count] = 0;
        _meta->Length = strlen(_meta->CStr);
    }
}

void String::Write(Stream *out) const
{
    if (out)
    {
        out->Write(GetCStr(), GetLength() + 1);
    }
}

/* static */ void String::WriteString(const char *cstr, Stream *out)
{
    if (out)
    {
        cstr = cstr ? cstr : "";
        out->Write(cstr, strlen(cstr) + 1);
    }
}

int String::Compare(const char *cstr) const
{
    return strcmp(GetCStr(), cstr ? cstr : "");
}

int String::CompareNoCase(const char *cstr) const
{
    return stricmp(GetCStr(), cstr ? cstr : "");
}

int String::CompareLeft(const char *cstr, int count) const
{
    cstr = cstr ? cstr : "";
    return strncmp(GetCStr(), cstr, count >= 0 ? count : strlen(cstr));
}

int String::CompareLeftNoCase(const char *cstr, int count) const
{
    cstr = cstr ? cstr : "";
    return strnicmp(GetCStr(), cstr, count >= 0 ? count : strlen(cstr));
}

int String::CompareMid(const char *cstr, int from, int count) const
{
    cstr = cstr ? cstr : "";
    from = Math::Min(from, GetLength());
    return strncmp(GetCStr() + from, cstr, count >= 0 ? count : strlen(cstr));
}

int String::CompareMidNoCase(const char *cstr, int from, int count) const
{
    cstr = cstr ? cstr : "";
    from = Math::Min(from, GetLength());
    return strnicmp(GetCStr() + from, cstr, count >= 0 ? count : strlen(cstr));
}

int String::CompareRight(const char *cstr, int count) const
{
    cstr = cstr ? cstr : "";
    count = count >= 0 ? count : strlen(cstr);
    int from = Math::Max(0, GetLength() - count);
    return strncmp(GetCStr() + from, cstr, count);
}

int String::CompareRightNoCase(const char *cstr, int count) const
{
    cstr = cstr ? cstr : "";
    count = count >= 0 ? count : strlen(cstr);
    int from = Math::Max(0, GetLength() - count);
    return strnicmp(GetCStr() + from, cstr, count);
}

int String::FindChar(char c, int from) const
{
    if (_meta && c && from < _meta->Length)
    {
        from = from >= 0 ? from : 0;
        const char * found_cstr = strchr(_meta->CStr + from, c);
        return found_cstr ? found_cstr - _meta->CStr : -1;
    }
    return -1;
}

int String::FindCharReverse(char c, int from) const
{
    if (!_meta || !c)
    {
        return -1;
    }

    from = from >= 0 ? Math::Min(from, _meta->Length - 1) : _meta->Length - 1;
    const char *seek_ptr = _meta->CStr + from;
    while (seek_ptr >= _meta->CStr)
    {
        if (*seek_ptr == c)
        {
            return seek_ptr - _meta->CStr;
        }
        seek_ptr--;
    }
    return -1;
}

bool String::FindSection(char separator, int first, int last, bool exclude_first_sep, bool exclude_last_sep,
                        int &from, int &to) const
{
    if (!_meta || !separator)
    {
        return false;
    }
    if (first > last)
    {
        return false;
    }

    int this_field = 0;
    int slice_from = 0;
    int slice_to = 0;
    int slice_at = -1;
    do
    {
        slice_at = FindChar(separator, slice_at + 1);
        if (slice_at < 0)
            slice_at = _meta->Length;
        // found where previous field ends
        if (this_field == last)
        {
            // if previous field is the last one to be included,
            // then set the section tail
            slice_to = exclude_last_sep ? slice_at : slice_at + 1;
        }
        if (slice_at != _meta->Length)
        {
            this_field++;
            if (this_field == first)
            {
                // if the new field is the first one to be included,
                // then set the section head
                slice_from = exclude_first_sep ? slice_at + 1 : slice_at;
            }
        }
    }
    while (slice_at < _meta->Length && this_field <= last);

    // the search is a success if at least the first field was found
    if (this_field >= first)
    {
        // correct the indices to stay in the [0; length] range
        from = slice_from;
        to = slice_to;
        assert(from <= to);
        Math::Clamp(0, _meta->Length, from);
        Math::Clamp(0, _meta->Length, to);
        return true;
    }
    return false;
}

int String::ToInt() const
{
    return atoi(GetCStr());
}

/* static */ String String::FromFormat(const char *fcstr, ...)
{
    fcstr = fcstr ? fcstr : "";
    String str;
    va_list argptr;
    va_start(argptr, fcstr);
    int length = vsnprintf(NULL, 0, fcstr, argptr);
    str.ReserveAndShift(false, length);
    va_start(argptr, fcstr);
    vsprintf(str._meta->CStr, fcstr, argptr);
    str._meta->Length = length;
    str._meta->CStr[str._meta->Length] = 0;
    va_end(argptr);
    return str;
}

/* static */ String String::FromStream(Stream *in, int max_chars, bool stop_at_limit)
{
    String str;
    str.Read(in, max_chars, stop_at_limit);
    return str;
}

/* static */ String String::FromStreamCount(Stream *in, int count)
{
    String str;
    str.ReadCount(in, count);
    return str;
}

String String::Left(int count) const
{
    count = count >= 0 ? Math::Min(count, GetLength()) : GetLength();
    return count == GetLength() ? *this : String(GetCStr(), count);
}

String String::Mid(int from, int count) const
{
    count = count >= 0 ? count : GetLength();
    Math::ClampLength(0, GetLength(), from, count);
    return count == GetLength() ? *this : String(GetCStr() + from, count);
}

String String::Right(int count) const
{
    count = count >= 0 ? Math::Min(count, GetLength()) : GetLength();
    return count == GetLength() ? *this : String(GetCStr() + GetLength() - count, count);
}

// Extract leftmost part, separated by the given char
String String::LeftSection(char separator, bool exclude_separator) const
{
    if (_meta && separator)
    {
        int slice_at = FindChar(separator);
        if (slice_at >= 0)
        {
            slice_at = exclude_separator ? slice_at : slice_at + 1;
            return Left(slice_at);
        }
    }
    return String();
}

// Extract rightmost part, separated by the given char
String String::RightSection(char separator, bool exclude_separator) const
{
    if (_meta && separator)
    {
        int slice_at = FindCharReverse(separator);
        if (slice_at >= 0)
        {
            int count = exclude_separator ? _meta->Length - slice_at - 1 : _meta->Length - slice_at;
            return Right(count);
        }
    }
    return String();
}

// Extract the section between Xth and Yth appearance of the given character
String String::Section(char separator, int first, int last,
                          bool exclude_first_sep, bool exclude_last_sep) const
{
    if (!_meta || !separator)
    {
        return String();
    }

    int slice_from;
    int slice_to;
    if (FindSection(separator, first, last, exclude_first_sep, exclude_last_sep,
        slice_from, slice_to))
    {
        return Mid(slice_from, slice_to - slice_from);
    }
    return String();
}

void String::Reserve(int max_length)
{
    max_length = max_length >= 0 ? max_length : 0;
    if (_meta)
    {
        if (max_length > _meta->Capacity)
        {
            // grow by 50% or at least to total_size
            int grow_length = _meta->Capacity + (_meta->Capacity >> 1);
            Copy(Math::Max(max_length, grow_length));
        }
    }
    else
    {
        Create(max_length);
    }
}

void String::ReserveMore(int more_length)
{
    Reserve(GetLength() + more_length);
}

void String::Compact()
{
    if (_meta && _meta->Capacity > _meta->Length)
    {
        Copy(_meta->Length);
    }
}

void String::Append(const char *cstr)
{
    if (cstr)
    {
        int length = strlen(cstr);
        if (length > 0)
        {
            ReserveAndShift(false, length);
            memcpy(_meta->CStr + _meta->Length, cstr, length);
            _meta->Length += length;
            _meta->CStr[_meta->Length] = 0;
        }
    }
}

void String::AppendChar(char c)
{
    if (c)
    {
        ReserveAndShift(false, 1);
        _meta->CStr[_meta->Length++] = c;
        _meta->CStr[_meta->Length] = 0;
    }
}

void String::ClipLeft(int count)
{
    if (_meta && _meta->Length > 0 && count > 0)
    {
        count = Math::Min(count, _meta->Length);
        BecomeUnique();
        _meta->Length -= count;
        _meta->CStr += count;
    }
}

void String::ClipMid(int from, int count)
{
    if (_meta && _meta->Length > 0)
    {
        count = count >= 0 ? count : _meta->Length - from;
        Math::ClampLength(0, _meta->Length, from, count);
        if (count > 0)
        {
            BecomeUnique();
            if (!from)
            {
                _meta->Length -= count;
                _meta->CStr += count;
            }
            else if (from + count == _meta->Length)
            {
                _meta->Length -= count;
                _meta->CStr[_meta->Length] = 0;
            }
            else
            {
                char *cstr_mid = _meta->CStr + from;
                memmove(cstr_mid, _meta->CStr + from + count, _meta->Length - from - count + 1);
                _meta->Length -= count;
            }
        }
    }
}

void String::ClipRight(int count)
{
    if (_meta && count > 0)
    {
        count = Math::Min(count, GetLength());
        BecomeUnique();
        _meta->Length -= count;
        _meta->CStr[_meta->Length] = 0;
    }
}

void String::ClipLeftSection(char separator, bool include_separator)
{
    if (_meta && separator)
    {
        int slice_at = FindChar(separator);
        if (slice_at >= 0)
        {
            ClipLeft(include_separator ? slice_at + 1 : slice_at);
        }
    }
}

void String::ClipRightSection(char separator, bool include_separator)
{
    if (_meta && separator)
    {
        int slice_at = FindCharReverse(separator);
        if (slice_at >= 0)
        {
            ClipRight(include_separator ? _meta->Length - slice_at : _meta->Length - slice_at - 1);
        }
    }
}

void String::ClipSection(char separator, int first, int last,
                              bool include_first_sep, bool include_last_sep)
{
    if (!_meta || !separator)
    {
        return;
    }

    int slice_from;
    int slice_to;
    if (FindSection(separator, first, last, !include_first_sep, !include_last_sep,
        slice_from, slice_to))
    {
        ClipMid(slice_from, slice_to - slice_from);
    }
}

void String::Empty()
{
    if (_meta)
    {
        BecomeUnique();
        _meta->Length = 0;
        _meta->CStr[0] = 0;
    }
}

void String::FillString(char c, int count)
{
    Empty();
    if (count > 0)
    {
        ReserveAndShift(false, count);
        memset(_meta->CStr, c, count);
        _meta->Length = count;
        _meta->CStr[count] = 0;
    }
}

void String::Format(const char *fcstr, ...)
{
    fcstr = fcstr ? fcstr : "";
    va_list argptr;
    va_start(argptr, fcstr);
    int length = vsnprintf(NULL, 0, fcstr, argptr);
    ReserveAndShift(false, Math::Max(0, length - GetLength()));
    va_start(argptr, fcstr);
    vsprintf(_meta->CStr, fcstr, argptr);
    _meta->Length = length;
    _meta->CStr[_meta->Length] = 0;
    va_end(argptr);
}

void String::Free()
{
    if (_meta)
    {
        assert(_meta->RefCount > 0);
        _meta->RefCount--;
        if (!_meta->RefCount)
        {
            delete [] _data;
        }
    }
    _data = NULL;
}

void String::MakeLower()
{
    if (_meta)
    {
        BecomeUnique();
        strlwr(_meta->CStr);
    }
}

void String::MakeUpper()
{
    if (_meta)
    {
        BecomeUnique();
        strupr(_meta->CStr);
    }
}

void String::Prepend(const char *cstr)
{
    if (cstr)
    {
        int length = strlen(cstr);
        if (length > 0)
        {
            ReserveAndShift(true, length);
            memcpy(_meta->CStr - length, cstr, length);
            _meta->Length += length;
            _meta->CStr -= length;
        }
    }
}

void String::PrependChar(char c)
{
    if (c)
    {
        ReserveAndShift(true, 1);
        _meta->Length++;
        _meta->CStr--;
        _meta->CStr[0] = c;
    }
}

void String::Replace(char what, char with)
{
    if (_meta && what && with && what != with)
    {
        BecomeUnique();
        char *rep_ptr = _meta->CStr;
        while (*rep_ptr)
        {
            if (*rep_ptr == what)
            {
                *rep_ptr = with;
            }
            rep_ptr++;
        }
    }
}

void String::SetAt(int index, char c)
{
    if (_meta && index >= 0 && index < GetLength() && c)
    {
        BecomeUnique();
        _meta->CStr[index] = c;
    }
}

void String::SetString(const char *cstr, int length)
{
    if (cstr)
    {
        length = length >= 0 ? Math::Min((size_t)length, strlen(cstr)) : strlen(cstr);
        if (length > 0)
        {
            ReserveAndShift(false, Math::Max(0, length - GetLength()));
            memcpy(_meta->CStr, cstr, length);
            _meta->Length = length;
            _meta->CStr[length] = 0;
        }
        else
        {
            Empty();
        }
    }
    else
    {
        Empty();
    }
}

void String::Trim(char c)
{
    TrimLeft(c);
    TrimRight(c);
}

void String::TrimLeft(char c)
{
    if (!_meta || !_meta->Length)
    {
        return;
    }

    const char *trim_ptr = _meta->CStr;
    while (*trim_ptr &&
        (c && *trim_ptr == c ||
        !c && (*trim_ptr == ' ' || *trim_ptr == '\t' || *trim_ptr == '\r' || *trim_ptr == '\n')))
    {
        trim_ptr++;
    }
    int trimmed = trim_ptr - _meta->CStr;
    if (trimmed > 0)
    {
        BecomeUnique();
        _meta->Length -= trimmed;
        _meta->CStr += trimmed;
    }
}

void String::TrimRight(char c)
{
    if (!_meta || !_meta->Length)
    {
        return;
    }

    const char *trim_ptr = _meta->CStr + _meta->Length - 1;
    while (trim_ptr >= _meta->CStr &&
        (c && *trim_ptr == c ||
        !c && (*trim_ptr == ' ' || *trim_ptr == '\t' || *trim_ptr == '\r' || *trim_ptr == '\n')))
    {
        trim_ptr--;
    }
    int trimmed = (_meta->CStr + _meta->Length - 1) - trim_ptr;
    if (trimmed > 0)
    {
        BecomeUnique();
        _meta->Length -= trimmed;
        _meta->CStr[_meta->Length] = 0;
    }
}

void String::TruncateToLeft(int count)
{
    if (_meta && count >= 0)
    {
        count = Math::Min(count, _meta->Length);
        if (count < _meta->Length)
        {
            BecomeUnique();
            _meta->Length = count;
            _meta->CStr[_meta->Length] = 0;
        }
    }
}

void String::TruncateToMid(int from, int count)
{
    if (_meta)
    {
        count = count >= 0 ? count : _meta->Length - from;
        Math::ClampLength(0, _meta->Length, from, count);
        if (from > 0 || count < _meta->Length)
        {
            BecomeUnique();
            _meta->Length = count;
            _meta->CStr += from;
            _meta->CStr[_meta->Length] = 0;
        }
    }
}

void String::TruncateToRight(int count)
{
    if (_meta && count >= 0)
    {
        count = Math::Min(count, GetLength());
        if (count < _meta->Length)
        {
            BecomeUnique();
            _meta->CStr += _meta->Length - count;
            _meta->Length = count;
        }
    }
}

void String::TruncateToLeftSection(char separator, bool exclude_separator)
{
    if (_meta && separator)
    {
        int slice_at = FindChar(separator);
        if (slice_at >= 0)
        {
            TruncateToLeft(exclude_separator ? slice_at : slice_at + 1);
        }
    }
}

void String::TruncateToRightSection(char separator, bool exclude_separator)
{
    if (_meta && separator)
    {
        int slice_at = FindCharReverse(separator);
        if (slice_at >= 0)
        {
            TruncateToRight(exclude_separator ? _meta->Length - slice_at - 1 : _meta->Length - slice_at);
        }
    }
}

void String::TruncateToSection(char separator, int first, int last,
                          bool exclude_first_sep, bool exclude_last_sep)
{
    if (!_meta || !separator)
    {
        return;
    }

    int slice_from;
    int slice_to;
    if (FindSection(separator, first, last, exclude_first_sep, exclude_last_sep,
        slice_from, slice_to))
    {
        TruncateToMid(slice_from, slice_to - slice_from);
    }
    else
    {
        Empty();
    }
}

String &String::operator=(const String& str)
{
    if (_data != str._data)
    {
        Free();
        if (str._data && str._meta->Length > 0)
        {
            _data = str._data;
            if (_meta)
            {
                _meta->RefCount++;
            }
        }
    }
    return *this;
}

String &String::operator=(const char *cstr)
{
    SetString(cstr);
    return *this;
}

void String::Create(int max_length)
{
    _data = new char[sizeof(String::Header) + max_length + 1];
    _meta->RefCount = 1;
    _meta->Capacity = max_length;
    _meta->Length = 0;
    _meta->CStr = _data + sizeof(String::Header);
    _meta->CStr[_meta->Length] = 0;
}

void String::Copy(int max_length, int offset)
{
    if (!_meta)
    {
        return;
    }

    char *new_data = new char[sizeof(String::Header) + max_length + 1];
    // remember, that _meta->CStr may point to any address in buffer
    char *cstr_head = new_data + sizeof(String::Header) + offset;
    memcpy(new_data, _data, sizeof(String::Header));
    int copy_length = Math::Min(_meta->Length, max_length);
    memcpy(cstr_head, _meta->CStr, copy_length);
    Free();
    _data = new_data;
    _meta->RefCount = 1;
    _meta->Capacity = max_length;
    _meta->Length = copy_length;
    _meta->CStr = cstr_head;
    _meta->CStr[_meta->Length] = 0;
}

void String::Align(int offset)
{
    char *cstr_head = _data + sizeof(String::Header) + offset;
    memmove(cstr_head, _meta->CStr, _meta->Length + 1);
    _meta->CStr = cstr_head;
}

void String::BecomeUnique()
{
    if (_meta && _meta->RefCount > 1)
    {
        Copy(_meta->Length);
    }
}

void String::ReserveAndShift(bool left, int more_length)
{
    if (_meta)
    {
        int total_length = _meta->Length + more_length;
        if (_meta->Capacity < total_length)
        {
            // grow by 50% or at least to total_size
            int grow_length = _meta->Capacity + (_meta->Capacity >> 1);
            Copy(Math::Max(total_length, grow_length), left ? more_length : 0);
        }
        else if (_meta->RefCount > 1)
        {
            Copy(total_length, left ? more_length : 0);
        }
        else
        {
            // make sure we make use of all of our space
            const char *cstr_head = _data + sizeof(String::Header);
            int free_space = left ?
                _meta->CStr - cstr_head :
                (cstr_head + _meta->Capacity) - (_meta->CStr + _meta->Length);
            if (free_space < more_length)
            {
                Align((left ?
                    _meta->CStr + (more_length - free_space) :
                    _meta->CStr - (more_length - free_space)) - cstr_head);
            }
        }
    }
    else
    {
        Create(more_length);
    }
}

} // namespace Common
} // namespace AGS
