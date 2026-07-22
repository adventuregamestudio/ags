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
// Helper functions related to reading or writing game data.
//
//=============================================================================
#ifndef __AGS_CN_GAME__DATAHELPERS_H
#define __AGS_CN_GAME__DATAHELPERS_H

#include <vector>
#include "util/string.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{
    // This *double escapes *an escaped '[' character(old - style linebreak,
    // which must be escaped by user if they want a literal '[' in text).
    // This is required before doing standard unescaping for this line,
    // for in such case "\[" will be treated as a unknown escape sequence,
    // while "\\[" will be converted to "\[" by merging "\\" pair.
    String PreprocessLineForOldStyleLinebreaks(const String &line);

    // Text encryption/decryption functions which apply a password string
    // using ADD operation (SUB when decrypting).
    // Decrypts text found in the given buffer, writes back to the same buffer
    void DecryptText(char *buf, size_t buf_sz);
    // Reads an encrypted string from the stream and decrypts into the provided buffer
    void ReadStringDecrypt(Stream *in, char *buf, size_t buf_sz);
    // Reads an encrypted string from the stream and returns as a string
    String ReadStringDecrypt(Stream *in);
    // Reads an encrypted string from the stream and returns as a string;
    // uses provided vector as a temporary decryption buffer (avoid extra allocs)
    String ReadStringDecrypt(Stream *in, std::vector<char> &dec_buf);

    // Password used for encryption; exposed for tests and editor (temporarily)
    extern const char *EncryptPassword;

    // Encrypts string in-place
    void EncryptText(char *buf, size_t buf_sz);
    // Encrypts input string and stores result in the vector of chars;
    // returns a pointer to the buffer.
    const char *EncryptText(std::vector<char> &en_buf, const String &s);
    // Encrypts empty string. A helper function in case you don't have any source text
    const char *EncryptEmptyString(std::vector<char> &en_buf);
    // Encrypts string and writes into the output stream
    void WriteStringEncrypt(Stream *out, const char *s);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__DATAHELPERS_H
