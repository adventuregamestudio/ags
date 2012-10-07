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

#ifndef __CC_SYSTEMIMPORTS_H
#define __CC_SYSTEMIMPORTS_H

#include "script/cc_instance.h"    // ccInstance
#include "script/cc_treemap.h"     // ccTreeMap

struct SystemImports
{
private:
    char **name;
    char **addr;
    ccInstance **isScriptImp;
    int numimports;
    int bufferSize;
    ccTreeMap btree;

public:
    int  add(char *, char *, ccInstance*);
    void remove(char *);
    char *get_addr_of(char *);
    int  get_index_of(char *);
    ccInstance* is_script_import(char *);
    void remove_range(char *, unsigned long);
    void clear() {
        numimports = 0;
        btree.clear();
    }
    //  void remove_all_script_exports();

    SystemImports()
    {
        numimports = 0;
        bufferSize = 0;
        name = NULL;
        addr = NULL;
        isScriptImp = NULL;
    }
};

extern SystemImports simp;

#endif  // __CC_SYSTEMIMPORTS_H