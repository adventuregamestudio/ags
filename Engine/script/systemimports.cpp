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
#include "script/systemimports.h"
#include <stdlib.h>
#include <string.h>

using namespace AGS::Common;


void ScriptSymbolsMap::Add(const String &name, uint32_t index)
{
    _lookup[name] = index;
}

void ScriptSymbolsMap::Remove(const String &name)
{
    _lookup.erase(name);
}

void ScriptSymbolsMap::Clear()
{
    _lookup.clear();
}

uint32_t ScriptSymbolsMap::GetIndexOf(const String &name) const
{
    auto it = _lookup.find(name);
    if (it != _lookup.end())
        return it->second;

    // Not found...
    return UINT32_MAX;
}

uint32_t ScriptSymbolsMap::GetIndexOfAny(const String &name) const
{
    // Import names may be potentially formed as:
    //
    //     [type::]name[^argnum]
    //
    // where "type" is the name of a type, "name" is the name of a function,
    // "argnum" is the number of arguments.
    // The appendage separator may be an import separator '^', or a script
    // export separator '$', and only one kind is used in a string.
    // The direct match of a full symbol name is always a priority.
    // If no direct match is found, then we search for alternatives:
    // 
    // * If the search name does not have any appendages, then we don't know
    // which arg list to expect, and so lookup for the first found script
    // export with arg list matching the symbol name.
    // * If the search name is a import name with arg list, then we first
    // lookup for the name only without args, and then for a script export
    // with both matching names and args.
    // * If the search name is an export name with arg list, then we only
    // lookup for the name only without args.

    // First try direct match
    auto item = _lookup.find(name);
    if (item != _lookup.end())
        return item->second;

    // If direct match failed, then split the request into name and args
    size_t args_at = name.FindAnyChar(AnySeparator);
    char args_separator = 0;
    if (args_at != String::NoIndex)
        args_separator = name[args_at];
    else
        args_at = name.GetLength(); // clamp for simpler algorithm below

    const String name_only = name.Left(args_at);
    const String argnum_only = name.Mid(args_at + 1);

    // Request has no args, or
    // Request is an import symbol
    if ((args_separator == 0) || (args_separator == ImportSeparator))
    {
        // Try lookup a symbol either matching base name
        // or a script export symbol matching arg list
        // (or any arg list, if request dont have one).
        // The script export matching arg list has a priority here.
        uint32_t name_only_match = UINT32_MAX;
        for (item = _lookup.lower_bound(name_only); item != _lookup.end(); ++item)
        {
            const String &try_sym = item->first;
            if (try_sym.CompareLeft(name_only, name_only.GetLength()) != 0)
                break; // base name not matched, no reason to scan further

            if (try_sym.GetLength() == name_only.GetLength())
                name_only_match = item->second; // save in case we won't find a arg list match
            // if request had a arg list, then choose a script export with a matching one
            // if request did not have any arg list, then choose a first found script export with any args
            else if ((try_sym[args_at] == ScriptExportSeparator) && ((args_separator == 0) || (try_sym.Mid(args_at + 1) == argnum_only)))
                return item->second;
        }
        return name_only_match;
    }
    // Request is a script export symbol
    else
    {
        // Try lookup a symbol matching base name
        auto item = _lookup.find(name_only);
        if (item != _lookup.end())
            return item->second;
    }

    // Failed to find any acceptable match
    return UINT32_MAX;
}


uint32_t SystemImports::Add(const String &name, const RuntimeScriptValue &value, const ccInstance *inst)
{
    assert(!name.IsEmpty());
    assert(value.IsValid());

    if (name.IsEmpty() || !value.IsValid())
        return UINT32_MAX;

    uint32_t ixof = GetIndexOf(name);
    // Check if symbol already exists
    if (ixof != UINT32_MAX)
    {
        // Only allow override if not a script-exported function
        if (inst == nullptr)
        {
            _imports[ixof] = ScriptImport(name, value, nullptr);
        }
        return ixof;
    }

    ixof = _imports.size();
    for (size_t i = 0; i < _imports.size(); ++i)
    {
        if (_imports[i].Name.IsEmpty())
        {
            ixof = i;
            break;
        }
    }

    if (ixof == _imports.size())
        _imports.emplace_back(name, value, inst);
    else
        _imports[ixof] = ScriptImport(name, value, inst);
    _lookup.Add(name, ixof);
    return ixof;
}

void SystemImports::Remove(const String &name)
{
    uint32_t idx = GetIndexOf(name);
    if (idx == UINT32_MAX)
        return;

    _lookup.Remove(_imports[idx].Name);
    _imports[idx] = {};
}

const ScriptImport *SystemImports::GetByName(const String &name) const
{
    uint32_t o = GetIndexOf(name);
    if (o == UINT32_MAX)
        return nullptr;

    return &_imports[o];
}

const ScriptImport *SystemImports::GetByIndex(uint32_t index) const
{
    if (index >= _imports.size())
        return nullptr;

    return &_imports[index];
}

String SystemImports::FindName(const RuntimeScriptValue &value) const
{
    for (const auto &import : _imports)
    {
        if (import.Value == value)
        {
            return import.Name;
        }
    }
    return String();
}

void SystemImports::RemoveScriptExports(const ccInstance *inst)
{
    if (!inst)
    {
        return;
    }

    for (auto &import : _imports)
    {
        if (import.Name.IsEmpty())
            continue;

        if (import.InstancePtr == inst)
        {
            _lookup.Remove(import.Name);
            import = {};
        }
    }
}

void SystemImports::Clear()
{
    _lookup.Clear();
    _imports.clear();
}
