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
// Managed script object wrapping std::map<String, String> and
// unordered_map<String, String>.
//
// TODO: support wrapping non-owned Dictionary, passed by the reference, -
// that would let expose internal engine's dicts using same interface.
// TODO: maybe optimize key lookup operations further by not creating a String
// object from const char*. It seems, C++14 standard allows to use convertible
// types as keys; need to research what performance impact that would make.
//
//=============================================================================
#ifndef __AC_SCRIPTDICT_H
#define __AC_SCRIPTDICT_H

#include <map>
#include <unordered_map>
#include <string.h>
#include "ac/dynobj/cc_agsdynamicobject.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_types.h"

using namespace AGS::Common;

class ScriptDictBase : public AGSCCDynamicObject
{
public:
    int Dispose(void *address, bool force) override;
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    virtual bool IsCaseSensitive() const = 0;
    virtual bool IsSorted() const = 0;

    virtual void Clear() = 0;
    virtual bool Contains(const char *key) = 0;
    virtual const char *Get(const char *key) = 0;
    virtual bool Remove(const char *key) = 0;
    virtual bool Set(const char *key, const char *value) = 0;
    virtual int GetItemCount() = 0;
    virtual void GetKeys(std::vector<const char*> &buf) const = 0;
    virtual void GetValues(std::vector<const char*> &buf) const = 0;

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

private:
    virtual size_t CalcContainerSize() = 0;
    virtual void SerializeContainer(AGS::Common::Stream *out) = 0;
    virtual void UnserializeContainer(AGS::Common::Stream *in) = 0;
};

template <typename TDict, bool is_sorted, bool is_casesensitive>
class ScriptDictImpl final : public ScriptDictBase
{
public:
    typedef typename TDict::const_iterator ConstIterator;

    ScriptDictImpl() = default;

    bool IsCaseSensitive() const override { return is_casesensitive; }
    bool IsSorted() const override { return is_sorted; }

    void Clear() override
    {
        for (auto it = _dic.begin(); it != _dic.end(); ++it)
            DeleteItem(it);
        _dic.clear();
    }
    bool Contains(const char *key) override { return _dic.count(String::Wrapper(key)) != 0; }
    const char *Get(const char *key) override
    {
        auto it = _dic.find(String::Wrapper(key));
        if (it == _dic.end()) return nullptr;
        return it->second.GetCStr();
    }
    bool Remove(const char *key) override
    {
        auto it = _dic.find(String::Wrapper(key));
        if (it == _dic.end()) return false;
        DeleteItem(it);
        _dic.erase(it);
        return true;
    }
    bool Set(const char *key, const char *value) override
    {
        if (!key) return false;
        if (!value)
        { // remove keys with null value
            Remove(key);
            return true;
        }
        return TryAddItem(String(key), String(value));
    }
    int GetItemCount() override { return _dic.size(); }
    void GetKeys(std::vector<const char*> &buf) const override
    {
        for (auto it = _dic.begin(); it != _dic.end(); ++it)
            buf.push_back(it->first.GetCStr());
    }
    void GetValues(std::vector<const char*> &buf) const override
    {
        for (auto it = _dic.begin(); it != _dic.end(); ++it)
            buf.push_back(it->second.GetCStr());
    }

private:
    bool TryAddItem(const String &key, const String &value)
    {
        _dic[key] = value;
        return true;
    }
    void DeleteItem(ConstIterator /*it*/) { /* do nothing */ }

    size_t CalcContainerSize() override
    {
        // 2 class properties + item count
        size_t total_sz = sizeof(int32_t) * 3;
        // (int32 + string buffer) per item
        for (auto it = _dic.begin(); it != _dic.end(); ++it)
        {
            total_sz += sizeof(int32_t) + it->first.GetLength();
            total_sz += sizeof(int32_t) + it->second.GetLength();
        }
        return total_sz;
    }

    void SerializeContainer(AGS::Common::Stream *out) override
    {
        out->WriteInt32((int)_dic.size());
        for (auto it = _dic.begin(); it != _dic.end(); ++it)
        {
            out->WriteInt32((int)it->first.GetLength());
            out->Write(it->first.GetCStr(), it->first.GetLength());
            out->WriteInt32((int)it->second.GetLength());
            out->Write(it->second.GetCStr(), it->second.GetLength());
        }
    }

    void UnserializeContainer(AGS::Common::Stream *in) override
    {
        size_t item_count = in->ReadInt32();
        for (size_t i = 0; i < item_count; ++i)
        {
            size_t key_len = in->ReadInt32();
            String key = String::FromStreamCount(in, key_len);
            size_t value_len = in->ReadInt32();
            if (value_len != (size_t)-1) // do not restore keys with null value (old format)
            {
                String value = String::FromStreamCount(in, value_len);
                TryAddItem(key, value);
            }
        }
    }

    TDict _dic;
};

typedef ScriptDictImpl< std::map<String, String>, true, true > ScriptDict;
typedef ScriptDictImpl< std::map<String, String, StrLessNoCase>, true, false > ScriptDictCI;
typedef ScriptDictImpl< std::unordered_map<String, String>, false, true > ScriptHashDict;
typedef ScriptDictImpl< std::unordered_map<String, String, HashStrNoCase, StrEqNoCase>, false, false > ScriptHashDictCI;

#endif // __AC_SCRIPTDICT_H
