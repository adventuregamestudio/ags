#ifndef __CC_VARIABLESYMLIST_H
#define __CC_VARIABLESYMLIST_H

struct VariableSymlist {
    int len;
    long *syml;

    void init(int pLen) {
        len = pLen;
        syml = (long*)malloc(sizeof(long) * len);
    }
    void destroy() {
        free(syml);
        syml = NULL;
    }
};
#define MAX_VARIABLE_PATH 10

#endif // __CC_VARIABLESYMLIST_H