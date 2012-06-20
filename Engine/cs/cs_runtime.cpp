/*
C-Script run-time interpreter (c) 2001 Chris Jones

You must DISABLE OPTIMIZATIONS AND REGISTER VARIABLES in your compiler
when compiling this, or strange results can happen.

There is a problem with importing functions on 16-bit compilers: the
script system assumes that all parameters are passed as 4 bytes, which
ints are not on 16-bit systems. Be sure to define all parameters as longs,
or join the 21st century and switch to DJGPP or Visual C++.

This is UNPUBLISHED PROPRIETARY SOURCE CODE;
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form, in whole or in part, without
prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cs/cs_runtime.h"
#include "cs/cs_common.h"
#include "cs/cc_systemimports.h"
#include "cs/cc_error.h"
#include "cs/cc_spans.h"
#include "cs/cc_options.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "cs/cc_managedobjectpool.h"
#include "cs/cc_instance.h"

#include "bigend.h"
#include "misc.h"


#ifdef AGS_BIG_ENDIAN
Spans gSpans;
#endif

void ccAddExternalSymbol(char *namof, void *addrof)
{
    simp.add(namof, (char *)addrof, NULL);
}

void ccRemoveExternalSymbol(char *namof)
{
    simp.remove(namof);
}

void ccRemoveAllSymbols()
{
    simp.clear();
}

#define INSTANCE_ID_SHIFT 24
#define INSTANCE_ID_MASK  0x00000ff
#define INSTANCE_ID_REMOVEMASK 0x00ffffff
// 256 because we use 8 bits to hold instance number
#define MAX_LOADED_INSTANCES 256

ccInstance *loadedInstances[MAX_LOADED_INSTANCES] = {NULL,
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL, NULL, NULL};

void nullfree(void *data)
{
    if (data != NULL)
        free(data);
}

ccInstance *ccCreateInstanceEx(ccScript * scri, ccInstance * joined)
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
            ccFreeInstance(cinst);
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
            ccFreeInstance(cinst);
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
#ifdef AGS_BIG_ENDIAN
            // supposedly these are only used for strings...
            long *dataPtr = (long *)(&cinst->globaldata[fixup]);
            *dataPtr = __int_swap_endian(*dataPtr);
#endif
            long temp;
            memcpy(&temp, (char*)&(cinst->globaldata[fixup]), 4);
            temp += (long)cinst->globaldata;
            memcpy(&(cinst->globaldata[fixup]), &temp, 4);
#ifdef AGS_BIG_ENDIAN
            // leave the address swapped - will be read in and flipped every time
            *dataPtr = __int_swap_endian(*dataPtr);
#endif
        }
        break;
    case FIXUP_STACK:
        cinst->code[fixup] += (long)&cinst->stack[0];
        break;
    default:
        nullfree(import_addrs);
        cc_error("internal fixup index error");
        ccFreeInstance(cinst);
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
            ccFreeInstance(cinst);
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
                ccFreeInstance(cinst);
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

ccInstance *ccCreateInstance(ccScript * scri)
{
    return ccCreateInstanceEx(scri, NULL);
}

ccInstance *ccForkInstance(ccInstance * instoff)
{
    return ccCreateInstanceEx(instoff->instanceof, instoff);
}

void ccFreeInstance(ccInstance * cinst)
{
    if (cinst->instanceof != NULL) {
        cinst->instanceof->instances--;
        if (cinst->instanceof->instances == 0) {
            simp.remove_range((char *)&cinst->globaldata[0], cinst->globaldatasize);
            simp.remove_range((char *)&cinst->code[0], cinst->codesize * sizeof(long));
        }
    }

    // remove from the Active Instances list
    if (loadedInstances[cinst->loadedInstanceId] == cinst)
        loadedInstances[cinst->loadedInstanceId] = NULL;

#ifdef AGS_BIG_ENDIAN
    gSpans.RemoveSpan(Span((char *)cinst->globaldata, cinst->globaldatasize));
#endif

    if ((cinst->flags & INSTF_SHAREDATA) == 0)
        nullfree(cinst->globaldata);

    nullfree(cinst->code);
    cinst->strings = NULL;
    nullfree(cinst->stack);
    nullfree(cinst->exportaddr);
    free(cinst);
}

// get a pointer to a variable or function exported by the script
char *ccGetSymbolAddr(ccInstance * inst, char *symname)
{
    int k;
    char altName[200];
    sprintf(altName, "%s$", symname);

    for (k = 0; k < inst->instanceof->numexports; k++) {
        if (strcmp(inst->instanceof->exports[k], symname) == 0)
            return inst->exportaddr[k];
        // mangled function name
        if (strncmp(inst->instanceof->exports[k], altName, strlen(altName)) == 0)
            return inst->exportaddr[k];
    }
    return NULL;
}

void *ccGetSymbolAddress(char *namof)
{
    return simp.get_addr_of(namof);
}


char ccRunnerCopyright[] = "ScriptExecuter32 v" SCOM_VERSIONSTR " (c) 2001 Chris Jones";
static int maxWhileLoops = 0;

#define MAX_FUNC_PARAMS 20

void dump_instruction(unsigned long *codeptr, int cps, int spp)
{
    static int line_num = 0;

    if (codeptr[0] == SCMD_LINENUM) {
        line_num = codeptr[1];
        return;
    }

    FILE *dto = ci_fopen("script.log", "at");
    fprintf(dto, "Line %3d, IP:%8d (SP:%8d) ", line_num, cps, spp);

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
    fprintf(dto, "%s", toprint);

    for (l = 0; l < sccmdargs[thisop]; l++) {
        t++;
        if (l > 0)
            fprintf(dto, ",");

        if ((l == 0) && (isreg & 1))
            fprintf(dto, " %s", regnames[codeptr[t]]);
        else if ((l == 1) && (isreg & 2))
            fprintf(dto, " %s", regnames[codeptr[t]]);
        else
            // MACPORT FIX 9/6/5: changed %d to %ld
            fprintf(dto, " %ld", codeptr[t]);
    }
    fprintf(dto, "\n");
    fclose(dto);
}

new_line_hook_type new_line_hook = NULL;

#define CHECK_STACK \
    if ((inst->registers[SREG_SP] - ((long)&inst->stack[0])) >= CC_STACK_SIZE) { \
    cc_error("stack overflow"); \
    return -1; \
    }

// Macros to maintain the call stack
#define PUSH_CALL_STACK(inst) \
    if (inst->callStackSize >= MAX_CALL_STACK) { \
    cc_error("Call stack overflow (recursive call error?)"); \
    return -1; \
    } \
    inst->callStackLineNumber[inst->callStackSize] = inst->line_number;  \
    inst->callStackCodeInst[inst->callStackSize] = inst->runningInst;  \
    inst->callStackAddr[inst->callStackSize] = inst->pc;  \
    inst->callStackSize++ 

#define POP_CALL_STACK(inst) \
    if (inst->callStackSize < 1) { \
    cc_error("Call stack underflow -- internal error"); \
    return -1; \
    } \
    inst->callStackSize--;\
    inst->line_number = inst->callStackLineNumber[inst->callStackSize];\
    currentline = inst->line_number


// parm list is backwards (last arg is parms[0])
int call_function(long addr, int numparm, long *parms, int offset)
{
    parms += offset;

    if (numparm == 1) {
        int (*fparam) (long);
        fparam = (int (*)(long))addr;
        return fparam(parms[0]);
    }

    if (numparm == 2) {
        int (*fparam) (long, long);
        fparam = (int (*)(long, long))addr;
        return fparam(parms[1], parms[0]);
    }

    if (numparm == 3) {
        int (*fparam) (long, long, long);
        fparam = (int (*)(long, long, long))addr;
        return fparam(parms[2], parms[1], parms[0]);
    }

    if (numparm == 4) {
        int (*fparam) (long, long, long, long);
        fparam = (int (*)(long, long, long, long))addr;
        return fparam(parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 5) {
        int (*fparam) (long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long))addr;
        return fparam(parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 6) {
        int (*fparam) (long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long))addr;
        return fparam(parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 7) {
        int (*fparam) (long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long))addr;
        return fparam(parms[6], parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 8) {
        int (*fparam) (long, long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long, long))addr;
        return fparam(parms[7], parms[6], parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    if (numparm == 9) {
        int (*fparam) (long, long, long, long, long, long, long, long, long);
        fparam = (int (*)(long, long, long, long, long, long, long, long, long))addr;
        return fparam(parms[8], parms[7], parms[6], parms[5], parms[4], parms[3], parms[2], parms[1], parms[0]);
    }

    cc_error("too many arguments in call to function");
    return -1;
}

#define MAXNEST 50  // number of recursive function calls allowed
int cc_run_code(ccInstance * inst, long curpc)
{
    inst->pc = curpc;
    inst->returnValue = -1;

    if ((curpc < 0) || (curpc >= inst->runningInst->codesize)) {
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
    funcstart[0] = inst->pc;
    current_instance = inst;
    float *freg1, *freg2;
    ccInstance *codeInst = inst->runningInst;
    unsigned long thisInstruction;
    int write_debug_dump = ccGetOption(SCOPT_DEBUGRUN);

    while (1) {
        thisInstruction = codeInst->code[inst->pc] & INSTANCE_ID_REMOVEMASK;
        if (write_debug_dump)
            dump_instruction(&codeInst->code[inst->pc], inst->pc, inst->registers[SREG_SP]);

        // save the arguments for quick access
        if (inst->pc != (codeInst->codesize - 1)) {
            arg1 = codeInst->code[inst->pc + 1];
            freg1 = (float*)&inst->registers[arg1];
            if (inst->pc != (codeInst->codesize - 2)) {
                arg2 = codeInst->code[inst->pc + 2];
                freg2 = (float*)&inst->registers[arg2];
            }
        }

        switch (thisInstruction) {
      case SCMD_LINENUM:
          inst->line_number = arg1;
          currentline = arg1;
          if (new_line_hook)
              new_line_hook(inst, currentline);
          break;
      case SCMD_ADD:
          inst->registers[arg1] += arg2;
          CHECK_STACK 
              break;
      case SCMD_SUB:
          inst->registers[arg1] -= arg2;
          break;
      case SCMD_REGTOREG:
          inst->registers[arg2] = inst->registers[arg1];
          break;
      case SCMD_WRITELIT:
          // poss something dodgy about this routine
          mptr = (char *)(inst->registers[SREG_MAR]);
          memcpy(&mptr[0], &arg2, arg1);
          break;
      case SCMD_RET:
          if (loopIterationCheckDisabled > 0)
              loopIterationCheckDisabled--;

          inst->registers[SREG_SP] -= 4;
          curnest--;
          memcpy(&(inst->pc), (char*)inst->registers[SREG_SP], 4);
          if (inst->pc == 0)
          {
              inst->returnValue = inst->registers[SREG_AX];
              return 0;
          }
          current_instance = inst;
          POP_CALL_STACK(inst);
          continue;                 // continue so that the PC doesn't get overwritten
      case SCMD_LITTOREG:
          inst->registers[arg1] = arg2;
          break;
      case SCMD_MEMREAD:
          memcpy(&(inst->registers[arg1]), (char*)inst->registers[SREG_MAR], 4);
#ifdef AGS_BIG_ENDIAN
          {
              // check if we're reading from the script's global data
              // if so, swap endian
              char *charPtr = (char *)inst->registers[SREG_MAR];
              if (gSpans.IsInSpan(charPtr))
              {
                  inst->registers[arg1] = __int_swap_endian(inst->registers[arg1]);
              }
          }
#endif
          break;
      case SCMD_MEMWRITE:
          memcpy((char*)inst->registers[SREG_MAR], &(inst->registers[arg1]), 4);
#ifdef AGS_BIG_ENDIAN
          {
              // check if we're writing to the script's global data
              // if so, swap endian
              char *charPtr = (char *)inst->registers[SREG_MAR];
              if (gSpans.IsInSpan(charPtr))
              {
                  long *dataPtr = (long *)charPtr;
                  *dataPtr = __int_swap_endian(*dataPtr);
              }
          }
#endif
          break;
      case SCMD_LOADSPOFFS:
          inst->registers[SREG_MAR] = inst->registers[SREG_SP] - arg1;
          break;
      case SCMD_MULREG:
          inst->registers[arg1] *= inst->registers[arg2];
          break;
      case SCMD_DIVREG:
          if (inst->registers[arg2] == 0) {
              cc_error("!Integer divide by zero");
              return -1;
          } 
          inst->registers[arg1] /= inst->registers[arg2];
          break;
      case SCMD_ADDREG:
          inst->registers[arg1] += inst->registers[arg2];
          break;
      case SCMD_SUBREG:
          inst->registers[arg1] -= inst->registers[arg2];
          break;
      case SCMD_BITAND:
          inst->registers[arg1] = inst->registers[arg1] & inst->registers[arg2];
          break;
      case SCMD_BITOR:
          inst->registers[arg1] = inst->registers[arg1] | inst->registers[arg2];
          break;
      case SCMD_ISEQUAL:
          inst->registers[arg1] = (inst->registers[arg1] == inst->registers[arg2]);
          break;
      case SCMD_NOTEQUAL:
          inst->registers[arg1] = (inst->registers[arg1] != inst->registers[arg2]);
          break;
      case SCMD_GREATER:
          inst->registers[arg1] = (inst->registers[arg1] > inst->registers[arg2]);
          break;
      case SCMD_LESSTHAN:
          inst->registers[arg1] = (inst->registers[arg1] < inst->registers[arg2]);
          break;
      case SCMD_GTE:
          inst->registers[arg1] = (inst->registers[arg1] >= inst->registers[arg2]);
          break;
      case SCMD_LTE:
          inst->registers[arg1] = (inst->registers[arg1] <= inst->registers[arg2]);
          break;
      case SCMD_AND:
          inst->registers[arg1] = (inst->registers[arg1] && inst->registers[arg2]);
          break;
      case SCMD_OR:
          inst->registers[arg1] = (inst->registers[arg1] || inst->registers[arg2]);
          break;
      case SCMD_XORREG:
          inst->registers[arg1] = (inst->registers[arg1] ^ inst->registers[arg2]);
          break;
      case SCMD_MODREG:
          if (inst->registers[arg2] == 0) {
              cc_error("!Integer divide by zero");
              return -1;
          } 
          inst->registers[arg1] %= inst->registers[arg2];
          break;
      case SCMD_NOTREG:
          inst->registers[arg1] = !(inst->registers[arg1]);
          break;
      case SCMD_CALL:
          // Call another function within same script, just save PC
          // and continue from there
          if (curnest >= MAXNEST - 1) {
              cc_error("!call stack overflow, recursive call problem?");
              return -1;
          }

          PUSH_CALL_STACK(inst);

          temp_variable = inst->pc + sccmdargs[thisInstruction] + 1;
          memcpy((char*)inst->registers[SREG_SP], &temp_variable, 4);

          inst->registers[SREG_SP] += 4;

          if (thisbase[curnest] == 0)
              inst->pc = inst->registers[arg1];
          else {
              inst->pc = funcstart[curnest];
              inst->pc += (inst->registers[arg1] - thisbase[curnest]);
          }

          if (next_call_needs_object)  // is this right?
              next_call_needs_object = 0;

          if (loopIterationCheckDisabled)
              loopIterationCheckDisabled++;

          curnest++;
          thisbase[curnest] = 0;
          funcstart[curnest] = inst->pc;
          CHECK_STACK
              continue;
      case SCMD_MEMREADB:
          tbyte = *((unsigned char *)inst->registers[SREG_MAR]);
          inst->registers[arg1] = tbyte;
          break;
      case SCMD_MEMREADW:
          tshort = *((short *)inst->registers[SREG_MAR]);
          inst->registers[arg1] = tshort;
#ifdef AGS_BIG_ENDIAN
          {
              // check if we're reading from the script's global data
              // if so, swap endian
              char *charPtr = (char *)inst->registers[SREG_MAR];
              if (gSpans.IsInSpan(charPtr))
              {
                  inst->registers[arg1] = __short_swap_endian(inst->registers[arg1]);
              }
          }
#endif
          break;
      case SCMD_MEMWRITEB:
          tbyte = (unsigned char)inst->registers[arg1];
          *((unsigned char *)inst->registers[SREG_MAR]) = tbyte;
          break;
      case SCMD_MEMWRITEW:
          tshort = (short)inst->registers[arg1];
          *((short *)inst->registers[SREG_MAR]) = tshort;
#ifdef AGS_BIG_ENDIAN
          {
              // check if we're writing to the script's global data
              // if so, swap endian
              char *charPtr = (char *)inst->registers[SREG_MAR];
              if (gSpans.IsInSpan(charPtr))
              {
                  short *dataPtr = (short *)charPtr;
                  *dataPtr = __short_swap_endian(*dataPtr);
              }
          }
#endif
          break;
      case SCMD_JZ:
          if (inst->registers[SREG_AX] == 0)
              inst->pc += arg1;
          break;
      case SCMD_JNZ:
          if (inst->registers[SREG_AX] != 0)
              inst->pc += arg1;
          break;
      case SCMD_PUSHREG:
          memcpy((char*)inst->registers[SREG_SP], &(inst->registers[arg1]), 4);
          inst->registers[SREG_SP] += 4;
          CHECK_STACK
              break;
      case SCMD_POPREG:
          inst->registers[SREG_SP] -= 4;
          memcpy(&(inst->registers[arg1]), (char*)inst->registers[SREG_SP], 4);
          break;
      case SCMD_JMP:
          inst->pc += arg1;
          if ((arg1 < 0) && (maxWhileLoops > 0) && (loopIterationCheckDisabled == 0)) {
              // Make sure it's not stuck in a While loop
              loopIterations ++;
              if (inst->flags & INSTF_RUNNING) {
                  loopIterations = 0;
                  inst->flags &= ~INSTF_RUNNING;
              }
              else if (loopIterations > maxWhileLoops) {
                  cc_error("!Script appears to be hung (a while loop ran %d times). The problem may be in a calling function; check the call stack.", loopIterations);
                  return -1;
              }
          }
          break;
      case SCMD_MUL:
          inst->registers[arg1] *= arg2;
          break;
      case SCMD_CHECKBOUNDS:
          if ((inst->registers[arg1] < 0) ||
              (inst->registers[arg1] >= arg2)) {
                  cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", inst->registers[arg1], arg2 - 1);
                  return -1;
          }
          break;
      case SCMD_DYNAMICBOUNDS:
          {
              long upperBoundInBytes = *((long *)(inst->registers[SREG_MAR] - 4));
              if ((inst->registers[arg1] < 0) ||
                  (inst->registers[arg1] >= upperBoundInBytes)) {
                      long upperBound = *((long *)(inst->registers[SREG_MAR] - 8)) & (~ARRAY_MANAGED_TYPE_FLAG);
                      int elementSize = (upperBoundInBytes / upperBound);
                      cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", inst->registers[arg1] / elementSize, upperBound - 1);
                      return -1;
              }
              break;
          }
      case SCMD_MEMREADPTR:
          ccError = 0;

          long handle;
          memcpy(&handle, (char*)(inst->registers[SREG_MAR]), 4);
          inst->registers[arg1] = (long)ccGetObjectAddressFromHandle(handle);

          // if error occurred, cc_error will have been set
          if (ccError)
              return -1;
          break;
      case SCMD_MEMWRITEPTR: {
          long ptr;
          memcpy(&ptr, ((char*)inst->registers[SREG_MAR]), 4);

          long newHandle = ccGetObjectHandleFromAddress((char*)inst->registers[arg1]);
          if (newHandle == -1)
              return -1;

          if (ptr != newHandle) {

              ccReleaseObjectReference(ptr);
              ccAddObjectReference(newHandle);
              memcpy(((char*)inst->registers[SREG_MAR]), &newHandle, 4);
          }
          break;
                             }
      case SCMD_MEMINITPTR: { 
          // like memwriteptr, but doesn't attempt to free the old one

          long ptr;
          memcpy(&ptr, ((char*)inst->registers[SREG_MAR]), 4);

          long newHandle = ccGetObjectHandleFromAddress((char*)inst->registers[arg1]);
          if (newHandle == -1)
              return -1;

          ccAddObjectReference(newHandle);
          memcpy(((char*)inst->registers[SREG_MAR]), &newHandle, 4);
          break;
                            }
      case SCMD_MEMZEROPTR: {
          long ptr;
          memcpy(&ptr, ((char*)inst->registers[SREG_MAR]), 4);
          ccReleaseObjectReference(ptr);
          memset(((char*)inst->registers[SREG_MAR]), 0, 4);

          break;
                            }
      case SCMD_MEMZEROPTRND: {
          long ptr;
          memcpy(&ptr, ((char*)inst->registers[SREG_MAR]), 4);

          // don't do the Dispose check for the object being returned -- this is
          // for returning a String (or other pointer) from a custom function.
          // Note: we might be freeing a dynamic array which contains the DisableDispose
          // object, that will be handled inside the recursive call to SubRef.
          pool.disableDisposeForObject = (const char*)inst->registers[SREG_AX];
          ccReleaseObjectReference(ptr);
          pool.disableDisposeForObject = NULL;
          memset(((char*)inst->registers[SREG_MAR]), 0, 4);
          break;
                              }
      case SCMD_CHECKNULL:
          if (inst->registers[SREG_MAR] == 0) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          break;
      case SCMD_CHECKNULLREG:
          if (inst->registers[arg1] == 0) {
              cc_error("!Null string referenced");
              return -1;
          }
          break;
      case SCMD_NUMFUNCARGS:
          num_args_to_func = arg1;
          break;
      case SCMD_CALLAS:{
          PUSH_CALL_STACK(inst);

          // Call to a function in another script
          int startArg = 0;
          // If there are nested CALLAS calls, the stack might
          // contain 2 calls worth of parameters, so only
          // push args for this call
          if (num_args_to_func >= 0)
              startArg = callstacksize - num_args_to_func;

          for (aa = startArg; aa < callstacksize; aa++) {
              memcpy((char*)inst->registers[SREG_SP], &(callstack[aa]), 4);
              inst->registers[SREG_SP] += 4;
          }

          // 0, so that the cc_run_code returns
          memset((char*)inst->registers[SREG_SP], 0, 4);

          long oldstack = inst->registers[SREG_SP];
          inst->registers[SREG_SP] += 4;
          CHECK_STACK
              int oldpc = inst->pc;
          ccInstance *wasRunning = inst->runningInst;

          // extract the instance ID
          unsigned long instId = (codeInst->code[inst->pc] >> INSTANCE_ID_SHIFT) & INSTANCE_ID_MASK;
          // determine the offset into the code of the instance we want
          inst->runningInst = loadedInstances[instId];
          unsigned long callAddr = inst->registers[arg1] - (unsigned long)(&inst->runningInst->code[0]);
          if (callAddr % 4 != 0) {
              cc_error("call address not aligned");
              return -1;
          }
          callAddr /= 4;

          if (cc_run_code(inst, callAddr))
              return -1;

          inst->runningInst = wasRunning;

          if (inst->flags & INSTF_ABORTED)
              return 0;

          if (oldstack != inst->registers[SREG_SP]) {
              cc_error("stack corrupt after function call");
              return -1;
          }

          if (next_call_needs_object)
              next_call_needs_object = 0;

          inst->pc = oldpc;
          was_just_callas = callstacksize;
          num_args_to_func = -1;
          POP_CALL_STACK(inst);
          break;
                       }
      case SCMD_CALLEXT: {
          int call_uses_object = 0;
          // Call to a real 'C' code function
          was_just_callas = -1;
          if (num_args_to_func < 0)
              num_args_to_func = callstacksize;

          if (next_call_needs_object) {
              // member function call
              // use the callstack +1 size allocation to squeeze
              // the object address on as the last parameter
              call_uses_object = 1;
              next_call_needs_object = 0;
              callstack[callstacksize] = inst->registers[SREG_OP];
              inst->registers[SREG_AX] = call_function(inst->registers[arg1], num_args_to_func + 1, callstack, callstacksize - num_args_to_func);
          }
          else if (num_args_to_func == 0) {
              realfunc = (int (*)())inst->registers[arg1];
              inst->registers[SREG_AX] = realfunc();
          } 
          else
              inst->registers[SREG_AX] = call_function(inst->registers[arg1], num_args_to_func, callstack, callstacksize - num_args_to_func);

          if (ccError)
              return -1;

          if (call_uses_object) {
              // Pop OP?
          }

          current_instance = inst;
          num_args_to_func = -1;
          break;
                         }
      case SCMD_PUSHREAL:
          //        printf("pushing arg%d as %ld\n",callstacksize,inst->registers[arg1]);
          if (callstacksize >= MAX_FUNC_PARAMS) {
              cc_error("Call stack overflow");
              return -1;
          }
          callstack[callstacksize] = inst->registers[arg1];
          callstacksize++;
          break;
      case SCMD_SUBREALSTACK:
          if (was_just_callas >= 0) {
              inst->registers[SREG_SP] -= arg1 * 4;
              was_just_callas = -1;
          }
          callstacksize -= arg1;
          break;
      case SCMD_CALLOBJ:
          // set the OP register
          if (inst->registers[arg1] == 0) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          inst->registers[SREG_OP] = inst->registers[arg1];
          next_call_needs_object = 1;
          break;
      case SCMD_SHIFTLEFT:
          inst->registers[arg1] = (inst->registers[arg1] << inst->registers[arg2]);
          break;
      case SCMD_SHIFTRIGHT:
          inst->registers[arg1] = (inst->registers[arg1] >> inst->registers[arg2]);
          break;
      case SCMD_THISBASE:
          thisbase[curnest] = arg1;
          break;
      case SCMD_NEWARRAY:
          {
              int arg3 = codeInst->code[inst->pc + 3];
              int numElements = inst->registers[arg1];
              if ((numElements < 1) || (numElements > 1000000))
              {
                  cc_error("invalid size for dynamic array; requested: %d, range: 1..1000000", numElements);
                  return -1;
              }
              inst->registers[arg1] = (long)ccGetObjectAddressFromHandle(globalDynamicArray.Create(numElements, arg2, (arg3 == 1)));
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
          mptr = (char *)(inst->registers[SREG_MAR]);
          if (inst->registers[SREG_MAR] == inst->registers[SREG_SP]) {
              // creating a local variable -- check the stack to ensure no mem overrun
              int currentStackSize = inst->registers[SREG_SP] - ((long)&inst->stack[0]);
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
          inst->registers[arg1] = (long)stringClassImpl->CreateString((const char *)(inst->registers[arg1]));
          break;
      case SCMD_STRINGSEQUAL:
          if ((inst->registers[arg1] == 0) || (inst->registers[arg2] == 0)) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          if (strcmp((const char*)inst->registers[arg1], (const char*)inst->registers[arg2]) == 0)
              inst->registers[arg1] = 1;
          else
              inst->registers[arg1] = 0;
          break;
      case SCMD_STRINGSNOTEQ:
          if ((inst->registers[arg1] == 0) || (inst->registers[arg2] == 0)) {
              cc_error("!Null pointer referenced");
              return -1;
          }
          if (strcmp((const char*)inst->registers[arg1], (const char*)inst->registers[arg2]) != 0)
              inst->registers[arg1] = 1;
          else
              inst->registers[arg1] = 0;
          break;
      case SCMD_LOOPCHECKOFF:
          if (loopIterationCheckDisabled == 0)
              loopIterationCheckDisabled++;
          break;
      default:
          cc_error("invalid instruction %d found in code stream", thisInstruction);
          return -1;
        }

        if (inst->flags & INSTF_ABORTED)
            return 0;

        inst->pc += sccmdargs[thisInstruction] + 1;
    }
}

int ccCallInstance(ccInstance * inst, char *funcname, long numargs, ...)
{
    ccError = 0;
    currentline = 0;

    if ((numargs >= 20) || (numargs < 0)) {
        cc_error("too many arguments to function");
        return -3;
    }

    if (inst->pc != 0) {
        cc_error("instance already being executed");
        return -4;
    }

    long startat = -1;
    int k;
    char mangledName[200];
    sprintf(mangledName, "%s$", funcname);

    for (k = 0; k < inst->instanceof->numexports; k++) {
        char *thisExportName = inst->instanceof->exports[k];
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
            long etype = (inst->instanceof->export_addr[k] >> 24L) & 0x000ff;
            if (etype != EXPORT_FUNCTION) {
                cc_error("symbol is not a function");
                return -1;
            }
            startat = (inst->instanceof->export_addr[k] & 0x00ffffff);
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
    inst->flags &= ~INSTF_ABORTED;

    // object pointer needs to start zeroed
    inst->registers[SREG_OP] = 0;

    ccInstance* currentInstanceWas = current_instance;
    long stoffs = 0;
    for (tssize = numargs - 1; tssize >= 0; tssize--) {
        memcpy(&inst->stack[stoffs], &tempstack[tssize], sizeof(long));
        stoffs += sizeof(long);
    }
    inst->registers[SREG_SP] = (long)(&inst->stack[0]);
    inst->registers[SREG_SP] += (numargs * sizeof(long));
    inst->runningInst = inst;

    int reterr = cc_run_code(inst, startat);
    inst->registers[SREG_SP] -= (numargs - 1) * sizeof(long);
    inst->pc = 0;
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

    if (inst->flags & INSTF_ABORTED) {
        inst->flags &= ~INSTF_ABORTED;

        if (inst->flags & INSTF_FREE)
            ccFreeInstance(inst);
        return 100;
    }

    if (inst->registers[SREG_SP] != (long)&inst->stack[0]) {
        cc_error("stack pointer was not zero at completion of script");
        return -5;
    }
    return ccError;
}

void ccAbortInstance(ccInstance * cinst)
{
    if ((cinst != NULL) && (cinst->pc != 0))
        cinst->flags |= INSTF_ABORTED;
}

void ccAbortAndDestroyInstance(ccInstance * inst)
{
    if (inst != NULL) {
        ccAbortInstance(inst);
        inst->flags |= INSTF_FREE;
    }
}

// If a while loop does this many iterations without the
// NofityScriptAlive function getting called, the script
// aborts. Set to 0 to disable.
void ccSetScriptAliveTimer (int numloop) {
    maxWhileLoops = numloop;
}

void ccNotifyScriptStillAlive () {
    if (current_instance != NULL)
        current_instance->flags |= INSTF_RUNNING;
}

void ccSetDebugHook(new_line_hook_type jibble)
{
    new_line_hook = jibble;
}

// changes all pointer variables (ie. strings) to have the relative address, to allow
// the data segment to be saved to disk
void ccFlattenGlobalData(ccInstance * cinst)
{
    ccScript *scri = cinst->instanceof;
    int i;

    if (cinst->flags & INSTF_SHAREDATA)
        return;

    // perform the fixups
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        if (scri->fixuptypes[i] == FIXUP_DATADATA) {
#ifdef AGS_BIG_ENDIAN
            // supposedly these are only used for strings...
            long *dataPtr = (long *)(&cinst->globaldata[fixup]);
            *dataPtr = __int_swap_endian(*dataPtr);
#endif
            long temp;
            memcpy(&temp, (char*)&(cinst->globaldata[fixup]), 4);
            temp -= (long)cinst->globaldata;
            memcpy(&(cinst->globaldata[fixup]), &temp, 4);
#ifdef AGS_BIG_ENDIAN
            // leave the address swapped - will be read in and flipped every time
            *dataPtr = __int_swap_endian(*dataPtr);
#endif
        }
    }

}

// restores the pointers after a save
void ccUnFlattenGlobalData(ccInstance * cinst)
{
    ccScript *scri = cinst->instanceof;
    int i;

    if (cinst->flags & INSTF_SHAREDATA)
        return;

    // perform the fixups
    for (i = 0; i < scri->numfixups; i++) {
        long fixup = scri->fixups[i];
        if (scri->fixuptypes[i] == FIXUP_DATADATA) {
#ifdef AGS_BIG_ENDIAN
            // supposedly these are only used for strings...
            long *dataPtr = (long *)(&cinst->globaldata[fixup]);
            *dataPtr = __int_swap_endian(*dataPtr);
#endif
            long temp;
            memcpy(&temp, (char*)&(cinst->globaldata[fixup]), 4);
            temp += (long)cinst->globaldata;
            memcpy(&(cinst->globaldata[fixup]), &temp, 4);
#ifdef AGS_BIG_ENDIAN
            // leave the address swapped - will be read in and flipped every time
            *dataPtr = __int_swap_endian(*dataPtr);
#endif
        }
    }

}
