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
// Managed script object wrapping std::set<String> and unordered_set<String>.
//
// TODO: support wrapping non-owned Set, passed by the reference, -
// that would let expose internal engine's sets using same interface.
// TODO: maybe optimize key lookup operations further by not creating a String
// object from const char*. It seems, C++14 standard allows to use convertible
// types as keys; need to research what performance impact that would make.
//
//=============================================================================
#ifndef __AC_SCRIPTSET_H
#define __AC_SCRIPTSET_H

#include <set>
#include <unordered_set>
#include <string.h>
#include "ac/runtime_defines.h"
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_types.h"

using namespace AGS::Common;

class ScriptSetBase : public AGSCCDynamicObject
{
public:
    int Dispose(void *address, bool force) override;
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    virtual ScriptSortStyle GetSortStyle() const = 0;
    virtual ScriptStringComparison GetStringCompareStyle() const = 0;

    virtual bool Add(const char *item) = 0;
    virtual void Clear() = 0;
    virtual bool Contains(const char *item) const = 0;
    virtual bool Remove(const char *item) = 0;
    virtual int GetItemCount() const = 0;
    virtual void GetItems(std::vector<const char*> &buf) const = 0;

protected:
    // Calculate and return required space for serialization, in bytes
    virtual size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

private:
    virtual size_t CalcContainerSize() = 0;
    virtual void SerializeContainer(AGS::Common::Stream *out) = 0;
    virtual void UnserializeContainer(AGS::Common::Stream *in) = 0;
};

template <typename TSet, ScriptSortStyle SortStyle, ScriptStringComparison CompareStyle>
class ScriptSetBaseImpl : public ScriptSetBase
{
public:
    typedef typename TSet::const_iterator ConstIterator;

    ScriptSetBaseImpl() = default;
    ScriptSetBaseImpl(const TSet &set)
        : _set(set) { }
    ScriptSetBaseImpl(TSet &&set)
        : _set(std::move(set)) { }

    ScriptSortStyle GetSortStyle() const override { return SortStyle; }
    ScriptStringComparison GetStringCompareStyle() const override { return CompareStyle; }

    bool Add(const char *item) override
    {
        if (!item) return false;
        return TryAddItem(String(item));
    }
    void Clear() override
    {
        for (auto it = _set.begin(); it != _set.end(); ++it)
            DeleteItem(it);
        _set.clear();
    }
    bool Contains(const char *item) const override { return _set.count(String::Wrapper(item)) != 0; }
    bool Remove(const char *item) override
    {
        auto it = _set.find(String::Wrapper(item));
        if (it == _set.end()) return false;
        DeleteItem(it);
        _set.erase(it);
        return true;
    }
    int GetItemCount() const override { return _set.size(); }
    void GetItems(std::vector<const char*> &buf) const override
    {
        for (auto it = _set.begin(); it != _set.end(); ++it)
            buf.push_back(it->GetCStr());
    }

private:
    bool TryAddItem(const String &s)
    {
        return _set.insert(s).second;
    }
    void DeleteItem(ConstIterator /*it*/) { /* do nothing */ }

    size_t CalcContainerSize() override
    {
        // 2 class properties + item count
        size_t total_sz = sizeof(int32_t) * 3;
        // (int32 + string buffer) per item
        for (auto it = _set.begin(); it != _set.end(); ++it)
            total_sz += sizeof(int32_t) + it->GetLength();
        return total_sz;
    }

    void SerializeContainer(AGS::Common::Stream *out) override
    {
        out->WriteInt32((int)_set.size());
        for (auto it = _set.begin(); it != _set.end(); ++it)
        {
            out->WriteInt32((int)it->GetLength());
            out->Write(it->GetCStr(), it->GetLength());
        }
    }

    void UnserializeContainer(AGS::Common::Stream *in) override
    {
        size_t item_count = in->ReadInt32();
        for (size_t i = 0; i < item_count; ++i)
        {
            size_t len = in->ReadInt32();
            String item = String::FromStreamCount(in, len);
            TryAddItem(item);
        }
    }

    TSet _set;
};

template <typename TKey, typename TLess, ScriptStringComparison CompareStyle>
class ScriptSetStdImpl final : public ScriptSetBaseImpl<std::set<TKey, TLess>,
    kScSorted, CompareStyle>
{
public:
    ScriptSetStdImpl() = default;
    ScriptSetStdImpl(const TLess &less)
        : ScriptSetBaseImpl<std::set<TKey, TLess>, kScSorted, CompareStyle>
        (std::move(std::set<TKey, TLess>(less)))
    {
    }
};

template <typename TKey, typename THash, typename TEqual, ScriptStringComparison CompareStyle>
class ScriptSetStdHashImpl final : public ScriptSetBaseImpl<std::unordered_set<TKey, THash, TEqual>,
    kScNotSorted, CompareStyle>
{
public:
    ScriptSetStdHashImpl() = default;
    ScriptSetStdHashImpl(const THash &hash, const TEqual &equal)
        : ScriptSetBaseImpl<std::unordered_set<TKey, THash, TEqual>, kScNotSorted, CompareStyle>
        (std::move(std::unordered_set<TKey, THash, TEqual>(0, hash, equal)))
    {
    }
};

typedef ScriptSetStdImpl< String, std::less<String>, kScCaseSensitive > ScriptSet;
typedef ScriptSetStdImpl< String, StrLessNoCase, kScCaseInsensitive > ScriptSetCI;
typedef ScriptSetStdImpl< String, StrLessUtf8NoCase, kScCaseInsensitive > ScriptSetUtf8CI;
typedef ScriptSetStdImpl< String, LexographicalStrLess, kScCaseSensitiveLocaleAware > ScriptSetLocaleAware;
typedef ScriptSetStdImpl< String, LexographicalStrLessNoCase, kScCaseInsensitiveLocaleAware > ScriptSetLocaleAwareCI;
typedef ScriptSetStdHashImpl< String, std::hash<String>, std::equal_to<String>, kScCaseSensitive > ScriptHashSet;
typedef ScriptSetStdHashImpl< String, HashStrNoCase, StrEqNoCase, kScCaseInsensitive > ScriptHashSetCI;
typedef ScriptSetStdHashImpl< String, HashStrUtf8NoCase, StrEqUtf8NoCase, kScCaseInsensitive > ScriptHashSetUtf8CI;

#endif // __AC_SCRIPTSET_H
