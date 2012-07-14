
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
