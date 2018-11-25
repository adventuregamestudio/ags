#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H
#include <vector>

struct VariableSymlist {
    int len;
    ags::SymbolScript syml;
    void init(int pLen)
    {
        len = pLen;
        syml = static_cast<ags::SymbolScript>(malloc(sizeof(ags::Symbol) * len));
    }

    void destroy()
    {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H