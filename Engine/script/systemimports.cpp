//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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

    const size_t argnum_at = std::min(name.GetLength(), name.FindChar(_appendageSeparator));
    const size_t append_end = name.GetLength();
    // TODO: optimize this by supporting string views! or compare methods which let compare with a substring of input
    const String name_only = name.Left(argnum_at);
    const String argnum_only = name.Mid(argnum_at + 1, append_end - argnum_at - 1);

    // Scan the range of possible matches, starting with pure name without appendages.
    // The match logic is this:
    // * the request is compared one section after another, first base name, then first
    //   appendage, then second appendage, and so forth (in case we support multiple appendages);
    // * as we go we save and update the best match: symbol that has only base name,
    //   symbol that has base name + first appendage, and so forth.
    // * if no exact match is found, then we use the best saved match.
    //
    // We know as a fact that in an ordered string container shorter names come first
    // and longer names come after, meaning that symbols without appendage will be met first.
    uint32_t best_match = UINT32_MAX;
    for (auto it = _lookup.lower_bound(name_only); it != _lookup.end(); ++it)
    {
        const String &try_sym = it->first;
        // First - compare base name section
        // If base name is not matching, then there's no reason to continue the range
        if (try_sym.CompareLeft(name_only, argnum_at) != 0)
            break;

        // If the symbol is longer, but there's no separator after base name,
        // then the symbol has a different, longer base name (e.g. "FindChar" vs "FindCharacter")
        if ((try_sym.GetLength() > name_only.GetLength()) && try_sym[name_only.GetLength()] != _appendageSeparator)
            continue;

        // Search for appendage separators in the matching symbol
        const size_t sym_argnum_at = std::min(try_sym.GetLength(), try_sym.FindChar(_appendageSeparator, argnum_at));
        const size_t sym_append_end = try_sym.GetLength();

        //---------------------------------------------------------------------
        // Check the argnum appendage

        // If the symbol does not have any appendages, then:
        // - if request does not have any either, that's the exact match;
        // - otherwise save it as a best match and continue searching;
        if (sym_argnum_at == try_sym.GetLength())
        {
            if (argnum_at == name.GetLength())
                return it->second;
            best_match = it->second;
            continue;
        }

        // If the request is without argnum, then optionally choose the first found symbol
        // which has at least base name matching
        if (argnum_at == name.GetLength())
        {
            if (_allowMatchExpanded)
                return it->second;
            break; // exact base-name match would be first in order, so no reason to continue
        }

        // Compare argnum appendage, and skip on failure
        if ((sym_append_end != append_end) || try_sym.CompareMid(argnum_only, argnum_at + 1) != 0)
            continue;

        // Matched whole appendage, found exact match
        return it->second;
    }
    
    // If no exact match was found, then select the closest found match
    if (best_match != UINT32_MAX)
        return best_match;

    // Not found...
    return UINT32_MAX;
}


SystemImports::SystemImports()
    : _lookup('^', true /* allow to match symbols with more appendages */)
{
}

uint32_t SystemImports::Add(const String &name, const RuntimeScriptValue &value, const ccInstance *inst, ScriptValueHint val_hint)
{
    assert(value.IsValid());
    uint32_t ixof = GetIndexOf(name);
    // Check if symbol already exists
    if (ixof != UINT32_MAX)
    {
        // Only allow override if not a script-exported function
        if (inst == nullptr)
        {
            _imports[ixof] = ScriptImport(name, value, nullptr, val_hint);
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
        _imports.emplace_back(name, value, inst, val_hint);
    else
        _imports[ixof] = ScriptImport(name, value, inst, val_hint);
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
