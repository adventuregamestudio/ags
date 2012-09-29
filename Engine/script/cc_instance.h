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

#ifndef __CC_INSTANCE_H
#define __CC_INSTANCE_H

#include "script/script_common.h"
#include "script/cc_script.h"  // ccScript
#include "script/nonblockingscriptfunction.h"

#define INSTF_SHAREDATA   1
#define INSTF_ABORTED     2
#define INSTF_FREE        4
#define INSTF_RUNNING     8   // set by main code to confirm script isn't stuck
#define CC_STACK_SIZE     (1000 * sizeof(long))
#define MAX_CALL_STACK    100

// 256 because we use 8 bits to hold instance number
#define MAX_LOADED_INSTANCES 256

#define INSTANCE_ID_SHIFT 24
#define INSTANCE_ID_MASK  0x00000ff
#define INSTANCE_ID_REMOVEMASK 0x00ffffff

struct ccInstance;
struct ScriptImport;

enum ScriptValueType
{
    kScValUndefined,    // to detect errors
    kScValGeneric,      // as long
    kScValInteger,      // as strictly 32-bit integer (for integer math)
    kScValFloat,        // as float (for floating point math)
};

struct CodeHelper
{
    CodeHelper()
    {
        code_index      = 0;
        fixup_type      = 0;     
    }

    long            code_index;
    char            fixup_type;
};

struct CodeInstruction
{
	CodeInstruction()
	{
		Code		= 0;
		InstanceId	= 0;
	}

	long	Code;
	long	InstanceId;
};

struct CodeArgument
{
	CodeArgument()
		: FValue((float&)Value)
	{
		Value		= 0;
	}

	long	Value;		// generic Value
	float	&FValue;	// access Value as float type
};


struct CodeOperation
{
	CodeOperation()
	{
		ArgCount = 0;
	}

	CodeInstruction	Instruction;
	CodeArgument	Args[MAX_SCMD_ARGS];
	int				ArgCount;
};

// Running instance of the script
struct ccInstance
{
public:
    long flags;
    char *globaldata;
    long globaldatasize;
    unsigned long *code;
    ccInstance *runningInst;  // might point to another instance if in far call
    long codesize;
    char *strings;
    long stringssize;
    char **exportaddr;  // real pointer to export
    char *stack;
    long stacksize;
    long registers[CC_NUM_REGISTERS];
    long pc;                        // program counter
    long line_number;               // source code line number
    ccScript *instanceof;
    long callStackLineNumber[MAX_CALL_STACK];
    long callStackAddr[MAX_CALL_STACK];
    ccInstance *callStackCodeInst[MAX_CALL_STACK];
    int  callStackSize;
    int  loadedInstanceId;
    int  returnValue;

    // array of real import indexes used in script
    int  *resolved_imports;
    int  numimports;

    CodeHelper  *code_helpers;
    int  codehelpers_capacity;
    int  num_codehelpers;
    int  codehelper_index;

#if defined(AGS_64BIT)
    // 64 bit: Variables to keep track of the size of the variables on the stack.
    // This is necessary because the compiled code accesses values on the stack with
    // absolute offsets that don't take into account the 8 byte long pointers on
    // 64 bit systems. These variables help with rewriting the offsets for the
    // modified stack.
    int stackSizes[CC_STACK_SIZE];
    int stackSizeIndex;
#endif

    // returns the currently executing instance, or NULL if none
    static ccInstance *GetCurrentInstance(void);
    // create a runnable instance of the supplied script
    static ccInstance *CreateFromScript(ccScript *script);
    static ccInstance *CreateEx(ccScript * scri, ccInstance * joined);

    ccInstance();
    ~ccInstance();
    // create a runnable instance of the same script, sharing global memory
    ccInstance *Fork();
    // specifies that when the current function returns to the script, it
    // will stop and return from CallInstance
    void    Abort();
    // aborts instance, then frees the memory later when it is done with
    void    AbortAndDestroy();
    
    // call an exported function in the script (2nd arg is number of params)
    int     CallScriptFunction(char *, long, ...);
    void    DoRunScriptFuncCantBlock(NonBlockingScriptFunction* funcToRun, bool *hasTheFunc);
    int     PrepareTextScript(char**tsname);
    int     Run(long curpc);
    int     RunScriptFunctionIfExists(char*tsname,int numParam, long iparam, long iparam2, long iparam3 = 0);
    int     RunTextScript(char*tsname);
    int     RunTextScriptIParam(char*tsname, long iparam);
    int     RunTextScript2IParam(char*tsname,long iparam,long param2);
    
    void    GetCallStack(char *buffer, int maxLines);
    void    GetScriptName(char *curScrName);
    // get the address of an exported variable in the script
    char    *GetSymbolAddress(char *);
    void    DumpInstruction(unsigned long *codeptr, int cps, int spp);

    // changes all pointer variables (ie. strings) to have the relative address, to allow
    // the data segment to be saved to disk
    void    FlattenGlobalData();
    // restores the pointers after a save
    void    UnFlattenGlobalData();

protected:
    bool    _Create(ccScript * scri, ccInstance * joined);
    // free the memory associated with the instance
    void    Free();

    bool    ResolveScriptImports(ccScript * scri);
	bool    ReadOperation(CodeOperation &op, long at_pc);

    const   CodeHelper *GetCodeHelper(long at_pc);
    void    FixupInstruction(const CodeHelper &helper, CodeInstruction &instruction);
    void    FixupArgument(const CodeHelper &helper, CodeArgument &argument);
};

#endif // __CC_INSTANCE_H
