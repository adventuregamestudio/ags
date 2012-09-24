
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

// Internally used names for commands, registers
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

// Number of arguments for each command
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

ccInstance *current_instance;

ccInstance *ccInstance::GetCurrentInstance()
{
    return current_instance;
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

int ccInstance::RunTextScriptIParam(char*tsname,long iparam) {
    if ((strcmp(tsname, "on_key_press") == 0) || (strcmp(tsname, "on_mouse_click") == 0)) {
        bool eventWasClaimed;
        int toret = run_claimable_event(tsname, true, 1, iparam, 0, &eventWasClaimed);

        if (eventWasClaimed)
            return toret;
    }

    return RunScriptFunctionIfExists(tsname, 1, iparam, 0);
}

extern int gameHasBeenRestored; // in ac/game
extern ExecutingScript*curscript; // in script/script

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

extern int guis_need_update; // in gui/guimain

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

extern int displayed_room; // in ac/game

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

extern roomstruct thisroom; // ac/game

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

extern ccInstance *loadedInstances[MAX_LOADED_INSTANCES]; // in script/script_runtime
extern void nullfree(void *data); // in script/script_runtime

ccInstance *ccInstance::CreateEx(ccScript * scri, ccInstance * joined)
{
    int i;

    currentline = -1;
    if ((scri == NULL) && (joined != NULL))
        scri = joined->instanceof;

    if (scri == NULL) {
        cc_error("null pointer passed");
        return NULL;
    }

    // allocate and copy all the memory with data, code and strings across
    ccInstance *cinst = (ccInstance *) malloc(sizeof(ccInstance));
    cinst->instanceof = NULL;
    cinst->exportaddr = NULL;
    cinst->callStackSize = 0;

    if (joined != NULL) {
        // share memory space with an existing instance (ie. this is a thread/fork)
        cinst->globaldatasize = joined->globaldatasize;
        cinst->globaldata = joined->globaldata;
    } 
    else {
        // create own memory space
        cinst->globaldatasize = scri->globaldatasize;
        if (cinst->globaldatasize > 0) {
            cinst->globaldata = (char *)malloc(cinst->globaldatasize);
            memcpy(cinst->globaldata, scri->globaldata, cinst->globaldatasize);
        }
        else
            cinst->globaldata = NULL;
    }
    cinst->codesize = scri->codesize;

    if (cinst->codesize > 0) {
        cinst->code = (unsigned long *)malloc(cinst->codesize * sizeof(long));
        memcpy(cinst->code, scri->code, cinst->codesize * sizeof(long));
    }
    else
        cinst->code = NULL;
    // just use the pointer to the strings since they don't change
    cinst->strings = scri->strings;
    cinst->stringssize = scri->stringssize;
    // create a stack
    cinst->stacksize = CC_STACK_SIZE;
    cinst->stack = (char *)malloc(cinst->stacksize);
    if (cinst->stack == NULL) {
        cc_error("not enough memory to allocate stack");
        return NULL;
    }

    // find a LoadedInstance slot for it
    for (i = 0; i < MAX_LOADED_INSTANCES; i++) {
        if (loadedInstances[i] == NULL) {
            loadedInstances[i] = cinst;
            cinst->loadedInstanceId = i;
            break;
        }
        if (i == MAX_LOADED_INSTANCES - 1) {
            cc_error("too many active instances");
            cinst->Free();
            return NULL;
        }
    }

    // set up the initial registers to zero
    memset(&cinst->registers[0], 0, sizeof(long) * CC_NUM_REGISTERS);

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
            cinst->Free();
            return NULL;
        }
    }

    // perform the fixups
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        switch (scri->fixuptypes[i]) {
    case FIXUP_GLOBALDATA:
        cinst->code[fixup] += (long)&cinst->globaldata[0];
        break;
    case FIXUP_FUNCTION:
        //      cinst->code[fixup] += (long)&cinst->code[0];
        break;
    case FIXUP_STRING:
        cinst->code[fixup] += (long)&cinst->strings[0];
        break;
    case FIXUP_IMPORT: {
        unsigned long setTo = import_addrs[cinst->code[fixup]];
        ccInstance *scriptImp = simp.is_script_import(scri->imports[cinst->code[fixup]]);
        // If the call is to another script function (in a different
        // instance), replace the call with CALLAS so it doesn't do
        // a real x86 JMP to the instruction
        if (scriptImp != NULL) {
            if (cinst->code[fixup + 1] == SCMD_CALLEXT) {
                // save the instance ID in the top 4 bits of the instruction
                cinst->code[fixup + 1] = SCMD_CALLAS;
                cinst->code[fixup + 1] |= ((unsigned long)scriptImp->loadedInstanceId) << INSTANCE_ID_SHIFT;
            }
        }
        cinst->code[fixup] = setTo;
        break;
                       }
    case FIXUP_DATADATA:
        if (joined == NULL)
        {
            // supposedly these are only used for strings...

            int32_t temp;
            memcpy(&temp, (char*)&(cinst->globaldata[fixup]), 4);
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            temp += (long)cinst->globaldata;
#if defined(AGS_BIG_ENDIAN)
            AGS::Common::BitByteOperations::SwapBytesInt32(temp);
#endif
            memcpy(&(cinst->globaldata[fixup]), &temp, 4);
        }
        break;
    case FIXUP_STACK:
        cinst->code[fixup] += (long)&cinst->stack[0];
        break;
    default:
        nullfree(import_addrs);
        cc_error("internal fixup index error");
        cinst->Free();
        return NULL;
        }
    }
    nullfree(import_addrs);

    cinst->exportaddr = (char**)malloc(sizeof(char*) * scri->numexports);

    // find the real address of the exports
    for (i = 0; i < scri->numexports; i++) {
        long etype = (scri->export_addr[i] >> 24L) & 0x000ff;
        long eaddr = (scri->export_addr[i] & 0x00ffffff);
        if (etype == EXPORT_FUNCTION)
            cinst->exportaddr[i] = (char *)(eaddr * sizeof(long) + (long)(&cinst->code[0]));
        else if (etype == EXPORT_DATA)
            cinst->exportaddr[i] = eaddr + (&cinst->globaldata[0]);
        else {
            cc_error("internal export fixup error");
            cinst->Free();
            return NULL;
        }
    }
    cinst->instanceof = scri;
    cinst->pc = 0;
    cinst->flags = 0;
    if (joined != NULL)
        cinst->flags = INSTF_SHAREDATA;
    scri->instances++;

    if ((scri->instances == 1) && (ccGetOption(SCOPT_AUTOIMPORT) != 0)) {
        // import all the exported stuff from this script
        for (i = 0; i < scri->numexports; i++) {
            if (simp.add(scri->exports[i], cinst->exportaddr[i], cinst)) {
                cinst->Free();
                cc_error("Export table overflow at '%s'", scri->exports[i]);
                return NULL;
            }
        }
    }

#ifdef AGS_BIG_ENDIAN
    gSpans.AddSpan(Span((char *)cinst->globaldata, cinst->globaldatasize));
#endif

    return cinst;
}

ccInstance *ccInstance::CreateFromScript(ccScript * scri)
{
    return CreateEx(scri, NULL);
}

ccInstance *ccInstance::Fork()
{
    return CreateEx(instanceof, this);
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

    nullfree(code);
    strings = NULL;
    nullfree(stack);
    nullfree(exportaddr);
    free(this);
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


extern new_line_hook_type new_line_hook;

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
    const char *toprint = sccmdnames[thisop];
    if (toprint[0] == '$') {
        isreg = 1;
        toprint++;
    }

    if (toprint[0] == '$') {
        isreg |= 2;
        toprint++;
    }
    writer.WriteString(toprint);

    for (l = 0; l < sccmdargs[thisop]; l++) {
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


extern int maxWhileLoops;

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

    long arg1, arg2;
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
    unsigned long thisInstruction;
    int write_debug_dump = ccGetOption(SCOPT_DEBUGRUN);

    while (1) {
        thisInstruction = codeInst->code[pc] & INSTANCE_ID_REMOVEMASK;
        if (write_debug_dump)
            DumpInstruction(&codeInst->code[pc], pc, registers[SREG_SP]);

        // save the arguments for quick access
        if (pc != (codeInst->codesize - 1)) {
            arg1 = codeInst->code[pc + 1];
            freg1 = (float*)&registers[arg1];
            if (pc != (codeInst->codesize - 2)) {
                arg2 = codeInst->code[pc + 2];
                freg2 = (float*)&registers[arg2];
            }
        }

        switch (thisInstruction) {
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

          temp_variable = pc + sccmdargs[thisInstruction] + 1;
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
          unsigned long instId = (codeInst->code[pc] >> INSTANCE_ID_SHIFT) & INSTANCE_ID_MASK;
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
              int arg3 = codeInst->code[pc + 3];
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
          cc_error("invalid instruction %d found in code stream", thisInstruction);
          return -1;
        }

        if (flags & INSTF_ABORTED)
            return 0;

        pc += sccmdargs[thisInstruction] + 1;
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
