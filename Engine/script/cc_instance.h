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
//
// 'C'-style script interpreter
//
//=============================================================================

#ifndef __CC_INSTANCE_H
#define __CC_INSTANCE_H

#include <unordered_map>
#include "script/script_common.h"
#include "script/cc_script.h"  // ccScript
#include "script/nonblockingscriptfunction.h"
#include "util/string.h"

using namespace AGS;

#define INSTF_SHAREDATA     1
#define INSTF_ABORTED       2
#define INSTF_FREE          4
#define INSTF_RUNNING       8   // set by main code to confirm script isn't stuck
#define CC_STACK_SIZE       250
#define CC_STACK_DATA_SIZE  (1000 * sizeof(int32_t))
#define MAX_CALL_STACK      100

// 256 because we use 8 bits to hold instance number
#define MAX_LOADED_INSTANCES 256

#define INSTANCE_ID_SHIFT 24LL
#define INSTANCE_ID_MASK  0x00000000000000ffLL
#define INSTANCE_ID_REMOVEMASK 0x0000000000ffffffLL

struct ccInstance;
struct ScriptImport;

struct ScriptInstruction
{
    ScriptInstruction()
    {
        Code		= 0;
        InstanceId	= 0;
    }

    int32_t	Code;
    int32_t	InstanceId;
};

struct ScriptOperation
{
	ScriptOperation()
	{
		ArgCount = 0;
	}

	ScriptInstruction   Instruction;
	RuntimeScriptValue	Args[MAX_SCMD_ARGS];
	int				    ArgCount;
};

struct ScriptVariable
{
    ScriptVariable()
    {
        ScAddress   = -1; // address = 0 is valid one, -1 means undefined
    }

    int32_t             ScAddress;  // original 32-bit relative data address, written in compiled script;
                                    // if we are to use Map or HashMap, this could be used as Key
    RuntimeScriptValue  RValue;
};

struct FunctionCallStack;

struct ScriptPosition
{
    ScriptPosition()
        : Line(0)
    {
    }

    ScriptPosition(const Common::String &section, int32_t line)
        : Section(section)
        , Line(line)
    {
    }

    Common::String  Section;
    int32_t         Line;
};

// Running instance of the script
struct ccInstance
{
public:
    // TODO: change to std:: if moved to C++11
    typedef std::tr1::unordered_map<int32_t, ScriptVariable> ScVarMap;
    typedef std::tr1::shared_ptr<ScVarMap>                   PScVarMap;
public:
    int32_t flags;
    PScVarMap globalvars;
    char *globaldata;
    int32_t globaldatasize;
    intptr_t *code;
    ccInstance *runningInst;  // might point to another instance if in far call
    int32_t codesize;
    char *strings;
    int32_t stringssize;
    RuntimeScriptValue *exports;
    RuntimeScriptValue *stack;
    int  num_stackentries;
    // An array for keeping stack data; stack entries reference unknown data from here
    // TODO: probably change to dynamic array later
    char *stackdata;    // for storing stack data of unknown type
    char *stackdata_ptr;// works similar to original stack pointer, points to the next unused byte in stack data array
    int32_t stackdatasize; // conventional size of stack data in bytes
    //
    RuntimeScriptValue registers[CC_NUM_REGISTERS];
    int32_t pc;                     // program counter
    int32_t line_number;            // source code line number
    ccScript *instanceof;
    int  loadedInstanceId;
    int  returnValue;

    int  callStackSize;
    int32_t callStackLineNumber[MAX_CALL_STACK];
    int32_t callStackAddr[MAX_CALL_STACK];
    ccInstance *callStackCodeInst[MAX_CALL_STACK];

    // array of real import indexes used in script
    int  *resolved_imports;
    int  numimports;

    char *code_fixups;

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
    int     CallScriptFunction(const char *funcname, int32_t num_params, RuntimeScriptValue *params);
    bool    DoRunScriptFuncCantBlock(NonBlockingScriptFunction* funcToRun, bool hasTheFunc);
    int     PrepareTextScript(const char **tsname);
    int     Run(int32_t curpc);
    int     RunScriptFunctionIfExists(const char *tsname,int numParam, RuntimeScriptValue *params);
    int     RunTextScript(const char *tsname);
    int     RunTextScriptIParam(const char *tsname, RuntimeScriptValue &iparam);
    int     RunTextScript2IParam(const char *tsname,RuntimeScriptValue &iparam, RuntimeScriptValue &param2);
    
    void    GetCallStack(char *buffer, int maxLines);
    void    GetScriptName(char *curScrName);
    void    GetScriptPosition(ScriptPosition &script_pos);
    // get the address of an exported variable in the script
    RuntimeScriptValue GetSymbolAddress(const char *symname);
    void    DumpInstruction(const ScriptOperation &op);

protected:
    bool    _Create(ccScript * scri, ccInstance * joined);
    // free the memory associated with the instance
    void    Free();

    bool    ResolveScriptImports(ccScript * scri);
    bool    CreateGlobalVars(ccScript * scri);
    bool    AddGlobalVar(const ScriptVariable &glvar);
    ScriptVariable *FindGlobalVar(int32_t var_addr);
    bool    CreateRuntimeCodeFixups(ccScript * scri);
	//bool    ReadOperation(ScriptOperation &op, int32_t at_pc);

    // Runtime fixups
    //bool    FixupArgument(intptr_t code_value, char fixup_type, RuntimeScriptValue &argument);

    // Stack processing
    // Push writes new value and increments stack ptr;
    // stack ptr now points to the __next empty__ entry
    void    PushValueToStack(const RuntimeScriptValue &rval);
    void    PushDataToStack(int32_t num_bytes);
    // Pop decrements stack ptr, returns last stored value and invalidates! stack tail;
    // stack ptr now points to the __next empty__ entry
    RuntimeScriptValue PopValueFromStack();
    // helper function to pop & dump several values
    void    PopValuesFromStack(int32_t num_entries);
    void    PopDataFromStack(int32_t num_bytes);
    // Return stack ptr at given offset from stack head;
    // Offset is in data bytes; program stack ptr is __not__ changed
    RuntimeScriptValue GetStackPtrOffsetFw(int32_t fw_offset);
    // Return stack ptr at given offset from stack tail;
    // Offset is in data bytes; program stack ptr is __not__ changed
    RuntimeScriptValue GetStackPtrOffsetRw(int32_t rw_offset);

    // Function call stack processing
    void    PushToFuncCallStack(FunctionCallStack &func_callstack, const RuntimeScriptValue &rval);
    void    PopFromFuncCallStack(FunctionCallStack &func_callstack, int32_t num_entries);
};

#endif // __CC_INSTANCE_H
