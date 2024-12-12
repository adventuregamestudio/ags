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
#ifndef __CC_SYSTEMIMPORTS_H
#define __CC_SYSTEMIMPORTS_H

#include <map>
#include "script/runtimescriptvalue.h"
#include "util/string.h"


// ScriptSymbolsMap is a wrapper around a lookup table, meant for storing
// script's imported and exported symbols, whether per individual script,
// or a joint table from all scripts.
// Its purpose is to provide lookup by both full and partial symbol names,
// which consist of several partitions, some of which considered optional;
// such as: function name with a number of arguments appended to it.
class ScriptSymbolsMap
{
    using String = AGS::Common::String;
public:
    ScriptSymbolsMap(char appendage_separator)
        : _appendageSeparator(appendage_separator) {}

    // Maps a symbol name to linear index
    void Add(const String &name, uint32_t index);
    // Removes a symbol name
    void Remove(const String &name);
    // Clears the map, removes all entries
    void Clear();
    // Gets an index of a symbol with exact name match;
    // returns UINT32_MAX on failure
    uint32_t GetIndexOf(const String &name) const;
    // Gets an index of a symbol, matching either exactly,
    // or one of the simpler variants in case of a composite input name;
    // returns UINT32_MAX on failure
    uint32_t GetIndexOfAny(const String &name) const;

private:
    const char _appendageSeparator;
    // Note we can't use a hash-map here, because we sometimes need to search
    // by partial keys, so sorting is cruicial
    std::map<String, uint32_t> _lookup;
};

class ccInstance;

struct ScriptImport
{
    using String = AGS::Common::String;

    ScriptImport() = default;
    ScriptImport(const String &name, const RuntimeScriptValue &rval, ccInstance *inst, ScriptValueHint val_hint = kScValHint_Unknown)
        : Name(name), Value(rval), InstancePtr(inst) {}

    String              Name;
    RuntimeScriptValue  Value;
    ccInstance         *InstancePtr = nullptr;
    ScriptValueHint     ValueHint = kScValHint_Unknown;
};

class SystemImports
{
    using String = AGS::Common::String;
public:
    SystemImports();

    // Adds a resolved import under given name
    uint32_t Add(const String &name, const RuntimeScriptValue &value, ccInstance *inst, ScriptValueHint val_hint = kScValHint_Unknown);
    // Removes an import
    void Remove(const String &name);
    // Removes all imports registered for the given script instance
    void RemoveScriptExports(ccInstance *inst);
    // Clears the map, removes all entries
    void Clear();
    // Gets an import by exact name match
    const ScriptImport *GetByName(const String &name) const;
    // Gets an import by its linear index
    const ScriptImport *GetByIndex(uint32_t index) const;
    // Searches for an import's name by its representation
    String FindName(const RuntimeScriptValue &value) const;

    // Gets an index of an imported symbol with exact name match;
    // returns UINT32_MAX on failure
    uint32_t GetIndexOf(const String &name) const { return _lookup.GetIndexOf(name); }
    // Gets an index of an imported symbol, matching either exactly,
    // or one of the simpler variants in case of a composite input name;
    // returns UINT32_MAX on failure
    uint32_t GetIndexOfAny(const String &name) const { return _lookup.GetIndexOfAny(name); }

private:
    std::vector<ScriptImport> _imports;
    ScriptSymbolsMap _lookup;
};

extern SystemImports simp;
// This is to register symbols exclusively for plugins, to allow them
// perform old style unsafe function calls
extern SystemImports simp_for_plugin;

#endif  // __CC_SYSTEMIMPORTS_H
