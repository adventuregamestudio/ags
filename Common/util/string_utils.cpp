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
#include "util/string_utils.h"
#include <errno.h>
#include <regex>
#include <string.h>
#include "core/platform.h"
#include "util/math.h"
#include "util/stream.h"
#include "util/string_compat.h"
#include "util/utf8.h"

using namespace AGS::Common;

namespace AGS
{
namespace Common
{

String StrUtil::IntToString(int d)
{
    return String::FromFormat("%d", d);
}

int StrUtil::StringToInt(const String &s, int def_val)
{
    if (!s.GetCStr())
        return def_val;
    char *stop_ptr;
    int val = strtol(s.GetCStr(), &stop_ptr, 0);
    return (stop_ptr == s.GetCStr() + s.GetLength()) ? val : def_val;
}

StrUtil::ConversionError StrUtil::StringToInt(const String &s, int &val, int def_val)
{
    val = def_val;
    if (!s.GetCStr())
        return StrUtil::kFailed;
    char *stop_ptr;
    errno = 0;
    long lval = strtol(s.GetCStr(), &stop_ptr, 0);
    if (stop_ptr != s.GetCStr() + s.GetLength())
        return StrUtil::kFailed;
    if (lval > INT_MAX || lval < INT_MIN || errno == ERANGE)
        return StrUtil::kOutOfRange;
    val = static_cast<int>(lval);
    return StrUtil::kNoError;
}

float StrUtil::StringToFloat(const String &s, float def_val)
{
    if (!s.GetCStr())
        return def_val;
    char *stop_ptr;
    float val = strtof(s.GetCStr(), &stop_ptr);
    return (stop_ptr == s.GetCStr() + s.GetLength()) ? val : def_val;
}

String StrUtil::Unescape(const String &s)
{
    size_t at = s.FindChar('\\');
    if (at == String::NoIndex)
        return s; // no unescaping necessary, return original string
    char *buf = new char[s.GetLength()];
    strncpy(buf, s.GetCStr(), at);
    char *pb = buf + at;
    for (const char *ptr = s.GetCStr() + at; *ptr; ++ptr)
    {
        if (*ptr != '\\')
        {
            *(pb++) = *ptr;
            continue;
        }

        char next = *(++ptr);
        switch (next)
        {
        case 'a':  *(pb++) = '\a'; break;
        case 'b':  *(pb++) = '\b'; break;
        case 'f':  *(pb++) = '\f'; break;
        case 'n':  *(pb++) = '\n'; break;
        case 'r':  *(pb++) = '\r'; break;
        case 't':  *(pb++) = '\t'; break;
        case 'v':  *(pb++) = '\v'; break;
        case '\\': *(pb++) = '\\'; break;
        case '\'': *(pb++) = '\''; break;
        case '\"': *(pb++) = '\"'; break;
        case '\?': *(pb++) = '\?'; break;
        default: *(pb++) = next; break;
        }
    }
    *pb = 0;
    String dst(buf);
    delete [] buf;
    return dst;
}

String StrUtil::WildcardToRegex(const String &wildcard)
{
    // https://stackoverflow.com/questions/40195412/c11-regex-search-for-exact-string-escape
    // matches any characters that need to be escaped in RegEx
    std::regex esc{ R"([-[\]{}()*+?.,\^$|#\s])" };
    std::string sanitized = std::regex_replace(wildcard.GetCStr(), esc, R"(\$&)");
    // convert (now escaped) wildcard "\\*" and "\\?" into ".*" and "." respectively
    String pattern(sanitized.c_str());
    pattern.Replace("\\*", ".*");
    pattern.Replace("\\?", ".");
    return pattern;
}

String StrUtil::ReadString(Stream *in)
{
    size_t len = in->ReadInt32();
    if (len > 0)
        return String::FromStreamCount(in, len);
    return String();
}

void StrUtil::ReadString(char *cstr, Stream *in, size_t buf_limit)
{
    size_t len = in->ReadInt32();
    if (buf_limit == 0)
    {
        in->Seek(len);
        return;
    }

    len = std::min(len, buf_limit - 1);
    if (len > 0)
        in->Read(cstr, len);
    cstr[len] = 0;
}

void StrUtil::ReadString(String &s, Stream *in)
{
    size_t len = in->ReadInt32();
    s.ReadCount(in, len);
}

void StrUtil::ReadString(char **cstr, Stream *in)
{
    size_t len = in->ReadInt32();
    *cstr = new char[len + 1];
    if (len > 0)
        in->Read(*cstr, len);
    (*cstr)[len] = 0;
}

void StrUtil::SkipString(Stream *in)
{
    size_t len = in->ReadInt32();
    in->Seek(len);
}

void StrUtil::WriteString(const String &s, Stream *out)
{
    size_t len = s.GetLength();
    out->WriteInt32(len);
    if (len > 0)
        out->Write(s.GetCStr(), len);
}

void StrUtil::WriteString(const char *cstr, Stream *out)
{
    size_t len = strlen(cstr);
    out->WriteInt32(len);
    if (len > 0)
        out->Write(cstr, len);
}

void StrUtil::WriteString(const char *cstr, size_t len, Stream *out)
{
    out->WriteInt32(len);
    if (len > 0)
        out->Write(cstr, len);
}

void StrUtil::ReadCStr(char *buf, Stream *in, size_t buf_limit)
{
    if (buf_limit == 0)
    {
        while (in->ReadByte() > 0);
        return;
    }

    auto ptr = buf;
    auto last = buf + buf_limit - 1;
    for (;;)
    {
        if (ptr >= last) {
            *ptr = 0;
            while (in->ReadByte() > 0); // must still read until 0
            break;
        }

        auto ichar = in->ReadByte();
        if (ichar <= 0) {
            *ptr = 0;
            break;
        }
        *ptr = static_cast<char>(ichar);
        ptr++;
    }
}

void StrUtil::ReadCStrCount(char *buf, Stream *in, size_t count)
{
    in->Read(buf, count);
    buf[count - 1] = 0; // for safety
}

char *StrUtil::ReadMallocCStrOrNull(Stream *in)
{
    char buf[1024];
    for (auto ptr = buf; (ptr < buf + sizeof(buf)); ++ptr)
    {
        auto ichar = in->ReadByte();
        if (ichar <= 0)
        {
            *ptr = 0;
            break;
        }
        *ptr = static_cast<char>(ichar);
    }
    return buf[0] != 0 ? ags_strdup(buf) : nullptr;
}

void StrUtil::SkipCStr(Stream *in)
{
    while (in->ReadByte() > 0);
}

void StrUtil::WriteCStr(const char *cstr, Stream *out)
{
    if (cstr)
        out->Write(cstr, strlen(cstr) + 1);
    else
        out->WriteByte(0);
}

void StrUtil::WriteCStr(const String &s, Stream *out)
{
    out->Write(s.GetCStr(), s.GetLength() + 1);
}

void StrUtil::ReadStringMap(StringMap &map, Stream *in)
{
    size_t count = in->ReadInt32();
    for (size_t i = 0; i < count; ++i)
    {
        String key = StrUtil::ReadString(in);
        String value = StrUtil::ReadString(in);
        map.insert(std::make_pair(key, value));
    }
}

void StrUtil::WriteStringMap(const StringMap &map, Stream *out)
{
    out->WriteInt32(map.size());
    for (const auto &kv : map)
    {
        StrUtil::WriteString(kv.first, out);
        StrUtil::WriteString(kv.second, out);
    }
}

size_t StrUtil::ConvertUtf8ToAscii(const char *mbstr, const char *loc_name, char *out_cstr, size_t out_sz)
{
    // TODO: later consider using alternative conversion methods
    // (e.g. see C++11 features), as setlocale is unreliable.
    if (setlocale(LC_CTYPE, loc_name) == nullptr)
    { // If failed setlocale, then resort to plain copy the mb string
        return static_cast<size_t>(snprintf(out_cstr, out_sz, "%s", mbstr));
    }
    // First convert utf-8 string into widestring;
    std::vector<wchar_t> wcsbuf; // widechar buffer
    wcsbuf.resize(Utf8::GetLength(mbstr) + 1);
    // NOTE: we don't use mbstowcs, because unfortunately ".utf-8" locale
    // is not normally supported on all systems (e.g. Windows 7 and earlier)
    for (size_t at = 0, chr_sz = 0; *mbstr; mbstr += chr_sz, ++at)
    {
        Utf8::Rune r;
        chr_sz = Utf8::GetChar(mbstr, Utf8::UtfSz, &r);
        wcsbuf[at] = static_cast<wchar_t>(r);
    }
    // Then convert widestring to single-byte string using specified locale
    size_t res_sz = wcstombs(out_cstr, &wcsbuf[0], out_sz);
    setlocale(LC_CTYPE, "");
    return res_sz;
}

size_t StrUtil::ConvertUtf8ToWstr(const char *mbstr, wchar_t *out_wcstr, size_t out_sz)
{
    size_t len = 0;
    for (size_t mb_sz = 1; *mbstr && (mb_sz > 0) && (len < out_sz);
        mbstr += mb_sz, ++out_wcstr, ++len)
    {
        Utf8::Rune r;
        mb_sz = Utf8::GetChar(mbstr, Utf8::UtfSz, &r);
        *out_wcstr = static_cast<wchar_t>(r);
    }
    *out_wcstr = 0;
    return len;
}

size_t StrUtil::ConvertWstrToUtf8(const wchar_t *wcstr, char *out_mbstr, size_t out_sz)
{
    size_t len = 0;
    for (size_t mb_sz = 1; *wcstr && (mb_sz > 0) && (len + mb_sz < out_sz);
        ++wcstr, out_mbstr += mb_sz, len += mb_sz)
    {
        mb_sz = Utf8::SetChar(*wcstr, out_mbstr, out_sz - len);
    }
    *out_mbstr = 0;
    return len;
}

} // namespace Common
} // namespace AGS
