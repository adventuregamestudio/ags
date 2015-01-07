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
#ifndef __AGS_CN_UTIL__STRINGTYPES_H
#define __AGS_CN_UTIL__STRINGTYPES_H

#include <cctype>
#if defined (_MSC_VER)
#include <functional>
#include <unordered_map>
#elif defined (__GNUC__)
#include <tr1/functional>
#include <tr1/unordered_map>
#endif
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


namespace std
{
namespace tr1 // TODO: change to std:: if moved to C++11
{
// A std::hash specialization for AGS String
template<>
struct hash<AGS::Common::String> : public unary_function<AGS::Common::String, size_t>
{
    size_t operator ()(const AGS::Common::String &key) const
    {
        return FNV::Hash(key.GetCStr(), key.GetLength());
    }
};
} // namespace tr1
} // namespace std


namespace AGS
{
namespace Common
{

struct StrCmpNoCase : public std::binary_function<String, String, bool>
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareNoCase(s2) == 0;
    }
};

struct HashStrNoCase : public std::unary_function<String, size_t>
{
    size_t operator ()(const String &key) const
    {
        return FNV::Hash_LowerCase(key.GetCStr(), key.GetLength());
    }
};

typedef std::vector<String> StringV;
typedef std::tr1::unordered_map<String, String> StringMap;
typedef std::tr1::unordered_map<String, String, HashStrNoCase, StrCmpNoCase> StringIMap;

} // namespace Common
} // namespace AGS

#endif //__AGS_CN_UTIL__STRINGTYPES_H
