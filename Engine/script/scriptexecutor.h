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
    kScExecErr_Busy = -4, // is busy executing script
};

enum ScriptExecState
{
    kScExecState_None    = 0x00,
    kScExecState_Aborted = 0x01, // scheduled to abort
    kScExecState_Running = 0x02, // has any script loaded and in running state (may be suspended though)
    kScExecState_Busy    = 0x04, // in the bytecode execution loop;
                                 // reset while waiting for the nested engine calls
    kScExecState_Alive   = 0x08, // updated periodically to confirm that script exec isn't stuck
};

// ScriptPosition defines position in the executed script
struct ScriptExecPosition
{
    const RuntimeScript *Script = nullptr;
    int32_t PC = 0;
    int32_t LineNumber = 0;

    ScriptExecPosition() = default;
    ScriptExecPosition(const RuntimeScript *script, int pc, int linenumber)
        : Script(script), PC(pc), LineNumber(linenumber) {}
};


class ScriptThread
{
    using String = Common::String;
public:
    ScriptThread();
    ScriptThread(const String &name);

    const String &GetName() const { return _name; }
    const std::vector<RuntimeScriptValue> &GetStack() const { return _stack; }
    const std::vector<uint8_t> &GetStackData() const { return _stackdata; }
    std::vector<RuntimeScriptValue> &GetStack() { return _stack; }
    std::vector<uint8_t> &GetStackData() { return _stackdata; }
    const std::deque<ScriptExecPosition> &GetCallStack() const { return _callstack; }
    const ScriptExecPosition &GetPosition() const { return _pos; }
    size_t GetStackBegin() const { return _stackBeginOff; }
    size_t GetStackDataBegin() const { return _stackDataBeginOff; }
    size_t GetStackOffset() const { return _stackOffset; }
    size_t GetStackDataOffset() const { return _stackDataOffset; }

    // Get the script's execution position and callstack as human-readable text
    String FormatCallStack(uint32_t max_lines = UINT32_MAX) const;

    // Save script execution state in the thread object
    void SaveState(const ScriptExecPosition &pos, std::deque<ScriptExecPosition> &callstack,
        size_t stack_begin, size_t stackdata_begin, size_t stack_off, size_t stackdata_off);
    // Resets execution state; this effectively invalidates the thread
    void ResetState();

private:
    void Alloc();

    // An arbitrary name for this script thread
    String _name;
    // Data stack, contains function args, local variables, temporary values
    std::vector<RuntimeScriptValue> _stack;
    // An array for keeping stack data; stack entries reference data of variable size from here
    std::vector<uint8_t> _stackdata;
    // Executed script callstack, contains *previous* script positions
    std::deque<ScriptExecPosition> _callstack; // deque for easier iterating over
    // Latest recorded script position, used when thread gets suspended
    ScriptExecPosition _pos;
    // Stack state
    size_t _stackBeginOff = 0u;
    size_t _stackDataBeginOff = 0u;
    size_t _stackOffset = 0u;
    size_t _stackDataOffset = 0u;
};


struct FunctionCallStack;

class ScriptExecutor
{
    using String = Common::String;
public:
    ScriptExecutor() = default;

    // Begin executing the function in the given script, passing an array of parameters;
    // the script will be executed on the provided script thread.
    ScriptExecError Run(ScriptThread *thread, const RuntimeScript *script, const String &funcname, const RuntimeScriptValue *params, size_t param_count);
    // Schedule abortion of the current script execution;
    // the actual stop will occur whenever control returns to the ScriptExecutor.
    void    Abort();

    // Tells whether any script is loaded into and being executed;
    // note that this returns positive even when executor is suspended
    bool    IsRunning() const { return (_flags & kScExecState_Running) != 0; }
    // Tells if the executor is busy running bytecode, and cannot start a nested run right now
    bool    IsBusy() const { return (_flags & kScExecState_Busy) != 0; }
    // Get the currently used script thread
    ScriptThread *GetRunningThread() const { return _thread; }
    // Get the currently executed script
    const RuntimeScript *GetRunningScript() const { return _current; }
    // Get current program pointer (position in bytecode)
    int     GetPC() const { return _pc; }
    // Get the script's execution position
    void    GetScriptPosition(ScriptPosition &script_pos) const;
    // Gets the top entry of this instance's stack
    const RuntimeScriptValue *GetCurrentStack() const { return _registers[SREG_SP].RValue; }
    // Get latest return value
    int     GetReturnValue() const { return _returnValue; }
    // TODO: this is a hack, required for dialog script; redo this later!
    void    SetReturnValue(int val) { _returnValue = val; }

    // Get the script's execution position and callstack as human-readable text
    String  FormatCallStack(uint32_t max_lines = UINT32_MAX) const;
    
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

    // Switches to the new script thread;
    // if there was any thread currently in running, then saves its state and
    // pushes to the thread stack.
    void    PushThread(ScriptThread *thread);
    // Pops out a script thread from the thread stack,
    // if there was any, and makes it active.
    void    PopThread();
    // Assigns thread state to the ScriptExecutor
    void    SelectThread(ScriptThread *thread);
    // Saves ScriptExecutor's state to the active thread
    void    SaveThreadState();

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

    // Current used thread
    ScriptThread *_thread = nullptr;
    // Thread stack holds a list of running or suspended script threads;
    // In AGS currently only one script thread is running, others are waiting in the queue.
    // An example situation is "repeatedly_execute_always" callback running while
    // other thread(s) are waiting at the blocking action or Wait().
    std::deque<ScriptThread*> _threadStack;

    // Thread stack pointers for faster access to the current thread
    RuntimeScriptValue *_stackBegin = nullptr; // ptr to beginning of stack (or stack's section)
    uint8_t    *_stackdataBegin = nullptr; // ptr to beginning of stackdata (or stackdata's section)
    uint8_t    *_stackdataPtr = nullptr; // points to the next unused byte in stack data array

    // Current executed script
    const RuntimeScript *_current = nullptr;
    // Code pointers for faster access to the current script
    const intptr_t *_code = nullptr;
    uint32_t    _codesize = 0; // size of code is limited under 32-bit due to bytecode format
    const uint8_t *_code_fixups = nullptr;
    const char *_strings = nullptr; // pointer to ccScript's string data
    size_t      _stringsize = 0u;
    // Script table pointers
    const JointRTTI *_rtti = nullptr;
    const std::unordered_map<uint32_t, uint32_t> *_typeidLocal2Global = nullptr;
    
    //
    // Virtual machine state
    // Registers
    RuntimeScriptValue _registers[CC_NUM_REGISTERS];
    uint32_t    _flags = kScExecState_None; // executor state flags ScriptExecState
    int         _pc = 0; // program counter
    int         _lineNumber = 0; // source code line number
    int         _returnValue = 0; // last executed function's return value
    // Executed script callstack, contains *previous* script positions;
    // this is copied from the ScriptThread on start, and copied back when done
    std::deque<ScriptExecPosition> _callstack;


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
