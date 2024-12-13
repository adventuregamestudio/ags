//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_SCRIPT__SCRIPTEXECUTOR_H
#define __AGS_EE_SCRIPT__SCRIPTEXECUTOR_H

#include <deque>
#include "ac/timer.h"
#include "script/runtimescript.h"

namespace AGS
{
namespace Engine
{

enum ScriptExecError
{
    kScExecErr_None = 0, // ok
    kScExecErr_Aborted = 100, // aborted by request
    kScExecErr_Generic = -1, // any generic exec error; use cc_get_error()
    kScExecErr_FuncNotFound = -2, // requested function is not found in script
    kScExecErr_InvalidArgNum = -3, // invalid number of args (not in supported range)
    kScExecErr_Busy = -4, // is busy executing script (unused?)
};

enum ScriptExecState
{
    kScExecState_None    = 0x00,
    kScExecState_Aborted = 0x01, // scheduled to abort
    kScExecState_Running = 0x02  // set by main code to confirm script isn't stuck
};

// ScriptPosition defines position in the executed script
struct ScriptExecPosition
{
    const RuntimeScript *Script = nullptr;
    int32_t PC = 0;
    int32_t LineNumber = 0;

    ScriptExecPosition(const RuntimeScript *script, int pc, int linenumber)
        : Script(script), PC(pc), LineNumber(linenumber) {}
};

struct FunctionCallStack;


class ScriptExecutor
{
    using String = Common::String;
public:
    ScriptExecutor();

    // Begin executing the function in the given script, passing an array of parameters
    ScriptExecError Run(const RuntimeScript *script, const String &funcname, const RuntimeScriptValue *params, size_t param_count);
    // Schedule abortion of the current script execution;
    // the actual stop will occur whenever control returns to the ScriptExecutor.
    void    Abort();
    
    // Configures script executor timeout in case of a "hanging" script
    void    SetExecTimeout(unsigned sys_poll_ms, unsigned abort_ms, unsigned abort_loops);
    // Notifies that the game was being updated (script not hanging)
    void    NotifyAlive();

    // Assigns a return value for the currently called plugin API;
    // NOTE: this is a sort of a hack, see comment to _pluginReturnValue for details.
    static void SetPluginReturnValue(const RuntimeScriptValue &value);

private:
    // Begin executing the given script starting from the given bytecode index
    ScriptExecError Run(const RuntimeScript *script, int32_t curpc, const RuntimeScriptValue *params, size_t param_count);
    // Begin executing latest run script starting from the given bytecode index
    ScriptExecError Run(int32_t curpc);

    // Sets current script and fast-access pointers
    void    SetCurrentScript(const RuntimeScript *script);
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

    //
    // Virtual machine state
    // Executed script callstack, contains previous script positions
    std::deque<ScriptExecPosition> _callstack; // deque for easier iterating over
    // Registers
    RuntimeScriptValue _registers[CC_NUM_REGISTERS];
    // Data stack, contains function args, local variables, temporary values
    std::vector<RuntimeScriptValue> _stack;
    // An array for keeping stack data; stack entries reference data of variable size from here
    std::vector<uint8_t> _stackdata;
    RuntimeScriptValue *_stackBegin = nullptr; // fast-access ptr to beginning of _stack
    uint8_t    *_stackdataBegin = nullptr; // fast-access ptr to beginning of _stackdata
    uint8_t    *_stackdataPtr = nullptr; // points to the next unused byte in stack data array

    // Current executed script
    const RuntimeScript *_current = nullptr;
    // Code pointers for faster access to the current script
    const intptr_t *_code = nullptr;
    uint32_t    _codesize = 0; // size of code is limited under 32-bit due to bytecode format
    const uint8_t *_code_fixups = nullptr;
    const char *_strings = nullptr; // pointer to ccScript's string data
    size_t      _stringsize = 0u;
    // Table pointers for simplicity
    const JointRTTI *_rtti = nullptr;
    const std::unordered_map<uint32_t, uint32_t> *_typeidLocal2Global = nullptr;
    
    uint32_t    _flags = kScExecState_None; // executor state flags ScriptExecState
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
    unsigned _timeoutCheckMs = 0u;
    // Critical timeout: how much time may pass without any engine update
    // before we abort or post a warning
    unsigned _timeoutAbortMs = 0u;
    // Maximal while loops without any engine update in between,
    // after which the interpreter will abort
    unsigned _maxWhileLoops = 0u;
    // Last time the script was noted of being "alive"
    AGS_FastClock::time_point _lastAliveTs;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_SCRIPT__SCRIPTEXECUTOR_H
