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
//
// UTF-8 utilities.
// Based on utf8 code from https://c9x.me/git/irc.git/tree/irc.c
//
//=============================================================================
#ifndef __AGS_CN_UTIL__UTF8_H
#define __AGS_CN_UTIL__UTF8_H

#include <algorithm>
#include <cstring>
#include "core/types.h"

namespace Utf8
{

typedef int32_t Rune;
const size_t UtfSz = 4;
const Rune RuneInvalid = 0xFFFD;

const unsigned char utfbyte[UtfSz + 1] = { 0x80,    0, 0xC0, 0xE0, 0xF0 };
const unsigned char utfmask[UtfSz + 1] = { 0xC0, 0x80, 0xE0, 0xF0, 0xF8 };
const Rune utfmin[UtfSz + 1] = { 0,    0,  0x80,  0x800,  0x10000 };
const Rune utfmax[UtfSz + 1] = { 0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF };


inline size_t Validate(Rune *u, size_t i)
{
    if (*u < utfmin[i] || *u > utfmax[i] || (0xD800 <= *u && *u <= 0xDFFF))
        *u = RuneInvalid;
    for (i = 1; *u > utfmax[i]; ++i)
        ;
    return i;
}

inline Rune DecodeByte(unsigned char c, size_t *i)
{
    for (*i = 0; *i < UtfSz + 1; ++(*i))
        if ((c & utfmask[*i]) == utfbyte[*i])
            return c & ~utfmask[*i];
    return 0;
}

inline char EncodeByte(Rune u, size_t i)
{
    return utfbyte[i] | (u & ~utfmask[i]);
}

// Read a single utf8 codepoint from the c-string;
// returns codepoint's size in bytes (may be used to advance string pos)
inline size_t GetChar(const char *c, size_t clen, Rune *u)
{
    size_t i, j, len, type;
    Rune udecoded;
    *u = RuneInvalid;
    if (!clen || !*c)
        return 0;
    udecoded = DecodeByte(c[0], &len);
    if (len < 1 || len > UtfSz)
        return 1;
    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | DecodeByte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    Validate(u, len);
    return len;
}

// Convert utf8 codepoint to the string representation and write to the buffer
inline size_t SetChar(Rune u, char *c, size_t clen)
{
    size_t len, i;
    len = Validate(&u, 0);
    if (len > UtfSz || len > clen)
        return 0;
    for (i = len - 1; i != 0; --i) {
        c[i] = EncodeByte(u, 0);
        u >>= 6;
    }
    c[0] = EncodeByte(u, len);
    return len;
}

// Calculates utf8 string length in characters
inline size_t GetLength(const char *c)
{
    size_t len = 0;
    Rune r;
    for (size_t chr_sz = 0; (chr_sz = GetChar(c, UtfSz, &r)) > 0; c += chr_sz, ++len);
    return len;
}

} // namespace Utf8

#endif // __AGS_CN_UTIL__UTF8_H
