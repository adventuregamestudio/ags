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
#ifndef __AGS_CN_UTIL__STRINGTYPES_H
#define __AGS_CN_UTIL__STRINGTYPES_H

#include <array>
#include <cctype>
#include <functional>
#include <locale>
#include <memory>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include "util/string.h"
#include "util/utf8.h"

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

inline size_t Hash_Utf8LowerCase(const char *data, const size_t len)
{
    uint32_t hash = PRIME_NUMBER;
    Utf8::Rune r;
    char buf[Utf8::UtfSz + 1]{};
    for (const char *ptr = data; *ptr;)
    {
        ptr += Utf8::GetChar(ptr, Utf8::UtfSz, &r);
        size_t lower_sz = Utf8::SetChar(Utf8::ToLower(r), buf, Utf8::UtfSz);
        for (size_t i = 0; i < lower_sz; ++i)
            hash = (SECONDARY_NUMBER * hash) ^ (uint8_t)(buf[i]);
    }
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

// Returns std::locale object, either one identified using the given name, or a
// standard C locale, in case something went wrong.
inline std::locale GetLocaleSafe(const char *locale_name)
{
    try
    {
        if (locale_name && *locale_name)
            return std::locale(locale_name);
        else
            return std::locale();
    }
    catch (const std::runtime_error&)
    {
        return std::locale();
    }
}

//
// Various comparison functors
//

// Test case-sensitive String equality
struct StrEq
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1 == s2;
    }
};

// Test case-insensitive String equality
struct StrEqNoCase
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareNoCase(s2) == 0;
    }
};

// Test case-insensitive String equality for UTF-8 strings
struct StrEqUtf8NoCase
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareUtf8NoCase(s2) == 0;
    }
};

// Test case-insensitive String equality as a pre-defined unary predicate
struct StrEqNoCasePred
{
    StrEqNoCasePred(const String &look_for)
        : _lookFor(look_for) {}

    bool operator()(const String &s) const
    {
        return _lookFor.CompareNoCase(s) == 0;
    }

private:
    String _lookFor;
};

// Case-insensitive String less predicate
struct StrLessNoCase
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareNoCase(s2) < 0;
    }
};

// Case-insensitive String less for UTF-8 strings
struct StrLessUtf8NoCase
{
    bool operator()(const String &s1, const String &s2) const
    {
        return s1.CompareUtf8NoCase(s2) < 0;
    }
};

// Unicode String less predicate, comparing strings lexographically.
// With this predicate, characters are compared by their meaning; for example,
// 'À' follows 'A' and 'Č' follows 'C', as opposed to common char code-based
// comparison, where 'À' is positioned after 'Z'.
struct LexographicalStrLess
{
public:
    LexographicalStrLess()
        : _loc(std::locale())
    {
    }

    LexographicalStrLess(const char *locale_name)
        : _loc(GetLocaleSafe(locale_name))
    {
    }

    bool operator()(const String &s1, const String &s2) const
    {
        return std::use_facet<std::collate<char>>(_loc).
            compare(s1.GetCStr(), s1.GetCStr() + s1.GetLength(), s2.GetCStr(), s2.GetCStr() + s2.GetLength()) < 0;
    }

private:
    std::locale _loc;
};

// Unicode String less predicate, comparing strings lexographically, and
// case-insensitively.
// With this predicate, characters are compared by their meaning; for example,
// 'À' follows 'A' and 'Č' follows 'C', as opposed to common char code-based
// comparison, where 'À' is positioned after 'Z'.
struct LexographicalStrLessNoCase
{
    LexographicalStrLessNoCase()
        : _loc(std::locale())
    {
    }

    LexographicalStrLessNoCase(const char *locale_name)
        : _loc(GetLocaleSafe(locale_name))
    {
    }

    bool operator()(const String &s1, const String &s2) const
    {
        // TODO: find out if there's a way to avoid allocating lowercase strings,
        // and do collation over original strings instead (does C++ stdlib support that?)
        const String s1lower = s1.LowerUTF8();
        const String s2lower = s2.LowerUTF8();
        return std::use_facet<std::collate<char>>(_loc).
            compare(s1lower.GetCStr(), s1lower.GetCStr() + s1lower.GetLength(), s2lower.GetCStr(), s2lower.GetCStr() + s2lower.GetLength()) < 0;
    }

private:
    std::locale _loc;
};

// Intreface for the multi-purpose string comparison predicates
struct IStrCmp
{
    virtual ~IStrCmp(){}
    virtual int operator()(const String &s1, const String &s2) const = 0;
};

// The multi-mode 'equal' predicate, which acts according to its configuration
struct StrEqAuto
{
    StrEqAuto(std::unique_ptr<IStrCmp> &&cmp_impl)
        : _cmpImpl(std::move(cmp_impl)) { }

    bool operator()(const String &s1, const String &s2) const
    {
        return _cmpImpl->operator()(s1, s2) == 0;
    }

private:
    // It's shared ptr, because STL requires a copy constructor for predicates
    std::shared_ptr<IStrCmp> _cmpImpl;
};

// The multi-mode 'less' predicate, which acts according to its configuration
struct StrLessAuto
{
    StrLessAuto(std::unique_ptr<IStrCmp> &&cmp_impl)
        : _cmpImpl(std::move(cmp_impl)) { }

    bool operator()(const String &s1, const String &s2) const
    {
        return _cmpImpl->operator()(s1, s2) < 0;
    }

private:
    // It's shared ptr, because STL requires a copy constructor for predicates
    std::shared_ptr<IStrCmp> _cmpImpl;
};

struct StrCmpDirect : public IStrCmp
{
    int operator()(const String &s1, const String &s2) const override
    {
        return s1.Compare(s2);
    }
};

struct StrCmpNoCase : public IStrCmp
{
    int operator()(const String &s1, const String &s2) const override
    {
        return s1.CompareNoCase(s2);
    }
};

struct StrCmpUtf8NoCase : public IStrCmp
{
    int operator()(const String &s1, const String &s2) const override
    {
        return s1.CompareUtf8NoCase(s2);
    }
};

struct StrCmpLexographical : public IStrCmp
{
    StrCmpLexographical()
        : _loc(std::locale()) {}

    StrCmpLexographical(const char *locale_name)
        : _loc(GetLocaleSafe(locale_name)) {}

    int operator()(const String &s1, const String &s2) const override
    {
        return std::use_facet<std::collate<char>>(_loc).
            compare(s1.GetCStr(), s1.GetCStr() + s1.GetLength(), s2.GetCStr(), s2.GetCStr() + s2.GetLength());
    }

private:
    std::locale _loc;
};

struct StrCmpLexographicalNoCase : public IStrCmp
{
    StrCmpLexographicalNoCase()
        : _loc(std::locale()) {}

    StrCmpLexographicalNoCase(const char *locale_name)
        : _loc(GetLocaleSafe(locale_name)) {}

    int operator()(const String &s1, const String &s2) const override
    {
        // TODO: find out if there's a way to avoid allocating lowercase strings,
        // and do collation over original strings instead (does C++ stdlib support that?)
        const String s1lower = s1.LowerUTF8();
        const String s2lower = s2.LowerUTF8();
        return std::use_facet<std::collate<char>>(_loc).
            compare(s1lower.GetCStr(), s1lower.GetCStr() + s1lower.GetLength(), s2lower.GetCStr(), s2lower.GetCStr() + s2lower.GetLength());
    }

private:
    std::locale _loc;
};

// Compute case-insensitive hash for a String object
struct HashStrNoCase
{
    size_t operator ()(const String &key) const
    {
        return FNV::Hash_LowerCase(key.GetCStr(), key.GetLength());
    }
};

// Compute case-insensitive hash for a String object containing UTF-8 string
struct HashStrUtf8NoCase
{
    size_t operator ()(const String &key) const
    {
        return FNV::Hash_Utf8LowerCase(key.GetCStr(), key.GetLength());
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
