//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// C-Script run-time interpreter (c) 2001 Chris Jones
//
// You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
// when compiling this, or strange results can happen.
//
// There is a problem with importing functions on 16-bit compilers: the
// script system assumes that all parameters are passed as 4 bytes, which
// ints are not on 16-bit systems. Be sure to define all parameters as longs,
// or join the 21st century and switch to DJGPP or Visual C++.
//
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "script/script_runtime.h"
#include "script/script_common.h"
#include "script/cc_error.h"
#include "script/cc_options.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/managedobjectpool.h"
#include "script/spans.h"
#include "script/systemimports.h"
#include "ac/statobj/staticobject.h"

extern ccInstance *current_instance; // in script/cc_instance

/*
bool ccAddExternalData(const char *name, void *ptr)
{
    return simp.add(kScImportData, name, ptr, NULL, NULL) == 0;
}
*/

bool ccAddExternalStaticFunction(const char *name, void *ptr)
{
    return simp.add(kScImportStaticFunction, name, ptr, NULL, NULL) == 0;
}

bool ccAddExternalStaticObject(const char *name, void *ptr, ICCStaticObject *manager)
{
    return simp.add(kScImportStaticObject, name, ptr, manager, NULL) == 0;
}

bool ccAddExternalDynamicObject(const char *name, void *ptr, ICCDynamicObject *manager)
{
    return simp.add(kScImportDynamicObject, name, ptr, manager, NULL) == 0;
}

bool ccAddExternalObjectFunction(const char *name, void *ptr)
{
    return simp.add(kScImportObjectFunction, name, ptr, NULL, NULL) == 0;
}

bool ccAddExternalScriptSymbol(const char *name, void *ptr, ccInstance *inst)
{
    return simp.add(kScImportScriptData, name, ptr, NULL, inst) == 0;
}

void ccRemoveExternalSymbol(const char *namof)
{
    simp.remove(namof);
}

void ccRemoveAllSymbols()
{
    simp.clear();
}

ccInstance *loadedInstances[MAX_LOADED_INSTANCES] = {NULL,
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL, NULL, NULL};

void nullfree(void *data)
{
    if (data != NULL)
        free(data);
}

void *ccGetSymbolAddress(char *namof)
{
    const ScriptImport *import = simp.getByName(namof);
    if (import)
    {
        return import->Ptr;
    }
    return NULL;
}

new_line_hook_type new_line_hook = NULL;

char ccRunnerCopyright[] = "ScriptExecuter32 v" SCOM_VERSIONSTR " (c) 2001 Chris Jones";
int maxWhileLoops = 0;

// If a while loop does this many iterations without the
// NofityScriptAlive function getting called, the script
// aborts. Set to 0 to disable.
void ccSetScriptAliveTimer (int numloop) {
    maxWhileLoops = numloop;
}

void ccNotifyScriptStillAlive () {
    if (current_instance != NULL)
        current_instance->flags |= INSTF_RUNNING;
}

void ccSetDebugHook(new_line_hook_type jibble)
{
    new_line_hook = jibble;
}

// parm list is backwards (last arg is parms[0])
int call_function(long addr, int numparm, const RuntimeScriptValue *parms, int offset)
{
    if (!addr)
    {
        cc_error("null function pointer in call_function");
        return -1;
    }
    if (numparm > 0 && !parms)
    {
        cc_error("invalid parameters array in call_function");
        return -1;
    }

    parms += offset;

    long parm_value[9];
    for (int i = 0; i < numparm; ++i)
    {
        const RuntimeScriptValue *real_param;
        if (parms[i].GetType() == kScValStackPtr)
        {
            // There's at least one known case when this may be a stack pointer:
            // AGS 2.x style local strings that have their address pushed to stack
            // after array of chars; in the new interpreter implementation we push
            // these addresses as runtime values of stack ptr type to keep correct
            // value size.
            // It is not a good idea to pass stack ptr to function, pass the value
            // it points to instead.
            real_param = parms[i].GetStackEntry();
        }
        else
        {
            real_param = &parms[i];
        }

        // NOTE: in case of generic type this will return just Value
        parm_value[i] = (intptr_t)real_param->GetDataPtrWithOffset();
    }

    if (numparm == 1) {
        int (*fparam) (long);
        fparam = (int (*)(long))addr;
        return fparam(parm_value[0]);
    }

    if (numparm == 2) {
        int (*fparam) (long, long);
        fparam = (int (*)(long, long))addr;
        return fparam(parm_value[1], parm_value[0]);
    }

    if (numparm == 3) {
        int (*fparam) (long, long, long);
        fparam = (int (*)(long, long, long))addr;
        return fparam(parm_value[2], parm_value[1], parm_value[0]);
    }

    if (numparm == 4) {
        int (*fparam) (long, long, long, long);
        fparam = (int (*)(long, long, long, long))addr;
        return fparam(parm_value[3], parm_value[2], parm_value[1], parm_value[0]);
    }

    if (numparm == 5) {
        int (*fparam) (long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long))addr;
        return fparam(parm_value[4], parm_value[3], parm_value[2], parm_value[1], parm_value[0]);
    }

    if (numparm == 6) {
        int (*fparam) (long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long))addr;
        return fparam(parm_value[5], parm_value[4], parm_value[3], parm_value[2], parm_value[1], parm_value[0]);
    }

    if (numparm == 7) {
        int (*fparam) (long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long))addr;
        return fparam(parm_value[6], parm_value[5], parm_value[4], parm_value[3], parm_value[2], parm_value[1], parm_value[0]);
    }

    if (numparm == 8) {
        int (*fparam) (long, long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long, long))addr;
        return fparam(parm_value[7], parm_value[6], parm_value[5], parm_value[4], parm_value[3], parm_value[2], parm_value[1], parm_value[0]);
    }

    if (numparm == 9) {
        int (*fparam) (long, long, long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long, long, long))addr;
        return fparam(parm_value[8], parm_value[7], parm_value[6], parm_value[5], parm_value[4], parm_value[3], parm_value[2], parm_value[1], parm_value[0]);
    }

    cc_error("too many arguments in call to function");
    return -1;
}
