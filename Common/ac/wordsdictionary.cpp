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
#include "data/data_helpers.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

uint16_t WordsDictionary::FindWord(const String &word) const
{
    const auto it_found = _words.find(word);
    if (it_found != _words.end())
        return it_found->second;
    return WordsDictionary::INVALIDWORD;
}

void WordsDictionary::ReadFromFile(Stream* in)
{
    uint32_t word_count = static_cast<uint32_t>(in->ReadInt32());
    for (uint32_t i = 0; i < word_count; ++i)
    {
        String word = ReadStringDecrypt(in);
        uint16_t word_id = static_cast<uint16_t>(in->ReadInt16());
        _words[word] = word_id;
    }
}

void WordsDictionary::WriteToFile(Stream* out) const
{
    out->WriteInt32(_words.size());
    for (auto item : _words)
    {
        WriteStringEncrypt(out, item.first.GetCStr());
        out->WriteInt16(item.second);
    }
}

} // Common
} // AGS
