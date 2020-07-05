#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H

#include "cs_parser_common.h"

struct VariableSymlist {
    int len;
    AGS::SymbolScript syml;
    void init(int pLen)
    {
        len = pLen;
        syml = static_cast<AGS::SymbolScript>(malloc(sizeof(AGS::Symbol) * len));
    }

    void destroy()
    {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H
