#ifndef __CC_SYMBOLDEF_H
#define __CC_SYMBOLDEF_H

#include "cs_parser_common.h"   // macro definitions

#define EXPORT_FUNCTION   1
#define EXPORT_DATA       2

#define STYPE_DYNARRAY  0x10000000
#define STYPE_CONST     0x20000000
#define STYPE_POINTER   0x40000000
#define SYM_TEMPORARYTYPE -99
struct SymbolDef {
    short stype;
    long  flags;
    long  ssize;  // or return type size for function
    short sscope;  // or num arguments for function
    long  arrsize;
    unsigned long funcparamtypes[MAX_FUNCTION_PARAMETERS+1]; 
    short funcParamDefaultValues[MAX_FUNCTION_PARAMETERS+1]; 
};

#endif // __CC_SYMBOLDEF_H