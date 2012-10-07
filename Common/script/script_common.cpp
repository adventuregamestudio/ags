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

#include "script/script_common.h"

int currentline;

const char *sccmdnames[] = {
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

const char *regnames[] = { "null", "sp", "mar", "ax", "bx", "cx", "op", "dx" };
const short sccmdargs[] = {
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
const char scfilesig[5] = "SCOM";
