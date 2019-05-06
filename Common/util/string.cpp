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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util/math.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_utils.h" // ags_stricmp

namespace AGS
{
namespace Common
{

String::Header::Header()
    : RefCount(0)
    , Capacity(0)
    , Length(0)
    , CStr(nullptr)
{
}

String::String()
    : _data(nullptr)
{
}

String::String(const String &str)
    : _data(nullptr)
{
    *this = str;
}

String::String(const char *cstr)
    :_data(nullptr)
{
    *this = cstr;
}

String::String(const char *cstr, size_t length)
    : _data(nullptr)
{
    SetString(cstr, length);
}

String::String(char c, size_t count)
    : _data(nullptr)
{
    FillString(c, count);
}

String::~String()
{
    Free();
}

void String::Read(std::shared_ptr<AGS::Common::Stream> in, size_t max_chars, bool stop_at_limit)
{
    Empty();
    if (!in)
    {
        return;
    }
    if (max_chars == 0 && stop_at_limit)
    {
        return;
    }

    char buffer[1024];
    char *read_ptr = buffer;
    size_t read_size = 0;
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
        if (!*read_ptr || ((read_ptr - buffer) == (sizeof(buffer) - 1 - 1)))
        {
            buffer[sizeof(buffer) - 1] = 0;
            Append(buffer);
            read_ptr = buffer;
        }
        else
        {
            read_ptr++;
        }
    }
    while(ichar > 0 && !(stop_at_limit && read_size == max_chars));
}

void String::ReadCount(std::shared_ptr<AGS::Common::Stream> in, size_t count)
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

void String::Write(std::shared_ptr<AGS::Common::Stream> out) const
{
    if (out)
    {
        out->Write(GetCStr(), GetLength() + 1);
    }
}

void String::WriteCount(std::shared_ptr<AGS::Common::Stream> out, size_t count) const
{
    if (out)
    {
        size_t str_out_len = Math::Min(count - 1, GetLength());
        if (str_out_len > 0)
            out->Write(GetCStr(), str_out_len);
        size_t null_out_len = count - str_out_len;
        if (null_out_len > 0)
            out->WriteByteCount(0, null_out_len);
    }
}

/* static */ void String::WriteString(const char *cstr, std::shared_ptr<AGS::Common::Stream> out)
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
    return ags_stricmp(GetCStr(), cstr ? cstr : "");
}

int String::CompareLeft(const char *cstr, size_t count) const
{
    cstr = cstr ? cstr : "";
    return strncmp(GetCStr(), cstr, count != -1 ? count : strlen(cstr));
}

int String::CompareLeftNoCase(const char *cstr, size_t count) const
{
    cstr = cstr ? cstr : "";
    return ags_strnicmp(GetCStr(), cstr, count != -1 ? count : strlen(cstr));
}

int String::CompareMid(const char *cstr, size_t from, size_t count) const
{
    cstr = cstr ? cstr : "";
    from = Math::Min(from, GetLength());
    return strncmp(GetCStr() + from, cstr, count != -1 ? count : strlen(cstr));
}

int String::CompareMidNoCase(const char *cstr, size_t from, size_t count) const
{
    cstr = cstr ? cstr : "";
    from = Math::Min(from, GetLength());
    return ags_strnicmp(GetCStr() + from, cstr, count != -1 ? count : strlen(cstr));
}

int String::CompareRight(const char *cstr, size_t count) const
{
    cstr = cstr ? cstr : "";
    count = count != -1 ? count : strlen(cstr);
    size_t off = Math::Min(GetLength(), count);
    return strncmp(GetCStr() + GetLength() - off, cstr, count);
}

int String::CompareRightNoCase(const char *cstr, size_t count) const
{
    cstr = cstr ? cstr : "";
    count = count != -1 ? count : strlen(cstr);
    size_t off = Math::Min(GetLength(), count);
    return ags_strnicmp(GetCStr() + GetLength() - off, cstr, count);
}

size_t String::FindChar(char c, size_t from) const
{
    if (_meta && c && from < _meta->Length)
    {
        const char * found_cstr = strchr(_meta->CStr + from, c);
        return found_cstr ? found_cstr - _meta->CStr : -1;
    }
    return -1;
}

size_t String::FindCharReverse(char c, size_t from) const
{
    if (!_meta || !c)
    {
        return -1;
    }

    from = Math::Min(from, _meta->Length - 1);
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

size_t String::FindString(const char *cstr, size_t from) const
{
    if (_meta && cstr && from < _meta->Length)
    {
        const char * found_cstr = strstr(_meta->CStr + from, cstr);
        return found_cstr ? found_cstr - _meta->CStr : -1;
    }
    return -1;
}

bool String::FindSection(char separator, size_t first, size_t last, bool exclude_first_sep, bool exclude_last_sep,
                        size_t &from, size_t &to) const
{
    if (!_meta || !separator)
    {
        return false;
    }
    if (first > last)
    {
        return false;
    }

    size_t this_field = 0;
    size_t slice_from = 0;
    size_t slice_to = _meta->Length;
    size_t slice_at = -1;
    do
    {
        slice_at = FindChar(separator, slice_at + 1);
        if (slice_at == -1)
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
        assert(slice_from <= slice_to);
        from = Math::Clamp(slice_from, (size_t)0, _meta->Length);
        to   = Math::Clamp(slice_to, (size_t)0, _meta->Length);
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
    str.FormatV(fcstr, argptr);
    va_end(argptr);
    return str;
}

/* static */ String String::FromFormatV(const char *fcstr, va_list argptr)
{
    String str;
    str.FormatV(fcstr, argptr);
    return str;
}

/* static */ String String::FromStream(std::shared_ptr<AGS::Common::Stream> in, size_t max_chars, bool stop_at_limit)
{
    String str;
    str.Read(in, max_chars, stop_at_limit);
    return str;
}

/* static */ String String::FromStreamCount(std::shared_ptr<AGS::Common::Stream> in, size_t count)
{
    String str;
    str.ReadCount(in, count);
    return str;
}

String String::Lower() const
{
    String str = *this;
    str.MakeLower();
    return str;
}

String String::Upper() const
{
    String str = *this;
    str.MakeUpper();
    return str;
}

String String::Left(size_t count) const
{
    count = Math::Min(count, GetLength());
    return count == GetLength() ? *this : String(GetCStr(), count);
}

String String::Mid(size_t from, size_t count) const
{
    Math::ClampLength(from, count, (size_t)0, GetLength());
    return count == GetLength() ? *this : String(GetCStr() + from, count);
}

String String::Right(size_t count) const
{
    count = Math::Min(count, GetLength());
    return count == GetLength() ? *this : String(GetCStr() + GetLength() - count, count);
}

String String::LeftSection(char separator, bool exclude_separator) const
{
    if (_meta && separator)
    {
        size_t slice_at = FindChar(separator);
        if (slice_at != -1)
        {
            slice_at = exclude_separator ? slice_at : slice_at + 1;
            return Left(slice_at);
        }
    }
    return *this;
}

String String::RightSection(char separator, bool exclude_separator) const
{
    if (_meta && separator)
    {
        size_t slice_at = FindCharReverse(separator);
        if (slice_at != -1)
        {
            size_t count = exclude_separator ? _meta->Length - slice_at - 1 : _meta->Length - slice_at;
            return Right(count);
        }
    }
    return *this;
}

String String::Section(char separator, size_t first, size_t last,
                          bool exclude_first_sep, bool exclude_last_sep) const
{
    if (!_meta || !separator)
    {
        return String();
    }

    size_t slice_from;
    size_t slice_to;
    if (FindSection(separator, first, last, exclude_first_sep, exclude_last_sep,
        slice_from, slice_to))
    {
        return Mid(slice_from, slice_to - slice_from);
    }
    return String();
}

void String::Reserve(size_t max_length)
{
    if (_meta)
    {
        if (max_length > _meta->Capacity)
        {
            // grow by 50%
            size_t grow_length = _meta->Capacity + (_meta->Capacity / 2);
            Copy(Math::Max(max_length, grow_length));
        }
    }
    else
    {
        Create(max_length);
    }
}

void String::ReserveMore(size_t more_length)
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
        size_t length = strlen(cstr);
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

void String::ClipLeft(size_t count)
{
    if (_meta && _meta->Length > 0 && count > 0)
    {
        count = Math::Min(count, _meta->Length);
        BecomeUnique();
        _meta->Length -= count;
        _meta->CStr += count;
    }
}

void String::ClipMid(size_t from, size_t count)
{
    if (_meta && from < _meta->Length)
    {
        count = Math::Min(count, _meta->Length - from);
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

void String::ClipRight(size_t count)
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
        size_t slice_at = FindChar(separator);
        if (slice_at != -1)
        {
            ClipLeft(include_separator ? slice_at + 1 : slice_at);
        }
        else
            Empty();
    }
}

void String::ClipRightSection(char separator, bool include_separator)
{
    if (_meta && separator)
    {
        size_t slice_at = FindCharReverse(separator);
        if (slice_at != -1)
        {
            ClipRight(include_separator ? _meta->Length - slice_at : _meta->Length - slice_at - 1);
        }
        else
            Empty();
    }
}

void String::ClipSection(char separator, size_t first, size_t last,
                              bool include_first_sep, bool include_last_sep)
{
    if (!_meta || !separator)
    {
        return;
    }

    size_t slice_from;
    size_t slice_to;
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

void String::FillString(char c, size_t count)
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
    va_list argptr;
    va_start(argptr, fcstr);
    FormatV(fcstr, argptr);
    va_end(argptr);
}

void String::FormatV(const char *fcstr, va_list argptr)
{
    fcstr = fcstr ? fcstr : "";
    va_list argptr_cpy;
    va_copy(argptr_cpy, argptr);
    size_t length = vsnprintf(nullptr, 0u, fcstr, argptr);
    ReserveAndShift(false, Math::Surplus(length, GetLength()));
    vsprintf(_meta->CStr, fcstr, argptr_cpy);
    va_end(argptr_cpy);
    _meta->Length = length;
    _meta->CStr[_meta->Length] = 0;
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
    _data = nullptr;
}

void String::MakeLower()
{
    if (_meta)
    {
        BecomeUnique();
        ags_strlwr(_meta->CStr);
    }
}

void String::MakeUpper()
{
    if (_meta)
    {
        BecomeUnique();
        ags_strupr(_meta->CStr);
    }
}

void String::Prepend(const char *cstr)
{
    if (cstr)
    {
        size_t length = strlen(cstr);
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

void String::ReplaceMid(size_t from, size_t count, const char *cstr)
{
    if (!cstr)
        cstr = "";
    size_t length = strlen(cstr);
    Math::ClampLength(from, count, (size_t)0, GetLength());
    ReserveAndShift(false, Math::Surplus(length, count));
    memmove(_meta->CStr + from + length, _meta->CStr + from + count, GetLength() - (from + count) + 1);
    memcpy(_meta->CStr + from, cstr, length);
    _meta->Length += length - count;
}

void String::SetAt(size_t index, char c)
{
    if (_meta && index < GetLength() && c)
    {
        BecomeUnique();
        _meta->CStr[index] = c;
    }
}

void String::SetString(const char *cstr, size_t length)
{
    if (cstr)
    {
        length = Math::Min(length, strlen(cstr));
        if (length > 0)
        {
            ReserveAndShift(false, Math::Surplus(length, GetLength()));
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
    for (;;)
    {
        auto t = *trim_ptr;
        if (t == 0) { break; }
        if (c && t != c) { break; }
        if (!c && !isspace(t)) { break; }
        trim_ptr++;
    }
    size_t trimmed = trim_ptr - _meta->CStr;
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
    for (;;) 
    {
        if (trim_ptr < _meta->CStr) { break; }
        auto t = *trim_ptr;
        if (c && t != c) { break; }
        if (!c && !isspace(t)) { break; }
        trim_ptr--;
    }
    size_t trimmed = (_meta->CStr + _meta->Length - 1) - trim_ptr;
    if (trimmed > 0)
    {
        BecomeUnique();
        _meta->Length -= trimmed;
        _meta->CStr[_meta->Length] = 0;
    }
}

void String::TruncateToLeft(size_t count)
{
    if (_meta)
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

void String::TruncateToMid(size_t from, size_t count)
{
    if (_meta)
    {
        Math::ClampLength(from, count, (size_t)0, _meta->Length);
        if (from > 0 || count < _meta->Length)
        {
            BecomeUnique();
            _meta->Length = count;
            _meta->CStr += from;
            _meta->CStr[_meta->Length] = 0;
        }
    }
}

void String::TruncateToRight(size_t count)
{
    if (_meta)
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
        size_t slice_at = FindChar(separator);
        if (slice_at != -1)
        {
            TruncateToLeft(exclude_separator ? slice_at : slice_at + 1);
        }
    }
}

void String::TruncateToRightSection(char separator, bool exclude_separator)
{
    if (_meta && separator)
    {
        size_t slice_at = FindCharReverse(separator);
        if (slice_at != -1)
        {
            TruncateToRight(exclude_separator ? _meta->Length - slice_at - 1 : _meta->Length - slice_at);
        }
    }
}

void String::TruncateToSection(char separator, size_t first, size_t last,
                          bool exclude_first_sep, bool exclude_last_sep)
{
    if (!_meta || !separator)
    {
        return;
    }

    size_t slice_from;
    size_t slice_to;
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

void String::Create(size_t max_length)
{
    _data = new char[sizeof(String::Header) + max_length + 1];
    _meta->RefCount = 1;
    _meta->Capacity = max_length;
    _meta->Length = 0;
    _meta->CStr = _data + sizeof(String::Header);
    _meta->CStr[_meta->Length] = 0;
}

void String::Copy(size_t max_length, size_t offset)
{
    if (!_meta)
    {
        return;
    }

    char *new_data = new char[sizeof(String::Header) + max_length + 1];
    // remember, that _meta->CStr may point to any address in buffer
    char *cstr_head = new_data + sizeof(String::Header) + offset;
    memcpy(new_data, _data, sizeof(String::Header));
    size_t copy_length = Math::Min(_meta->Length, max_length);
    memcpy(cstr_head, _meta->CStr, copy_length);
    Free();
    _data = new_data;
    _meta->RefCount = 1;
    _meta->Capacity = max_length;
    _meta->Length = copy_length;
    _meta->CStr = cstr_head;
    _meta->CStr[_meta->Length] = 0;
}

void String::Align(size_t offset)
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

void String::ReserveAndShift(bool left, size_t more_length)
{
    if (_meta)
    {
        size_t total_length = _meta->Length + more_length;
        if (_meta->Capacity < total_length)
        {
            // grow by 50% or at least to total_size
            size_t grow_length = _meta->Capacity + (_meta->Capacity >> 1);
            Copy(Math::Max(total_length, grow_length), left ? more_length : 0u);
        }
        else if (_meta->RefCount > 1)
        {
            Copy(total_length, left ? more_length : 0u);
        }
        else
        {
            // make sure we make use of all of our space
            const char *cstr_head = _data + sizeof(String::Header);
            size_t free_space = left ?
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
