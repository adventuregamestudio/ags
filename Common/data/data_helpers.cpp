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
#include "data/data_helpers.h"
#include <vector>

namespace AGS
{
namespace Common
{

String PreprocessLineForOldStyleLinebreaks(const String &line)
{
    const size_t first_at = line.FindChar('\\');
    if (first_at == String::NoIndex)
        return line; // no escaping chars, nothing to do

    std::vector<char> out;
    const char *begin_ptr = line.GetCStr();
    const char *end_ptr = line.GetCStr() + line.GetLength();
    char last_char = 0;
    for (const char *src_ptr = line.GetCStr() + first_at; src_ptr < end_ptr; ++src_ptr)
    {
        if (last_char == '\\')
        {
            // Copy accumulated portion
            const size_t old_size = out.size();
            out.resize(out.size() + (src_ptr - begin_ptr));
            std::copy(begin_ptr, src_ptr, out.begin() + old_size);
            if (*src_ptr == '[')
            {
                // add double escaping of '['
                out.push_back('\\');
                out.push_back('[');
            }
            else
            {
                out.push_back(*src_ptr);
            }
            last_char = 0; // eat escaping char
            begin_ptr = src_ptr + 1;
        }
        else
        {
            last_char = *src_ptr;
        }
    }
    // Copy final accumulated portion (if any)
    const size_t old_size = out.size();
    out.resize(out.size() + (end_ptr - begin_ptr));
    std::copy(begin_ptr, end_ptr, out.begin() + old_size);
    return String(out.data(), out.size());
}

const char *EncryptPassword = "Avis Durgan";

void DecryptText(char *buf, size_t buf_sz)
{
    for (size_t i = 0, psw_i = 0; i < buf_sz; ++i, ++psw_i)
    {
        if (psw_i > 10)
            psw_i = 0;

        buf[i] -= EncryptPassword[psw_i];
        if (buf[i] == 0)
            break;
    }
}

void ReadStringDecrypt(Stream *in, char *buf, size_t buf_sz)
{
    size_t len = in->ReadInt32();
    size_t slen = std::min(buf_sz - 1, len);
    in->Read(buf, slen);
    if (len > slen)
        in->Seek(len - slen);
    DecryptText(buf, slen);
    buf[slen] = 0;
}

String ReadStringDecrypt(Stream *in)
{
    std::vector<char> dec_buf;
    return ReadStringDecrypt(in, dec_buf);
}

String ReadStringDecrypt(Stream *in, std::vector<char> &dec_buf)
{
    size_t len = in->ReadInt32();
    dec_buf.resize(len + 1);
    in->Read(dec_buf.data(), len);
    DecryptText(dec_buf.data(), len);
    dec_buf.back() = 0; // null terminate in case read string does not have one
    return String(dec_buf.data());
}

void EncryptText(char *buf, size_t buf_sz)
{
    for (size_t i = 0, psw_i = 0; i < buf_sz; ++i, ++psw_i)
    {
        if (psw_i > 10)
            psw_i = 0;

        char src_c = buf[i];
        buf[i] += EncryptPassword[psw_i];
        if (src_c == 0)
            break;
    }
}

const char *EncryptText(std::vector<char> &en_buf, const String &s)
{
    en_buf.resize(s.GetLength() + 1);
    std::copy(s.GetCStr(), s.GetCStr() + s.GetLength() + 1, en_buf.data());
    EncryptText(en_buf.data(), en_buf.size());
    return en_buf.data();
}

const char *EncryptEmptyString(std::vector<char> &en_buf)
{
    en_buf.resize(1);
    en_buf[0] = 0;
    EncryptText(en_buf.data(), en_buf.size());
    return en_buf.data();
}

void WriteStringEncrypt(Stream *out, const char *s)
{
    size_t len = strlen(s) + 1;
    out->WriteInt32(len);
    std::vector<char> buf(len);
    std::copy(s, s + len, buf.data());
    EncryptText(buf.data(), buf.size());
    out->Write(buf.data(), len);
}

} // namespace Common
} // namespace AGS