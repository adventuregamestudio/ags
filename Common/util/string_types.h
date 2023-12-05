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
#ifndef __AGS_CN_UTIL__STRINGTYPES_H
#define __AGS_CN_UTIL__STRINGTYPES_H

#include <cctype>
#include <functional>
#include <unordered_map>

#include <vector>
#include "util/string.h"

namespace FNV
{

const uint32_t PRIME_NUMBER = 2166136261U;
const uint32_t SECONDARY_NUMBER = 16777619U;

inline size_t Hash(const char *data, const size_t len)
{
    uint32_t hash = PRIME_NUMBER;
    for (size_t i = 0; i < len; ++i)
        hash = (SECONDARY_NUMBER * hash) ^ (uint8_t)(data[i]);
    return hash;
}

inline size_t Hash_LowerCase(const char *data, const size_t len)
{
    uint32_t hash = PRIME_NUMBER;
    for (size_t i = 0; i < len; ++i)
        hash = (SECONDARY_NUMBER * hash) ^ (uint8_t)(tolower(data[i]));
    return hash;
}

} // namespace FNV


// A std::hash specialization for AGS String
namespace std
{
// std::hash for String object
template<>
struct hash<AGS::Common::String>
{
    size_t operator ()(const AGS::Common::String &key) const
    {
        return FNV::Hash(key.GetCStr(), key.GetLength());
    }
};
}


namespace AGS
{
namespace Common
{

//
// Various comparison functors
//

// Test case-insensitive String equality
struct StrEqNoCase
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareNoCase(s2) == 0;
    }
};

// Case-insensitive String less
struct StrLessNoCase
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareNoCase(s2) < 0;
    }
};

// Compute case-insensitive hash for a String object
struct HashStrNoCase
{
    size_t operator ()(const String &key) const
    {
        return FNV::Hash_LowerCase(key.GetCStr(), key.GetLength());
    }
};

// Alias for vector of Strings
typedef std::vector<String> StringV;
// Alias for case-sensitive hash-map of Strings
typedef std::unordered_map<String, String> StringMap;
// Alias for case-insensitive hash-map of Strings
typedef std::unordered_map<String, String, HashStrNoCase, StrEqNoCase> StringIMap;
// Alias for std::array of C-strings
template <size_t SIZE> using CstrArr = std::array<const char*, SIZE>;

} // namespace Common
} // namespace AGS

#endif //__AGS_CN_UTIL__STRINGTYPES_H
