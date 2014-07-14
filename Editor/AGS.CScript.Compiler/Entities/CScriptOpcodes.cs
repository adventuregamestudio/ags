using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal enum Opcodes
    {
        None = 0,
        SCMD_ADD = 1,     // reg1 += arg2
        SCMD_SUB = 2,     // reg1 -= arg2
        SCMD_REGTOREG = 3,     // reg2 = reg1
        SCMD_WRITELIT = 4,     // m[MAR] = arg2 (copy arg1 bytes)
        SCMD_RET = 5,     // return from subroutine
        SCMD_LITTOREG = 6,     // set reg1 to literal value arg2
        SCMD_MEMREAD = 7,     // reg1 = m[MAR]
        SCMD_MEMWRITE = 8,     // m[MAR] = reg1
        SCMD_MULREG = 9,     // reg1 *= reg2
        SCMD_DIVREG = 10,    // reg1 /= reg2
        SCMD_ADDREG = 11,    // reg1 += reg2
        SCMD_SUBREG = 12,    // reg1 -= reg2
        SCMD_BITAND = 13,    // bitwise  reg1 & reg2
        SCMD_BITOR = 14,    // bitwise  reg1 | reg2
        SCMD_ISEQUAL = 15,    // reg1 == reg2   reg1=1 if true, =0 if not
        SCMD_NOTEQUAL = 16,    // reg1 != reg2
        SCMD_GREATER = 17,    // reg1 > reg2
        SCMD_LESSTHAN = 18,    // reg1 < reg2
        SCMD_GTE = 19,    // reg1 >= reg2
        SCMD_LTE = 20,    // reg1 <= reg2
        SCMD_AND = 21,    // (reg1!=0) && (reg2!=0) -> reg1
        SCMD_OR = 22,    // (reg1!=0) || (reg2!=0) -> reg1
        SCMD_CALL = 23,    // jump to subroutine at reg1
        SCMD_MEMREADB = 24,    // reg1 = m[MAR] (1 byte)
        SCMD_MEMREADW = 25,    // reg1 = m[MAR] (2 bytes)
        SCMD_MEMWRITEB = 26,    // m[MAR] = reg1 (1 byte)
        SCMD_MEMWRITEW = 27,    // m[MAR] = reg1 (2 bytes)
        SCMD_JZ = 28,    // jump if ax==0 to arg1
        SCMD_PUSHREG = 29,    // m[sp]=reg1; sp++
        SCMD_POPREG = 30,    // sp--; reg1=m[sp]
        SCMD_JMP = 31,    // jump to arg1
        SCMD_MUL = 32,    // reg1 *= arg2
        SCMD_CALLEXT = 33,    // call external (imported) function reg1
        SCMD_PUSHREAL = 34,    // push reg1 onto real stack
        SCMD_SUBREALSTACK = 35,
        SCMD_LINENUM = 36,    // debug info - source code line number
        SCMD_CALLAS = 37,    // call external script function
        SCMD_THISBASE = 38,    // current relative address
        SCMD_NUMFUNCARGS = 39,    // number of arguments for ext func call
        SCMD_MODREG = 40,    // reg1 %= reg2
        SCMD_XORREG = 41,    // reg1 ^= reg2
        SCMD_NOTREG = 42,    // reg1 = !reg1
        SCMD_SHIFTLEFT = 43,    // reg1 = reg1 << reg2
        SCMD_SHIFTRIGHT = 44,    // reg1 = reg1 >> reg2
        SCMD_CALLOBJ = 45,    // next call is member function of reg1
        SCMD_CHECKBOUNDS = 46,    // check reg1 is between 0 and arg2
        SCMD_MEMWRITEPTR = 47,    // m[MAR] = reg1 (adjust ptr addr)
        SCMD_MEMREADPTR = 48,    // reg1 = m[MAR] (adjust ptr addr)
        SCMD_MEMZEROPTR = 49,    // m[MAR] = 0    (blank ptr)
        SCMD_MEMINITPTR = 50,    // m[MAR] = reg1 (but don't free old one)
        SCMD_LOADSPOFFS = 51,    // MAR = SP - arg1 (optimization for local var access)
        SCMD_CHECKNULL = 52,    // error if MAR==0
        SCMD_FADD = 53,    // reg1 += arg2 (float,int)
        SCMD_FSUB = 54,    // reg1 -= arg2 (float,int)
        SCMD_FMULREG = 55,    // reg1 *= reg2 (float)
        SCMD_FDIVREG = 56,    // reg1 /= reg2 (float)
        SCMD_FADDREG = 57,    // reg1 += reg2 (float)
        SCMD_FSUBREG = 58,    // reg1 -= reg2 (float)
        SCMD_FGREATER = 59,    // reg1 > reg2 (float)
        SCMD_FLESSTHAN = 60,    // reg1 < reg2 (float)
        SCMD_FGTE = 61,    // reg1 >= reg2 (float)
        SCMD_FLTE = 62,    // reg1 <= reg2 (float)
        SCMD_ZEROMEMORY = 63,    // m[MAR]..m[MAR+(arg1-1)] = 0
        SCMD_CREATESTRING = 64,    // reg1 = new String(reg1)
        SCMD_STRINGSEQUAL = 65,    // (char*)reg1 == (char*)reg2   reg1=1 if true, =0 if not
        SCMD_STRINGSNOTEQ = 66,    // (char*)reg1 != (char*)reg2
        SCMD_CHECKNULLREG = 67,    // error if reg1 == NULL
        SCMD_LOOPCHECKOFF = 68,    // no loop checking for this function
        SCMD_MEMZEROPTRND = 69,    // m[MAR] = 0    (blank ptr, no dispose if = ax)
        SCMD_JNZ = 70,    // jump to arg1 if ax!=0
        SCMD_DYNAMICBOUNDS = 71,   // check reg1 is between 0 and m[MAR-4]
        SCMD_NEWARRAY = 72    // reg1 = new array of reg1 elements, each of size arg2 (arg3=managed type?)
    }
}
