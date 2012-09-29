/*
C-Script run-time interpreter (c) 2001 Chris Jones

You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
when compiling this, or strange results can happen.

There is a problem with importing functions on 16-bit compilers: the
script system assumes that all parameters are passed as 4 bytes, which
ints are not on 16-bit systems. Be sure to define all parameters as longs,
or join the 21st century and switch to DJGPP or Visual C++.

This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

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

extern ccInstance *current_instance; // in script/cc_instance

#ifdef AGS_BIG_ENDIAN
Spans gSpans;
#endif

/*
void ccAddExternalSymbol(char *namof, void *addrof)
{
    simp.add(namof, (char *)addrof, NULL);
}
*/

bool ccAddExternalData(const char *name, void *ptr)
{
    return simp.add(kScImportData, name, ptr, NULL) == 0;
}

bool ccAddExternalStaticFunction(const char *name, void *ptr)
{
    return simp.add(kScImportStaticFunction, name, ptr, NULL) == 0;
}

bool ccAddExternalObject(const char *name, void *ptr)
{
    return simp.add(kScImportObject, name, ptr, NULL) == 0;
}

bool ccAddExternalObjectFunction(const char *name, void *ptr)
{
    return simp.add(kScImportObjectFunction, name, ptr, NULL) == 0;
}

bool ccAddExternalScriptSymbol(const char *name, void *ptr, ccInstance *inst)
{
    return simp.add(kScImportScriptData, name, ptr, inst) == 0;
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
int call_function(long addr, int numparm, long *parms, int offset)
{
    parms += offset;

    if (numparm == 1) {
        int (*fparam) (long);
        fparam = (int (*)(long))addr;
        return fparam(parms[0]);
    }

    if (numparm == 2) {
        int (*fparam) (long, long);
        fparam = (int (*)(long, long))addr;
        return fparam(parms[1], parms[0]);
    }

    if (numparm == 3) {
        int (*fparam) (long, long, long);
        fparam = (int (*)(long, long, long))addr;
        return fparam(parms[2], parms[1], parms[0]);
    }

    if (numparm == 4) {
        int (*fparam) (long, long, long, long);
        fparam = (int (*)(long, long, long, long))addr;
        return fparam(parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 5) {
        int (*fparam) (long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long))addr;
        return fparam(parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 6) {
        int (*fparam) (long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long))addr;
        return fparam(parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 7) {
        int (*fparam) (long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long))addr;
        return fparam(parms[6], parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 8) {
        int (*fparam) (long, long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long, long))addr;
        return fparam(parms[7], parms[6], parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 9) {
        int (*fparam) (long, long, long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long, long, long))addr;
        return fparam(parms[8], parms[7], parms[6], parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    cc_error("too many arguments in call to function");
    return -1;
}
