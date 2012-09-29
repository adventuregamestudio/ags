
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
#include "script/spans.h"
#include "script/systemimports.h"
#include "util/bbop.h"
#include "util/datastream.h"
#include "util/misc.h"
#include "util/textstreamwriter.h"

using AGS::Common::DataStream;
using AGS::Common::TextStreamWriter;

extern ccInstance *loadedInstances[MAX_LOADED_INSTANCES]; // in script/script_runtime
extern void nullfree(void *data); // in script/script_runtime
extern int gameHasBeenRestored; // in ac/game
extern ExecutingScript*curscript; // in script/script
extern int guis_need_update; // in gui/guimain
extern int displayed_room; // in ac/game
extern roomstruct thisroom; // ac/game
extern int maxWhileLoops;
extern new_line_hook_type new_line_hook;

struct ScriptCommandInfo
{
    ScriptCommandInfo(long code, const char *cmdname, int arg_count, ScriptValueType param_type)
    {
        Code        = code;
        CmdName     = cmdname;
        ArgCount    = arg_count;
        ParamType   = param_type;
    }

    long            Code;
    const char      *CmdName;
    int             ArgCount;
    ScriptValueType ParamType;
};

const ScriptCommandInfo sccmd_info[CC_NUM_SCCMDS] =
{
    ScriptCommandInfo( 0                    , "NULL"                , 0, kScValUndefined ),
    ScriptCommandInfo( SCMD_ADD             , "$add"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_SUB             , "$sub"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_REGTOREG        , "$$mov"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_WRITELIT        , "memwritelit"         , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_RET             , "ret"                 , 0, kScValGeneric ),
    ScriptCommandInfo( SCMD_LITTOREG        , "$mov"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMREAD         , "$memread"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMWRITE        , "$memwrite"           , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MULREG          , "$$mul"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_DIVREG          , "$$div"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_ADDREG          , "$$add"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_SUBREG          , "$$sub"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_BITAND          , "$$bit_and"           , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_BITOR           , "$$bit_or"            , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_ISEQUAL         , "$$cmp"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_NOTEQUAL        , "$$ncmp"              , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_GREATER         , "$$gt"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_LESSTHAN        , "$$lt"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_GTE             , "$$gte"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_LTE             , "$$lte"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_AND             , "$$and"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_OR              , "$$or"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_CALL            , "$call"               , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMREADB        , "$memread.b"          , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMREADW        , "$memread.w"          , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMWRITEB       , "$memwrite.b"         , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMWRITEW       , "$memwrite.w"         , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_JZ              , "jz"                  , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_PUSHREG         , "$push"               , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_POPREG          , "$pop"                , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_JMP             , "jmp"                 , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MUL             , "$mul"                , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_CALLEXT         , "$farcall"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_PUSHREAL        , "$farpush"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_SUBREALSTACK    , "farsubsp"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_LINENUM         , "sourceline"          , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_CALLAS          , "$callscr"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_THISBASE        , "thisaddr"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_NUMFUNCARGS     , "setfuncargs"         , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MODREG          , "$$mod"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_XORREG          , "$$xor"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_NOTREG          , "$not"                , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_SHIFTLEFT       , "$$shl"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_SHIFTRIGHT      , "$$shr"               , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_CALLOBJ         , "$callobj"            , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_CHECKBOUNDS     , "$checkbounds"        , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMWRITEPTR     , "$memwrite.ptr"       , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMREADPTR      , "$memread.ptr"        , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMZEROPTR      , "memwrite.ptr.0"      , 0, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMINITPTR      , "$meminit.ptr"        , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_LOADSPOFFS      , "load.sp.offs"        , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_CHECKNULL       , "checknull.ptr"       , 0, kScValGeneric ),
    ScriptCommandInfo( SCMD_FADD            , "$f.add"              , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FSUB            , "$f.sub"              , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FMULREG         , "$$f.mul"             , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FDIVREG         , "$$f.div"             , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FADDREG         , "$$f.add"             , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FSUBREG         , "$$f.sub"             , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FGREATER        , "$$f.gt"              , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FLESSTHAN       , "$$f.lt"              , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FGTE            , "$$f.gte"             , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_FLTE            , "$$f.lte"             , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_ZEROMEMORY      , "zeromem"             , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_CREATESTRING    , "$newstring"          , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_STRINGSEQUAL    , "$$strcmp"            , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_STRINGSNOTEQ    , "$$strnotcmp"         , 2, kScValGeneric ),
    ScriptCommandInfo( SCMD_CHECKNULLREG    , "$checknull"          , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_LOOPCHECKOFF    , "loopcheckoff"        , 0, kScValGeneric ),
    ScriptCommandInfo( SCMD_MEMZEROPTRND    , "memwrite.ptr.0.nd"   , 0, kScValGeneric ),
    ScriptCommandInfo( SCMD_JNZ             , "jnz"                 , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_DYNAMICBOUNDS   , "$dynamicbounds"      , 1, kScValGeneric ),
    ScriptCommandInfo( SCMD_NEWARRAY        , "$newarray"           , 3, kScValGeneric ),
};

const char *regnames[] = { "null", "sp", "mar", "ax", "bx", "cx", "op", "dx" };

const char *fixupnames[] = { "null", "fix_gldata", "fix_func", "fix_string", "fix_import", "fix_datadata", "fix_stack" };

ccInstance *current_instance;



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
    globaldata          = NULL;
    globaldatasize      = 0;
    code                = NULL;
    runningInst         = NULL;
    codesize            = 0;
    strings             = NULL;
    stringssize         = 0;
    exportaddr          = NULL;
    stack               = NULL;
    stacksize           = 0;
    pc                  = 0;
    line_number         = 0;
    instanceof          = NULL;
    callStackSize       = 0;
    loadedInstanceId    = 0;
    returnValue         = 0;

    code_helpers        = NULL;
    codehelpers_capacity= 0;
    num_codehelpers     = 0;
    codehelper_index    = 0;

#if defined(AGS_64BIT)
    stackSizeIndex      = 0;
#endif
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

int ccInstance::CallScriptFunction(char *funcname, long numargs, ...)
{
    ccError = 0;
    currentline = 0;

    if ((numargs >= 20) || (numargs < 0)) {
        cc_error("too many arguments to function");
        return -3;
    }

    if (pc != 0) {
        cc_error("instance already being executed");
        return -4;
    }

    long startat = -1;
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
            long etype = (instanceof->export_addr[k] >> 24L) & 0x000ff;
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

    long tempstack[20];
    int tssize = 1;
    tempstack[0] = 0;             // return address on stack
    if (numargs > 0) {
        va_list ap;
        va_start(ap, numargs);
        while (tssize <= numargs) {
            tempstack[tssize] = va_arg(ap, long);
            tssize++;
        }
        va_end(ap);
    }
    numargs++;                    // account for return address
    flags &= ~INSTF_ABORTED;

    // object pointer needs to start zeroed
    registers[SREG_OP] = 0;

    ccInstance* currentInstanceWas = current_instance;
    long stoffs = 0;
    for (tssize = numargs - 1; tssize >= 0; tssize--) {
        memcpy(&stack[stoffs], &tempstack[tssize], sizeof(long));
        stoffs += sizeof(long);
    }
    registers[SREG_SP] = (long)(&stack[0]);
    registers[SREG_SP] += (numargs * sizeof(long));
    runningInst = this;

#if defined(AGS_64BIT)
    // 64 bit: Initialize array for stack variable sizes with the argument values
    stackSizeIndex = 0;
    int i;
    for (i = 0; i < numargs; i++)
    {
        stackSizes[stackSizeIndex] = -1;
        stackSizeIndex++;
    }
#endif

    int reterr = Run(startat);
    registers[SREG_SP] -= (numargs - 1) * sizeof(long);
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

    if (registers[SREG_SP] != (long)&stack[0]) {
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

    if (funcToRun->numParameters == 0)
        result = CallScriptFunction((char*)funcToRun->functionName, 0);
    else if (funcToRun->numParameters == 1)
        result = CallScriptFunction((char*)funcToRun->functionName, 1, funcToRun->param1);
    else if (funcToRun->numParameters == 2)
        result = CallScriptFunction((char*)funcToRun->functionName, 2, funcToRun->param1, funcToRun->param2);
    else
        quit("->DoRunScriptFuncCantBlock called with too many parameters");

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

char scfunctionname[30];
int ccInstance::PrepareTextScript(char**tsname) {
    ccError=0;
    if (this==NULL) return -1;
    if (GetSymbolAddress(tsname[0]) == NULL) {
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
    strcpy(scfunctionname,tsname[0]);
    tsname[0]=&scfunctionname[0];
    update_script_mouse_coords();
    inside_script++;
    //  aborted_ip=0;
    //  abort_executor=0;
    return 0;
}

#define CHECK_STACK \
    if ((registers[SREG_SP] - ((long)&stack[0])) >= CC_STACK_SIZE) { \
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
int ccInstance::Run(long curpc)
{
    pc = curpc;
    returnValue = -1;

#if defined(AGS_64BIT)
    // 64 bit: For dealing with the stack
    int original_sp_diff = 0;
    int new_sp_diff = 0;
    int sp_index = 0;
#endif

    if ((curpc < 0) || (curpc >= runningInst->codesize)) {
        cc_error("specified code offset is not valid");
        return -1;
    }

    // Needed to avoid unaligned variable access.
    long temp_variable;

    char *mptr;
    unsigned char tbyte;
    short tshort;
    int (*realfunc) ();
    long callstack[MAX_FUNC_PARAMS + 1];
    long thisbase[MAXNEST], funcstart[MAXNEST];
    int callstacksize = 0, aa, was_just_callas = -1;
    int curnest = 0;
    int loopIterations = 0;
    int num_args_to_func = -1;
    int next_call_needs_object = 0;
    int loopIterationCheckDisabled = 0;
    thisbase[0] = 0;
    funcstart[0] = pc;
    current_instance = this;
    float *freg1, *freg2;
    ccInstance *codeInst = runningInst;
    int write_debug_dump = ccGetOption(SCOPT_DEBUGRUN);
	CodeOperation codeOp;

    codehelper_index = -1;

    while (1) {
        if (write_debug_dump)
            // FIXME: this will write non-fixuped values
            DumpInstruction(&codeInst->code[pc], pc, registers[SREG_SP]);

		if (!codeInst->ReadOperation(codeOp, pc))
        {
            return -1;
        }

        // save the arguments for quick access
        long &arg1 = codeOp.Args[0].Value;
        long &arg2 = codeOp.Args[1].Value;
        long &arg3 = codeOp.Args[2].Value;
        if (codeOp.ArgCount > 0) {
            freg1 = (float*)&registers[codeOp.Args[0].Value]; // [IKM] this is crazy... what is the guarantee that was register index and not some literal?!
            if (codeOp.ArgCount > 1) {
                freg2 = (float*)&registers[codeOp.Args[1].Value];
            }
        }

        switch (codeOp.Instruction.Code) {
      case SCMD_LINENUM:
          line_number = arg1;
          currentline = arg1;
          if (new_line_hook)
              new_line_hook(this, currentline);
          break;
      case SCMD_ADD:
#if defined(AGS_64BIT)
          // 64 bit: Keeping track of the stack variables
          if (arg1 == SREG_SP)
          {
              stackSizes[stackSizeIndex] = arg2;
              stackSizeIndex++;
          }
#endif

          registers[arg1] += arg2;
          CHECK_STACK 
              break;
      case SCMD_SUB:
#if defined(AGS_64BIT)
          // 64 bit: Rewrite the offset so that it doesn't point inside a variable on the stack.
          // AGS 2.x games also perform relative stack access by copying SREG_SP to SREG_MAR
          // and then subtracting from that.
          if ((arg1 == SREG_SP) || ((arg1 == SREG_MAR) && (registers[SREG_MAR] == registers[SREG_SP])))
          {
              int orig_sub = arg2;
              int new_sub = 0;
              int temp_index = stackSizeIndex;
              while (orig_sub > 0)
              {
                  if (temp_index < 1)
                      cc_error("Subtracting from stack variable would underflow stack. Stack corrupted?");

                  if (stackSizes[temp_index - 1] == -1)
                  {
                      orig_sub -= 4;
                      new_sub += sizeof(long);
                  }
                  else
                  {
                      orig_sub -= stackSizes[temp_index - 1];
                      new_sub += stackSizes[temp_index - 1];
                  }
                  temp_index--;
              }
              if (arg1 == SREG_SP)
                  stackSizeIndex = temp_index;

              registers[arg1] -= new_sub;
          }
          else
              registers[arg1] -= arg2;
#else
          registers[arg1] -= arg2;
#endif  
          break;
      case SCMD_REGTOREG:
          registers[arg2] = registers[arg1];
          break;
      case SCMD_WRITELIT:
          // poss something dodgy about this routine
          mptr = (char *)(registers[SREG_MAR]);
          memcpy(&mptr[0], &arg2, arg1);
          break;
      case SCMD_RET:
          if (loopIterationCheckDisabled > 0)
              loopIterationCheckDisabled--;

          registers[SREG_SP] -= sizeof(long);
#if defined(AGS_64BIT)
          stackSizeIndex--;
#endif

          curnest--;
          memcpy(&(pc), (char*)registers[SREG_SP], sizeof(long));
          if (pc == 0)
          {
              returnValue = (int)registers[SREG_AX];
              return 0;
          }
          current_instance = this;
          POP_CALL_STACK;
          continue;                 // continue so that the PC doesn't get overwritten
      case SCMD_LITTOREG:
          registers[arg1] = arg2;
          break;
      case SCMD_MEMREAD:
          // 64 bit: Memory reads are still 32 bit
          memset(&(registers[arg1]), 0, sizeof(long));
          memcpy(&(registers[arg1]), (char*)registers[SREG_MAR], 4);

#if defined(AGS_BIG_ENDIAN)
          if (gSpans.IsInSpan((char*)registers[SREG_MAR]))
          {
              int32_t temp = registers[arg1];
              AGS::Common::BitByteOperations::SwapBytesInt32(temp);
              registers[arg1] = temp;
          }
#endif

          break;
      case SCMD_MEMWRITE:
#if defined(AGS_BIG_ENDIAN)
          if (gSpans.IsInSpan((char*)registers[SREG_MAR]))
          {
              int32_t temp = registers[arg1];
              AGS::Common::BitByteOperations::SwapBytesInt32(temp);
              memcpy((char*)registers[SREG_MAR], &temp, 4);
          }
          else
          {
              memcpy((char*)registers[SREG_MAR], &(registers[arg1]), 4);
          }
#else
          // 64 bit: Memory writes are still 32 bit
          memcpy((char*)registers[SREG_MAR], &(registers[arg1]), 4);
#endif
          break;
      case SCMD_LOADSPOFFS:
#if defined(AGS_64BIT)
          // 64 bit: Rewrite offset so that it doesn't point inside a variable
          original_sp_diff = arg1;
          new_sp_diff = 0;
          sp_index = stackSizeIndex - 1;

          while (original_sp_diff > 0)
          {
              if (stackSizes[sp_index] == -1)
              {
                  original_sp_diff -= 4;
                  new_sp_diff += sizeof(long);//stackSizes[sp_index];
                  sp_index--;
              }
              else
              {
                  original_sp_diff -= stackSizes[sp_index];
                  new_sp_diff += stackSizes[sp_index];
                  sp_index--;
              }
          }

          if (sp_index < -1)
              cc_error("Stack offset cannot be rewritten. Stack corrupted?");

          registers[SREG_MAR] = registers[SREG_SP] - new_sp_diff;
#else
          registers[SREG_MAR] = registers[SREG_SP] - arg1;
#endif
          break;

          // 64 bit: Force 32 bit math

      case SCMD_MULREG:
          registers[arg1] = (int)((int)registers[arg1] * (int)registers[arg2]);
          break;
      case SCMD_DIVREG:
          if (registers[arg2] == 0) {
              cc_error("!Integer divide by zero");
              return -1;
          } 
          registers[arg1] = (int)((int)registers[arg1] / (int)registers[arg2]);
          break;
      case SCMD_ADDREG:
          registers[arg1] = (int)((int)registers[arg1] + (int)registers[arg2]);
          break;
      case SCMD_SUBREG:
          registers[arg1] = (int)((int)registers[arg1] - (int)registers[arg2]);
          break;
      case SCMD_BITAND:
          registers[arg1] = (int)((int)registers[arg1] & (int)registers[arg2]);
          break;
      case SCMD_BITOR:
          registers[arg1] = (int)((int)registers[arg1] | (int)registers[arg2]);
          break;
      case SCMD_ISEQUAL:
          registers[arg1] = (int)((int)registers[arg1] == (int)registers[arg2]);
          break;
      case SCMD_NOTEQUAL:
          registers[arg1] = (int)((int)registers[arg1] != (int)registers[arg2]);
          break;
      case SCMD_GREATER:
          registers[arg1] = (int)((int)registers[arg1] > (int)registers[arg2]);
          break;
      case SCMD_LESSTHAN:
          registers[arg1] = (int)((int)registers[arg1] < (int)registers[arg2]);
          break;
      case SCMD_GTE:
          registers[arg1] = (int)((int)registers[arg1] >= (int)registers[arg2]);
          break;
      case SCMD_LTE:
          registers[arg1] = (int)((int)registers[arg1] <= (int)registers[arg2]);
          break;
      case SCMD_AND:
          registers[arg1] = (int)((int)registers[arg1] && (int)registers[arg2]);
          break;
      case SCMD_OR:
          registers[arg1] = (int)((int)registers[arg1] || (int)registers[arg2]);
          break;
      case SCMD_XORREG:
          registers[arg1] = (int)((int)registers[arg1] ^ (int)registers[arg2]);
          break;
      case SCMD_MODREG:
          if (registers[arg2] == 0) {
              cc_error("!Integer divide by zero");
              return -1;
          } 
          registers[arg1] = (int)((int)registers[arg1] % (int)registers[arg2]);
          break;
      case SCMD_NOTREG:
          registers[arg1] = !(registers[arg1]);
          break;
      case SCMD_CALL:
          // CallScriptFunction another function within same script, just save PC
          // and continue from there
          if (curnest >= MAXNEST - 1) {
              cc_error("!call stack overflow, recursive call problem?");
              return -1;
          }

          PUSH_CALL_STACK;

          temp_variable = pc + sccmd_info[codeOp.Instruction.Code].ArgCount + 1;
          memcpy((char*)registers[SREG_SP], &temp_variable, sizeof(long));

          registers[SREG_SP] += sizeof(long);

#if defined(AGS_64BIT)
          stackSizes[stackSizeIndex] = -1;
          stackSizeIndex++;
#endif

          if (thisbase[curnest] == 0)
              pc = registers[arg1];
          else {
              pc = funcstart[curnest];
              pc += (registers[arg1] - thisbase[curnest]);
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
          tbyte = *((unsigned char *)registers[SREG_MAR]);
          registers[arg1] = tbyte;
          break;
      case SCMD_MEMREADW:
          tshort = *((short *)registers[SREG_MAR]);
#if defined(AGS_BIG_ENDIAN)
          if (gSpans.IsInSpan((char*)registers[SREG_MAR]))
          {
              AGS::Common::BitByteOperations::SwapBytesInt16(tshort);
          }
#endif
          registers[arg1] = tshort;
          break;
      case SCMD_MEMWRITEB:
          tbyte = (unsigned char)registers[arg1];
          *((unsigned char *)registers[SREG_MAR]) = tbyte;
          break;
      case SCMD_MEMWRITEW:
          tshort = (short)registers[arg1];
#if defined(AGS_BIG_ENDIAN)
          if (gSpans.IsInSpan((char*)registers[SREG_MAR]))
          {
              AGS::Common::BitByteOperations::SwapBytesInt16(tshort);
          }
#endif
          *((short *)registers[SREG_MAR]) = tshort;
          break;
      case SCMD_JZ:
          if (registers[SREG_AX] == 0)
              pc += arg1;
          break;
      case SCMD_JNZ:
          if (registers[SREG_AX] != 0)
              pc += arg1;
          break;
      case SCMD_PUSHREG:
#if defined(AGS_64BIT)
          // 64 bit: Registers are pushed as 8 byte values. Their size is set to "-1" so that
          // they can be identified later.
          stackSizes[stackSizeIndex] = -1;
          stackSizeIndex++;
#endif

          memcpy((char*)registers[SREG_SP], &(registers[arg1]), sizeof(long));
          registers[SREG_SP] += sizeof(long);
          CHECK_STACK
              break;
      case SCMD_POPREG:
#if defined(AGS_64BIT)
          // 64 bit: Registers are pushed as 8 byte values
          if (stackSizes[stackSizeIndex - 1] != -1)
              cc_error("Trying to pop value that was not pushed. Stack corrupted?");

          stackSizeIndex--;
#endif

          registers[SREG_SP] -= sizeof(long);
          memcpy(&(registers[arg1]), (char*)registers[SREG_SP], sizeof(long));
          break;
      case SCMD_JMP:
          pc += arg1;
          // JJS: FIXME! This is a hack to get 64 bit working again but the real
          // issue is that arg1 sometimes has additional upper bits set
          pc &= 0xFFFFFFFF;

          if ((arg1 < 0) && (maxWhileLoops > 0) && (loopIterationCheckDisabled == 0)) {
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
          registers[arg1] *= arg2;
          break;
      case SCMD_CHECKBOUNDS:
          if ((registers[arg1] < 0) ||
              (registers[arg1] >= arg2)) {
                  cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", registers[arg1], arg2 - 1);
                  return -1;
          }
          break;
      case SCMD_DYNAMICBOUNDS:
          {
              long upperBoundInBytes = *((long *)(registers[SREG_MAR] - 4));
              if ((registers[arg1] < 0) ||
                  (registers[arg1] >= upperBoundInBytes)) {
                      long upperBound = *((long *)(registers[SREG_MAR] - 8)) & (~ARRAY_MANAGED_TYPE_FLAG);
                      int elementSize = (upperBoundInBytes / upperBound);
                      cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", registers[arg1] / elementSize, upperBound - 1);
                      return -1;
              }
              break;
          }

          // 64 bit: Handles are always 32 bit values. They are not C pointer.

      case SCMD_MEMREADPTR:
          ccError = 0;

          int handle;
          memcpy(&handle, (char*)(registers[SREG_MAR]), 4);
          registers[arg1] = (long)ccGetObjectAddressFromHandle(handle);

          // if error occurred, cc_error will have been set
          if (ccError)
              return -1;
          break;
      case SCMD_MEMWRITEPTR: {

          int handle;
          memcpy(&handle, (char*)(registers[SREG_MAR]), 4);

          int newHandle = ccGetObjectHandleFromAddress((char*)registers[arg1]);
          if (newHandle == -1)
              return -1;

          if (handle != newHandle) {
              ccReleaseObjectReference(handle);
              ccAddObjectReference(newHandle);
              memcpy(((char*)registers[SREG_MAR]), &newHandle, 4);
          }
          break;
                             }
      case SCMD_MEMINITPTR: { 
          // like memwriteptr, but doesn't attempt to free the old one

          int handle;
          memcpy(&handle, ((char*)registers[SREG_MAR]), 4);

          int newHandle = ccGetObjectHandleFromAddress((char*)registers[arg1]);
          if (newHandle == -1)
              return -1;

          ccAddObjectReference(newHandle);
          memcpy(((char*)registers[SREG_MAR]), &newHandle, 4);
          break;
                            }
      case SCMD_MEMZEROPTR: {
          int handle;
          memcpy(&handle, ((char*)registers[SREG_MAR]), 4);
          ccReleaseObjectReference(handle);
          memset(((char*)registers[SREG_MAR]), 0, 4);

          break;
                            }
      case SCMD_MEMZEROPTRND: {
          int handle;
          memcpy(&handle, ((char*)registers[SREG_MAR]), 4);

          // don't do the Dispose check for the object being returned -- this is
          // for returning a String (or other pointer) from a custom function.
          // Note: we might be freeing a dynamic array which contains the DisableDispose
          // object, that will be handled inside the recursive call to SubRef.
          pool.disableDisposeForObject = (const char*)registers[SREG_AX];
          ccReleaseObjectReference(handle);
          pool.disableDisposeForObject = NULL;
          memset(((char*)registers[SREG_MAR]), 0, 4);
          break;
                              }
      case SCMD_CHECKNULL:
          if (registers[SREG_MAR] == 0) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          break;
      case SCMD_CHECKNULLREG:
          if (registers[arg1] == 0) {
              cc_error("!Null string referenced");
              return -1;
          }
          break;
      case SCMD_NUMFUNCARGS:
          num_args_to_func = arg1;
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

          for (aa = startArg; aa < callstacksize; aa++) {
              // 64 bit: Arguments are pushed as 64 bit values
              memcpy((char*)registers[SREG_SP], &(callstack[aa]), sizeof(long));
              registers[SREG_SP] += sizeof(long);

#if defined(AGS_64BIT)
              stackSizes[stackSizeIndex] = -1;//sizeof(long);
              stackSizeIndex++;
#endif
          }

          // 0, so that the cc_run_code returns
          memset((char*)registers[SREG_SP], 0, sizeof(long));

          long oldstack = registers[SREG_SP];
          registers[SREG_SP] += sizeof(long);
          CHECK_STACK

#if defined(AGS_64BIT)
              stackSizes[stackSizeIndex] = -1;//sizeof(long);
          stackSizeIndex++;
#endif

          int oldpc = pc;
          ccInstance *wasRunning = runningInst;

          // extract the instance ID
          unsigned long instId = codeOp.Instruction.InstanceId;
              //(codeInst->code[pc] >> INSTANCE_ID_SHIFT) & INSTANCE_ID_MASK;
          // determine the offset into the code of the instance we want
          runningInst = loadedInstances[instId];
          unsigned long callAddr = registers[arg1] - (unsigned long)(&runningInst->code[0]);
          if (callAddr % 4 != 0) {
              cc_error("call address not aligned");
              return -1;
          }
          callAddr /= sizeof(long);

          if (Run(callAddr))
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

          if (next_call_needs_object) {
              // member function call
              // use the callstack +1 size allocation to squeeze
              // the object address on as the last parameter
              call_uses_object = 1;
              next_call_needs_object = 0;
              callstack[callstacksize] = registers[SREG_OP];
              registers[SREG_AX] = call_function(registers[arg1], num_args_to_func + 1, callstack, callstacksize - num_args_to_func);
          }
          else if (num_args_to_func == 0) {
              realfunc = (int (*)())registers[arg1];
              registers[SREG_AX] = realfunc();
          } 
          else
              registers[SREG_AX] = call_function(registers[arg1], num_args_to_func, callstack, callstacksize - num_args_to_func);

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
          //        printf("pushing arg%d as %ld\n",callstacksize,registers[arg1]);
          if (callstacksize >= MAX_FUNC_PARAMS) {
              cc_error("CallScriptFunction stack overflow");
              return -1;
          }
          callstack[callstacksize] = registers[arg1];
          callstacksize++;
          break;
      case SCMD_SUBREALSTACK:
          if (was_just_callas >= 0) {
              registers[SREG_SP] -= arg1 * sizeof(long);
#if defined(AGS_64BIT)
              stackSizeIndex -= arg1;
#endif
              was_just_callas = -1;
          }
          callstacksize -= arg1;
          break;
      case SCMD_CALLOBJ:
          // set the OP register
          if (registers[arg1] == 0) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          registers[SREG_OP] = registers[arg1];
          next_call_needs_object = 1;
          break;
      case SCMD_SHIFTLEFT:
          registers[arg1] = (int)((int)registers[arg1] << (int)registers[arg2]);
          break;
      case SCMD_SHIFTRIGHT:
          registers[arg1] = (int)((int)registers[arg1] >> (int)registers[arg2]);
          break;
      case SCMD_THISBASE:
          thisbase[curnest] = arg1;
          break;
      case SCMD_NEWARRAY:
          {
              //int arg3 = codeInst->code[pc + 3];
              int numElements = registers[arg1];
              if ((numElements < 1) || (numElements > 1000000))
              {
                  cc_error("invalid size for dynamic array; requested: %d, range: 1..1000000", numElements);
                  return -1;
              }
              registers[arg1] = (long)ccGetObjectAddressFromHandle(globalDynamicArray.Create(numElements, arg2, (arg3 == 1)));
              break;
          }
      case SCMD_FADD:
          *freg1 += arg2;
          break;
      case SCMD_FSUB:
          *freg1 -= arg2;
          break;
      case SCMD_FMULREG:
          freg1[0] *= freg2[0];
          break;
      case SCMD_FDIVREG:
          if (freg2[0] == 0.0) {
              cc_error("!Floating point divide by zero");
              return -1;
          } 
          freg1[0] /= freg2[0];
          break;
      case SCMD_FADDREG:
          freg1[0] += freg2[0];
          break;
      case SCMD_FSUBREG:
          freg1[0] -= freg2[0];
          break;
      case SCMD_FGREATER:
          freg1[0] = (freg1[0] > freg2[0]);
          break;
      case SCMD_FLESSTHAN:
          freg1[0] = (freg1[0] < freg2[0]);
          break;
      case SCMD_FGTE:
          freg1[0] = (freg1[0] >= freg2[0]);
          break;
      case SCMD_FLTE:
          freg1[0] = (freg1[0] <= freg2[0]);
          break;
      case SCMD_ZEROMEMORY:
          mptr = (char *)(registers[SREG_MAR]);
          if (registers[SREG_MAR] == registers[SREG_SP]) {
              // creating a local variable -- check the stack to ensure no mem overrun
              int currentStackSize = registers[SREG_SP] - ((long)&stack[0]);
              if (currentStackSize + arg1 >= CC_STACK_SIZE) {
                  cc_error("stack overflow, attempted grow to %d bytes", currentStackSize + arg1);
                  return -1;
              }
          }
          memset(&mptr[0], 0, arg1);
          break;
      case SCMD_CREATESTRING:
          if (stringClassImpl == NULL) {
              cc_error("No string class implementation set, but opcode was used");
              return -1;
          }
          registers[arg1] = (long)stringClassImpl->CreateString((const char *)(registers[arg1]));
          break;
      case SCMD_STRINGSEQUAL:
          if ((registers[arg1] == 0) || (registers[arg2] == 0)) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          if (strcmp((const char*)registers[arg1], (const char*)registers[arg2]) == 0)
              registers[arg1] = 1;
          else
              registers[arg1] = 0;
          break;
      case SCMD_STRINGSNOTEQ:
          if ((registers[arg1] == 0) || (registers[arg2] == 0)) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          if (strcmp((const char*)registers[arg1], (const char*)registers[arg2]) != 0)
              registers[arg1] = 1;
          else
              registers[arg1] = 0;
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

int ccInstance::RunScriptFunctionIfExists(char*tsname,int numParam, long iparam, long iparam2, long iparam3) {
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

    if (numParam == 0) 
        toret = curscript->inst->CallScriptFunction(tsname,numParam);
    else if (numParam == 1)
        toret = curscript->inst->CallScriptFunction(tsname,numParam, iparam);
    else if (numParam == 2)
        toret = curscript->inst->CallScriptFunction(tsname,numParam,iparam, iparam2);
    else if (numParam == 3)
        toret = curscript->inst->CallScriptFunction(tsname,numParam,iparam, iparam2, iparam3);
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
            if (moduleRepExecAddr[kk] != NULL)
                moduleInst[kk]->RunScriptFunctionIfExists(tsname, 0, 0, 0);

            if ((room_changes_was != play.room_changes) ||
                (restore_game_count_was != gameHasBeenRestored))
                return 0;
        }
    }

    int toret = RunScriptFunctionIfExists(tsname, 0, 0, 0);
    if ((toret == -18) && (this == roominst)) {
        // functions in room script must exist
        quitprintf("prepare_script: error %d (%s) trying to run '%s'   (Room %d)",toret,ccErrorString,tsname, displayed_room);
    }
    return toret;
}

int ccInstance::RunTextScriptIParam(char*tsname,long iparam) {
    if ((strcmp(tsname, "on_key_press") == 0) || (strcmp(tsname, "on_mouse_click") == 0)) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 1, iparam, 0, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    return RunScriptFunctionIfExists(tsname, 1, iparam, 0);
}

int ccInstance::RunTextScript2IParam(char*tsname,long iparam,long param2) {
    if (strcmp(tsname, "on_event") == 0) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 2, iparam, param2, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    // response to a button click, better update guis
    if (strnicmp(tsname, "interface_click", 15) == 0)
        guis_need_update = 1;

    int toret = RunScriptFunctionIfExists(tsname, 2, iparam, param2);

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
char *ccInstance::GetSymbolAddress(char *symname)
{
    int k;
    char altName[200];
    sprintf(altName, "%s$", symname);

    for (k = 0; k < instanceof->numexports; k++) {
        if (strcmp(instanceof->exports[k], symname) == 0)
            return exportaddr[k];
        // mangled function name
        if (strncmp(instanceof->exports[k], altName, strlen(altName)) == 0)
            return exportaddr[k];
    }
    return NULL;
}

void ccInstance::DumpInstruction(unsigned long *codeptr, int cps, int spp)
{
    static int line_num = 0;

    if (codeptr[0] == SCMD_LINENUM) {
        line_num = codeptr[1];
        return;
    }

    DataStream *data_s = ci_fopen("script.log", Common::kFile_Create, Common::kFile_Write);
    TextStreamWriter writer(data_s);
    writer.WriteFormat("Line %3d, IP:%8d (SP:%8d) ", line_num, cps, spp);

    int l, thisop = codeptr[0] & INSTANCE_ID_REMOVEMASK, isreg = 0, t = 0;
    const char *toprint = sccmd_info[thisop].CmdName;
    if (toprint[0] == '$') {
        isreg = 1;
        toprint++;
    }

    if (toprint[0] == '$') {
        isreg |= 2;
        toprint++;
    }
    writer.WriteString(toprint);

    for (l = 0; l < sccmd_info[thisop].ArgCount; l++) {
        t++;
        if (l > 0)
            writer.WriteChar(',');

        if ((l == 0) && (isreg & 1))
            writer.WriteFormat(" %s", regnames[codeptr[t]]);
        else if ((l == 1) && (isreg & 2))
            writer.WriteFormat(" %s", regnames[codeptr[t]]);
        else
            // MACPORT FIX 9/6/5: changed %d to %ld
            writer.WriteFormat(" %ld", codeptr[t]);
    }
    writer.WriteLineBreak();
    // the writer will delete data stream internally
}

// changes all pointer variables (ie. strings) to have the relative address, to allow
// the data segment to be saved to disk
void ccInstance::FlattenGlobalData()
{
    ccScript *scri = instanceof;
    int i;

    if (flags & INSTF_SHAREDATA)
        return;

    // perform the fixups
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        if (scri->fixuptypes[i] == FIXUP_DATADATA) {
            // supposedly these are only used for strings...
            int32_t temp;
            memcpy(&temp, (char*)&(globaldata[fixup]), 4);

#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            temp -= (long)globaldata;
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            memcpy(&(globaldata[fixup]), &temp, 4);
        }
    }

}

// restores the pointers after a save
void ccInstance::UnFlattenGlobalData()
{
    ccScript *scri = instanceof;
    int i;

    if (flags & INSTF_SHAREDATA)
        return;

    // perform the fixups
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        if (scri->fixuptypes[i] == FIXUP_DATADATA) {
            // supposedly these are only used for strings...
            int32_t temp;
            memcpy(&temp, (char*)&(globaldata[fixup]), 4);
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            temp += (long)globaldata;
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            memcpy(&(globaldata[fixup]), &temp, 4);
        }
    }
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
        globaldatasize = joined->globaldatasize;
        globaldata = joined->globaldata;
    } 
    else {
        // create own memory space
        globaldatasize = scri->globaldatasize;
        if (globaldatasize > 0) {
            globaldata = (char *)malloc(globaldatasize);
            memcpy(globaldata, scri->globaldata, globaldatasize);
        }
        else
            globaldata = NULL;
    }
    codesize = scri->codesize;

    /*  // [IKM] 2012-09-26:
        // Do not allocate its own copy of code.
    if (codesize > 0) {
        code = (unsigned long *)malloc(codesize * sizeof(long));
        memcpy(code, scri->code, codesize * sizeof(long));
    }
    else
        code = NULL;
    */
    code = (unsigned long*)scri->code; // CHECKME later

    // just use the pointer to the strings since they don't change
    strings = scri->strings;
    stringssize = scri->stringssize;
    // create a stack
    stacksize = CC_STACK_SIZE;
    stack = (char *)malloc(stacksize);
    if (stack == NULL) {
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

    // set up the initial registers to zero
    memset(&registers[0], 0, sizeof(long) * CC_NUM_REGISTERS);

    // find the real address of all the imports
    long *import_addrs = (long *)malloc(scri->numimports * sizeof(long));
    if (scri->numimports == 0)
        import_addrs = NULL;

    for (i = 0; i < scri->numimports; i++) {
        // MACPORT FIX 9/6/5: changed from NULL TO 0
        if (scri->imports[i] == 0) {
            import_addrs[i] = NULL;
            continue;
        }
        import_addrs[i] = (long)simp.get_addr_of(scri->imports[i]);
        if (import_addrs[i] == NULL) {
            nullfree(import_addrs);
            cc_error("unresolved import '%s'", scri->imports[i]);
            return false;
        }
    }

    /* // [IKM] 2012-09-26:
       // Do not perform most fixups at startup, store fixup data instead
       // for real-time fixups during instance run.
       */
    // So far the helpers are needed only for fixups, but FIXUP_IMPORT may
    // change two code values; here we assume the worst possible scenario
    // (until dynamic lists are implemented in AGS base source)
    codehelpers_capacity = scri->numfixups << 1;
    code_helpers = new CodeHelper[codehelpers_capacity];
    codehelper_index = -1;
    for (i = 0; i < scri->numfixups; ++i)
    {
        codehelper_index++;
        long fixup = scri->fixups[i];
        code_helpers[codehelper_index].code_index = fixup;
        code_helpers[codehelper_index].fixup_type = scri->fixuptypes[i];
        switch (scri->fixuptypes[i])
        {
        case FIXUP_GLOBALDATA:
        case FIXUP_FUNCTION:
        case FIXUP_STRING:
        case FIXUP_STACK:
            break; // do nothing yet
        case FIXUP_IMPORT:
            // save the import's address
            {
                // The code slot actually contains a script-relative index of import;
                // here it is translated to an actual import (address and type)
                code_helpers[codehelper_index].import_address = import_addrs[code[fixup]];
                ccInstance *scriptImp = simp.is_script_import(scri->imports[code[fixup]]);
                // If the call is to another script function next CALLEXT
                // must be replaced with CALLAS
                if (scriptImp != NULL && (code[fixup + 1] == SCMD_CALLEXT))
                {
                    codehelper_index++;
                    code_helpers[codehelper_index].code_index = fixup + 1;
                    code_helpers[codehelper_index].fixup_type = FIXUP_IMPORT;
                    code_helpers[codehelper_index].import_inst_id = ((unsigned long)scriptImp->loadedInstanceId);
                }
            }
            break;
        case FIXUP_DATADATA:
            // this is original fixup behavior;
            // instance still has its own copy of globaldata
            if (joined == NULL)
            {
                // supposedly these are only used for strings...
                int32_t temp;
                memcpy(&temp, (char*)&(globaldata[fixup]), 4);
#if defined(AGS_BIG_ENDIAN)
                AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
                temp += (long)globaldata;
#if defined(AGS_BIG_ENDIAN)
                AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
                memcpy(&(globaldata[fixup]), &temp, 4);
            }
            break;
        default:
            nullfree(import_addrs);
            cc_error("internal fixup index error: %d", scri->fixuptypes[i]);
            return false;
        }
    }
    num_codehelpers = codehelper_index + 1;
    /*
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        switch (scri->fixuptypes[i]) {
    case FIXUP_GLOBALDATA:
        code[fixup] += (long)&globaldata[0];
        break;
    case FIXUP_FUNCTION:
        //      code[fixup] += (long)&code[0];
        break;
    case FIXUP_STRING:
        code[fixup] += (long)&strings[0];
        break;
    case FIXUP_IMPORT: {
        unsigned long setTo = import_addrs[code[fixup]];
        ccInstance *scriptImp = simp.is_script_import(scri->imports[code[fixup]]);
        // If the call is to another script function (in a different
        // instance), replace the call with CALLAS so it doesn't do
        // a real x86 JMP to the instruction
        if (scriptImp != NULL) {
            if (code[fixup + 1] == SCMD_CALLEXT) {
                // save the instance ID in the top 4 bits of the instruction
                code[fixup + 1] = SCMD_CALLAS;
                code[fixup + 1] |= ((unsigned long)scriptImp->loadedInstanceId) << INSTANCE_ID_SHIFT;
            }
        }
        code[fixup] = setTo;
        break;
                       }
    case FIXUP_DATADATA:
        if (joined == NULL)
        {
            // supposedly these are only used for strings...

            int32_t temp;
            memcpy(&temp, (char*)&(globaldata[fixup]), 4);
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            temp += (long)globaldata;
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            memcpy(&(globaldata[fixup]), &temp, 4);
        }
        break;
    case FIXUP_STACK:
        code[fixup] += (long)&stack[0];
        break;
    default:
        nullfree(import_addrs);
        cc_error("internal fixup index error");
        return false;
        }
    }
    */
    nullfree(import_addrs);

    exportaddr = (char**)malloc(sizeof(char*) * scri->numexports);

    // find the real address of the exports
    for (i = 0; i < scri->numexports; i++) {
        long etype = (scri->export_addr[i] >> 24L) & 0x000ff;
        long eaddr = (scri->export_addr[i] & 0x00ffffff);
        if (etype == EXPORT_FUNCTION)
            exportaddr[i] = (char *)(eaddr * sizeof(long) + (long)(&code[0]));
        else if (etype == EXPORT_DATA)
            exportaddr[i] = eaddr + (&globaldata[0]);
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
            if (!ccAddExternalScriptSymbol(scri->exports[i], exportaddr[i], this)) {
                cc_error("Export table overflow at '%s'", scri->exports[i]);
                return false;
            }
        }
    }

#ifdef AGS_BIG_ENDIAN
    gSpans.AddSpan(Span((char *)globaldata, globaldatasize));
#endif
    return true;
}

void ccInstance::Free()
{
    if (instanceof != NULL) {
        instanceof->instances--;
        if (instanceof->instances == 0) {
            simp.remove_range((char *)&globaldata[0], globaldatasize);
            simp.remove_range((char *)&code[0], codesize * sizeof(long));
        }
    }

    // remove from the Active Instances list
    if (loadedInstances[loadedInstanceId] == this)
        loadedInstances[loadedInstanceId] = NULL;

#ifdef AGS_BIG_ENDIAN
    gSpans.RemoveSpan(Span((char *)globaldata, globaldatasize));
#endif

    if ((flags & INSTF_SHAREDATA) == 0)
        nullfree(globaldata);

    // [IKM] 2012-09-26: ccInstance share ccScript code now
    // and should not free it, only ccScript object should
    //nullfree(code);
    strings = NULL;
    nullfree(stack);
    nullfree(exportaddr);

    delete [] code_helpers;
}

bool ccInstance::ReadOperation(CodeOperation &op, long at_pc)
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

    const CodeHelper *helper = GetCodeHelper(at_pc);
    if (helper)
    {
        FixupInstruction(*helper, op.Instruction);
    }

    for (int i = 0; i < op.ArgCount; ++i)
    {
        op.Args[i].Value    = code[at_pc + i + 1];
        helper              = GetCodeHelper(at_pc + i + 1);
        if (helper)
        {
            FixupArgument(*helper, op.Args[i]);
        }
    }

    return true;
}

const CodeHelper *ccInstance::GetCodeHelper(long at_pc)
{
    if (!num_codehelpers)
    {
        return NULL;
    }
    else if (code_helpers[0].code_index > at_pc)
    {
        codehelper_index = -1;
        return NULL;
    } 
    else if (code_helpers[num_codehelpers - 1].code_index < at_pc)
    {
        codehelper_index = num_codehelpers - 1;
        return NULL;
    }

    //
    // What we are going to do is to set code helper index either
    // at helper which corresponds to given program counter, or
    // at helper which corresponds to closest previous program counter.
    //

    // If index is out of range, set it to valid element to start with
    if (codehelper_index < 0)
    {
        codehelper_index = 0;
    }
    else if (codehelper_index >= num_codehelpers)
    {
        codehelper_index = num_codehelpers - 1;
    }

    // We take into consideration the fact that in most usual case
    // the program counter advances forward by 1-3 slots each time
    // an operation and arguments are being read, so at first we just
    // move helper index one step forward, to save time.
    if (codehelper_index != num_codehelpers - 1 &&
        at_pc > code_helpers[codehelper_index].code_index)
    {
        codehelper_index++;
        if (at_pc < code_helpers[codehelper_index].code_index)
        {
            // Jumped over pc, rewind and return nothing
            codehelper_index--;
            return NULL;
        }
    }

    // No luck? Do a binary search.
    if (at_pc != code_helpers[codehelper_index].code_index)
    {
        int first   = 0;
        int last    = num_codehelpers;
        int mid;
        while (first < last)
        {
            mid = first + ((last - first) >> 1);
            if (at_pc <= code_helpers[mid].code_index)
            {
                last = mid;
            }
            else
            {
                first = mid + 1;
            }
        }
        codehelper_index = last;
    }

    if (at_pc == code_helpers[codehelper_index].code_index)
    {
        return &code_helpers[codehelper_index];
    }
    // Since we just did binary search we should be as close to pc
    // as possible; therefore if we have jumped over pc, rewind one
    // step back and stay: this will put code helper index just
    // before the pc and we may benefit from the fast check next time.
    if (at_pc < code_helpers[codehelper_index].code_index)
    {
        codehelper_index--;
    }
    return NULL;
}

void ccInstance::FixupInstruction(const CodeHelper &helper, CodeInstruction &instruction)
{
    // There are not so much acceptable variants here
    if (helper.fixup_type == FIXUP_IMPORT)
    {
        if (instruction.Code == SCMD_CALLEXT) {
            // save the instance ID in the top 4 bits of the instruction
            instruction.Code        = SCMD_CALLAS;
            instruction.InstanceId  = helper.import_inst_id;
            return;
        }
    }

    const char *cmd_name = sccmd_info[instruction.Code].CmdName;
    while (cmd_name[0] == '$')
    {
        cmd_name++;
    }
    cc_error("unexpected instruction/fixup pair: %s - %s", cmd_name, fixupnames[helper.fixup_type]);
}

void ccInstance::FixupArgument(const CodeHelper &helper, CodeArgument &argument)
{
    switch (helper.fixup_type)
    {
    case FIXUP_GLOBALDATA:
        argument.Value += (long)&globaldata[0];
        break;
    case FIXUP_FUNCTION:
        // originally commented --
        //      code[fixup] += (long)&code[0];
        break;
    case FIXUP_STRING:
        argument.Value += (long)&strings[0];
        break;
    case FIXUP_IMPORT:
        argument.Value = helper.import_address;
        break;
    case FIXUP_DATADATA:
        // fixup is being made at instance init
        break;
    case FIXUP_STACK:
        argument.Value += (long)&stack[0];
        break;
    default:
        cc_error("internal fixup index error: %d", helper.fixup_type);
        break;
    }
}
