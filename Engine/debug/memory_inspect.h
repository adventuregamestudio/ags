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
//
// Methods for inspecting game and script memory.
//
//=============================================================================
#ifndef __AGS_EE_DEBUG__MEMORYINSPECT_H
#define __AGS_EE_DEBUG__MEMORYINSPECT_H

#include "util/error.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using AGS::Common::HError;
using AGS::Common::String;

namespace MemoryInspect
{
    struct VariableInfo
    {
        String Value;
        String TypeName;
        String TypeHint;

        VariableInfo() = default;
        VariableInfo(const String &value, const String &type, const String &hint)
            : Value(value), TypeName(type), TypeHint(hint) {}
    };

    // Search a script variable by a *context-dependent* name chain ("foo.bar.baz"),
    // on success returns its type and value as a string, formatted according to its type.
    // Supports name chain of any complexity. Resolves structs, pointers and arrays using RTTI.
    // Context is defined by the current position of script execution,
    // which means - script module and a function. Starting variables are searched in an
    // order reverse to the order of symbol overriding:
    // - local variables of the current scope (of any nesting level),
    // - global variables of the current script,
    // - imports from other scripts, plugins or engine itself.
    // Requirements: ScriptTOC and RTTI.
    // TODO: value format option?
    HError QueryScriptVariableInContext(const String &var_ref, VariableInfo &var_info);
}

} // namespace Engine
} // namespace AGS


#endif // __AGS_EE_DEBUG__MEMORYINSPECT_H
