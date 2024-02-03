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
#include "script/script_runtime.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "ac/dynobj/cc_dynamicarray.h"
#include "script/cc_common.h"
#include "script/systemimports.h"


bool ccAddExternalStaticFunction(const String &name, ScriptAPIFunction *scfn, void *dirfn)
{
    return simp.add(name, RuntimeScriptValue().SetStaticFunction(scfn), nullptr) != UINT32_MAX &&
        (!dirfn ||
        simp_for_plugin.add(name, RuntimeScriptValue().SetPluginFunction(dirfn), nullptr) != UINT32_MAX);
}

bool ccAddExternalObjectFunction(const String &name, ScriptAPIObjectFunction *scfn, void *dirfn)
{
    return simp.add(name, RuntimeScriptValue().SetObjectFunction(scfn), nullptr) != UINT32_MAX &&
        (!dirfn ||
        simp_for_plugin.add(name, RuntimeScriptValue().SetPluginFunction(dirfn), nullptr) != UINT32_MAX);
}

bool ccAddExternalFunction(const ScFnRegister &scfnreg)
{
    String name = String::Wrapper(scfnreg.Name);
    return simp.add(name, scfnreg.Fn, nullptr) != UINT32_MAX &&
        (scfnreg.PlFn.IsNull() ||
        simp_for_plugin.add(name, scfnreg.PlFn, nullptr) != UINT32_MAX);
}

bool ccAddExternalPluginFunction(const String &name, void *pfn)
{
    return simp.add(name, RuntimeScriptValue().SetPluginFunction(pfn), nullptr) != UINT32_MAX;
}

bool ccAddExternalStaticArray(const String &name, void *ptr, CCStaticArray *array_mgr)
{
    return simp.add(name, RuntimeScriptValue().SetStaticArray(ptr, array_mgr), nullptr) != UINT32_MAX;
}

bool ccAddExternalScriptObject(const String &name, void *ptr, IScriptObject *manager)
{
    return simp.add(name, RuntimeScriptValue().SetScriptObject(ptr, manager), nullptr) != UINT32_MAX;
}

bool ccAddExternalScriptSymbol(const String &name, const RuntimeScriptValue &prval, ccInstance *inst)
{
    return simp.add(name, prval, inst) != UINT32_MAX;
}

void ccRemoveExternalSymbol(const String &name)
{
    simp.remove(name);
}

void ccRemoveAllSymbols()
{
    simp.clear();
}

void *ccGetSymbolAddress(const String &name)
{
    const ScriptImport *import = simp.getByName(name);
    if (import)
    {
        return import->Value.Ptr;
    }
    return nullptr;
}

void *ccGetSymbolAddressForPlugin(const String &name)
{
    const ScriptImport *import = simp_for_plugin.getByName(name);
    if (import)
    {
        return import->Value.Ptr;
    }
    else
    {
        // Also search the internal symbol table for non-function symbols
        import = simp.getByName(name);
        if (import)
        {
            return import->Value.Ptr;
        }
    }
    return nullptr;
}

void *ccGetScriptObjectAddress(const String &name, const String &type)
{
    const auto *imp = simp.getByName(name);
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

int call_function(void *fn_addr, const RuntimeScriptValue *object, int numparm, const RuntimeScriptValue *parms)
{
    if (!fn_addr)
    {
        cc_error("null function pointer in call_function");
        return -1;
    }
    if (numparm > 0 && !parms)
    {
        cc_error("invalid parameters array in call_function");
        return -1;
    }

    intptr_t parm_value[9];
    if (object)
    {
        parm_value[0] = (intptr_t)object->GetPtrWithOffset();
        numparm++;
    }

    for (int ival = object ? 1 : 0, iparm = 0; ival < numparm; ++ival, ++iparm)
    {
        switch (parms[iparm].Type)
        {
        case kScValInteger:
        case kScValFloat:   // AGS passes floats, copying their values into long variable
        case kScValPluginArg:
            parm_value[ival] = (intptr_t)parms[iparm].IValue;
            break;
            break;
        default:
            parm_value[ival] = (intptr_t)parms[iparm].GetPtrWithOffset();
            break;
        }
    }

    //
    // AN IMPORTANT NOTE ON PARAM TYPE
    // of 2012-11-10
    //
    //// NOTE of 2012-12-20:
    //// Everything said below is applicable only for calling
    //// exported plugin functions.
    //
    // Here we are sending parameters of type intptr_t to registered
    // function of unknown kind. Intptr_t is 32-bit for x32 build and
    // 64-bit for x64 build.
    // The exported functions usually have two types of parameters:
    // pointer and 'int' (32-bit). For x32 build those two have the
    // same size, but for x64 build first has 64-bit size while the
    // second remains 32-bit.
    // In formal case that would cause 'overflow' - function will
    // receive more data than needed (written to stack), with some
    // values shifted further by 32 bits.
    //
    // Upon testing, however, it was revealed that AMD64 processor,
    // the only platform we support x64 Linux AGS build on right now,
    // treats all the function parameters pushed to stack as 64-bit
    // values (few first parameters are sent via registers, and hence
    // are least concern anyway). Therefore, no 'overflow' occurs,
    // and 64-bit values are being effectively truncated to 32-bit
    // integers in the callee.
    //
    // Since this is still quite unreliable, this should be
    // reimplemented when there's enough free time available for
    // developers both for coding & testing.
    //
    // Most basic idea is to pass array of RuntimeScriptValue
    // objects (that hold type description) and get same RSV as a
    // return result. Keep in mind, though, that this solution will
    // require fixing ALL exported functions, so a good amount of
    // time and energy should be allocated for this task.
    //

    switch (numparm)
    {
    case 0:
        {
            int (*fparam) ();
            fparam = (int (*)())fn_addr;
            return fparam();
        }
    case 1:
        {
            int (*fparam) (intptr_t);
            fparam = (int (*)(intptr_t))fn_addr;
            return fparam(parm_value[0]);
        }
    case 2:
        {
            int (*fparam) (intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1]);
        }
    case 3:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2]);
        }
    case 4:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2], parm_value[3]);
        }
    case 5:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4]);
        }
    case 6:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5]);
        }
    case 7:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5], parm_value[6]);
        }
    case 8:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5], parm_value[6], parm_value[7]);
        }
    case 9:
        {
            int (*fparam) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
            fparam = (int (*)(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t))fn_addr;
            return fparam(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5], parm_value[6], parm_value[7], parm_value[8]);
        }
    }

    cc_error("too many arguments in call to function");
    return -1;
}
