//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_SCRIPT_H
#define __AC_SCRIPT_H

#include "script/cc_script.h"   // ccScript

extern int in_interaction_editor;  // whether to remove script functions/etc

#pragma pack(1)
struct ScriptEvent {
    long type     ;   // eg. display message, or if is less
    char sort     ;
    long _using   ;   // ^var1
    long with     ;   // number 3 than 9
    long data     ;
    long branchto ;
    long screeny  ;
    void settype(long);
};

#define MAXINBLOCK 10
struct ScriptBlock {
    long        numevents           ;
    ScriptEvent events[MAXINBLOCK]  ;
};
#pragma pack()


// permission flags
#define SMP_NOEDITINFO    1
#define SMP_NOEDITSCRIPTS 2
struct ScriptModule {
    char *name;
    char *author;
    char *version;
    char *description;
    char *scriptHeader;
    char *script;
    int  uniqueKey;
    int  permissions;
    int  weAreOwner;
    PScript compiled;

    void init();

    ScriptModule() { init(); }
};

#endif // __AC_SCRIPT_H