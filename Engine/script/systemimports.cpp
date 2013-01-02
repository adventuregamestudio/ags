//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "script/systemimports.h"


extern void quit(char *);

struct CompareStringsPartial : ICompareStrings {
    virtual int compare(const char *left, const char *right) {
        return strncmp(left, right, strlen(left));
    }
};
CompareStringsPartial ccCompareStringsPartial;

SystemImports simp;

int SystemImports::add(const char *name, const RuntimeScriptValue &value, ccInstance *anotherscr)
{
    int ixof;

    if ((ixof = get_index_of(name)) >= 0) {
        // Only allow override if not a script-exported function
        if (anotherscr == NULL) {
            imports[ixof].Value = value;
            imports[ixof].InstancePtr = anotherscr;
        }
        return 0;
    }

    ixof = numimports;
    for (int ii = 0; ii < numimports; ii++) {
        if (imports[ii].Name == NULL) {
            ixof = ii;
            break;
        }
    }

    if (ixof >= this->bufferSize)
    {
        if (this->bufferSize > 50000)
            return -1;  // something has gone badly wrong
        this->bufferSize += 1000;
        this->imports = (ScriptImport*)realloc(this->imports, sizeof(ScriptImport) * this->bufferSize);
    }

    btree.addEntry(name, ixof);
    imports[ixof].Name          = name; // TODO: rather make a string copy here for safety reasons
    imports[ixof].Value         = value;
    imports[ixof].InstancePtr   = anotherscr;

    if (ixof == numimports)
        numimports++;
    return 0;
}

void SystemImports::remove(const char *nameToRemove) {
    int idx = get_index_of(nameToRemove);
    if (idx < 0)
        return;
    btree.removeEntry(imports[idx].Name);
    imports[idx].Name = NULL;
    imports[idx].Value.Invalidate();
    imports[idx].InstancePtr = NULL;
}

const ScriptImport *SystemImports::getByName(const char *name)
{
    int o = get_index_of(name);
    if (o < 0)
        return NULL;

    return &imports[o];
}

const ScriptImport *SystemImports::getByIndex(int index)
{
    if (index < 0 || index > numimports)
    {
        return NULL;
    }
    return &imports[index];
}

int SystemImports::get_index_of(const char *namw)
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

void SystemImports::RemoveScriptExports(ccInstance *inst)
{
    if (!inst)
    {
        return;
    }

    for (int i = 0; i < numimports; ++i)
    {
        if (imports[i].Name == NULL)
            continue;

        if (imports[i].InstancePtr == inst)
        {
            btree.removeEntry(imports[i].Name);
            imports[i].Name = NULL;
            imports[i].Value.Invalidate();
            imports[i].InstancePtr = 0;
        }
    }
}
