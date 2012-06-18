#ifndef __AC_NONBLOCKINGSCRIPTFUNCTION_H
#define __AC_NONBLOCKINGSCRIPTFUNCTION_H

#include "acmain/ac_maindefines.h"

struct NonBlockingScriptFunction
{
    const char* functionName;
    int numParameters;
    void* param1;
    void* param2;
    bool roomHasFunction;
    bool globalScriptHasFunction;
    bool moduleHasFunction[MAX_SCRIPT_MODULES];
    bool atLeastOneImplementationExists;

    NonBlockingScriptFunction(const char*funcName, int numParams)
    {
        this->functionName = funcName;
        this->numParameters = numParams;
        atLeastOneImplementationExists = false;
        roomHasFunction = true;
        globalScriptHasFunction = true;

        for (int i = 0; i < MAX_SCRIPT_MODULES; i++)
        {
            moduleHasFunction[i] = true;
        }
    }
};



#endif // __AC_NONBLOCKINGSCRIPTFUNCTION_H