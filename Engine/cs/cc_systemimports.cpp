
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_systemimports.h"


extern void quit(char *);

struct CompareStringsPartial : ICompareStrings {
    virtual int compare(const char *left, const char *right) {
        return strncmp(left, right, strlen(left));
    }
};
CompareStringsPartial ccCompareStringsPartial;

SystemImports simp;

/*
void SystemImports::remove_all_script_exports()
{
int o;

// Cut off at the first script exports - used to reset script system
// Although FreeInstance removes them anyway, we might be
// in AbortAndDestroy so we can't free it yet
for (o = 0; o < numimports; o++) {
if (isScriptImp[o]) {
numimports = o;
break;
}
}

}*/

int SystemImports::add(char *namm, char *add, ccInstance *anotherscr = NULL)
{
    int ixof;

    if ((ixof = get_index_of(namm)) >= 0) {
        // Only allow override if not a script-exported function
        if (anotherscr == NULL) {
            addr[ixof] = add;
            isScriptImp[ixof] = anotherscr;
        }
        return 0;
    }

    ixof = numimports;
    for (int ii = 0; ii < numimports; ii++) {
        if (name[ii] == NULL) {
            ixof = ii;
            break;
        }
    }

    if (ixof >= this->bufferSize)
    {
        if (this->bufferSize > 50000)
            return -1;  // something has gone badly wrong
        this->bufferSize += 1000;
        this->name = (char**)realloc(this->name, sizeof(char*) * this->bufferSize);
        this->addr = (char**)realloc(this->addr, sizeof(char*) * this->bufferSize);
        this->isScriptImp = (ccInstance**)realloc(this->isScriptImp, sizeof(ccInstance*) * this->bufferSize);
    }

    btree.addEntry(namm, ixof);
    name[ixof] = namm;
    addr[ixof] = add;
    isScriptImp[ixof] = anotherscr;

    if (ixof == numimports)
        numimports++;
    return 0;
}

void SystemImports::remove(char *nameToRemove) {
    int idx = get_index_of(nameToRemove);
    if (idx < 0)
        return;
    btree.removeEntry(name[idx]);
    name[idx] = NULL;
    addr[idx] = NULL;
    isScriptImp[idx] = 0;
    /*numimports--;
    for (int ii = idx; ii < numimports; ii++) {
    this->name[ii] = this->name[ii + 1];
    addr[ii] = addr[ii + 1];
    isScriptImp[ii] = isScriptImp[ii + 1];
    }*/
}

char *SystemImports::get_addr_of(char *namw)
{
    int o = get_index_of(namw);
    if (o < 0)
        return NULL;

    return addr[o];
}

int SystemImports::get_index_of(char *namw)
{
    int bestMatch = -1;
    char altName[200];
    sprintf(altName, "%s$", namw);

    int idx = btree.findValue((const char*)namw);
    if (idx >= 0)
        return idx;

    // if it's a function with a mangled name, allow it
    idx = btree.findValue(altName, &ccCompareStringsPartial);
    if (idx >= 0)
        return idx;

    /*
    int o;
    for (o = 0; o < numimports; o++) {
    if (strcmp(name[o], namw) == 0)
    return o;
    // if it's a function with a mangled name, allow it
    if (strncmp(name[o], altName, strlen(altName)) == 0)
    return o;
    }*/

    if ((strlen(namw) > 3) && 
        ((namw[strlen(namw) - 2] == '^') || (namw[strlen(namw) - 3] == '^'))) {
            // Function with number of prametrs on the end
            // attempt to find it without the param count
            strcpy(altName, namw);
            strrchr(altName, '^')[0] = 0;

            return get_index_of(altName);
    }

    return -1;
}

ccInstance* SystemImports::is_script_import(char *namw)
{
    if (namw == NULL) {
        quit("is_script_import: NULL pointer passed");
    }

    int idx = get_index_of(namw);
    if (idx < 0)
        return NULL;

    return isScriptImp[idx];
}

// Remove all symbols whose addresses are in the supplied range
void SystemImports::remove_range(char *from, unsigned long dist)
{
    unsigned long startaddr = (unsigned long)from;
    for (int o = 0; o < numimports; o++) {
        if (name[o] == NULL)
            continue;

        unsigned long thisaddr = (unsigned long)addr[o];
        if ((thisaddr >= startaddr) && (thisaddr < startaddr + dist)) {
            btree.removeEntry(name[o]);
            name[o] = NULL;
            addr[o] = NULL;
            isScriptImp[o] = 0;
            /*numimports--;
            for (int p = o; p < numimports; p++) {
            name[p] = name[p + 1];
            addr[p] = addr[p + 1];
            isScriptImp[p] = isScriptImp[p + 1];
            }
            o--;*/
        }
    }
}
