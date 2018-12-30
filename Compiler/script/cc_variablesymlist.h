#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H
#include <vector>

struct VariableSymlist {
    int len;
    ags::SymbolScript_t syml;
    void init(int pLen)
    {
        len = pLen;
        syml = static_cast<ags::SymbolScript_t>(malloc(sizeof(ags::Symbol_t) * len));
    }

    void destroy()
    {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H