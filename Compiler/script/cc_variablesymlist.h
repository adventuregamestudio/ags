#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H

struct VariableSymlist {
    int len;
    int32_t *syml;

    void init(int pLen) {
        len = pLen;
        syml = (int32_t*)malloc(sizeof(int32_t) * len);
    }
    void destroy() {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H