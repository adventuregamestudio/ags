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
#include "ac/wordsdictionary.h"
#include <algorithm>
#include <map>
#include <string.h>
#include "util/stream.h"
#include "util/string_compat.h"

namespace AGS
{
namespace Common
{

uint16_t WordsDictionary::FindWord(const String &word) const
{
    const auto it_found = _words.find(word);
    if (it_found != _words.end())
        return it_found->second;
    return WordsDictionary::INVALID;
}

void WordsDictionary::ReadFromFile(Stream* in)
{
    uint32_t word_count = static_cast<uint32_t>(in->ReadInt32());
    for (uint32_t i = 0; i < word_count; ++i)
    {
        String word = read_string_decrypt(in);
        uint16_t word_id = static_cast<uint16_t>(in->ReadInt16());
        _words[word] = word_id;
    }
}

void WordsDictionary::WriteToFile(Stream* out) const
{
    out->WriteInt32(_words.size());
    for (auto item : _words)
    {
        write_string_encrypt(out, item.first.GetCStr());
        out->WriteInt16(item.second);
    }
}

} // Common
} // AGS

using namespace AGS::Common;

const char *passwencstring = "Avis Durgan";

void decrypt_text(char *buf, size_t buf_sz)
{
    int adx = 0;
    const char *p_end = buf + buf_sz;

    while (buf < p_end)
    {
        *buf -= passwencstring[adx];
        if (*buf == 0)
            break;

        adx++;
        buf++;

        if (adx > 10)
            adx = 0;
    }
}

void read_string_decrypt(Stream *in, char *buf, size_t buf_sz)
{
    size_t len = in->ReadInt32();
    size_t slen = std::min(buf_sz - 1, len);
    in->Read(buf, slen);
    if (len > slen)
        in->Seek(len - slen);
    decrypt_text(buf, slen);
    buf[slen] = 0;
}

String read_string_decrypt(Stream *in)
{
    std::vector<char> dec_buf;
    return read_string_decrypt(in, dec_buf);
}

String read_string_decrypt(Stream *in, std::vector<char> &dec_buf)
{
    size_t len = in->ReadInt32();
    dec_buf.resize(len + 1);
    in->Read(dec_buf.data(), len);
    decrypt_text(dec_buf.data(), len);
    dec_buf.back() = 0; // null terminate in case read string does not have one
    return String(dec_buf.data());
}

void skip_string_decrypt(Stream *in)
{
    size_t len = in->ReadInt32();
    in->Seek(len);
}

void encrypt_text(char *toenc) {
  int adx = 0, tobreak = 0;

  while (tobreak == 0) {
    if (toenc[0] == 0)
      tobreak = 1;

    toenc[0] += passwencstring[adx];
    adx++;
    toenc++;

    if (adx > 10)
      adx = 0;
  }
}

void write_string_encrypt(Stream *out, const char *s) {
  int stlent = (int)strlen(s) + 1;

  out->WriteInt32(stlent);
  char *enc = ags_strdup(s);
  encrypt_text(enc);
  out->WriteArray(enc, stlent, 1);
  free(enc);
}
