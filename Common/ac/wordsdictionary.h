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

    typedef std::unordered_map<String, uint16_t, HashStrUtf8NoCase, StrEqUtf8NoCase>
        MapType;

    WordsDictionary() = default;

    uint16_t FindWord(const String &word) const;
    const MapType &GetWords() const { return _words; }
    MapType &GetWords() { return _words; }

    void ReadFromFile(Stream *in);
    void WriteToFile(Stream *out) const;

private:
    // Map word to word ID;
    // Text Parser's dictionary is case-insensitive
    MapType _words;
};

} // Common
} // AGS

#endif // __AGS_CN_UTIL__WORDSDICTIONARY_H
