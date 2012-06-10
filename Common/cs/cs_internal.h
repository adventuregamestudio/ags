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

#ifndef __CS_INTERNAL_H
#define __CS_INTERNAL_H

// *
// ***************** INTERNAL USE ONLY *******************
// *

// [IKM] 2012-06-10: What does 'internal' mean here? these functions
// are used all around the place, both in compiler and runtime.

extern void cc_error(char *, ...);
extern int currentline;


extern void fputstring(char *sss, FILE *ddd);
extern void fgetstring_limit(char *sss, FILE *ddd, int bufsize);
extern void fgetstring(char *sss, FILE *ddd);




//=============================================================================
#include "cc_treemap.h"
//=============================================================================


static char *sccmdnames[] = {
    "NULL", "$add", "$sub", "$$mov", "memwritelit", "ret", "$mov",
    "$memread", "$memwrite", "$$mul", "$$div", "$$add", "$$sub", "$$bit_and", "$$bit_or",
    "$$cmp", "$$ncmp", "$$gt", "$$lt", "$$gte", "$$lte", "$$and", "$$or",
    "$call", "$memread.b", "$memread.w", "$memwrite.b", "$memwrite.w", "jz",
    "$push", "$pop", "jmp", "$mul", "$farcall", "$farpush", "farsubsp", "sourceline",
    "$callscr", "thisaddr", "setfuncargs", "$$mod", "$$xor", "$not",
    "$$shl", "$$shr", "$callobj", "$checkbounds", "$memwrite.ptr",
    "$memread.ptr", "memwrite.ptr.0", "$meminit.ptr", "load.sp.offs",
    "checknull.ptr", "$f.add", "$f.sub", "$$f.mul", "$$f.div", "$$f.add",
    "$$f.sub", "$$f.gt", "$$f.lt", "$$f.gte", "$$f.lte",
    "zeromem", "$newstring", "$$strcmp", "$$strnotcmp", "$checknull",
    "loopcheckoff", "memwrite.ptr.0.nd", "jnz", "$dynamicbounds", "$newarray"
};

static char *regnames[] = { "null", "sp", "mar", "ax", "bx", "cx", "op", "dx" };
static short sccmdargs[] = {
    0, 2, 2, 2, 2, 0, 2,
    1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 2, 1, 1, 1, 1,
    1, 1, 1, 2, 2, 1,
    2, 2, 1, 2, 1,
    1, 0, 1, 1,
    0, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2,
    1, 1, 2, 2, 1,
    0, 0, 1, 1, 3
};

// file signatures
static char scfilesig[5] = "SCOM";
#define ENDFILESIG 0xbeefcafe


#endif // __CS_INTERNAL_H