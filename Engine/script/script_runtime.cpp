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
#include "script/script_runtime.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/dynobj_manager.h"
#include "script/cc_common.h"
#include "script/script.h"
#include "script/systemimports.h"

using namespace AGS::Engine;

SystemImports simp;
SystemImports simp_for_plugin;


bool ccAddExternalStaticFunction(const String &name, ScriptAPIFunction *scfn, void *dirfn)
{
    return simp.Add(name, RuntimeScriptValue().SetStaticFunction(scfn), nullptr) != UINT32_MAX &&
        (!dirfn ||
        simp_for_plugin.Add(name, RuntimeScriptValue().SetPluginFunction(dirfn), nullptr) != UINT32_MAX);
}

bool ccAddExternalObjectFunction(const String &name, ScriptAPIObjectFunction *scfn, void *dirfn)
{
    return simp.Add(name, RuntimeScriptValue().SetObjectFunction(scfn), nullptr) != UINT32_MAX &&
        (!dirfn ||
        simp_for_plugin.Add(name, RuntimeScriptValue().SetPluginFunction(dirfn), nullptr) != UINT32_MAX);
}

bool ccAddExternalFunction(const ScFnRegister &scfnreg)
{
    String name = String::Wrapper(scfnreg.Name);
    return simp.Add(name, scfnreg.Fn, nullptr) != UINT32_MAX &&
        (scfnreg.PlFn.IsNull() ||
        simp_for_plugin.Add(name, scfnreg.PlFn, nullptr) != UINT32_MAX);
}

bool ccAddExternalPluginFunction(const String &name, void *pfn)
{
    return simp.Add(name, RuntimeScriptValue().SetPluginFunction(pfn), nullptr) != UINT32_MAX;
}

bool ccAddExternalStaticArray(const String &name, void *ptr, CCStaticObjectArray *array_mgr)
{
    return simp.Add(name, RuntimeScriptValue().SetStaticArray(ptr, array_mgr), nullptr) != UINT32_MAX;
}

bool ccAddExternalScriptObject(const String &name, void *ptr, IScriptObject *manager)
{
    return simp.Add(name, RuntimeScriptValue().SetScriptObject(ptr, manager), nullptr) != UINT32_MAX;
}

bool ccAddExternalScriptObjectHandle(const String &name, void *ptr)
{
     return simp.Add(name, RuntimeScriptValue().SetScriptObject(ptr, &GlobalStaticManager), nullptr, kScValHint_Handle) != UINT32_MAX;
}

bool ccAddExternalScriptSymbol(const String &name, const RuntimeScriptValue &prval, RuntimeScript *script)
{
    return simp.Add(name, prval, script) != UINT32_MAX;
}

void ccRemoveExternalSymbol(const String &name)
{
    simp.Remove(name);
}

void ccRemoveAllSymbols()
{
    simp.Clear();
}

void *ccGetSymbolAddress(const String &name)
{
    const ScriptImport *import = simp.GetByName(name);
    if (import)
    {
        return import->Value.Ptr;
    }
    return nullptr;
}

void *ccGetSymbolAddressForPlugin(const String &name)
{
    const ScriptImport *import = simp_for_plugin.GetByName(name);
    if (import)
    {
        return import->Value.Ptr;
    }
    else
    {
        // Also search the internal symbol table for non-function symbols
        import = simp.GetByName(name);
        if (import)
        {
            return import->Value.Ptr;
        }
    }
    return nullptr;
}

bool ccGetScriptObjectAddress(const String &name, void *&obj_out, IScriptObject *&mgr_out)
{
    const auto *imp = simp.GetByName(name);
    if (!imp)
        return false;
    if (imp->Value.Type != kScValScriptObject && imp->Value.Type != kScValPluginObject)
        return false;

    void *object;
    IScriptObject *mgr;
    if (imp->ValueHint == kScValHint_Handle)
    {
        const int32_t handle = *static_cast<const int32_t*>(imp->Value.Ptr);
        if (ccGetObjectAddressAndManagerFromHandle(handle, object, mgr) == kScValUndefined)
            return false;
    }
    else
    {
        object = imp->Value.Ptr;
        mgr = imp->Value.ObjMgr;
    }

    obj_out = object;
    mgr_out = mgr;
    return true;
}

void *ccGetScriptObjectAddress(const String &name, const String &type)
{
    void *object;
    IScriptObject *mgr;
    if (!ccGetScriptObjectAddress(name, object, mgr))
        return nullptr;

    return (type == mgr->GetType()) ? object : nullptr;
}

void *ccGetScriptObjectAddress(const String &name, const std::vector<String> &types)
{
    void *object;
    IScriptObject *mgr;
    if (!ccGetScriptObjectAddress(name, object, mgr))
        return nullptr;

    for (const auto &type : types)
    {
        if (type == mgr->GetType())
            return object;
    }
    return nullptr;
}

new_line_hook_type new_line_hook = nullptr;


void ccSetScriptAliveTimer(unsigned sys_poll_timeout, unsigned abort_timeout,
    unsigned abort_loops)
{
    scriptExecutor->SetExecTimeout(sys_poll_timeout, abort_timeout, abort_loops);
}

void ccNotifyScriptStillAlive ()
{
    scriptExecutor->NotifyAlive();
}

void ccSetDebugHook(new_line_hook_type jibble)
{
    new_line_hook = jibble;
}

void ccSetDebugLogging(bool on)
{
    ccSetOption(SCOPT_DEBUGRUN, on);
}
