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
#include "script/cc_instance.h"    // ccInstance

struct IScriptObject;

using AGS::Common::String;

struct ScriptImport
{
    ScriptImport()
    {
        InstancePtr = nullptr;
    }

    String              Name;           // import's uid
    RuntimeScriptValue  Value;
    ccInstance          *InstancePtr;   // script instance
};

struct SystemImports
{
private:
    // Note we can't use a hash-map here, because we sometimes need to search
    // by partial keys.
    typedef std::map<String, uint32_t> IndexMap;

    std::vector<ScriptImport> imports;
    IndexMap btree;

public:
    uint32_t add(const String &name, const RuntimeScriptValue &value, ccInstance *inst);
    void remove(const String &name);
    const ScriptImport *getByName(const String &name);
    uint32_t get_index_of(const String &name);
    const ScriptImport *getByIndex(uint32_t index);
    String findName(const RuntimeScriptValue &value);
    void RemoveScriptExports(ccInstance *inst);
    void clear();
};

extern SystemImports simp;
// This is to register symbols exclusively for plugins, to allow them
// perform old style unsafe function calls
extern SystemImports simp_for_plugin;

#endif  // __CC_SYSTEMIMPORTS_H
