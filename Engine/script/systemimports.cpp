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


ScriptSymbolsMap::ScriptSymbolsMap(const char *appendage_separators, bool allow_match_expanded)
    : _allowMatchExpanded(allow_match_expanded)
{
    if (appendage_separators && appendage_separators[0])
    {
        for (; *appendage_separators ; ++appendage_separators)
            _appendageSeparators.push_back(*appendage_separators);
        _appendageSeparators.push_back(0); // we're going to use this vector as a C-string
    }
}

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
    // The section separator may be one of the _appendageSeparators, but only
    // one kind is used in a string, and this separator kind is also used to
    // distinguish between export sources (e.g. engine api vs script api).
    // If a valid separator was found in the requested name, then this
    // separator will be a priority when looking for a match. In case of no
    // direct match we are permitted to match close entries with any valid
    // separator in them.
    char req_separator = 0;
    size_t first_separator_at = String::NoIndex;
    if (!_appendageSeparators.empty())
    {
        first_separator_at = name.FindAnyChar(_appendageSeparators.data());
        if (first_separator_at != String::NoIndex)
            req_separator = name[first_separator_at];
    }

    const size_t argnum_at = std::min(name.GetLength(), first_separator_at);
    const size_t append_end = name.GetLength();
    // TODO: optimize this by supporting string views! or compare methods which let compare with a substring of input.
    // Note that if there's no appendage, then substring will return original string, no new string is allocated.
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
    char best_separator = 0;

    for (auto it = _lookup.lower_bound(name_only); it != _lookup.end(); ++it)
    {
        const String &try_sym = it->first;
        // First - compare base name section
        // If base name is not matching, then there's no reason to continue the range
        if (try_sym.CompareLeft(name_only, argnum_at) != 0)
            break;

        // Search for one of the valid separators in the tested symbol name.
        char sym_separator = 0;
        if ((try_sym.GetLength() > argnum_at) && !_appendageSeparators.empty())
        {
            const char *sym_sep_ptr = strchr(_appendageSeparators.data(), try_sym[argnum_at]);
            if (sym_sep_ptr)
                sym_separator = *sym_sep_ptr;

            // If we had some best match saved, then decide if we want to switch to another separator kind
            if ((sym_separator != 0) && (best_separator != 0))
            {
                // If the best match's separator is already matching the requested one, then skip this symbol;
                // If we already have the same separator kind saved as the best match, then skip this symbol
                if ((best_separator == req_separator) || (best_separator == sym_separator))
                    continue;

                // If this new symbol's separator does not match a requested one,
                // and the last best match's separator has a priority over new one, then skip this symbol
                if ((req_separator != sym_separator) &&
                    (strchr(_appendageSeparators.data(), best_separator) <
                     strchr(_appendageSeparators.data(), sym_separator)))
                    continue;
            }
        }

        // If the symbol is longer, but there's no separator after base name,
        // then the symbol has a different, longer base name (e.g. "FindChar" vs "FindCharacter");
        // or symbol has a different separator kind, not matching the requested name.
        if ((try_sym.GetLength() > name_only.GetLength()) && (sym_separator == 0))
            continue;

        const size_t sym_argnum_at = argnum_at;
        const size_t sym_append_end = try_sym.GetLength();

        //---------------------------------------------------------------------
        // Check the argnum appendage

        // If the tried symbol does not have any appendages, then:
        // - if request does not have any either, that's the exact match;
        // - otherwise save it as a best match and continue searching.
        if (sym_argnum_at == try_sym.GetLength())
        {
            if (argnum_at == name.GetLength())
                return it->second;

            best_match = it->second;
            best_separator = sym_separator;
            continue;
        }
        // If the request is without argnum, but tried symbol has appendages,
        // then optionally choose the first found symbol which has at least base name matching.
        else if (argnum_at == name.GetLength())
        {
            if (_allowMatchExpanded)
            {
                best_match = it->second;
                best_separator = sym_separator;
                continue;
            }
            else
            {
                // exact base-name match would be first in order, so no reason to continue
                break;
            }
        }

        // Compare argnum appendage, and skip on failure
        if ((sym_append_end != append_end) || try_sym.CompareMid(argnum_only, argnum_at + 1) != 0)
            continue;

        // Matched whole appendage, found exact appendage match;
        // if separator is also matching, then succeed immediately, otherwise save as best match and continue
        if (sym_separator == req_separator)
            return it->second;

        best_match = it->second;
        best_separator = sym_separator;
    }
    
    // If no exact match was found, then select the closest found match
    if (best_match != UINT32_MAX)
        return best_match;

    // Not found...
    return UINT32_MAX;
}


SystemImports::SystemImports()
    : _lookup("$^", true /* allow to match symbols with more appendages */)
{
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
