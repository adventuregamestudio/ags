/*
** 'C'-style script compiler
** Copyright (C) 2000-2001, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __CC_SCRIPT_H
#define __CC_SCRIPT_H

#include "platform/file.h"

struct ccScript
{
    char *globaldata;
    long globaldatasize;
    long *code;
    long codesize;
    char *strings;
    long stringssize;
    char *fixuptypes;             // global data/string area/ etc
    long *fixups;                 // code array index to fixup (in longs)
    int numfixups;
    int importsCapacity;
    char **imports;
    int numimports;
    int exportsCapacity;
    char **exports;   // names of exports
    long *export_addr;        // high byte is type; low 24-bits are offset
    int numexports;
    int instances;
    // 'sections' allow the interpreter to find out which bit
    // of the code came from header files, and which from the main file
    char **sectionNames;
    long *sectionOffsets;
    int numSections;
    int capacitySections;
};

// write the script to disk (after compiling)
extern void fwrite_script(ccScript *, FILE *);
// read back a script written with fwrite_script
extern ccScript *fread_script(FILE *);
// free the memory occupied by the script - do NOT attempt to run the
// script after calling this function
extern void ccFreeScript(ccScript *);

#endif // __CC_SCRIPT_H