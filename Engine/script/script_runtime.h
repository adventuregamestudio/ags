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
#ifndef __AGS_EE_CC__SCRIPTRUNTIME_H
#define __AGS_EE_CC__SCRIPTRUNTIME_H

#include "util/memory_compat.h"    // std::size
#include "script/cc_script.h"      // ccScript
#include "script/cc_instance.h"    // ccInstance

struct IScriptObject;

using AGS::Common::String;

// Helper struct for declaring a script API function registration
struct ScFnRegister
{
    const char *Name = nullptr;
    RuntimeScriptValue Fn; // for script VM calls
    RuntimeScriptValue PlFn; // for plugins, direct unsafe call style

    ScFnRegister() = default;
    ScFnRegister(const char *name, ScriptAPIFunction *fn, void *plfn = nullptr)
        : Name(name)
        , Fn(RuntimeScriptValue().SetStaticFunction(fn))
        , PlFn(RuntimeScriptValue().SetPluginFunction(plfn)) {}
    ScFnRegister(const char *name, ScriptAPIObjectFunction *fn, void *plfn = nullptr)
        : Name(name)
        , Fn(RuntimeScriptValue().SetObjectFunction(fn))
        , PlFn(RuntimeScriptValue().SetPluginFunction(plfn)) {}
    template <typename TPlFn>
    ScFnRegister(const char *name, ScriptAPIFunction *fn, TPlFn plfn)
        : Name(name)
        , Fn(RuntimeScriptValue().SetStaticFunction(fn))
        , PlFn(RuntimeScriptValue().SetPluginFunction(reinterpret_cast<void*>(plfn))) {}
    template <typename TPlFn>
    ScFnRegister(const char *name, ScriptAPIObjectFunction *fn, TPlFn plfn)
        : Name(name)
        , Fn(RuntimeScriptValue().SetObjectFunction(fn))
        , PlFn(RuntimeScriptValue().SetPluginFunction(reinterpret_cast<void*>(plfn))) {}
};

// Following two functions register engine API symbols for script and plugins.
// Calls from script is handled by specific "translator" functions, which
// unpack script interpreter's values into the real arguments and call the
// actual engine's function. For plugins we have to provide actual engine
// function directly (dirfn parameter).
bool ccAddExternalStaticFunction(const String &name, ScriptAPIFunction *scfn, void *dirfn = nullptr);
bool ccAddExternalObjectFunction(const String &name, ScriptAPIObjectFunction *scfn, void *dirfn = nullptr);
bool ccAddExternalFunction(const ScFnRegister &scfnreg);
// Register a function, exported from a plugin. Requires direct function pointer only.
bool ccAddExternalPluginFunction(const String &name, void *pfn);
// Register engine objects for script's access.
bool ccAddExternalStaticArray(const String &name, void *ptr, CCStaticArray *array_mgr);
bool ccAddExternalScriptObject(const String &name, void *ptr, IScriptObject *manager);
// Register script own functions (defined in the linked scripts)
bool ccAddExternalScriptSymbol(const String &name, const RuntimeScriptValue &prval, ccInstance *inst);
// Remove the script access to a variable or function in your program
void ccRemoveExternalSymbol(const String &name);
// Remove all external symbols, allowing you to start from scratch
void ccRemoveAllSymbols();

// Registers an array of static functions
template <size_t N>
inline void ccAddExternalFunctions(const ScFnRegister (&arr)[N])
{
    for (const ScFnRegister *it = arr; it != (arr + std::size(arr)); ++it)
        ccAddExternalFunction(*it);
}

// Get the address of an exported variable in the script
void *ccGetSymbolAddress(const String &name);
// Get a registered symbol's direct pointer; this is used solely for plugins
void *ccGetSymbolAddressForPlugin(const String &name);
// Get a registered Script Object, optionally restricting to the given type name
void *ccGetScriptObjectAddress(const String &name, const String &type);

// DEBUG HOOK
typedef void (*new_line_hook_type) (ccInstance *, int);
void ccSetDebugHook(new_line_hook_type jibble);

// Set the script interpreter timeout values:
// * sys_poll_timeout - defines the timeout (ms) at which the interpreter will run system events poll;
// * abort_timeout - [temp disabled] defines the timeout (ms) at which the interpreter will cancel with error.
// * abort_loops - max script loops without an engine update after which the interpreter will error;
void ccSetScriptAliveTimer(unsigned sys_poll_timeout, unsigned abort_timeout, unsigned abort_loops);
// reset the current while loop counter
void ccNotifyScriptStillAlive();
// for calling exported plugin functions old-style
int call_function(void *fn_addr, const RuntimeScriptValue *object, int numparm, const RuntimeScriptValue *parms);

#endif // __AGS_EE_CC__SCRIPTRUNTIME_H
