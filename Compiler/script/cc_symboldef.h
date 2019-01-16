#ifndef __CC_SYMBOLDEF_H
#define __CC_SYMBOLDEF_H

#include "cs_parser_common.h"   // macro definitions

// Types moved to cc_symboltable.h

struct SymbolDef { // deprecated, use structure in cc_symboltable.h instead
    short stype;
    long  flags;
    long  ssize;  // or return type size for function
    short sscope;  // or num arguments for function
    long  arrsize;
    unsigned long funcparamtypes[MAX_FUNCTION_PARAMETERS + 1];
    int funcParamDefaultValues[MAX_FUNCTION_PARAMETERS + 1];
    bool funcParamHasDefaultValues[MAX_FUNCTION_PARAMETERS + 1];
};

#endif // __CC_SYMBOLDEF_H
