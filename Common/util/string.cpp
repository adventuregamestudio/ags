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

#include <algorithm>
#include <stdexcept>
#include <stdio.h> // sprintf
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util/math.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_utils.h" // stricmp

namespace AGS
{
namespace Common
{

void String::Read(Stream *in, size_t max_chars, bool stop_at_limit)
{
    __data.clear();

    if (!in) { return; }

    for(;;) {
        auto ichar = in->ReadByte();
        auto ch = (char)(ichar >= 0 ? ichar : 0);

        if (!ch) { break; }

        if (__data.length() < max_chars) {
            __data += ch;
        } else {
            if (stop_at_limit) { break; }
        }
    } 
}

void String::ReadCount(Stream *in, size_t count)
{
    __data.clear();

    if (!in) { return; }
    if (count <= 0) { return; }

    std::vector<char> buf(count+1, '\0');
    count = in->Read(&buf[0], count);
    __data.assign(&buf[0]);
}

void String::Write(Stream *out) const
{
    if (!out) { return; }

    out->Write(GetCStr(), GetLength() + 1);
}

int String::Compare(const String& other) const
{
    return __data.compare(other.__data);
}

int String::CompareNoCase(const String& other) const
{
    return stricmp(GetCStr(), other.GetCStr());
}

bool String::StartsWith(const String& other) const
{
    auto lhs_len = std::min(this->__data.length(), other.__data.length());
    auto lhs = this->__data.substr(0, lhs_len);
    auto rhs = other.__data;
    return lhs == rhs;
}

bool String::StartsWithNoCase(const String& other) const
{
    auto lhs_len = std::min(this->__data.length(), other.__data.length());
    auto lhs = this->__data.substr(0, lhs_len);
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);

    auto rhs = other.__data;
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);

    return lhs == rhs;
}

bool String::EndsWithNoCase(const String& other) const
{
    auto lhs_len = std::min(this->__data.length(), other.__data.length());
    auto lhs = this->__data.substr(this->__data.length()-lhs_len, lhs_len);
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);

    auto rhs = other.__data;
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);

    return lhs == rhs;
}

size_t String::FindChar(char c, size_t from) const
{
    if (!c) { return -1; }
    auto result = __data.find(c, from);
    return result != std::string::npos ? result : -1;
}

size_t String::FindCharReverse(char c, size_t from) const
{
    if (!c) { return -1; }
    if (from == -1) {
        from = std::string::npos;
    }
    auto result = __data.rfind(c, from);
    return result != std::string::npos ? result : -1;
}

int String::ToInt() const
{
    try {
        return std::stoi(__data);
    } catch (std::invalid_argument &e) {
        return 0;
    }
}

/* static */ String String::FromFormat(const char *fcstr, ...)
{
    fcstr = fcstr ? fcstr : "";
    va_list argptr;
    va_start(argptr, fcstr);
    auto result = String::FromFormatV(fcstr, argptr);
    va_end(argptr);
    return result;
}

/* static */ String String::FromFormatV(const char *fcstr, va_list argptr)
{
    fcstr = fcstr ? fcstr : "";

    va_list argptr_cpy;
    va_copy(argptr_cpy, argptr);
    size_t length = vsnprintf(NULL, 0u, fcstr, argptr);

    auto data = std::string();
    data.resize(length);
    vsprintf(&data[0], fcstr, argptr_cpy);
    va_end(argptr_cpy);
    data.resize(length);

    return String(data);
}

/* static */ String String::FromStream(Stream *in, size_t max_chars, bool stop_at_limit)
{
    auto str = String();
    str.Read(in, max_chars, stop_at_limit);
    return str;
}

/* static */ String String::FromStreamCount(Stream *in, size_t count)
{
    auto str = String();
    str.ReadCount(in, count);
    return str;
}

String String::Lower() const
{
    auto str = *this;
    str.MakeLower();
    return str;
}

String String::Left(size_t count) const
{
    return String(__data.substr(0, count));
}

String String::Mid(size_t from, size_t count) const
{
    if (count <= 0) { return String(); }
    if (from >= __data.length()) { return String(); }
    return String(__data.substr(from, count));
}

std::vector<String> String::Split(const String& delims, int max_splits) const
{
    auto result = std::vector<String>();
    auto remaining = __data;
    auto remaining_splits = max_splits < 0 ? __data.length() : max_splits;

    while (remaining_splits > 0) {
        
        auto p = std::find_if(remaining.begin(), remaining.end(), 
            [&delims](int ch) {
                return delims.__data.find(ch) != std::string::npos;
        });

        if (p == remaining.end()) { break; }

        auto offset = p-remaining.begin();

        auto elem = remaining.substr(0, offset);
        result.push_back(String(elem));
        
        remaining = remaining.substr(offset+1);
        
        remaining_splits -= 1;
    }
    
    result.push_back(String(remaining));
    
    return result;
}

void String::ClipLeft(size_t count)
{
    __data = __data.substr(count, __data.length()-count);
}

void String::ClipMid(size_t from, size_t count)
{
    __data.erase(from, count);
}

void String::ClipRight(size_t count)
{
    if (count <= 0) { return; }
    __data = __data.substr(0, __data.length() - count);
}

void String::Format(const char *fcstr, ...)
{
    fcstr = fcstr ? fcstr : "";

    va_list argptr;
    va_start(argptr, fcstr);

    *this = String::FromFormatV(fcstr, argptr);

    va_end(argptr);
}

void String::Free()
{
    __data.clear();
    __data.resize(0);
    __data.shrink_to_fit();
 }

void String::MakeLower()
{
    std::transform (__data.begin(), __data.end(), __data.begin(), tolower);
}

void String::Prepend(const String& other)
{
    __data.insert(0, other.__data);
}

void String::PrependChar(char c)
{
    if (!c) { return; }
    __data.insert(0, 1, c);
}

void String::Replace(char what, char with)
{
    if (!what) { return; }
    if (!with) { return; }
    if (what == with) { return; }
    std::replace(__data.begin(), __data.end(), what, with);
}

void String::ReplaceMid(size_t from, size_t count, const String& other)
{
    __data.replace(from, count, other.__data);
}

void String::SetAt(size_t index, char c)
{
    if (!c) { return; }
    if (index >= GetLength()) { return; }

    __data[index] = c;
}

void String::SetString(const String& other, size_t length)
{
    length = Math::Min(length, other.__data.length());

    __data = other.__data;
    __data.resize(length);
}

void String::TrimRight(char c)
{
    // if c is 0, then trim spaces.

    __data.erase(std::find_if(__data.rbegin(), __data.rend(), [&c](int ch) {
        if (c) {
            return ch != c;
        }
        return !isspace(ch);
    }).base(), __data.end());
}

void String::TruncateToLeft(size_t count)
{
    __data.resize(count);    
}

} // namespace Common
} // namespace AGS