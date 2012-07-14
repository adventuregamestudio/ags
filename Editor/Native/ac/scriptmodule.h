#ifndef __AC_SCRIPT_H
#define __AC_SCRIPT_H

#include "ac/common_defines.h"  // PCKD
#include "script/cc_script.h"   // ccScript

extern int in_interaction_editor;  // whether to remove script functions/etc

#pragma pack(1)
struct ScriptEvent {
    long type     PCKD;   // eg. display message, or if is less
    char sort     PCKD;
    long _using   PCKD;   // ^var1
    long with     PCKD;   // number 3 than 9
    long data     PCKD;
    long branchto PCKD;
    long screeny  PCKD;
    void settype(long);
};

#define MAXINBLOCK 10
struct ScriptBlock {
    long        numevents           PCKD;
    ScriptEvent events[MAXINBLOCK]  PCKD;
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
    ccScript *compiled;

    void init();

    ScriptModule() { init(); }
};

#endif // __AC_SCRIPT_H