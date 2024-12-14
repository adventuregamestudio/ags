//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// 'C'-style script interpreter
//
//=============================================================================
#ifndef __CC_INSTANCE_H
#define __CC_INSTANCE_H

#if defined (DISABLED)
#include <map>
#include <memory>
#include <unordered_map>

#include "ac/timer.h"
#include "script/cc_script.h"  // ccScript
#include "script/cc_internal.h"  // bytecode constants
#include "script/runtimescript.h"
#include "script/runtimescriptvalue.h"
#include "util/string.h"

using namespace AGS;

#define INSTF_SHAREDATA     1
#define INSTF_ABORTED       2
#define INSTF_FREE          4
#define INSTF_RUNNING       8   // set by main code to confirm script isn't stuck

// Size of stack in RuntimeScriptValues (aka distinct variables)
#define CC_STACK_SIZE       256
// Size of stack in bytes (raw data storage)
#define CC_STACK_DATA_SIZE  (1024 * sizeof(int32_t))
#define MAX_CALL_STACK      128
#define MAX_FUNCTION_PARAMS 20

// This gives us 1024 unique instance IDs
// NOTE: these are given to the primary instances only, not forks
#define MAX_PRIMARY_INSTANCES   1024

// Script executor debugging flag:
// enables mistake checks, but slows things down!
#ifndef DEBUG_CC_EXEC
#define DEBUG_CC_EXEC (AGS_PLATFORM_DEBUG)
#endif


struct ScriptInstruction
{
    ScriptInstruction() = default;
    ScriptInstruction(int code, int instid) : Code(code), InstanceId(instid) {}

    int32_t	Code = 0;
    int32_t	InstanceId = 0;
};

struct ScriptOperation
{
	ScriptInstruction   Instruction;
	RuntimeScriptValue	Args[MAX_SCMD_ARGS];
	int				    ArgCount = 0;

    // Helper functions for clarity of intent:
    // returns argN, 1-based
    inline const RuntimeScriptValue &Arg1() const { return Args[0]; }
    inline const RuntimeScriptValue &Arg2() const { return Args[1]; }
    inline const RuntimeScriptValue &Arg3() const { return Args[2]; }
    // returns argN as a integer literal
    inline int Arg1i() const { return Args[0].IValue; }
    inline int Arg2i() const { return Args[1].IValue; }
    inline int Arg3i() const { return Args[2].IValue; }
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

enum ccInstError
{
    kInstErr_None = 0, // ok
    kInstErr_Aborted = 100, // aborted by request
    kInstErr_Generic = -1, // any generic exec error; use cc_get_error()
    kInstErr_FuncNotFound = -2, // requested function is not found in script
    kInstErr_InvalidArgNum = -3, // invalid number of args (not in supported range)
    kInstErr_Busy = -4, // instance is busy executing script
};

// Running instance of the script
class ccInstance
{
    using RuntimeScript = AGS::Engine::RuntimeScript;
    using PRuntimeScript = std::shared_ptr<RuntimeScript>;
public:
    // returns the currently executing instance, or NULL if none
    static ccInstance *GetCurrentInstance(void);
    // clears recorded stack of current instances
    // FIXME: reimplement this in a safer way, this must be done automatically
    // when destroying all script instances, e.g. on game quit.
    static void FreeInstanceStack();
    // create a runnable instance of the supplied script
    static std::unique_ptr<ccInstance> CreateFromScript(PRuntimeScript script);
    static std::unique_ptr<ccInstance> CreateEx(PRuntimeScript script, const ccInstance * joined);
    static void SetExecTimeout(unsigned sys_poll_ms, unsigned abort_ms, unsigned abort_loops);
    static void SetPluginReturnValue(const RuntimeScriptValue &value);

    ccInstance() = default;
    ~ccInstance();

    // Get the script that this Instance represents
    AGS::Engine::RuntimeScript *GetScript() const { return _instanceof.get(); }
    // Get the currently executed instance, which may be this instance,
    // or another one in case of a nested "far call"
    ccInstance *GetRunningInst() const { return _runningInst; }
    // Get a readonly access to the global script data
    const std::vector<uint8_t> &GetGlobalData() const { return _instanceof->GetGlobalData(); }
    // Get current program pointer (position in bytecode)
    int     GetPC() const { return _pc; }
    // Gets the top entry of this instance's stack
    const RuntimeScriptValue *GetCurrentStack() const { return _registers[SREG_SP].RValue; }
    // Get latest return value
    int     GetReturnValue() const { return _returnValue; }
    // TODO: this is a hack, required for dialog script; redo this later!
    void    SetReturnValue(int val) { _returnValue = val; }

    // Create a runnable instance of the same script, sharing global memory
    std::unique_ptr<ccInstance> Fork();
    // Specifies that when the current function returns to the script, it
    // will stop and return from CallInstance
    void    Abort();
    // Aborts instance, then frees the memory later when it is done with
    void    AbortAndDestroy();
    
    // Call an exported function in the script
    ccInstError CallScriptFunction(const Common::String &funcname, int32_t num_params, const RuntimeScriptValue *params);
    
    // Get the script's execution position and callstack as human-readable text
    Common::String GetCallStack(int max_lines = INT_MAX) const;
    // Get the script's execution position
    void    GetScriptPosition(ScriptPosition &script_pos) const;
    // Get the address of an exported symbol (function or variable) in the script
    RuntimeScriptValue GetSymbolAddress(const Common::String &symname) const;
    void    DumpInstruction(const ScriptOperation &op) const;
    // Tells whether this instance is in the process of executing the byte-code
    bool    IsBeingRun() const;
    // Notifies that the game was being updated (script not hanging)
    void    NotifyAlive();

    // Copies global data values over to this instance;
    // copies not more than the allocated size of global data
    void    CopyGlobalData(const std::vector<uint8_t> &data);

    // Returns a dictionary that maps local script's typeid to global typeid (in joint RTTI)
    // Requires RTTI
    const std::unordered_map<uint32_t, uint32_t> &
        GetLocal2GlobalTypeMap() const { return _instanceof->GetLocal2GlobalTypeMap(); }
    const std::unordered_map<Common::String, uint32_t> &
        GetGlobalVariableLookup() const { return _instanceof->GetGlobalVariableLookup(); }


private:
    bool    _Create(PRuntimeScript scri, const ccInstance * joined);
    // free the memory associated with the instance
    void    Free();

    // Begin executing script starting from the given bytecode index
    ccInstError Run(int32_t curpc);

    // For calling exported plugin functions old-style
    RuntimeScriptValue CallPluginFunction(void *fn_addr, const RuntimeScriptValue *object, const RuntimeScriptValue *params, int param_count);

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
    // Return stack ptr at given offset from stack tail;
    // Offset is in data bytes; program stack ptr is __not__ changed
    RuntimeScriptValue GetStackPtrOffsetRw(int32_t rw_offset);

    // Function call stack processing
    void    PushToFuncCallStack(FunctionCallStack &func_callstack, const RuntimeScriptValue &rval);
    void    PopFromFuncCallStack(FunctionCallStack &func_callstack, int32_t num_entries);

    int32_t _loadedInstanceId = -1;
    int     _flags = 0; // INSTF_* flags
    // Represented script object
    PRuntimeScript _instanceof;

    // Code pointers for faster access
    const intptr_t *_code = nullptr;
    uint32_t    _codesize = 0; // size of code is limited under 32-bit due to bytecode format
    const uint8_t *_code_fixups = nullptr;
    const char *_strings = nullptr; // pointer to ccScript's string data
    size_t      _stringsize = 0u;
    // Table pointers for simplicity
    const JointRTTI *_rtti = nullptr;
    const std::unordered_map<uint32_t, uint32_t> *_typeidLocal2Global = nullptr;

    // Virtual machine state
    RuntimeScriptValue _registers[CC_NUM_REGISTERS]; // registers
    std::vector<RuntimeScriptValue> _stack;
    // An array for keeping stack data; stack entries reference data of variable size from here
    std::vector<uint8_t> _stackdata;
    RuntimeScriptValue *_stackBegin = nullptr; // fast-access ptr to beginning of _stack
    uint8_t    *_stackdataBegin = nullptr; // fast-access ptr to beginning of _stackdata
    uint8_t    *_stackdataPtr = nullptr; // points to the next unused byte in stack data array
    // Callstack: for storing nested program positions
    int         _callStackLineNumber[MAX_CALL_STACK]{};
    int         _callStackAddr[MAX_CALL_STACK]{};
    ccInstance *_callStackCodeInst[MAX_CALL_STACK]{};
    uint32_t    _callStackSize = 0u;

    ccInstance *_runningInst = nullptr;  // might point to another instance if in far call
    int         _pc = 0; // program counter
    int         _lineNumber = 0; // source code line number
    int         _returnValue = 0; // last executed function's return value

    // A value returned from plugin functions saved as RuntimeScriptValue.
    // This is a temporary solution (*sigh*, one of many) which allows to
    // receive any pointer values from plugins, as RSV can only store plain
    // numeric values as 32-bit integers. Not to mention that this way we
    // can store an accompanying IScriptObject manager for managed objects.
    // The big problem with this is that plugins do not know about RSV,
    // so engine has to make educated GUESS, and assign this whenever plugin
    // prepares a pointer, e.g. by registering or retrieving a managed object.
    // This works, let's say, 95% of cases in practice, but is not formally
    // reliable.
    // FIXME: re-investigate this, find if there's a better solution?
    static RuntimeScriptValue _pluginReturnValue;

    // Minimal timeout: how much time may pass without any engine update
    // before we want to check on the situation and do system poll
    static unsigned _timeoutCheckMs;
    // Critical timeout: how much time may pass without any engine update
    // before we abort or post a warning
    static unsigned _timeoutAbortMs;
    // Maximal while loops without any engine update in between,
    // after which the interpreter will abort
    static unsigned _maxWhileLoops;
    // Last time the script was noted of being "alive"
    AGS_FastClock::time_point _lastAliveTs;
};

#endif // DISABLED

#endif // __CC_INSTANCE_H
