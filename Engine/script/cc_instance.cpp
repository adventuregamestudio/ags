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
#include <string.h>
#include "util/wgt2allg.h" // few allegro-specific types
#include "ac/common.h"
#include "ac/event.h"
#include "ac/mouse.h"
#include "ac/roomstruct.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/managedobjectpool.h"
#include "script/cc_error.h"
#include "script/cc_instance.h"
#include "debug/debug_log.h"
#include "script/cc_options.h"
#include "script/executingscript.h"
#include "script/script.h"
#include "script/script_runtime.h"
#include "script/systemimports.h"
#include "util/bbop.h"
#include "util/datastream.h"
#include "util/misc.h"
#include "util/textstreamwriter.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/statobj/agsstaticobject.h"
#include "ac/statobj/staticarray.h"
#include "util/string_utils.h" // linux strnicmp definition

using AGS::Common::DataStream;
using AGS::Common::TextStreamWriter;

extern ccInstance *loadedInstances[MAX_LOADED_INSTANCES]; // in script/script_runtime
extern int gameHasBeenRestored; // in ac/game
extern ExecutingScript*curscript; // in script/script
extern int guis_need_update; // in gui/guimain
extern int displayed_room; // in ac/game
extern roomstruct thisroom; // ac/game
extern int maxWhileLoops;
extern new_line_hook_type new_line_hook;

extern ScriptString myScriptStringImpl;

enum ScriptOpArgIsReg
{
    kScOpNoArgIsReg     = 0,
    kScOpArg1IsReg      = 0x0001,
    kScOpArg2IsReg      = 0x0002,
    kScOpArg3IsReg      = 0x0004,
    kScOpOneArgIsReg    = kScOpArg1IsReg,
    kScOpTwoArgsAreReg  = kScOpArg1IsReg | kScOpArg2IsReg,
    kScOpTreeArgsAreReg = kScOpArg1IsReg | kScOpArg2IsReg | kScOpArg3IsReg
};

struct ScriptCommandInfo
{
    ScriptCommandInfo(int32_t code, const char *cmdname, int arg_count, ScriptOpArgIsReg arg_is_reg)
    {
        Code        = code;
        CmdName     = cmdname;
        ArgCount    = arg_count;
        ArgIsReg[0] = (arg_is_reg & kScOpArg1IsReg) != 0;
        ArgIsReg[1] = (arg_is_reg & kScOpArg2IsReg) != 0;
        ArgIsReg[2] = (arg_is_reg & kScOpArg3IsReg) != 0;
    }

    int32_t             Code;
    const char          *CmdName;
    int                 ArgCount;
    bool                ArgIsReg[3];
};

const ScriptCommandInfo sccmd_info[CC_NUM_SCCMDS] =
{
    ScriptCommandInfo( 0                    , "NULL"              , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_ADD             , "add"               , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_SUB             , "sub"               , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_REGTOREG        , "mov"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_WRITELIT        , "memwritelit"       , 2, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_RET             , "ret"               , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_LITTOREG        , "mov"               , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREAD         , "memread"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITE        , "memwrite"          , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MULREG          , "mul"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_DIVREG          , "div"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_ADDREG          , "add"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_SUBREG          , "sub"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_BITAND          , "bit_and"           , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_BITOR           , "bit_or"            , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_ISEQUAL         , "cmp"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_NOTEQUAL        , "ncmp"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_GREATER         , "gt"                , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_LESSTHAN        , "lt"                , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_GTE             , "gte"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_LTE             , "lte"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_AND             , "and"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_OR              , "or"                , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_CALL            , "call"              , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREADB        , "memread.b"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREADW        , "memread.w"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITEB       , "memwrite.b"        , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITEW       , "memwrite.w"        , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_JZ              , "jz"                , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_PUSHREG         , "push"              , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_POPREG          , "pop"               , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_JMP             , "jmp"               , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MUL             , "mul"               , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_CALLEXT         , "farcall"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_PUSHREAL        , "farpush"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_SUBREALSTACK    , "farsubsp"          , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_LINENUM         , "sourceline"        , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_CALLAS          , "callscr"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_THISBASE        , "thisaddr"          , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_NUMFUNCARGS     , "setfuncargs"       , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MODREG          , "mod"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_XORREG          , "xor"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_NOTREG          , "not"               , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_SHIFTLEFT       , "shl"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_SHIFTRIGHT      , "shr"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_CALLOBJ         , "callobj"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_CHECKBOUNDS     , "checkbounds"       , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITEPTR     , "memwrite.ptr"      , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREADPTR      , "memread.ptr"       , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMZEROPTR      , "memwrite.ptr.0"    , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MEMINITPTR      , "meminit.ptr"       , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_LOADSPOFFS      , "load.sp.offs"      , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_CHECKNULL       , "checknull.ptr"     , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_FADD            , "f.add"             , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_FSUB            , "f.sub"             , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_FMULREG         , "f.mul"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FDIVREG         , "f.div"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FADDREG         , "f.add"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FSUBREG         , "f.sub"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FGREATER        , "f.gt"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FLESSTHAN       , "f.lt"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FGTE            , "f.gte"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FLTE            , "f.lte"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_ZEROMEMORY      , "zeromem"           , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_CREATESTRING    , "newstring"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_STRINGSEQUAL    , "strcmp"            , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_STRINGSNOTEQ    , "strnotcmp"         , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_CHECKNULLREG    , "checknull"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_LOOPCHECKOFF    , "loopcheckoff"      , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MEMZEROPTRND    , "memwrite.ptr.0.nd" , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_JNZ             , "jnz"               , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_DYNAMICBOUNDS   , "dynamicbounds"     , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_NEWARRAY        , "newarray"          , 3, kScOpOneArgIsReg ),
};

const char *regnames[] = { "null", "sp", "mar", "ax", "bx", "cx", "op", "dx" };

const char *fixupnames[] = { "null", "fix_gldata", "fix_func", "fix_string", "fix_import", "fix_datadata", "fix_stack" };

ccInstance *current_instance;
// [IKM] 2012-10-21:
// NOTE: This is temporary solution (*sigh*, one of many) which allows certain
// exported functions return value as a RuntimeScriptValue object;
// this is made so because I do not want to change all exported functions'
// return value at once.
RuntimeScriptValue GlobalReturnValue;


ccInstance *ccInstance::GetCurrentInstance()
{
    return current_instance;
}

ccInstance *ccInstance::CreateFromScript(ccScript * scri)
{
    return CreateEx(scri, NULL);
}

ccInstance *ccInstance::CreateEx(ccScript * scri, ccInstance * joined)
{
    // allocate and copy all the memory with data, code and strings across
    ccInstance *cinst = new ccInstance();
    if (!cinst->_Create(scri, joined))
    {
        delete cinst;
        return NULL;
    }

    return cinst;
}

ccInstance::ccInstance()
{
    flags               = 0;
    globalvars          = NULL;
    num_globalvars      = 0;
    num_globalvar_slots = 0;
    globaldata          = NULL;
    globaldatasize      = 0;
    code                = NULL;
    runningInst         = NULL;
    codesize            = 0;
    strings             = NULL;
    stringssize         = 0;
    exports             = NULL;
    stack               = NULL;
    num_stackentries    = 0;
    stackdata           = NULL;
    stackdatasize       = 0;
    pc                  = 0;
    line_number         = 0;
    instanceof          = NULL;
    callStackSize       = 0;
    loadedInstanceId    = 0;
    returnValue         = 0;

    code_fixups         = NULL;
}

ccInstance::~ccInstance()
{
    Free();
}

ccInstance *ccInstance::Fork()
{
    return CreateEx(instanceof, this);
}

void ccInstance::Abort()
{
    if ((this != NULL) && (pc != 0))
        flags |= INSTF_ABORTED;
}

void ccInstance::AbortAndDestroy()
{
    if (this != NULL) {
        Abort();
        flags |= INSTF_FREE;
    }
}

int ccInstance::CallScriptFunction(char *funcname, int32_t numargs, RuntimeScriptValue *params)
{
    ccError = 0;
    currentline = 0;

    if (numargs > 0 && !params)
    {
        cc_error("internal error in ccInstance::CallScriptFunction");
        return -1; // TODO: correct error value
    }

    if ((numargs >= 20) || (numargs < 0)) {
        cc_error("too many arguments to function");
        return -3;
    }

    if (pc != 0) {
        cc_error("instance already being executed");
        return -4;
    }

    int32_t startat = -1;
    int k;
    char mangledName[200];
    sprintf(mangledName, "%s$", funcname);

    for (k = 0; k < instanceof->numexports; k++) {
        char *thisExportName = instanceof->exports[k];
        int match = 0;

        // check for a mangled name match
        if (strncmp(thisExportName, mangledName, strlen(mangledName)) == 0) {
            // found, compare the number of parameters
            char *numParams = thisExportName + strlen(mangledName);
            if (atoi(numParams) != numargs) {
                cc_error("wrong number of parameters to exported function '%s' (expected %d, supplied %d)", funcname, atoi(numParams), numargs);
                return -1;
            }
            match = 1;
        }
        // check for an exact match (if the script was compiled with
        // an older version)
        if ((match == 1) || (strcmp(thisExportName, funcname) == 0)) {
            int32_t etype = (instanceof->export_addr[k] >> 24L) & 0x000ff;
            if (etype != EXPORT_FUNCTION) {
                cc_error("symbol is not a function");
                return -1;
            }
            startat = (instanceof->export_addr[k] & 0x00ffffff);
            break;
        }
    }

    if (startat < 0) {
        cc_error("function '%s' not found", funcname);
        return -2;
    }

    //numargs++;                    // account for return address
    flags &= ~INSTF_ABORTED;

    // object pointer needs to start zeroed
    registers[SREG_OP].SetDynamicObject(0, NULL);

    ccInstance* currentInstanceWas = current_instance;
    registers[SREG_SP].SetStackPtr( &stack[0] );
    stackdata_ptr = stackdata;
    // NOTE: Pushing parameters to stack in reverse order
    for (int i = numargs - 1; i >= 0; --i)
    {
        PushValueToStack(params[i]);
    }
    PushValueToStack(RuntimeScriptValue().SetInt32(0)); // return address on stack
    runningInst = this;

    int reterr = Run(startat);
    PopValuesFromStack(numargs);
    pc = 0;
    current_instance = currentInstanceWas;

    // NOTE that if proper multithreading is added this will need
    // to be reconsidered, since the GC could be run in the middle 
    // of a RET from a function or something where there is an 
    // object with ref count 0 that is in use
    pool.RunGarbageCollectionIfAppropriate();

    if (new_line_hook)
        new_line_hook(NULL, 0);

    if (reterr)
        return -6;

    if (flags & INSTF_ABORTED) {
        flags &= ~INSTF_ABORTED;

        if (flags & INSTF_FREE)
            Free();
        return 100;
    }

    if (registers[SREG_SP].RValue != &stack[0]) {
        cc_error("stack pointer was not zero at completion of script");
        return -5;
    }
    return ccError;
}

void ccInstance::DoRunScriptFuncCantBlock(NonBlockingScriptFunction* funcToRun, bool *hasTheFunc) {
    if (!hasTheFunc[0])
        return;

    no_blocking_functions++;
    int result;

    if (funcToRun->numParameters < 3)
    {
        result = CallScriptFunction((char*)funcToRun->functionName, funcToRun->numParameters, funcToRun->params);
    }
    else
        quit("DoRunScriptFuncCantBlock called with too many parameters");

    if (result == -2) {
        // the function doens't exist, so don't try and run it again
        hasTheFunc[0] = false;
    }
    else if ((result != 0) && (result != 100)) {
        quit_with_script_error(funcToRun->functionName);
    }
    else
    {
        funcToRun->atLeastOneImplementationExists = true;
    }
    // this might be nested, so don't disrupt blocked scripts
    ccErrorString[0] = 0;
    ccError = 0;
    no_blocking_functions--;
}

char scfunctionname[MAX_FUNCTION_NAME_LEN+1];
int ccInstance::PrepareTextScript(char**tsname) {
    ccError=0;
    if (this==NULL) return -1;
    if (GetSymbolAddress(tsname[0]).IsNull()) {
        strcpy (ccErrorString, "no such function in script");
        return -2;
    }
    if (pc!=0) {
        strcpy(ccErrorString,"script is already in execution");
        return -3;
    }
    scripts[num_scripts].init();
    scripts[num_scripts].inst = this;
    /*  char tempb[300];
    sprintf(tempb,"Creating script instance for '%s' room %d",tsname[0],displayed_room);
    write_log(tempb);*/
    if (pc != 0) {
        //    write_log("Forking instance");
        scripts[num_scripts].inst = Fork();
        if (scripts[num_scripts].inst == NULL)
            quit("unable to fork instance for secondary script");
        scripts[num_scripts].forked = 1;
    }
    curscript = &scripts[num_scripts];
    num_scripts++;
    if (num_scripts >= MAX_SCRIPT_AT_ONCE)
        quit("too many nested text script instances created");
    // in case script_run_another is the function name, take a backup
    strncpy(scfunctionname,tsname[0],MAX_FUNCTION_NAME_LEN);
    tsname[0]=&scfunctionname[0];
    update_script_mouse_coords();
    inside_script++;
    //  aborted_ip=0;
    //  abort_executor=0;
    return 0;
}

#define CHECK_STACK \
    if ((registers[SREG_SP].RValue - &stack[0]) >= CC_STACK_SIZE) { \
    cc_error("stack overflow"); \
    return -1; \
    }

// Macros to maintain the call stack
#define PUSH_CALL_STACK \
    if (callStackSize >= MAX_CALL_STACK) { \
    cc_error("CallScriptFunction stack overflow (recursive call error?)"); \
    return -1; \
    } \
    callStackLineNumber[callStackSize] = line_number;  \
    callStackCodeInst[callStackSize] = runningInst;  \
    callStackAddr[callStackSize] = pc;  \
    callStackSize++ 

#define POP_CALL_STACK \
    if (callStackSize < 1) { \
    cc_error("CallScriptFunction stack underflow -- internal error"); \
    return -1; \
    } \
    callStackSize--;\
    line_number = callStackLineNumber[callStackSize];\
    currentline = line_number

#define MAX_FUNC_PARAMS 20
#define MAXNEST 50  // number of recursive function calls allowed
int ccInstance::Run(int32_t curpc)
{
    pc = curpc;
    returnValue = -1;

    if ((curpc < 0) || (curpc >= runningInst->codesize)) {
        cc_error("specified code offset is not valid");
        return -1;
    }

    // Needed to avoid unaligned variable access.
    RuntimeScriptValue temp_variable;

    int (*realfunc) ();
    RuntimeScriptValue callstack[MAX_FUNC_PARAMS + 1];
    int32_t thisbase[MAXNEST], funcstart[MAXNEST];
    int callstacksize = 0, was_just_callas = -1;
    int curnest = 0;
    int loopIterations = 0;
    int num_args_to_func = -1;
    int next_call_needs_object = 0;
    int loopIterationCheckDisabled = 0;
    thisbase[0] = 0;
    funcstart[0] = pc;
    current_instance = this;
    ccInstance *codeInst = runningInst;
    int write_debug_dump = ccGetOption(SCOPT_DEBUGRUN);
	ScriptOperation codeOp;

    while (1) {

		if (!codeInst->ReadOperation(codeOp, pc))
        {
            return -1;
        }

        // save the arguments for quick access
        RuntimeScriptValue &arg1 = codeOp.Args[0];
        RuntimeScriptValue &arg2 = codeOp.Args[1];
        RuntimeScriptValue &arg3 = codeOp.Args[2];
        RuntimeScriptValue &reg1 = 
            registers[arg1.IValue >= 0 && arg1.IValue < CC_NUM_REGISTERS ? arg1.IValue : 0];
        RuntimeScriptValue &reg2 = 
            registers[arg2.IValue >= 0 && arg2.IValue < CC_NUM_REGISTERS ? arg2.IValue : 0];

        if (write_debug_dump)
        {
            DumpInstruction(codeOp);
        }

        switch (codeOp.Instruction.Code) {
      case SCMD_LINENUM:
          line_number = arg1.IValue;
          currentline = arg1.IValue;
          if (new_line_hook)
              new_line_hook(this, currentline);
          break;
      case SCMD_ADD:
          // If the the register is SREG_SP, we are allocating new variable on the stack
          if (arg1.IValue == SREG_SP)
          {
            // Only allocate new data if current stack entry is invalid;
            // in some cases this may be advancing over value that was written by MEMWRITE*
            if (reg1.RValue->IsValid())
            {
              // TODO: perhaps should add a flag here to ensure this happens only after MEMWRITE-ing to stack
              registers[SREG_SP].RValue++;
            }
            else
            {
              PushDataToStack(arg2.IValue);
            }
          }
          else
          {
            reg1.IValue += arg2.IValue;
          }
          CHECK_STACK 
              break;
      case SCMD_SUB:
          if (reg1.Type == kScValStackPtr)
          {
            // If this is SREG_SP, this is stack pop, which frees local variables;
            // Other than SREG_SP this may be AGS 2.x method to offset stack in SREG_MAR;
            // quote JJS:
            // // AGS 2.x games also perform relative stack access by copying SREG_SP to SREG_MAR
            // // and then subtracting from that.
            if (arg1.IValue == SREG_SP)
            {
                PopDataFromStack(arg2.IValue);
            }
            else
            {
                // This is practically LOADSPOFFS
                reg1 = GetStackPtrOffsetRw(arg2.IValue);
            }
          }
          else
          {
            reg1.IValue -= arg2.IValue;
          }
          break;
      case SCMD_REGTOREG:
          reg2 = reg1;
          break;
      case SCMD_WRITELIT:
          // Take the data address from reg[MAR] and copy there arg1 bytes from arg2 address
          //
          // NOTE: since it reads directly from arg2 (which originally was
          // long, or rather int32 due x32 build), written value may normally
          // be only up to 4 bytes large;
          // I guess that's an obsolete way to do WRITE, WRITEW and WRITEB
          switch (arg1.IValue)
          {
          case sizeof(char):
              registers[SREG_MAR].WriteByte(arg2.IValue);
              break;
          case sizeof(int16_t):
              registers[SREG_MAR].WriteInt16(arg2.IValue);
              break;
          case sizeof(int32_t):
              // We do not know if this is math integer or some pointer, etc
              registers[SREG_MAR].WriteValue(arg2);
              break;
          default:
              cc_error("unexpected data size for WRITELIT op: %d", arg1.IValue);
              break;
          }
          break;
      case SCMD_RET:
          {
          if (loopIterationCheckDisabled > 0)
              loopIterationCheckDisabled--;

          RuntimeScriptValue rval = PopValueFromStack();
          curnest--;
          pc = rval.IValue;
          if (pc == 0)
          {
              returnValue = registers[SREG_AX].IValue;
              return 0;
          }
          current_instance = this;
          POP_CALL_STACK;
          continue;                 // continue so that the PC doesn't get overwritten
          }
      case SCMD_LITTOREG:
          reg1 = arg2;
          break;
      case SCMD_MEMREAD:
          // Take the data address from reg[MAR] and copy int32_t to reg[arg1]
          reg1 = registers[SREG_MAR].ReadValue();
          break;
      case SCMD_MEMWRITE:
          // Take the data address from reg[MAR] and copy there int32_t from reg[arg1]
          registers[SREG_MAR].WriteValue(reg1);
          break;
      case SCMD_LOADSPOFFS:
          registers[SREG_MAR] = GetStackPtrOffsetRw(arg1.IValue);
          break;

          // 64 bit: Force 32 bit math
      case SCMD_MULREG:
          reg1.SetInt32(reg1.IValue * reg2.IValue);
          break;
      case SCMD_DIVREG:
          if (reg2.IValue == 0) {
              cc_error("!Integer divide by zero");
              return -1;
          } 
          reg1.SetInt32(reg1.IValue / reg2.IValue);
          break;
      case SCMD_ADDREG:
          // This may be pointer arithmetics, in which case IValue stores offset from base pointer
          reg1.IValue += reg2.IValue;
          break;
      case SCMD_SUBREG:
          // This may be pointer arithmetics, in which case IValue stores offset from base pointer
          reg1.IValue -= reg2.IValue;
          break;
      case SCMD_BITAND:
          reg1.SetInt32(reg1.IValue & reg2.IValue);
          break;
      case SCMD_BITOR:
          reg1.SetInt32(reg1.IValue | reg2.IValue);
          break;
      case SCMD_ISEQUAL:
          reg1.SetInt32AsBool(reg1 == reg2);
          break;
      case SCMD_NOTEQUAL:
          reg1.SetInt32AsBool(reg1 != reg2);
          break;
      case SCMD_GREATER:
          reg1.SetInt32AsBool(reg1.IValue > reg2.IValue);
          break;
      case SCMD_LESSTHAN:
          reg1.SetInt32AsBool(reg1.IValue < reg2.IValue);
          break;
      case SCMD_GTE:
          reg1.SetInt32AsBool(reg1.IValue >= reg2.IValue);
          break;
      case SCMD_LTE:
          reg1.SetInt32AsBool(reg1.IValue <= reg2.IValue);
          break;
      case SCMD_AND:
          reg1.SetInt32AsBool(reg1.IValue && reg2.IValue);
          break;
      case SCMD_OR:
          reg1.SetInt32AsBool(reg1.IValue || reg2.IValue);
          break;
      case SCMD_XORREG:
          reg1.SetInt32(reg1.IValue ^ reg2.IValue);
          break;
      case SCMD_MODREG:
          if (reg2.IValue == 0) {
              cc_error("!Integer divide by zero");
              return -1;
          } 
          reg1.SetInt32(reg1.IValue % reg2.IValue);
          break;
      case SCMD_NOTREG:
          reg1 = !(reg1);
          break;
      case SCMD_CALL:
          // CallScriptFunction another function within same script, just save PC
          // and continue from there
          if (curnest >= MAXNEST - 1) {
              cc_error("!call stack overflow, recursive call problem?");
              return -1;
          }

          PUSH_CALL_STACK;

          PushValueToStack(RuntimeScriptValue().SetInt32(pc + sccmd_info[codeOp.Instruction.Code].ArgCount + 1));

          if (thisbase[curnest] == 0)
              pc = reg1.IValue;
          else {
              pc = funcstart[curnest];
              pc += (reg1.IValue - thisbase[curnest]);
          }

          if (next_call_needs_object)  // is this right?
              next_call_needs_object = 0;

          if (loopIterationCheckDisabled)
              loopIterationCheckDisabled++;

          curnest++;
          thisbase[curnest] = 0;
          funcstart[curnest] = pc;
          CHECK_STACK
              continue;
      case SCMD_MEMREADB:
          // Take the data address from reg[MAR] and copy byte to reg[arg1]
          reg1.SetInt8(registers[SREG_MAR].ReadByte());
          break;
      case SCMD_MEMREADW:
          // Take the data address from reg[MAR] and copy int16_t to reg[arg1]
          reg1.SetInt16(registers[SREG_MAR].ReadInt16());
          break;
      case SCMD_MEMWRITEB:
          // Take the data address from reg[MAR] and copy there byte from reg[arg1]
          registers[SREG_MAR].WriteByte(reg1.IValue);
          break;
      case SCMD_MEMWRITEW:
          // Take the data address from reg[MAR] and copy there int16_t from reg[arg1]
          registers[SREG_MAR].WriteInt16(reg1.IValue);
          break;
      case SCMD_JZ:
          if (registers[SREG_AX].IsNull())
              pc += arg1.IValue;
          break;
      case SCMD_JNZ:
          if (!registers[SREG_AX].IsNull())
              pc += arg1.IValue;
          break;
      case SCMD_PUSHREG:
          // Script code analysis shows that statistically there's a moderate
          // chance (10-30% depending on game) that a PUSHREG instruction will be
          // immediately followed by POPREG.
          // This runtime fixup serves the purpose of slightly increasing
          // execution speed by skipping two stack operations.
          // Practically, this is identical to REGTOREG instruction.
          if (codeInst->code[pc + 2] == SCMD_POPREG)
          {
              registers[codeInst->code[pc + 3]] = reg1;
              pc += 2;
              break;
          }
          // Push reg[arg1] value to the stack
          PushValueToStack(reg1);
          CHECK_STACK
              break;
      case SCMD_POPREG:
          reg1 = PopValueFromStack();
          break;
      case SCMD_JMP:
          pc += arg1.IValue;

          if ((arg1.IValue < 0) && (maxWhileLoops > 0) && (loopIterationCheckDisabled == 0)) {
              // Make sure it's not stuck in a While loop
              loopIterations ++;
              if (flags & INSTF_RUNNING) {
                  loopIterations = 0;
                  flags &= ~INSTF_RUNNING;
              }
              else if (loopIterations > maxWhileLoops) {
                  cc_error("!Script appears to be hung (a while loop ran %d times). The problem may be in a calling function; check the call stack.", loopIterations);
                  return -1;
              }
          }
          break;
      case SCMD_MUL:
          reg1.IValue *= arg2.IValue;
          break;
      case SCMD_CHECKBOUNDS:
          if ((reg1.IValue < 0) ||
              (reg1.IValue >= arg2.IValue)) {
                  cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", reg1.IValue, arg2.IValue - 1);
                  return -1;
          }
          break;
      case SCMD_DYNAMICBOUNDS:
          {
              // TODO: test reg[MAR] type here;
              // That might be dynamic object, but also a non-managed dynamic array, "allocated"
              // on global or local memspace (buffer)
              int32_t upperBoundInBytes = *((int32_t *)(registers[SREG_MAR].GetPtrWithOffset() - 4));
              if ((reg1.IValue < 0) ||
                  (reg1.IValue >= upperBoundInBytes)) {
                      int32_t upperBound = *((int32_t *)(registers[SREG_MAR].GetPtrWithOffset() - 8)) & (~ARRAY_MANAGED_TYPE_FLAG);
                      int elementSize = (upperBoundInBytes / upperBound);
                      cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", reg1.IValue / elementSize, upperBound - 1);
                      return -1;
              }
              break;
          }

          // 64 bit: Handles are always 32 bit values. They are not C pointer.

      case SCMD_MEMREADPTR: {
          ccError = 0;

          int32_t handle = registers[SREG_MAR].ReadInt32();
          void *object;
          ICCDynamicObject *manager;
          ccGetObjectAddressAndManagerFromHandle(handle, object, manager);
          reg1.SetDynamicObject( object, manager );

          // if error occurred, cc_error will have been set
          if (ccError)
              return -1;
          break; }
      case SCMD_MEMWRITEPTR: {

          int32_t handle = registers[SREG_MAR].ReadInt32();
          char *address = NULL;

          if (reg1.Type == kScValStaticArray && reg1.StcArr->GetDynamicManager())
          {
              address = (char*)reg1.StcArr->GetElementPtr(reg1.Ptr, reg1.IValue);
          }
          else if (reg1.Type == kScValDynamicObject)
          {
              address = reg1.Ptr;
          }
          // There's one possible case when the reg1 is 0, which means writing nullptr
          else if (!reg1.IsNull())
          {
              cc_error("internal error: MEMWRITEPTR argument is not dynamic object");
              return -1;
          }

          int32_t newHandle = ccGetObjectHandleFromAddress(address);
          if (newHandle == -1)
              return -1;

          if (handle != newHandle) {
              ccReleaseObjectReference(handle);
              ccAddObjectReference(newHandle);
              registers[SREG_MAR].WriteInt32(newHandle);
          }
          break;
                             }
      case SCMD_MEMINITPTR: { 
          char *address = NULL;

          if (reg1.Type == kScValStaticArray && reg1.StcArr->GetDynamicManager())
          {
              address = (char*)reg1.StcArr->GetElementPtr(reg1.Ptr, reg1.IValue);
          }
          else if (reg1.Type == kScValDynamicObject)
          {
              address = reg1.Ptr;
          }
          // There's one possible case when the reg1 is 0, which means writing nullptr
          else if (!reg1.IsNull())
          {
              cc_error("internal error: SCMD_MEMINITPTR argument is not dynamic object");
              return -1;
          }
          // like memwriteptr, but doesn't attempt to free the old one
          int32_t newHandle = ccGetObjectHandleFromAddress(address);
          if (newHandle == -1)
              return -1;

          ccAddObjectReference(newHandle);
          registers[SREG_MAR].WriteInt32(newHandle);
          break;
                            }
      case SCMD_MEMZEROPTR: {
          int32_t handle = registers[SREG_MAR].ReadInt32();
          ccReleaseObjectReference(handle);
          registers[SREG_MAR].WriteInt32(0);
          break;
                            }
      case SCMD_MEMZEROPTRND: {
          int32_t handle = registers[SREG_MAR].ReadInt32();

          // don't do the Dispose check for the object being returned -- this is
          // for returning a String (or other pointer) from a custom function.
          // Note: we might be freeing a dynamic array which contains the DisableDispose
          // object, that will be handled inside the recursive call to SubRef.
          // CHECKME!! what type of data may reg1 point to?
          pool.disableDisposeForObject = (const char*)registers[SREG_AX].Ptr;
          ccReleaseObjectReference(handle);
          pool.disableDisposeForObject = NULL;
          registers[SREG_MAR].WriteInt32(0);
          break;
                              }
      case SCMD_CHECKNULL:
          if (registers[SREG_MAR].IsNull()) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          break;
      case SCMD_CHECKNULLREG:
          if (reg1.IsNull()) {
              cc_error("!Null string referenced");
              return -1;
          }
          break;
      case SCMD_NUMFUNCARGS:
          num_args_to_func = arg1.IValue;
          break;
      case SCMD_CALLAS:{
          PUSH_CALL_STACK;

          // CallScriptFunction to a function in another script
          int startArg = 0;
          // If there are nested CALLAS calls, the stack might
          // contain 2 calls worth of parameters, so only
          // push args for this call
          if (num_args_to_func >= 0)
              startArg = callstacksize - num_args_to_func;

          for (int i = startArg; i < callstacksize; ++i) {
              // 64 bit: Arguments are pushed as 64 bit values
              PushValueToStack(callstack[i]);
          }

          // 0, so that the cc_run_code returns
          RuntimeScriptValue oldstack = registers[SREG_SP];
          PushValueToStack(RuntimeScriptValue().SetInt32(0));
          CHECK_STACK

          int oldpc = pc;
          ccInstance *wasRunning = runningInst;

          // extract the instance ID
          int32_t instId = codeOp.Instruction.InstanceId;
          // determine the offset into the code of the instance we want
          runningInst = loadedInstances[instId];
          intptr_t callAddr = reg1.Ptr - (char*)&runningInst->code[0];
          if (callAddr % sizeof(intptr_t) != 0) {
              cc_error("call address not aligned");
              return -1;
          }
          callAddr /= sizeof(intptr_t); // size of ccScript::code elements

          if (Run((int32_t)callAddr))
              return -1;

          runningInst = wasRunning;

          if (flags & INSTF_ABORTED)
              return 0;

          if (oldstack != registers[SREG_SP]) {
              cc_error("stack corrupt after function call");
              return -1;
          }

          if (next_call_needs_object)
              next_call_needs_object = 0;

          pc = oldpc;
          was_just_callas = callstacksize;
          num_args_to_func = -1;
          POP_CALL_STACK;
          break;
                       }
      case SCMD_CALLEXT: {
          int call_uses_object = 0;
          // CallScriptFunction to a real 'C' code function
          was_just_callas = -1;
          if (num_args_to_func < 0)
              num_args_to_func = callstacksize;

          GlobalReturnValue.Invalidate();
          int32_t return_value;
          if (next_call_needs_object) {
              // member function call
              // use the callstack +1 size allocation to squeeze
              // the object address on as the last parameter
              call_uses_object = 1;
              next_call_needs_object = 0;
              callstack[callstacksize] = registers[SREG_OP];
              return_value = 
                  // FIXME later: use different getter
                  call_function((intptr_t)reg1.Ptr, num_args_to_func + 1, callstack, callstacksize - num_args_to_func);
          }
          else if (num_args_to_func == 0) {
              // FIXME later: use different getter
              realfunc = (int (*)())reg1.Ptr;
              return_value = realfunc();
          } 
          else
              return_value =
              // FIXME later: use different getter
                    call_function((intptr_t)reg1.Ptr, num_args_to_func, callstack, callstacksize - num_args_to_func);

          if (GlobalReturnValue.IsValid())
          {
            registers[SREG_AX] = GlobalReturnValue;
          }
          else
          {
            registers[SREG_AX].SetInt32(return_value);
          }

          if (ccError)
              return -1;

          if (call_uses_object) {
              // Pop OP?
          }

          current_instance = this;
          num_args_to_func = -1;
          break;
                         }
      case SCMD_PUSHREAL:
          //        printf("pushing arg%d as %ld\n",callstacksize,reg1);
          if (callstacksize >= MAX_FUNC_PARAMS) {
              cc_error("CallScriptFunction stack overflow");
              return -1;
          }
          callstack[callstacksize] = reg1;
          callstacksize++;
          break;
      case SCMD_SUBREALSTACK:
          if (was_just_callas >= 0) {
              PopValuesFromStack(arg1.IValue);
              was_just_callas = -1;
          }
          callstacksize -= arg1.IValue;
          break;
      case SCMD_CALLOBJ:
          // set the OP register
          if (reg1.IsNull()) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          if (reg1.Type == kScValDynamicObject ||
              // This might be an object of USER-DEFINED type, calling its MEMBER-FUNCTION.
              // Note, that this is the only case known when such object is written into reg[SREG_OP];
              // in any other case that would count as error.
              reg1.Type == kScValGlobalVar || reg1.Type == kScValStackPtr
              )
          {
              registers[SREG_OP] = reg1;
          }
          else if (reg1.Type == kScValStaticArray && reg1.StcArr->GetDynamicManager())
          {
              registers[SREG_OP].SetDynamicObject(
                  (char*)reg1.StcArr->GetElementPtr(reg1.Ptr, reg1.IValue),
                  reg1.StcArr->GetDynamicManager());
          }
          else
          {
              cc_error("internal error: SCMD_CALLOBJ argument is not an object of built-in or user-defined type");
              return -1;
          }
          next_call_needs_object = 1;
          break;
      case SCMD_SHIFTLEFT:
          reg1.SetInt32(reg1.IValue << reg2.IValue);
          break;
      case SCMD_SHIFTRIGHT:
          reg1.SetInt32(reg1.IValue >> reg2.IValue);
          break;
      case SCMD_THISBASE:
          thisbase[curnest] = arg1.IValue;
          break;
      case SCMD_NEWARRAY:
          {
              int numElements = reg1.IValue;
              if ((numElements < 1) || (numElements > 1000000))
              {
                  cc_error("invalid size for dynamic array; requested: %d, range: 1..1000000", numElements);
                  return -1;
              }
              int32_t handle = globalDynamicArray.Create(numElements, arg2.IValue, arg3.GetAsBool());
              reg1.SetDynamicObject((void*)ccGetObjectAddressFromHandle(handle), &globalDynamicArray);
              break;
          }
      case SCMD_FADD:
          reg1.SetFloat(reg1.FValue + arg2.IValue); // arg2 was used as int here originally
          break;
      case SCMD_FSUB:
          reg1.SetFloat(reg1.FValue - arg2.IValue); // arg2 was used as int here originally
          break;
      case SCMD_FMULREG:
          reg1.SetFloat(reg1.FValue * reg2.FValue);
          break;
      case SCMD_FDIVREG:
          if (reg2.FValue == 0.0) {
              cc_error("!Floating point divide by zero");
              return -1;
          } 
          reg1.SetFloat(reg1.FValue / reg2.FValue);
          break;
      case SCMD_FADDREG:
          reg1.SetFloat(reg1.FValue + reg2.FValue);
          break;
      case SCMD_FSUBREG:
          reg1.SetFloat(reg1.FValue - reg2.FValue);
          break;
      case SCMD_FGREATER:
          reg1.SetFloatAsBool(reg1.FValue > reg2.FValue);
          break;
      case SCMD_FLESSTHAN:
          reg1.SetFloatAsBool(reg1.FValue < reg2.FValue);
          break;
      case SCMD_FGTE:
          reg1.SetFloatAsBool(reg1.FValue >= reg2.FValue);
          break;
      case SCMD_FLTE:
          reg1.SetFloatAsBool(reg1.FValue <= reg2.FValue);
          break;
      case SCMD_ZEROMEMORY:
          // Check if we are zeroing at stack tail
          if (registers[SREG_MAR] == registers[SREG_SP]) {
              // creating a local variable -- check the stack to ensure no mem overrun
              int currentStackSize = registers[SREG_SP].RValue - &stack[0];
              int currentDataSize = stackdata_ptr - stackdata;
              if (currentStackSize + 1 >= CC_STACK_SIZE ||
                  currentDataSize + arg1.IValue >= CC_STACK_DATA_SIZE)
              {
                  cc_error("stack overflow, attempted grow to %d bytes", currentDataSize + arg1.IValue);
                  return -1;
              }
              // NOTE: according to compiler's logic, this is always followed
              // by SCMD_ADD, and that is where the data is "allocated", here we
              // just clean the place.
              // CHECKME -- since we zero memory in PushDataToStack anyway, this is not needed at all?
              memset(stackdata_ptr, 0, arg1.IValue);
          }
          else
          {
            cc_error("internal error: stack tail address expected on SCMD_ZEROMEMORY instruction, reg[MAR] type is %d",
				registers[SREG_MAR].Type);
            return -1;
          }
          break;
      case SCMD_CREATESTRING:
          if (stringClassImpl == NULL) {
              cc_error("No string class implementation set, but opcode was used");
              return -1;
          }
          // TODO: test reg1 type;
          // Might be local, global memory, and dynamic object too?
          reg1.SetDynamicObject(
              (void*)stringClassImpl->CreateString((const char *)(reg1.GetPtrWithOffset())),
              &myScriptStringImpl);
          break;
      case SCMD_STRINGSEQUAL:
          if ((reg1.IsNull()) || (reg2.IsNull())) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          reg1.SetInt32AsBool(
            strcmp((const char*)reg1.GetPtrWithOffset(), (const char*)reg2.GetPtrWithOffset()) == 0 );
          
          break;
      case SCMD_STRINGSNOTEQ:
          if ((reg1.IsNull()) || (reg2.IsNull())) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          reg1.SetInt32AsBool(
              strcmp((const char*)reg1.GetPtrWithOffset(), (const char*)reg2.GetPtrWithOffset()) != 0 );
          break;
      case SCMD_LOOPCHECKOFF:
          if (loopIterationCheckDisabled == 0)
              loopIterationCheckDisabled++;
          break;
      default:
          cc_error("invalid instruction %d found in code stream", codeOp.Instruction.Code);
          return -1;
        }

        if (flags & INSTF_ABORTED)
            return 0;

        pc += sccmd_info[codeOp.Instruction.Code].ArgCount + 1;
    }
}

int ccInstance::RunScriptFunctionIfExists(char*tsname,int numParam, RuntimeScriptValue *params) {
    int oldRestoreCount = gameHasBeenRestored;
    // First, save the current ccError state
    // This is necessary because we might be attempting
    // to run Script B, while Script A is still running in the
    // background.
    // If CallInstance here has an error, it would otherwise
    // also abort Script A because ccError is a global variable.
    int cachedCcError = ccError;
    ccError = 0;

    int toret = PrepareTextScript(&tsname);
    if (toret) {
        ccError = cachedCcError;
        return -18;
    }

    // Clear the error message
    ccErrorString[0] = 0;

    if (numParam < 3)
    {
        toret = curscript->inst->CallScriptFunction(tsname,numParam, params);
    }
    else
        quit("Too many parameters to RunScriptFunctionIfExists");

    // 100 is if Aborted (eg. because we are LoadAGSGame'ing)
    if ((toret != 0) && (toret != -2) && (toret != 100)) {
        quit_with_script_error(tsname);
    }

    post_script_cleanup_stack++;

    if (post_script_cleanup_stack > 50)
        quitprintf("!post_script_cleanup call stack exceeded: possible recursive function call? running %s", tsname);

    post_script_cleanup();

    post_script_cleanup_stack--;

    // restore cached error state
    ccError = cachedCcError;

    // if the game has been restored, ensure that any further scripts are not run
    if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
        eventClaimed = EVENT_CLAIMED;

    return toret;
}

int ccInstance::RunTextScript(char*tsname) {
    if (strcmp(tsname, REP_EXEC_NAME) == 0) {
        // run module rep_execs
        int room_changes_was = play.room_changes;
        int restore_game_count_was = gameHasBeenRestored;

        for (int kk = 0; kk < numScriptModules; kk++) {
            if (!moduleRepExecAddr[kk].IsNull())
                moduleInst[kk]->RunScriptFunctionIfExists(tsname, 0, NULL);

            if ((room_changes_was != play.room_changes) ||
                (restore_game_count_was != gameHasBeenRestored))
                return 0;
        }
    }

    int toret = RunScriptFunctionIfExists(tsname, 0, NULL);
    if ((toret == -18) && (this == roominst)) {
        // functions in room script must exist
        quitprintf("prepare_script: error %d (%s) trying to run '%s'   (Room %d)",toret,ccErrorString,tsname, displayed_room);
    }
    return toret;
}

int ccInstance::RunTextScriptIParam(char*tsname,RuntimeScriptValue &iparam) {
    if ((strcmp(tsname, "on_key_press") == 0) || (strcmp(tsname, "on_mouse_click") == 0)) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 1, &iparam, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    return RunScriptFunctionIfExists(tsname, 1, &iparam);
}

int ccInstance::RunTextScript2IParam(char*tsname,RuntimeScriptValue &iparam, RuntimeScriptValue &param2) {
    RuntimeScriptValue params[2];
    params[0] = iparam;
    params[1] = param2;

    if (strcmp(tsname, "on_event") == 0) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 2, params, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    // response to a button click, better update guis
    if (strnicmp(tsname, "interface_click", 15) == 0)
        guis_need_update = 1;

    int toret = RunScriptFunctionIfExists(tsname, 2, params);

    // tsname is no longer valid, because RunScriptFunctionIfExists might
    // have restored a save game and freed the memory. Therefore don't 
    // attempt any strcmp's here
    tsname = NULL;

    return toret;
}

void ccInstance::GetCallStack(char *buffer, int maxLines) {

    // FIXME: check ptr prior to function call instead
    if (this == NULL) {
        // not in a script, no call stack
        buffer[0] = 0;
        return;
    }

    sprintf(buffer, "in \"%s\", line %d\n", runningInst->instanceof->GetSectionName(pc), line_number);

    char lineBuffer[300];
    int linesDone = 0;
    for (int j = callStackSize - 1; (j >= 0) && (linesDone < maxLines); j--, linesDone++) {
        sprintf(lineBuffer, "from \"%s\", line %d\n",
            callStackCodeInst[j]->instanceof->GetSectionName(callStackAddr[j]), callStackLineNumber[j]);
        strcat(buffer, lineBuffer);
        if (linesDone == maxLines - 1)
            strcat(buffer, "(and more...)\n");
    }
}

void ccInstance::GetScriptName(char *curScrName) {
    if (this == NULL)
        strcpy (curScrName, "Not in a script");
    else if (instanceof == gamescript)
        strcpy (curScrName, "Global script");
    else if (instanceof == thisroom.compiled_script)
        sprintf (curScrName, "Room %d script", displayed_room);
    else
        strcpy (curScrName, "Unknown script");
}

// get a pointer to a variable or function exported by the script
RuntimeScriptValue ccInstance::GetSymbolAddress(char *symname)
{
    int k;
    char altName[200];
    sprintf(altName, "%s$", symname);
    RuntimeScriptValue rval_null;

    for (k = 0; k < instanceof->numexports; k++) {
        if (strcmp(instanceof->exports[k], symname) == 0)
            return exports[k];
        // mangled function name
        if (strncmp(instanceof->exports[k], altName, strlen(altName)) == 0)
            return exports[k];
    }
    return rval_null;
}

void ccInstance::DumpInstruction(const ScriptOperation &op)
{
    // line_num local var should be shared between all the instances
    static int line_num = 0;

    if (op.Instruction.Code == SCMD_LINENUM)
    {
        line_num = op.Args[0].IValue;
        return;
    }

    DataStream *data_s = ci_fopen("script.log", Common::kFile_Create, Common::kFile_Write);
    TextStreamWriter writer(data_s);
    writer.WriteFormat("Line %3d, IP:%8d (SP:%p) ", line_num, pc, registers[SREG_SP].RValue);

    const ScriptCommandInfo &cmd_info = sccmd_info[op.Instruction.Code];
    writer.WriteString(cmd_info.CmdName);

    for (int i = 0; i < cmd_info.ArgCount; ++i)
    {
        if (i > 0)
        {
            writer.WriteChar(',');
        }
        if (cmd_info.ArgIsReg[i])
        {
            writer.WriteFormat(" %s", regnames[op.Args[i].IValue]);
        }
        else
        {
            // MACPORT FIX 9/6/5: changed %d to %ld
            // FIXME: check type and write appropriate values
            writer.WriteFormat(" %ld", op.Args[i].GetPtrWithOffset());
        }
    }
    writer.WriteLineBreak();
    // the writer will delete data stream internally
}

bool ccInstance::_Create(ccScript * scri, ccInstance * joined)
{
    int i;
    currentline = -1;
    if ((scri == NULL) && (joined != NULL))
        scri = joined->instanceof;

    if (scri == NULL) {
        cc_error("null pointer passed");
        return false;
    }

    if (joined != NULL) {
        // share memory space with an existing instance (ie. this is a thread/fork)
        globalvars = joined->globalvars;
        num_globalvars = joined->num_globalvars;
        globaldatasize = joined->globaldatasize;
        globaldata = joined->globaldata;
        code = joined->code;
        codesize = joined->codesize;
    } 
    else {
        // create own memory space
        // NOTE: globalvars are created in CreateGlobalVars()
        globaldatasize = scri->globaldatasize;
        globaldata = NULL;
        if (globaldatasize > 0)
        {
            globaldata = (char *)malloc(globaldatasize);
            memcpy(globaldata, scri->globaldata, globaldatasize);
        }

        codesize = scri->codesize;
        code = NULL;
        if (codesize > 0)
        {
            code = (intptr_t*)malloc(codesize * sizeof(intptr_t));
            memcpy(code, scri->code, codesize * sizeof(intptr_t));
        }
    }

    // just use the pointer to the strings since they don't change
    strings = scri->strings;
    stringssize = scri->stringssize;
    // create a stack
    stackdatasize = CC_STACK_DATA_SIZE;
    // This is quite a random choice; there's no way to deduce number of stack
    // entries needed without knowing amount of local variables (at least)
    num_stackentries = CC_STACK_SIZE;
    stack       = new RuntimeScriptValue[num_stackentries];
    stackdata   = new char[stackdatasize];
    if (stack == NULL || stackdata == NULL) {
        cc_error("not enough memory to allocate stack");
        return false;
    }

    // find a LoadedInstance slot for it
    for (i = 0; i < MAX_LOADED_INSTANCES; i++) {
        if (loadedInstances[i] == NULL) {
            loadedInstances[i] = this;
            loadedInstanceId = i;
            break;
        }
        if (i == MAX_LOADED_INSTANCES - 1) {
            cc_error("too many active instances");
            return false;
        }
    }

    if (joined)
    {
        resolved_imports = joined->resolved_imports;
        code_fixups = joined->code_fixups;
    }
    else
    {
        if (!ResolveScriptImports(scri))
        {
            return false;
        }
        if (!CreateGlobalVars(scri))
        {
            return false;
        }
        if (!CreateRuntimeCodeFixups(scri))
        {
            return false;
        }
    }

    exports = new RuntimeScriptValue[scri->numexports];

    // find the real address of the exports
    for (i = 0; i < scri->numexports; i++) {
        int32_t etype = (scri->export_addr[i] >> 24L) & 0x000ff;
        int32_t eaddr = (scri->export_addr[i] & 0x00ffffff);
        if (etype == EXPORT_FUNCTION)
        {
            // NOTE: unfortunately, there seems to be no way to know if
            // that's an extender function that expects object pointer
            exports[i].SetStaticFunction((void *)((intptr_t)eaddr * sizeof(intptr_t) + (intptr_t)(&code[0])));
        }
        else if (etype == EXPORT_DATA)
        {
            ScriptVariable *gl_var = FindGlobalVar(eaddr);
            if (gl_var)
            {
                exports[i].SetGlobalVar(&gl_var->RValue);
            }
            else
            {
                cc_error("cannot resolve global variable, key = %d", eaddr);
                return false;
            }
        }
        else {
            cc_error("internal export fixup error");
            return false;
        }
    }
    instanceof = scri;
    pc = 0;
    flags = 0;
    if (joined != NULL)
        flags = INSTF_SHAREDATA;
    scri->instances++;

    if ((scri->instances == 1) && (ccGetOption(SCOPT_AUTOIMPORT) != 0)) {
        // import all the exported stuff from this script
        for (i = 0; i < scri->numexports; i++) {
            if (!ccAddExternalScriptSymbol(scri->exports[i], exports[i], this)) {
                cc_error("Export table overflow at '%s'", scri->exports[i]);
                return false;
            }
        }
    }
    return true;
}

void ccInstance::Free()
{
    if (instanceof != NULL) {
        instanceof->instances--;
        if (instanceof->instances == 0)
        {
            simp.RemoveScriptExports(this);
        }
    }

    // remove from the Active Instances list
    if (loadedInstances[loadedInstanceId] == this)
        loadedInstances[loadedInstanceId] = NULL;

    if ((flags & INSTF_SHAREDATA) == 0)
    {
        delete [] globalvars;
        nullfree(globaldata);
        nullfree(code);
    }
    globalvars = NULL;
    globaldata = NULL;
    code = NULL;
    strings = NULL;

    delete [] stack;
    delete [] stackdata;
    delete [] exports;
    stack = NULL;
    stackdata = NULL;
    exports = NULL;

    if ((flags & INSTF_SHAREDATA) == 0)
    {
        delete [] resolved_imports;
        delete [] code_fixups;
    }
    resolved_imports = NULL;
    code_fixups = NULL;
}

bool ccInstance::ResolveScriptImports(ccScript * scri)
{
    // When the import is referenced in code, it's being addressed
    // by it's index in the script imports array. That index is
    // NOT unique and relative to script only.
    // Script keeps information of used imports as an array of
    // names.
    // To allow real-time import use we should put resolved imports
    // to the array keeping the order of their names in script's
    // array of names.

    // resolve all imports referenced in the script
    numimports = scri->numimports;
    if (numimports == 0)
    {
        resolved_imports = NULL;
        return false;
    }
    resolved_imports = new int[numimports];

    for (int i = 0; i < scri->numimports; ++i) {
        // MACPORT FIX 9/6/5: changed from NULL TO 0
        if (scri->imports[i] == 0) {
            continue;
        }

        resolved_imports[i] = simp.get_index_of(scri->imports[i]);
        if (resolved_imports[i] < 0) {
            cc_error("unresolved import '%s'", scri->imports[i]);
            return false;
        }
    }
    return true;
}

// TODO: it is possible to deduce global var's size at start with
// certain accuracy after all global vars are registered. Each
// global var's size would be limited by closest next var's ScAddress
// and globaldatasize.
// TODO: rework this routine by the use of normal Array or Map class
bool ccInstance::CreateGlobalVars(ccScript * scri)
{
    ScriptVariable glvar;

    // Step One: deduce global variables from fixups
    for (int i = 0; i < scri->numfixups; ++i)
    {
        switch (scri->fixuptypes[i])
        {
        case FIXUP_GLOBALDATA:
            // GLOBALDATA fixup takes relative address of global data element from code array;
            // this is the address of actual data
            glvar.ScAddress = (int32_t)code[scri->fixups[i]];
            glvar.RValue.SetData(globaldata + glvar.ScAddress, 0);
            break;
        case FIXUP_DATADATA:
            {
            // DATADATA fixup takes relative address of global data element from fixups array;
            // this is the address of element, which stores address of actual data
            glvar.ScAddress = scri->fixups[i];
            int32_t data_addr = *(int32_t*)&globaldata[glvar.ScAddress];
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(data_addr);
#endif // AGS_BIG_ENDIAN
            if (glvar.ScAddress - data_addr != 200 /* size of old AGS string */)
            {
                // CHECKME: probably replace with mere warning in the log?
                cc_error("unexpected old-style string's alignment");
                return false;
            }
            // TODO: register this explicitly as a string instead (can do this later)
            glvar.RValue.SetStaticObject(globaldata + data_addr, &GlobalStaticManager);
            }
            break;
        default:
            // other fixups are of no use here
            continue;
        }

        TryAddGlobalVar(glvar);
    }

    // Step Two: deduce global variables from exports
    for (int i = 0; i < scri->numexports; ++i)
    {
        int32_t etype = (scri->export_addr[i] >> 24L) & 0x000ff;
        int32_t eaddr = (scri->export_addr[i] & 0x00ffffff);
        if (etype == EXPORT_DATA)
        {
            // NOTE: old-style strings could not be exported in AGS,
            // no need to worry about these here
            glvar.ScAddress = eaddr;
            glvar.RValue.SetData(globaldata + glvar.ScAddress, 0);
            TryAddGlobalVar(glvar);
        }
    }

    return true;
}

bool ccInstance::TryAddGlobalVar(const ScriptVariable &glvar)
{
    int index;
    if (!FindGlobalVar(glvar.ScAddress, &index))
    {
        AddGlobalVar(glvar, index);
        return true;
    }
    return false;
}

// TODO: use bsearch routine from Array or Map class, when we put one in use
ScriptVariable *ccInstance::FindGlobalVar(int32_t var_addr, int *pindex)
{
    if (pindex)
    {
        *pindex = -1;
    }

    if (!num_globalvars)
    {
        return NULL;
    }

    if (var_addr >= globaldatasize)
    {
        return NULL;
    }

    int first   = 0;
    int last    = num_globalvars;
    int mid;
    while (first < last)
    {
        mid = first + ((last - first) >> 1);
        if (var_addr <= globalvars[mid].ScAddress)
        {
            last = mid;
        }
        else
        {
            first = mid + 1;
        }
    }

    if (pindex)
    {
        *pindex = last;
    }

    if (last < num_globalvars && globalvars[last].ScAddress == var_addr)
    {
        return &globalvars[last];
    }
    return NULL;
}

void ccInstance::AddGlobalVar(const ScriptVariable &glvar, int at_index)
{
    if (at_index < 0 || at_index > num_globalvars)
    {
        at_index = num_globalvars;
    }

    // FIXME: some hardcore vector emulation here :/
    if (num_globalvars == num_globalvar_slots)
    {
        num_globalvar_slots = num_globalvars + 100;
        ScriptVariable *new_arr = new ScriptVariable[num_globalvar_slots];
        if (num_globalvars > 0)
        {
            memcpy(new_arr, globalvars, num_globalvars * sizeof(ScriptVariable));
        }
        delete [] globalvars;
        globalvars = new_arr;
    }

    if (at_index < num_globalvars)
    {
        memmove(globalvars + at_index + 1, globalvars + at_index, (num_globalvars - at_index) * sizeof(ScriptVariable));
    }
    globalvars[at_index] = glvar;
    num_globalvars++;
}

bool ccInstance::CreateRuntimeCodeFixups(ccScript * scri)
{
    code_fixups = new char[scri->codesize];
    memset(code_fixups, 0, scri->codesize);
    for (int i = 0; i < scri->numfixups; ++i)
    {
        if (scri->fixuptypes[i] == FIXUP_DATADATA)
        {
            continue;
        }

        int32_t fixup = scri->fixups[i];
        code_fixups[fixup] = scri->fixuptypes[i];

        switch (scri->fixuptypes[i])
        {
        case FIXUP_GLOBALDATA:
            {
                ScriptVariable *gl_var = FindGlobalVar((int32_t)code[fixup]);
                if (!gl_var)
                {
                    cc_error("cannot resolve global variable, key = %d", (int32_t)code[fixup]);
                    return false;
                }
                code[fixup] = (intptr_t)gl_var;
            }
            break;
        case FIXUP_FUNCTION:
        case FIXUP_STRING:
        case FIXUP_STACK:
            break; // do nothing yet
        case FIXUP_IMPORT:
            // we do not need to save import's address now when we have
            // resolved imports kept so far as instance exists, but we
            // must fixup the following instruction in certain case
            {
                int import_index = resolved_imports[code[fixup]];
                const ScriptImport *import = simp.getByIndex(import_index);
                if (!import)
                {
                    cc_error("cannot resolve import, key = %d", import_index);
                    return false;
                }
                code[fixup] = import_index;
                // If the call is to another script function next CALLEXT
                // must be replaced with CALLAS
                if (import->InstancePtr != NULL && (code[fixup + 1] & INSTANCE_ID_REMOVEMASK) == SCMD_CALLEXT)
                {
                    code[fixup + 1] = SCMD_CALLAS | (import->InstancePtr->loadedInstanceId << INSTANCE_ID_SHIFT);
                }
            }
            break;
        default:
            cc_error("internal fixup index error: %d", scri->fixuptypes[i]);
            return false;
        }
    }
    return true;
}

bool ccInstance::ReadOperation(ScriptOperation &op, int32_t at_pc)
{
	op.Instruction.Code			= code[at_pc];
	op.Instruction.InstanceId	= (op.Instruction.Code >> INSTANCE_ID_SHIFT) & INSTANCE_ID_MASK;
	op.Instruction.Code		   &= INSTANCE_ID_REMOVEMASK; // now this is pure instruction code

    int want_args = sccmd_info[op.Instruction.Code].ArgCount;
    if (at_pc + want_args >= codesize)
    {
        cc_error("unexpected end of code data at %d", at_pc + want_args);
        return false;
    }
    op.ArgCount = want_args;

    at_pc++;
    for (int i = 0; i < op.ArgCount; ++i, ++at_pc)
    {
        char fixup = code_fixups[at_pc];
        if (fixup > 0)
        {
            // could be relative pointer or import address
            if (!FixupArgument(code[at_pc], fixup, op.Args[i]))
            {
                return false;
            }
        }
        else
        {
            // should be a numeric literal (int32 or float)
            op.Args[i].SetInt32( (int32_t)code[at_pc] );
        }
    }

    return true;
}

bool ccInstance::FixupArgument(intptr_t code_value, char fixup_type, RuntimeScriptValue &argument)
{
    switch (fixup_type)
    {
    case FIXUP_GLOBALDATA:
        {
            ScriptVariable *gl_var = (ScriptVariable*)code_value;
            argument.SetGlobalVar(&gl_var->RValue);
        }
        break;
    case FIXUP_FUNCTION:
        // originally commented -- CHECKME: could this be used in very old versions of AGS?
        //      code[fixup] += (long)&code[0];
        // This is a program counter value, presumably will be used as SCMD_CALL argument
        argument.SetInt32((int32_t)code_value);
        break;
    case FIXUP_STRING:
        argument.SetStringLiteral(&strings[0] + code_value);
        break;
    case FIXUP_IMPORT:
        {
            const ScriptImport *import = simp.getByIndex((int32_t)code_value);
            if (import)
            {
                argument = import->Value;
            }
            else
            {
                cc_error("cannot resolve import, key = %ld", code_value);
                return false;
            }
        }
        break;
    case FIXUP_STACK:
        argument = GetStackPtrOffsetFw((int32_t)code_value);
        break;
    default:
        cc_error("internal fixup type error: %d", fixup_type);
        return false;;
    }
    return true;
}

//-----------------------------------------------------------------------------

void ccInstance::PushValueToStack(const RuntimeScriptValue &rval)
{
    if (registers[SREG_SP].RValue->IsValid())
    {
        cc_error("internal error: valid data beyond stack ptr");
        return;
    }
    // Write value to the stack tail and advance stack ptr
    registers[SREG_SP].WriteValue(rval);
    registers[SREG_SP].RValue++;
}

void ccInstance::PushDataToStack(int32_t num_bytes)
{
    if (registers[SREG_SP].RValue->IsValid())
    {
        cc_error("internal error: valid data beyond stack ptr");
        return;
    }
    // Zero memory, assign pointer to data block to the stack tail, advance both stack ptr and stack data ptr
    memset(stackdata_ptr, 0, num_bytes);
    registers[SREG_SP].RValue->SetData(stackdata_ptr, num_bytes);
    stackdata_ptr += num_bytes;
    registers[SREG_SP].RValue++;
}

RuntimeScriptValue ccInstance::PopValueFromStack()
{
    if (registers[SREG_SP].RValue->IsValid())
    {
        cc_error("internal error: valid data beyond stack ptr");
        return RuntimeScriptValue();
    }
    // rewind stack ptr to the last valid value, decrement stack data ptr if needed and invalidate the stack tail
    registers[SREG_SP].RValue--;
    RuntimeScriptValue rval = *registers[SREG_SP].RValue;
    if (rval.Type == kScValData)
    {
        stackdata_ptr -= rval.Size;
    }
    registers[SREG_SP].RValue->Invalidate();
    return rval;
}

void ccInstance::PopValuesFromStack(int32_t num_entries = 1)
{
    if (registers[SREG_SP].RValue->IsValid())
    {
        cc_error("internal error: valid data beyond stack ptr");
        return;
    }
    for (int i = 0; i < num_entries; ++i)
    {
        // rewind stack ptr to the last valid value, decrement stack data ptr if needed and invalidate the stack tail
        registers[SREG_SP].RValue--;
        if (registers[SREG_SP].RValue->Type == kScValData)
        {
            stackdata_ptr -= registers[SREG_SP].RValue->Size;
        }
        registers[SREG_SP].RValue->Invalidate();
    }
}

void ccInstance::PopDataFromStack(int32_t num_bytes)
{
    if (registers[SREG_SP].RValue->IsValid())
    {
        cc_error("internal error: valid data beyond stack ptr");
        return;
    }
    int32_t total_pop = 0;
    while (total_pop < num_bytes)
    {
        // rewind stack ptr to the last valid value, decrement stack data ptr if needed and invalidate the stack tail
        registers[SREG_SP].RValue--;
        // remember popped bytes count
        total_pop += registers[SREG_SP].RValue->Size;
        if (registers[SREG_SP].RValue->Type == kScValData)
        {
            stackdata_ptr -= registers[SREG_SP].RValue->Size;
        }
        registers[SREG_SP].RValue->Invalidate();
    }
    if (total_pop > num_bytes)
    {
        cc_error("stack pointer points inside local variable after pop, stack corrupted?");
    }
}

RuntimeScriptValue ccInstance::GetStackPtrOffsetFw(int32_t fw_offset)
{
    int32_t total_off = 0;
    RuntimeScriptValue *stack_entry = &stack[0];
    while (total_off < fw_offset)
    {
        if (stack_entry->Size > 0)
        {
            total_off += stack_entry->Size;
        }
        stack_entry++;
    }
    RuntimeScriptValue stack_ptr;
    stack_ptr.SetStackPtr(stack_entry);
    if (total_off > fw_offset)
    {
        // Forward offset should always set ptr at the beginning of stack entry
        cc_error("stack offset forward: trying to access stack data inside stack entry, stack corrupted?");
    }
    return stack_ptr;
}

RuntimeScriptValue ccInstance::GetStackPtrOffsetRw(int32_t rw_offset)
{
    if (registers[SREG_SP].RValue->IsValid())
    {
        cc_error("internal error: valid data beyond stack ptr");
        return RuntimeScriptValue();
    }
    int32_t total_off = 0;
    RuntimeScriptValue *stack_entry = registers[SREG_SP].RValue;
    while (total_off < rw_offset)
    {
        stack_entry--;
        total_off += stack_entry->Size;
    }
    RuntimeScriptValue stack_ptr;
    stack_ptr.SetStackPtr(stack_entry);
    if (total_off > rw_offset)
    {
        // Could be accessing array element, so state error only if stack entry does not refer to data array
        if (stack_entry->Type == kScValData)
        {
            stack_ptr.IValue += total_off - rw_offset;
        }
        else
        {
            cc_error("stack offset backward: trying to access stack data inside stack entry, stack corrupted?");
        }
    }
    return stack_ptr;
}
