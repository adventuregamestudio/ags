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
#include "script/script_runtime.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "ac/dynobj/cc_dynamicarray.h"
#include "script/cc_common.h"
#include "script/systemimports.h"


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

bool ccAddExternalStaticArray(const String &name, void *ptr, CCStaticArray *array_mgr)
{
    return simp.Add(name, RuntimeScriptValue().SetStaticArray(ptr, array_mgr), nullptr) != UINT32_MAX;
}

bool ccAddExternalScriptObject(const String &name, void *ptr, IScriptObject *manager)
{
    return simp.Add(name, RuntimeScriptValue().SetScriptObject(ptr, manager), nullptr) != UINT32_MAX;
}

bool ccAddExternalScriptSymbol(const String &name, const RuntimeScriptValue &prval, ccInstance *inst)
{
    return simp.Add(name, prval, inst) != UINT32_MAX;
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

void *ccGetScriptObjectAddress(const String &name, const String &type)
{
    const auto *imp = simp.GetByName(name);
    if (!imp)
        return nullptr;
    if (imp->Value.Type != kScValScriptObject && imp->Value.Type != kScValPluginObject)
        return nullptr;
    if (type != imp->Value.ObjMgr->GetType())
        return nullptr;
    return imp->Value.Ptr;
}

new_line_hook_type new_line_hook = nullptr;


void ccSetScriptAliveTimer(unsigned sys_poll_timeout, unsigned abort_timeout,
    unsigned abort_loops)
{
    ccInstance::SetExecTimeout(sys_poll_timeout, abort_timeout, abort_loops);
}

void ccNotifyScriptStillAlive () {
    ccInstance *cur_inst = ccInstance::GetCurrentInstance();
    if (cur_inst)
        cur_inst->NotifyAlive();
}

void ccSetDebugHook(new_line_hook_type jibble)
{
    new_line_hook = jibble;
}

void ccSetDebugLogging(bool on)
{
    ccSetOption(SCOPT_DEBUGRUN, on);
}
