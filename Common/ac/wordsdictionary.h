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
#ifndef __AGS_CN_UTIL__WORDSDICTIONARY_H
#define __AGS_CN_UTIL__WORDSDICTIONARY_H

#include <unordered_map>
#include <vector>
#include "util/string.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{

class WordsDictionary
{
public:
    // Special text parser entries
    static const uint16_t IGNOREWORD   = 0;
    static const uint16_t ANYWORD      = 29999;
    static const uint16_t RESTOFLINE   = 30000;
    static const uint16_t INVALIDWORD  = UINT16_MAX;

    WordsDictionary() = default;

    uint16_t FindWord(const String &word) const;
    const std::unordered_map<String, uint16_t, HashStrUtf8NoCase, StrEqUtf8NoCase> &
        GetWords() const { return _words; }

    void ReadFromFile(Stream *in);
    void WriteToFile(Stream *out) const;

private:
    // Map word to word ID;
    // Text Parser's dictionary is case-insensitive
    std::unordered_map<String, uint16_t, HashStrUtf8NoCase, StrEqUtf8NoCase> _words;
};

} // Common
} // AGS

extern const char *passwencstring;

// Decrypts text found in the given buffer, writes back to the same buffer
void decrypt_text(char *buf, size_t buf_sz);
// Reads an encrypted string from the stream and decrypts into the provided buffer
void read_string_decrypt(AGS::Common::Stream *in, char *buf, size_t buf_sz);
// Reads an encrypted string from the stream and returns as a string
AGS::Common::String read_string_decrypt(AGS::Common::Stream *in);
// Reads an encrypted string from the stream and returns as a string;
// uses provided vector as a temporary decryption buffer (avoid extra allocs)
AGS::Common::String read_string_decrypt(AGS::Common::Stream *in, std::vector<char> &dec_buf);
// Skip an encrypted string in stream
void skip_string_decrypt(AGS::Common::Stream *in);

void encrypt_text(char *toenc);
void write_string_encrypt(AGS::Common::Stream *out, const char *s);

#endif // __AGS_CN_UTIL__WORDSDICTIONARY_H
