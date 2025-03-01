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
#include "script/scriptexecutor.h"
#include "ac/sys_events.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/managedobjectpool.h"
#include "ac/dynobj/scriptuserobject.h"
#include "script/cc_common.h"
#include "script/script_runtime.h"
#include "util/memory.h"
#if (DEBUG_CC_EXEC)
#include "util/file.h"
#include "util/textstreamwriter.h"
#endif

using namespace AGS::Common;
using namespace AGS::Common::Memory;
using namespace AGS::Engine;

// FIXME: don't use global var like this
extern new_line_hook_type new_line_hook;
// FIXME: do something with this, don't use global variable
extern std::unique_ptr<ScriptExecutor> scriptExecutor;


namespace AGS
{
namespace Engine
{

// A helper function, formatting provided script pos and callstack into a human-readable text
static String FormatCallStack(const ScriptExecPosition &pos, const std::deque<ScriptExecPosition> &callstack, uint32_t max_lines)
{
    if (!pos.Script)
        return {};

    String buffer = String::FromFormat("in \"%s\", line %d\n", pos.Script->GetSectionName(pos.PC).GetCStr(), pos.LineNumber);

    uint32_t lines_done = 0u;
    for (auto it = callstack.crbegin(); it != callstack.crend() && (lines_done < max_lines); ++lines_done, ++it)
    {
        String lineBuffer = String::FromFormat("from \"%s\", line %d\n",
            it->Script->GetSectionName(it->PC).GetCStr(), it->LineNumber);
        buffer.Append(lineBuffer);
        if (lines_done == max_lines - 1)
            buffer.Append("(and more...)\n");
    }
    return buffer;
}


// Size of stack in RuntimeScriptValues (aka distinct variables)
#define CC_STACK_SIZE       256
// Size of stack in bytes (raw data storage)
#define CC_STACK_DATA_SIZE  (1024 * sizeof(int32_t))
#define MAX_CALL_STACK      128


ScriptThread::ScriptThread()
{
    Alloc();
}

ScriptThread::ScriptThread(const String &name)
    : _name(name)
{
    Alloc();
}

void ScriptThread::Alloc()
{
    // Create a stack
    // The size of a stack is quite an arbitrary choice; there's no way to deduce number of stack
    // entries needed without knowing amount of local variables (at least)
    _stack.resize(CC_STACK_SIZE);
    _stackdata.resize(CC_STACK_DATA_SIZE);
}

String ScriptThread::FormatCallStack(uint32_t max_lines) const
{
    return ::FormatCallStack(_pos, _callstack, max_lines);
}

void ScriptThread::SaveState(const ScriptExecPosition &pos, std::deque<ScriptExecPosition> &callstack,
    size_t stack_begin, size_t stackdata_begin, size_t stack_off, size_t stackdata_off)
{
    _callstack = callstack;
    _pos = pos;
    _stackBeginOff = stack_begin;
    _stackDataBeginOff = stackdata_begin;
    _stackOffset = stack_off;
    _stackDataOffset = stackdata_off;
}

void ScriptThread::ResetState()
{
    _callstack.clear();
    _pos = {};
    _stackBeginOff = 0u;
    _stackDataBeginOff = 0u;
    _stackOffset = 0u;
    _stackDataOffset = 0u;
}


// Function call stack is used to temporarily store
// values before passing them to script function
// An inverted parameter stack
struct FunctionCallStack
{
    FunctionCallStack() = default;

    inline RuntimeScriptValue *GetHead()
    {
        return &Entries[Head];
    }
    inline RuntimeScriptValue *GetTail()
    {
        return &Entries[Head + Count];
    }

    const static size_t MAX_FUNC_PARAMS = 20u;
    RuntimeScriptValue  Entries[MAX_FUNC_PARAMS + 1];
    size_t              Head = MAX_FUNC_PARAMS - 1;
    size_t              Count = 0u;
};


ScriptExecutor::~ScriptExecutor()
{
#if (DEBUG_CC_EXEC)
    CloseExecLog();
#endif
}

ScriptExecError ScriptExecutor::Run(ScriptThread *thread, const RuntimeScript *script, const String &funcname, const RuntimeScriptValue *params, size_t param_count)
{
    assert(thread);
    assert(script);
    assert(param_count == 0 || params);
    cc_clear_error();

    if (IsBusy())
    {
        cc_error("ScriptExecutor is busy");
        return kScExecErr_Busy;
    }

    if (!thread || !script || (param_count > 0) && !params)
    {
        cc_error("bad input arguments in ScriptExecutor::Run");
        return kScExecErr_Generic;
    }

    if ((param_count >= FunctionCallStack::MAX_FUNC_PARAMS) || (param_count < 0))
    {
        cc_error("invalid number of function arguments %d, supported range is %d - %d", param_count, 0, FunctionCallStack::MAX_FUNC_PARAMS - 1);
        return kScExecErr_InvalidArgNum;
    }

    int start_at, export_args;
    if (!script->FindExportedFunction(funcname, start_at, export_args))
    {
        cc_error("function '%s' not found", funcname.GetCStr());
        return kScExecErr_FuncNotFound;
    }

    // NOTE: passing more parameters than expected by the function is fine:
    // the function args are pushed to the stack in REVERSE order, first
    // parameters are always the last, so function code knows how to find them
    // using negative offsets, and does not care about any preceding entries.
    // But if there's not enough parameters, then we cannot call this function.
    if (export_args < 0)
    {
        export_args = param_count;
    }
    else if (static_cast<uint32_t>(export_args) > param_count)
    {
        cc_error("Not enough parameters to exported function '%s' (expected %d, supplied %zu)",
            funcname.GetCStr(), export_args, param_count);
        return kScExecErr_InvalidArgNum;
    }

#if DEBUG_CC_EXEC
    if (ccGetOption(SCOPT_DEBUGRUN) != 0)
    {
        OpenExecLog();
    }
#endif

    // Allow to pass less parameters if script callback has less declared args
    param_count = std::min<size_t>(param_count, export_args);
    // Prepare executor for run
    _flags = (_flags & ~kScExecState_Aborted) | kScExecState_Running | kScExecState_Busy;
    _returnValue = 0;
    currentline = 0; // FIXME: stop using a global variable

    PushThread(thread);

    const ScriptExecError reterr = Run(script, start_at, params, param_count);

    PopThread();

    const bool was_aborted = (_flags & kScExecState_Aborted) != 0;
    const bool has_work = _thread != nullptr;
    // Clear exec state
    _flags = (kScExecState_Running * has_work);
    currentline = 0;

#if DEBUG_CC_EXEC
    if (!IsRunning())
    {
        CloseExecLog();
    }
#endif

    if (reterr != kScExecErr_None)
        return reterr;

    // NOTE that if proper multithreading is added this will need
    // to be reconsidered, since the GC could be run in the middle 
    // of a RET from a function or something where there is an 
    // object with ref count 0 that is in use
    pool.RunGarbageCollectionIfAppropriate();

    if (new_line_hook)
        new_line_hook(nullptr, 0);

    return was_aborted ?
        kScExecErr_Aborted :
        kScExecErr_None;
}

void ScriptExecutor::Abort()
{
    if (_pc != 0)
        _flags |= kScExecState_Aborted;
}

void ScriptExecutor::PushThread(ScriptThread *thread)
{
    assert(thread);
    // If there's already an active thread, then save latest exec state,
    // then push the previous thread to the thread stack.
    ScriptThread *was_thread = _thread;
    if (_thread)
    {
        if (_thread != thread)
        {
            SaveThreadState();
        }
        _threadStack.push_back(_thread); // push always, simpler to do PopThread this way
    }
    
    // Assign new current thread, get its data if it's a different one
    _thread = thread;
    if (was_thread != _thread)
    {
        SelectThread(_thread);
    }
}

void ScriptExecutor::PopThread()
{
    assert(_thread);
    // If the previous thread is not the same thread, then save popped thread's state
    // NOTE: it's a curious problem, whether the thread should be simply reset here;
    // it depends on whether we let to run same thread nested with another in the middle...
    if (_threadStack.empty() || _threadStack.back() != _thread)
    {
        SaveThreadState();
    }
    // If there's anything in the thread stack, pop one back and restore the state
    if (_threadStack.empty())
    {
        SelectThread(nullptr);
    }
    else
    {
        ScriptThread *was_thread = _thread;
        _thread = _threadStack.back();
        _threadStack.pop_back();

        if (was_thread != _thread)
        {
            SelectThread(_thread);
        }
    }
}

void ScriptExecutor::SelectThread(ScriptThread *thread)
{
    if (thread)
    {
        _thread = thread;
        // Restore execution pos
        _callstack = thread->GetCallStack();
        const auto &pos = thread->GetPosition();
        SetCurrentScript(pos.Script);
        _pc = pos.PC;
        _lineNumber = pos.LineNumber;
        // Restore data stack state
        _stackBegin = thread->GetStack().data() + thread->GetStackBegin();
        _stackdataBegin = thread->GetStackData().data() + thread->GetStackDataBegin();
        _registers[SREG_SP].RValue = _stackBegin + thread->GetStackOffset();
        _stackdataPtr = _stackdataBegin + thread->GetStackDataOffset();
    }
    else
    {
        _thread = nullptr;
        SetCurrentScript(nullptr);
        _pc = 0;
        _lineNumber = 0;
    }
}

void ScriptExecutor::SaveThreadState()
{
    _thread->SaveState(ScriptExecPosition(_current, _pc, _lineNumber), _callstack,
        _stackBegin - _thread->GetStack().data(),
        _stackdataBegin - _thread->GetStackData().data(),
        _registers[SREG_SP].RValue - _stackBegin,
        _stackdataPtr - _stackdataBegin);
}

void ScriptExecutor::GetScriptPosition(ScriptPosition &script_pos) const
{
    if (!_current)
        return;

    script_pos = ScriptPosition(_current->GetSectionName(_pc), _lineNumber);
}

String ScriptExecutor::FormatCallStack(uint32_t max_lines) const
{
    if (!_thread)
        return {}; // not running on any thread

    String callstack;
    callstack.Append("in the active script:\n");
    callstack.Append(::FormatCallStack(ScriptExecPosition(_current, _pc, _lineNumber), _callstack, max_lines));
    callstack.Append("in the waiting script:\n");

    const ScriptThread *last_thread = nullptr;
    for (auto thread = _threadStack.crbegin(); thread != _threadStack.crend(); ++thread)
    {
        if (last_thread == *thread)
            continue;

        callstack.Append((*thread)->FormatCallStack(max_lines));
        last_thread = *thread;
    }
    return callstack;
}
    
void ScriptExecutor::SetExecTimeout(unsigned sys_poll_ms, unsigned abort_ms, unsigned abort_loops)
{
    _timeoutCheckMs = sys_poll_ms;
    _timeoutAbortMs = abort_ms;
    _maxWhileLoops = abort_loops;
}

void ScriptExecutor::NotifyAlive()
{
    _flags |= kScExecState_Alive;
    _lastAliveTs = AGS_FastClock::now();
}

//-----------------------------------------------------------------------------
//
// Script execution routine follows.
//
//-----------------------------------------------------------------------------

// Script executor debugging flag:
// enables mistake checks, but slows things down!
#ifndef DEBUG_CC_EXEC
#define DEBUG_CC_EXEC (AGS_PLATFORM_DEBUG)
#endif

// ASSERT_CC_OP tests for the internal function call return value and
// returns failure on error. These assertions are slowing things down
// when executing excessive scripts, and so are disabled in Release build.
#if (DEBUG_CC_EXEC)

#define CC_ERROR_IF(COND, ERROR, ...) \
    if (COND) \
    { \
        cc_error(ERROR, ##__VA_ARGS__); \
        return; \
    }

#define CC_ERROR_IF_RETCODE(COND, ERROR, ...) \
    if (COND) \
    { \
        cc_error(ERROR, ##__VA_ARGS__); \
        return kScExecErr_Generic; \
    }

#define CC_ERROR_IF_RETVAL(COND, T, ERROR, ...) \
    if (COND) \
    { \
        cc_error(ERROR, ##__VA_ARGS__); \
        return T(); \
    }

#define ASSERT_CC_ERROR() \
    if (cc_has_error()) \
    { \
        return kScExecErr_Generic; \
    }

#else

#define CC_ERROR_IF(COND, ERROR, ...)
#define CC_ERROR_IF_RETCODE(COND, ERROR, ...)
#define CC_ERROR_IF_RETVAL(COND, T, ERROR, ...)
#define ASSERT_CC_ERROR()

#endif // DEBUG_CC_EXEC


// A number of stack assertions that are always enabled:
// ASSERT_STACK_SPACE_AVAILABLE tests that we do not exceed stack limit
#define ASSERT_STACK_SPACE_AVAILABLE(N_VALS, N_BYTES) \
    if ((_registers[SREG_SP].RValue + N_VALS - _stackBegin) >= CC_STACK_SIZE || \
        (_stackdataPtr + N_BYTES - _stackdataBegin) >= CC_STACK_DATA_SIZE) \
    { \
        cc_error("stack overflow, attempted to grow from %zu by %zu bytes", (size_t)(_stackdataPtr - _stackdataBegin), (size_t)(N_BYTES)); \
        return kScExecErr_Generic; \
    }

// ASSERT_STACK_SPACE_BYTES tests that we do not exceed stack limit
// if we are going to add N_BYTES bytes to stack
#define ASSERT_STACK_SPACE_BYTES(N_BYTES) ASSERT_STACK_SPACE_AVAILABLE(1u, N_BYTES)

// ASSERT_STACK_SPACE_VALS tests that we do not exceed stack limit
// if we are going to add N_VALS values, sizeof(int32) each
#define ASSERT_STACK_SPACE_VALS(N_VALS) ASSERT_STACK_SPACE_AVAILABLE(N_VALS, sizeof(int32_t) * N_VALS)

// ASSERT_STACK_SIZE tests that we do not unwind stack past its beginning
#define ASSERT_STACK_SIZE(N) \
    if (_registers[SREG_SP].RValue - N < _stackBegin) \
    { \
        cc_error("stack underflow"); \
        return kScExecErr_Generic; \
    }

// ASSERT_STACK_UNWINDED tests that the stack pointer is at the expected position
#define ASSERT_STACK_UNWINDED(STACK_VAL, DATA_PTR) \
    if ((_registers[SREG_SP].RValue > STACK_VAL.RValue) || \
        (_stackdataPtr > DATA_PTR)) \
    { \
        cc_error("stack is not unwinded after function call, %d bytes remain", (_stackdataPtr - DATA_PTR)); \
        return kScExecErr_Generic; \
    }

// Macros to maintain the call stack
#define PUSH_CALL_STACK() \
    if (_callstack.size() >= MAX_CALL_STACK) { \
        cc_error("CallScriptFunction stack overflow (recursive call error?)"); \
        return kScExecErr_Generic; \
    } \
    _callstack.push_back(ScriptExecPosition(_current, _pc, _lineNumber));

#define POP_CALL_STACK() \
    if (_callstack.size() < 1) { \
        cc_error("CallScriptFunction stack underflow -- internal error"); \
        return kScExecErr_Generic; \
    } \
    _lineNumber = _callstack.back().LineNumber; \
    currentline = _lineNumber; \
    _callstack.pop_back();


ScriptExecError ScriptExecutor::Run(const RuntimeScript *script, int32_t curpc, const RuntimeScriptValue *params, size_t param_count)
{
    const bool is_nested_run = _current != nullptr;

    const RuntimeScript *was_running = _current;
    const int oldpc = _pc;
    const int oldlinenum = _lineNumber;
    RuntimeScriptValue * const oldstack_begin = _stackBegin;
    uint8_t * const oldstackdata_begin = _stackdataBegin;

    if (is_nested_run)
    {
        PUSH_CALL_STACK();
        // In the nested call we keep stack pointers where they are,
        // and treat their position as a new "beginning of stack" for this exec pass.
    }
    else
    {
        // On script thread entry we reset stack pointers to the actual beginning
        // of the thread's stack.
        _registers[SREG_SP].SetStackPtr(_thread->GetStack().data());
        _stackdataPtr = _thread->GetStackData().data();
    }

    SetCurrentScript(script);
    _stackBegin = _registers[SREG_SP].RValue;
    _stackdataBegin = _stackdataPtr;
    const RuntimeScriptValue oldstack = _registers[SREG_SP];
    const uint8_t * const oldstackdata = _stackdataPtr;

    // object pointer needs to start zeroed
    _registers[SREG_OP].SetScriptObject(nullptr, nullptr);

    // NOTE: Pushing parameters to stack in reverse order
    ASSERT_STACK_SPACE_VALS(param_count + 1 /* return address */);
    for (int i = param_count - 1; i >= 0; --i)
    {
        PushValueToStack(params[i]);
    }
    // Push placeholder for the return value (it will be popped before ret)
    PushValueToStack(RuntimeScriptValue().SetInt32(0));

    const ScriptExecError reterr = Run(curpc);

    if (reterr != kScExecErr_None)
        return reterr;

    // Cleanup and assert stack state:
    // stack must be reset to where it was before starting this script
    if ((_flags & kScExecState_Aborted) == 0)
    {
        ASSERT_STACK_SIZE(param_count);
        PopValuesFromStack(param_count);
        ASSERT_STACK_UNWINDED(oldstack, oldstackdata);
    }

    if (is_nested_run)
    {
        POP_CALL_STACK();
    }

    SetCurrentScript(was_running); // switch back to the previous script
    _pc = oldpc;
    _lineNumber = oldlinenum;
    _stackBegin = oldstack_begin;
    _stackdataBegin = oldstackdata_begin;

    return cc_has_error() ? kScExecErr_Generic : kScExecErr_None;
}


// Return stack ptr at given offset from stack head;
// Offset is in data bytes; program stack ptr is __not__ changed
inline RuntimeScriptValue GetStackPtrOffsetFw(RuntimeScriptValue *stack, int32_t fw_offset)
{
    int32_t total_off = 0;
    RuntimeScriptValue *stack_entry = stack;
    while (total_off < fw_offset && (stack_entry - stack) < CC_STACK_SIZE )
    {
        stack_entry++;
        total_off += stack_entry->Size;
    }
    CC_ERROR_IF_RETVAL(total_off < fw_offset, RuntimeScriptValue, "accessing address beyond stack's tail");
    CC_ERROR_IF_RETVAL(total_off > fw_offset, RuntimeScriptValue, "stack offset forward: trying to access stack data inside stack entry, stack corrupted?");
    RuntimeScriptValue stack_ptr;
    stack_ptr.SetStackPtr(stack_entry);
    return stack_ptr;
}

// Applies a runtime fixup to the given arg;
// Fixup of type `fixup` is applied to the `code` value,
// the result is assigned to the `arg`.
inline bool FixupArgument(RuntimeScriptValue &arg, const int fixup, const uintptr_t code,
    RuntimeScriptValue *stack, const char *strings)
{
    // could be relative pointer or import address
    switch (fixup)
    {
    case FIXUP_NOFIXUP:
        return true;
    case FIXUP_GLOBALDATA:
        {
            ScriptVariable *gl_var = (ScriptVariable*)code;
            assert(gl_var->RValue.IsValid());
            arg.SetGlobalVar(&gl_var->RValue);
        }
        return true;
    case FIXUP_FUNCTION:
        // originally commented -- CHECKME: could this be used in very old versions of AGS?
        //      code[fixup] += (long)&code[0];
        // This is a program counter value, presumably will be used as SCMD_CALL argument
        arg.SetInt32(static_cast<int32_t>(code));
        return true;
    case FIXUP_STRING:
        arg.SetStringLiteral(strings + code);
        return true;
    case FIXUP_IMPORT:
        {
            const ScriptImport *import = simp.GetByIndex(static_cast<uint32_t>(code));
            if (import)
            {
                assert(import->Value.IsValid());
                arg = import->Value;
            }
            else
            {
                cc_error("cannot resolve import, key = %ld", code);
                return false;
            }
        }
        return true;
    case FIXUP_DATADATA:
        return false; // placeholder, fail at this as not supposed to be here
    case FIXUP_STACK:
        arg = GetStackPtrOffsetFw(stack, static_cast<int32_t>(code));
        return true;
    default:
        cc_error("internal fixup type error: %d", fixup);
        return false;
    }
}


// FIXME: get rid of this
#define MAXNEST 50  // number of recursive function calls allowed

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

ScriptExecError ScriptExecutor::Run(int32_t curpc)
{
    _pc = curpc;
    _returnValue = -1;

    if ((curpc < 0) || (static_cast<uint32_t>(curpc) >= _codesize))
    {
        cc_error("specified code offset is not valid");
        return kScExecErr_Generic;
    }

    // TODO: many of these local vars should be made ScriptExecutor's private members,
    // and also have duplicates in ScriptThread, where they are copied from on Run
    // and copied back after Run. Noteably: thisbase, func_callstack, was_just_callas etc.
    // This will be required if we support thread suspending.
    int32_t thisbase[MAXNEST];
    int32_t funcstart[MAXNEST];
    int was_just_callas = -1;
    int curnest = 0;
    int num_args_to_func = -1;
    int next_call_needs_object = 0;
    thisbase[0] = 0;
    funcstart[0] = _pc;
    ScriptOperation codeOp;
    FunctionCallStack func_callstack;
    int loopIterationCheckDisabled = 0;
    unsigned loopIterations = 0u; // any loop iterations (needed for timeout test)
    unsigned loopCheckIterations = 0u; // loop iterations accumulated only if check is enabled

#if DEBUG_CC_EXEC
    const bool dump_opcodes = (ccGetOption(SCOPT_DEBUGRUN) != 0) && (_execWriter != nullptr);
#endif

    const auto timeout = std::chrono::milliseconds(_timeoutCheckMs);
    _lastAliveTs = AGS_FastClock::now();

    /* Main bytecode execution loop */
    //=====================================================================
    while ((_flags & kScExecState_Aborted) == 0)
    {
        // WARNING: a time-critical code ahead;
        // trying to pick some of the code out to separate function(s)
        // may lead to a performance loss in script-heavy games.
        // always compare execution speed before applying any major changes!
        //
        /* Read operation */
        //=====================================================================
        codeOp.Instruction.Code = _code[_pc];
        codeOp.Instruction.InstanceId = (codeOp.Instruction.Code >> INSTANCE_ID_SHIFT) & INSTANCE_ID_MASK;
        codeOp.Instruction.Code &= INSTANCE_ID_REMOVEMASK; // now this is pure instruction code

        CC_ERROR_IF_RETCODE((codeOp.Instruction.Code < 0 || codeOp.Instruction.Code >= CC_NUM_SCCMDS),
            "invalid instruction %d found in code stream", codeOp.Instruction.Code);

        codeOp.ArgCount = sccmd_info[codeOp.Instruction.Code].ArgCount;

        CC_ERROR_IF_RETCODE(static_cast<uint32_t>(_pc + codeOp.ArgCount) >= _codesize,
            "unexpected end of code data (%d; %u)", _pc + codeOp.ArgCount, _codesize);


        // Read arguments; use switch as it proved to be faster than the loop
        switch (codeOp.ArgCount)
        {
        case 3:
            codeOp.Args[2].SetInt32(static_cast<int32_t>(_code[_pc + 3]));
            /* fall-through */
        case 2:
            codeOp.Args[1].SetInt32(static_cast<int32_t>(_code[_pc + 2]));
            /* fall-through */
        case 1:
            codeOp.Args[0].SetInt32(static_cast<int32_t>(_code[_pc + 1]));
            break;
        default:
            break;
        }
        //---------------------------------------------------------------------
        /* End read operation */
        //=====================================================================

#if (DEBUG_CC_EXEC)
        if (dump_opcodes)
        {
            DumpInstruction(codeOp);
        }
#endif

        /* Perform operation */
        //=====================================================================
        switch (codeOp.Instruction.Code)
        {
        case SCMD_LINENUM:
            _lineNumber = codeOp.Arg1i();
            currentline = _lineNumber;
            if (new_line_hook)
                new_line_hook(this, currentline);
            break;
        case SCMD_ADD:
        {
            const auto arg_reg = codeOp.Arg1i();
            const auto arg_lit = codeOp.Arg2i();
            auto &reg1 = _registers[arg_reg];
            // If the the register is SREG_SP, we are allocating new variable on the stack
            if (arg_reg == SREG_SP)
            {
                // Only allocate new data if current stack entry is invalid;
                // in some cases this may be advancing over value that was written by MEMWRITE*
                // FIXME: this is bad, but seemed to be the way to separate PushValue and PushData
                // find if it's possible to do this in a uniform way (always same operation),
                // and don't rely on stack entries being valid/invalid beyond the stack ptr.
                ASSERT_STACK_SPACE_AVAILABLE(1, arg_lit);
                if (reg1.RValue->IsValid())
                {
                    // TODO: perhaps should add a flag here to ensure this happens only after MEMWRITE-ing to stack
                    _registers[SREG_SP].RValue++;
                    _stackdataPtr += arg_lit; // formality, to keep data ptr consistent
                }
                else
                {
                    PushDataToStack(arg_lit);
                    ASSERT_CC_ERROR();
                }
            }
            else
            {
                reg1.IValue += arg_lit;
            }
            break;
        }
        case SCMD_SUB:
        {
            const auto arg_reg = codeOp.Arg1i();
            const auto arg_lit = codeOp.Arg2i();
            auto &reg1 = _registers[arg_reg];
            if (reg1.Type == kScValStackPtr)
            {
                // If this is SREG_SP, this is stack pop, which frees local variables;
                // Other than SREG_SP this may be AGS 2.x method to offset stack in SREG_MAR;
                // quote JJS:
                // // AGS 2.x games also perform relative stack access by copying SREG_SP to SREG_MAR
                // // and then subtracting from that.
                // FIXME: try to do this in uniform way, call same func, save result in reg1
                if (arg_reg == SREG_SP)
                {
                    PopDataFromStack(arg_lit);
                }
                else
                {
                    // This is practically LOADSPOFFS
                    reg1 = GetStackPtrOffsetRw(arg_lit);
                }
                ASSERT_CC_ERROR();
            }
            else
            {
                reg1.IValue -= arg_lit;
            }
            break;
        }
        case SCMD_REGTOREG:
        {
            const auto &reg1 = _registers[codeOp.Arg1i()];
            auto       &reg2 = _registers[codeOp.Arg2i()];
            reg2 = reg1;
            break;
        }
        case SCMD_WRITELIT:
        {
            // Take the data address from reg[MAR] and copy there arg1 bytes from arg2 address
            //
            // NOTE: since it reads directly from arg2 (which originally was
            // long, or rather int32 due x32 build), written value may normally
            // be only up to 4 bytes large;
            // I guess that's an obsolete way to do WRITE, WRITEW and WRITEB
            const auto arg_size = codeOp.Arg1i();
            FixupArgument(codeOp.Args[1], _code_fixups[_pc + 2], _code[_pc + 2], _stackBegin, _strings);
            ASSERT_CC_ERROR();
            const auto &arg_value = codeOp.Arg2();
            switch (arg_size)
            {
            case sizeof(char) :
                _registers[SREG_MAR].WriteByte(arg_value.IValue);
                break;
            case sizeof(int16_t) :
                _registers[SREG_MAR].WriteInt16(arg_value.IValue);
                break;
            case sizeof(int32_t) :
                // We do not know if this is math integer or some pointer, etc
                _registers[SREG_MAR].WriteValue(arg_value);
                break;
            default:
                cc_error("unexpected data size for WRITELIT op: %d", arg_size);
                break;
            }
            break;
        }
        case SCMD_RET:
        {
            if (loopIterationCheckDisabled > 0)
                loopIterationCheckDisabled--;

            ASSERT_STACK_SIZE(1);
            RuntimeScriptValue rval = PopValueFromStack();
            curnest--;
            _pc = rval.IValue;
            if (_pc == 0)
            {
                _returnValue = _registers[SREG_AX].IValue;
                return kScExecErr_None;
            }
            POP_CALL_STACK();
            continue; // continue so that the PC doesn't get overwritten
        }
        case SCMD_LITTOREG:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            FixupArgument(codeOp.Args[1], _code_fixups[_pc + 2], _code[_pc + 2], _stackBegin, _strings);
            ASSERT_CC_ERROR();
            const auto &arg_value = codeOp.Arg2();
            reg1 = arg_value;
            break;
        }
        case SCMD_MEMREAD:
        {
            // Take the data address from reg[MAR] and copy int32_t to reg[arg1]
            auto &reg1 = _registers[codeOp.Arg1i()];
            reg1 = _registers[SREG_MAR].ReadValue();
            break;
        }
        case SCMD_MEMWRITE:
        {
            // Take the data address from reg[MAR] and copy there int32_t from reg[arg1]
            const auto &reg1 = _registers[codeOp.Arg1i()];
            _registers[SREG_MAR].WriteValue(reg1);
            break;
        }
        case SCMD_LOADSPOFFS:
        {
            const auto arg_off = codeOp.Arg1i();
            _registers[SREG_MAR] = GetStackPtrOffsetRw(arg_off);
            ASSERT_CC_ERROR();
            break;
        }
        case SCMD_MULREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32(reg1.IValue * reg2.IValue);
            break;
        }
        case SCMD_DIVREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            if (reg2.IValue == 0)
            {
                cc_error("!Integer divide by zero");
                return kScExecErr_Generic;
            }
            reg1.SetInt32(reg1.IValue / reg2.IValue);
            break;
        }
        case SCMD_ADDREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            // This may be pointer arithmetics, in which case IValue stores offset from base pointer
            reg1.IValue += reg2.IValue;
            break;
        }
        case SCMD_SUBREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            // This may be pointer arithmetics, in which case IValue stores offset from base pointer
            reg1.IValue -= reg2.IValue;
            break;
        }
        case SCMD_BITAND:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32(reg1.IValue & reg2.IValue);
            break;
        }
        case SCMD_BITOR:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32(reg1.IValue | reg2.IValue);
            break;
        }
        case SCMD_ISEQUAL:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1 == reg2);
            break;
        }
        case SCMD_NOTEQUAL:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1 != reg2);
            break;
        }
        case SCMD_GREATER:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1.IValue > reg2.IValue);
            break;
        }
        case SCMD_LESSTHAN:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1.IValue < reg2.IValue);
            break;
        }
        case SCMD_GTE:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1.IValue >= reg2.IValue);
            break;
        }
        case SCMD_LTE:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1.IValue <= reg2.IValue);
            break;
        }
        case SCMD_AND:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1.IValue && reg2.IValue);
            break;
        }
        case SCMD_OR:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32AsBool(reg1.IValue || reg2.IValue);
            break;
        }
        case SCMD_XORREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32(reg1.IValue ^ reg2.IValue);
            break;
        }
        case SCMD_MODREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            if (reg2.IValue == 0)
            {
                cc_error("!Integer divide by zero");
                return kScExecErr_Generic;
            }
            reg1.SetInt32(reg1.IValue % reg2.IValue);
            break;
        }
        case SCMD_NOTREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            reg1 = !(reg1);
            break;
        }
        case SCMD_CALL:
        {
            // Call another function within same script, just save PC
            // and continue from there
            if (curnest >= MAXNEST - 1)
            {
                cc_error("!call stack overflow, recursive call problem?");
                return kScExecErr_Generic;
            }

            PUSH_CALL_STACK();

            ASSERT_STACK_SPACE_VALS(1);
            PushValueToStack(RuntimeScriptValue().SetInt32(_pc + codeOp.ArgCount + 1));

            const auto &reg1 = _registers[codeOp.Arg1i()];
            if (thisbase[curnest] == 0)
                _pc = reg1.IValue;
            else {
                _pc = funcstart[curnest];
                _pc += (reg1.IValue - thisbase[curnest]);
            }

            next_call_needs_object = 0;

            if (loopIterationCheckDisabled)
                loopIterationCheckDisabled++;

            curnest++;
            thisbase[curnest] = 0;
            funcstart[curnest] = _pc;
            continue; // continue so that the PC doesn't get overwritten
        }
        case SCMD_MEMREADB:
        {
            // Take the data address from reg[MAR] and copy byte to reg[arg1]
            auto &reg1 = _registers[codeOp.Arg1i()];
            reg1.SetUInt8(_registers[SREG_MAR].ReadByte());
            break;
        }
        case SCMD_MEMREADW:
        {
            // Take the data address from reg[MAR] and copy int16_t to reg[arg1]
            auto &reg1 = _registers[codeOp.Arg1i()];
            reg1.SetInt16(_registers[SREG_MAR].ReadInt16());
            break;
        }
        case SCMD_MEMWRITEB:
        {
            // Take the data address from reg[MAR] and copy there byte from reg[arg1]
            const auto &reg1 = _registers[codeOp.Arg1i()];
            _registers[SREG_MAR].WriteByte(reg1.IValue);
            break;
        }
        case SCMD_MEMWRITEW:
        {
            // Take the data address from reg[MAR] and copy there int16_t from reg[arg1]
            const auto &reg1 = _registers[codeOp.Arg1i()];
            _registers[SREG_MAR].WriteInt16(reg1.IValue);
            break;
        }
        case SCMD_JZ:
        {
            const auto arg_lit = codeOp.Arg1i();
            if (_registers[SREG_AX].IsNull())
                _pc += arg_lit;
            break;
        }
        case SCMD_JNZ:
        {
            const auto arg_lit = codeOp.Arg1i();
            if (!_registers[SREG_AX].IsNull())
                _pc += arg_lit;
            break;
        }
        case SCMD_PUSHREG:
        {
            // Push reg[arg1] value to the stack
            const auto &reg1 = _registers[codeOp.Arg1i()];
            ASSERT_STACK_SPACE_VALS(1);
            PushValueToStack(reg1);
            break;
        }
        case SCMD_POPREG:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            ASSERT_STACK_SIZE(1);
            reg1 = PopValueFromStack();
            break;
        }
        case SCMD_JMP:
        {
            const auto arg_lit = codeOp.Arg1i();
            _pc += arg_lit;

            // Make sure it's not stuck in a While loop
            if (arg_lit < 0)
            {
                ++loopIterations;
                if (_flags & kScExecState_Alive)
                { // was notified still running, don't do anything
                    _flags &= ~kScExecState_Alive;
                    loopIterations = 0u;
                    loopCheckIterations = 0u;
                }
                else if ((loopIterationCheckDisabled == 0) && (_maxWhileLoops > 0) &&
                    (++loopCheckIterations > _maxWhileLoops))
                {
                    cc_error("!Script appears to be hung (a while loop ran %d times). The problem may be in a calling function; check the call stack.", loopCheckIterations);
                    return kScExecErr_Generic;
                }
                else if ((loopIterations & 0x3FF) == 0 && // test each 1024 loops (arbitrary)
                    (std::chrono::duration_cast<std::chrono::milliseconds>(
                        AGS_FastClock::now() - _lastAliveTs) > timeout))
                { // minimal timeout occured
                    // NOTE: removed timeout_abort check for now: was working *logically* wrong;
                    // at least let user to manipulate the game window
                    sys_evt_process_pending();
                    _lastAliveTs = AGS_FastClock::now();
                }
            }
            break;
        }
        case SCMD_MUL:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const auto arg_lit = codeOp.Arg2i();
            reg1.IValue *= arg_lit;
            break;
        }
        case SCMD_CHECKBOUNDS:
        {
            const auto &reg1 = _registers[codeOp.Arg1i()];
            const auto arg_lit = codeOp.Arg2i();
            if ((reg1.IValue < 0) ||
                (reg1.IValue >= arg_lit))
            {
                cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", reg1.IValue, arg_lit - 1);
                return kScExecErr_Generic;
            }
            break;
        }
        case SCMD_DYNAMICBOUNDS:
        {
            const auto &reg1 = _registers[codeOp.Arg1i()];
            void *arr_ptr = _registers[SREG_MAR].GetPtrWithOffset();
            const auto &hdr = CCDynamicArray::GetHeader(arr_ptr);
            if ((reg1.IValue < 0) ||
                (static_cast<uint32_t>(reg1.IValue) >= hdr.TotalSize))
            {
                if (hdr.ElemCount <= 0)
                {
                    cc_error("!Array has an invalid size (%d) and cannot be accessed", hdr.ElemCount);
                }
                else
                {
                    int elementSize = (hdr.TotalSize / hdr.ElemCount);
                    cc_error("!Array index out of bounds (index: %d, bounds: 0..%d)", reg1.IValue / elementSize, hdr.ElemCount - 1);
                }
                return kScExecErr_Generic;
            }
            break;
        }
        case SCMD_MEMREADPTR:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            int32_t handle = _registers[SREG_MAR].ReadInt32();
            // FIXME: make pool return a ready RuntimeScriptValue with these set?
            // or another struct, which may be assigned to RSV
            void *object;
            IScriptObject *manager;
            ScriptValueType obj_type = ccGetObjectAddressAndManagerFromHandle(handle, object, manager);
            reg1.SetScriptObject(obj_type, object, manager);
            ASSERT_CC_ERROR();
            break;
        }
        case SCMD_MEMWRITEPTR:
        {
            const auto &reg1 = _registers[codeOp.Arg1i()];
            int32_t handle = _registers[SREG_MAR].ReadInt32();
            void *address;

            switch (reg1.Type)
            {
            case kScValStaticArray:
                //FIXME: return manager type from interface?
                //CC_ERROR_IF_RETCODE(!reg1.ArrMgr->GetDynamicManager(), "internal error: MEMWRITEPTR argument is not a dynamic object");
                address = reg1.ArrMgr->GetElementPtr(reg1.Ptr, reg1.IValue);
                break;
            case kScValScriptObject:
            case kScValPluginObject:
            case kScValPluginArgPtr:
                address = reg1.Ptr;
                break;
            case kScValPluginArg:
                address = Int32ToPtr<void>(reg1.IValue);
                break;
            default:
                // There's one possible case when the reg1 is 0, which means writing nullptr
                CC_ERROR_IF_RETCODE(!reg1.IsNull(), "internal error: MEMWRITEPTR argument is not a dynamic object");
                address = nullptr;
                break;
            }

            int32_t newHandle = ccGetObjectHandleFromAddress(address);
            if (newHandle == -1)
                return kScExecErr_Generic;

            if (handle != newHandle)
            {
                ccReleaseObjectReference(handle);
                ccAddObjectReference(newHandle);
            }
            // Assign always, avoid leaving undefined value
            _registers[SREG_MAR].WriteInt32(newHandle);
            break;
        }
        case SCMD_MEMINITPTR:
        {
            void *address;
            const auto &reg1 = _registers[codeOp.Arg1i()];

            switch (reg1.Type)
            {
            case kScValStaticArray:
                //FIXME: return manager type from interface?
                //CC_ERROR_IF_RETCODE(!reg1.ArrMgr->GetDynamicManager(), "internal error: SCMD_MEMINITPTR argument is not a dynamic object");
                address = reg1.ArrMgr->GetElementPtr(reg1.Ptr, reg1.IValue);
                break;
            case kScValScriptObject:
            case kScValPluginObject:
            case kScValPluginArgPtr:
                address = reg1.Ptr;
                break;
            case kScValPluginArg:
                address = Int32ToPtr<void>(reg1.IValue);
                break;
            default:
                // There's one possible case when the reg1 is 0, which means writing nullptr
                CC_ERROR_IF_RETCODE(!reg1.IsNull(), "internal error: SCMD_MEMINITPTR argument is not a dynamic object");
                address = nullptr;
                break;
            }

            // like memwriteptr, but doesn't attempt to free the old one
            int32_t newHandle = ccGetObjectHandleFromAddress(address);
            if (newHandle == -1)
                return kScExecErr_Generic;

            ccAddObjectReference(newHandle);
            _registers[SREG_MAR].WriteInt32(newHandle);
            break;
        }
        case SCMD_MEMZEROPTR:
        {
            int32_t handle = _registers[SREG_MAR].ReadInt32();
            ccReleaseObjectReference(handle);
            _registers[SREG_MAR].WriteInt32(0);
            break;
        }
        case SCMD_MEMZEROPTRND:
        {
            int32_t handle = _registers[SREG_MAR].ReadInt32();

            // don't do the Dispose check for the object being returned -- this is
            // for returning a String (or other pointer) from a custom function.
            // Note: we might be freeing a dynamic array which contains the DisableDispose
            // object, that will be handled inside the recursive call to SubRef.
            // CHECKME!! what type of data may reg1 point to?
            pool.disableDisposeForObject = _registers[SREG_AX].Ptr;
            ccReleaseObjectReference(handle);
            pool.disableDisposeForObject = nullptr;
            _registers[SREG_MAR].WriteInt32(0);
            break;
        }
        case SCMD_CHECKNULL:
            if (_registers[SREG_MAR].IsNull())
            {
                cc_error("!Null pointer referenced");
                return kScExecErr_Generic;
            }
            break;
        case SCMD_CHECKNULLREG:
        {
            const auto &reg1 = _registers[codeOp.Arg1i()];
            if (reg1.IsNull())
            {
                cc_error("!Null string referenced");
                return kScExecErr_Generic;
            }
            break;
        }
        case SCMD_NUMFUNCARGS:
        {
            const auto arg_lit = codeOp.Arg1i();
            num_args_to_func = arg_lit;
            break;
        }
        case SCMD_CALLAS:
        {
            // Call to a function in another script
            const auto &reg1 = _registers[codeOp.Arg1i()];

            // If there are nested CALLAS calls, the stack might
            // contain 2 calls worth of parameters, so only
            // push args for this call
            if (num_args_to_func < 0)
            {
                num_args_to_func = func_callstack.Count;
            }
            ASSERT_STACK_SPACE_VALS(num_args_to_func + 1 /* return address */);
            for (const RuntimeScriptValue *prval = func_callstack.GetHead() + num_args_to_func;
                prval > func_callstack.GetHead(); --prval)
            {
                PushValueToStack(*prval);
            }

            // extract the script ID
            const int32_t instance_id = codeOp.Instruction.InstanceId;
            // determine the offset into the code of the instance we want
            const RuntimeScript *next_to_run = RuntimeScript::GetLinkedScript(instance_id);
            uintptr_t callAddr = reg1.PtrU8 - reinterpret_cast<const uint8_t*>(next_to_run->GetCode().data());
            if (callAddr % sizeof(uintptr_t) != 0)
            {
                cc_error("call address not aligned");
                return kScExecErr_Generic;
            }
            callAddr /= sizeof(uintptr_t); // size of code elements

            PUSH_CALL_STACK();
            const RuntimeScript *was_running = _current;
            const int oldpc = _pc;
            const RuntimeScriptValue oldstack = _registers[SREG_SP];
            const uint8_t * const oldstackdata = _stackdataPtr;

            // Push placeholder for the return value (it will be popped before ret)
            PushValueToStack(RuntimeScriptValue().SetInt32(0));

            SetCurrentScript(next_to_run); // switch to the new script

            // TODO: rewrite executor impl here to NOT have any nested Run calls;
            // instead continue same loop, just like in SCMD_CALL,
            // but push "current script" on SCMD_CALLAS and pop "current script" on SCMD_RET.
            if (Run(static_cast<int32_t>(callAddr)))
                return kScExecErr_Generic;

            if ((_flags & kScExecState_Aborted) == 0)
                ASSERT_STACK_UNWINDED(oldstack, oldstackdata);

            POP_CALL_STACK();
            SetCurrentScript(was_running); // switch back to the previous script
            _pc = oldpc;
            next_call_needs_object = 0;
            was_just_callas = func_callstack.Count;
            num_args_to_func = -1;
            break;
        }
        case SCMD_CALLEXT:
        {
            _flags &= ~kScExecState_Busy;

            // Call to a real 'C' code function
            const auto &reg1 = _registers[codeOp.Arg1i()];

            was_just_callas = -1;
            if (num_args_to_func < 0)
            {
                num_args_to_func = func_callstack.Count;
            }

            // Convert pointer arguments to simple types
            for (RuntimeScriptValue *prval = func_callstack.GetHead() + num_args_to_func;
                prval > func_callstack.GetHead(); --prval)
            {
                prval->DirectPtr();
            }

            RuntimeScriptValue return_value;

            if (reg1.Type == kScValPluginFunction)
            {
                if (next_call_needs_object)
                {
                    RuntimeScriptValue obj_rval = _registers[SREG_OP];
                    obj_rval.DirectPtrObj();
                    return_value = CallPluginFunction(reg1.Ptr, &obj_rval, func_callstack.GetHead() + 1, num_args_to_func);
                }
                else
                {
                    return_value = CallPluginFunction(reg1.Ptr, nullptr, func_callstack.GetHead() + 1, num_args_to_func);
                }
            }
            else if (next_call_needs_object)
            {
                // member function call
                if (reg1.Type == kScValObjectFunction)
                {
                    RuntimeScriptValue obj_rval = _registers[SREG_OP];
                    obj_rval.DirectPtrObj();
                    return_value = reg1.ObjPfn(obj_rval.Ptr, func_callstack.GetHead() + 1, num_args_to_func);
                }
                else
                {
                    cc_error("invalid pointer type for object function call: %d", reg1.Type);
                }
            }
            else if (reg1.Type == kScValStaticFunction)
            {
                return_value = reg1.SPfn(func_callstack.GetHead() + 1, num_args_to_func);
            }
            else if (reg1.Type == kScValObjectFunction)
            {
                cc_error("unexpected object function pointer on SCMD_CALLEXT");
            }
            else
            {
                cc_error("invalid pointer type for function call: %d", reg1.Type);
            }

            if (cc_has_error())
            {
                return kScExecErr_Generic;
            }

            _flags |= kScExecState_Busy;
            _registers[SREG_AX] = return_value;
            next_call_needs_object = 0;
            num_args_to_func = -1;
            break;
        }
        case SCMD_PUSHREAL:
        {
            const auto &reg1 = _registers[codeOp.Arg1i()];
            PushToFuncCallStack(func_callstack, reg1);
            break;
        }
        case SCMD_SUBREALSTACK:
        {
            const auto arg_lit = codeOp.Arg1i();
            PopFromFuncCallStack(func_callstack, arg_lit);
            ASSERT_CC_ERROR();
            if (was_just_callas >= 0)
            {
                ASSERT_STACK_SIZE(arg_lit);
                PopValuesFromStack(arg_lit);
                was_just_callas = -1;
            }
            break;
        }
        case SCMD_CALLOBJ:
        {
            // set the OP register
            const auto &reg1 = _registers[codeOp.Arg1i()];
            if (reg1.IsNull())
            {
                cc_error("!Null pointer referenced");
                return kScExecErr_Generic;
            }
            switch (reg1.Type)
            {
                // This might be a static object, passed to the user-defined extender function
            case kScValScriptObject:
            case kScValPluginObject:
            case kScValPluginArg:
            case kScValPluginArgPtr:
                // This might be an object of USER-DEFINED type, calling its MEMBER-FUNCTION.
                // Note, that this is the only case known when such object is written into reg[SREG_OP];
                // in any other case that would count as error. 
            case kScValGlobalVar:
            case kScValStackPtr:
                _registers[SREG_OP] = reg1;
                break;
            case kScValStaticArray:
                //FIXME: return manager type from interface?
                //CC_ERROR_IF_RETCODE(!reg1.ArrMgr->GetDynamicManager(), "internal error: SCMD_CALLOBJ argument is not a dynamic object");
                _registers[SREG_OP].SetScriptObject(
                        reg1.ArrMgr->GetElementPtr(reg1.Ptr, reg1.IValue),
                        reg1.ArrMgr->GetObjectManager());
                break;
            default:
                cc_error("internal error: SCMD_CALLOBJ argument is not an object of built-in or user-defined type");
                return kScExecErr_Generic;
            }
            next_call_needs_object = 1;
            break;
        }
        case SCMD_SHIFTLEFT:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32(reg1.IValue << reg2.IValue);
            break;
        }
        case SCMD_SHIFTRIGHT:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetInt32(reg1.IValue >> reg2.IValue);
            break;
        }
        case SCMD_THISBASE:
        {
            const auto arg_lit = codeOp.Arg1i();
            thisbase[curnest] = arg_lit;
            break;
        }
        case SCMD_NEWARRAY:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const int arg_elnum = reg1.IValue;
            const uint32_t arg_elsize = static_cast<uint32_t>(codeOp.Arg2i());
            const bool arg_managed = codeOp.Arg3().GetAsBool();
            if (arg_elnum < 0)
            {
                cc_error("Invalid size for dynamic array; requested: %d, range: 0..%d", arg_elnum, INT32_MAX);
                return kScExecErr_Generic;
            }
            DynObjectRef ref = CCDynamicArray::CreateOld(static_cast<uint32_t>(arg_elnum), arg_elsize, arg_managed);
            reg1.SetScriptObject(ref.Obj, ref.Mgr);
            break;
        }
        case SCMD_NEWARRAY2:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const int arg_elnum = reg1.IValue;
            const uint32_t arg_typeid = static_cast<uint32_t>(codeOp.Arg2i());
            const uint32_t arg_elsize = static_cast<uint32_t>(codeOp.Arg3i());
            if (arg_elnum < 0)
            {
                cc_error("Invalid size for dynamic array; requested: %d, range: 0..%d", arg_elnum, INT32_MAX);
                return kScExecErr_Generic;
            }
            // TODO: this likely may be optimized by doing a fixup,
            // which would replace a local typeid with a global one once the script is loaded;
            // but we need to implement such fixup in a compiler first.
            assert(_rtti && !_rtti->IsEmpty());
            const uint32_t global_tid = _typeidLocal2Global->at(arg_typeid);
            DynObjectRef ref = CCDynamicArray::CreateNew(global_tid, static_cast<uint32_t>(arg_elnum), arg_elsize);
            reg1.SetScriptObject(ref.Obj, ref.Mgr);
            break;
        }
        case SCMD_NEWUSEROBJECT:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const uint32_t arg_size = static_cast<uint32_t>(codeOp.Arg2i());
            if (arg_size > INT32_MAX)
            {
                cc_error("Invalid size for user object; requested: %u, range: 0..%d", arg_size, INT32_MAX);
                return kScExecErr_Generic;
            }
            DynObjectRef ref = ScriptUserObject::Create(RTTI::NoType, arg_size);
            reg1.SetScriptObject(ref.Obj, ref.Mgr);
            break;
        }
        case SCMD_NEWUSEROBJECT2:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const uint32_t arg_typeid = codeOp.Arg2i();
            const uint32_t arg_size = codeOp.Arg3i();
            if (arg_size > INT32_MAX)
            {
                cc_error("Invalid size for user object; requested: %u, range: 0..%d", arg_size, INT32_MAX);
                return kScExecErr_Generic;
            }
            // TODO: this likely may be optimized by doing a fixup,
            // which would replace a local typeid with a global one once the script is loaded;
            // but we need to implement such fixup in a compiler first.
            assert(_rtti && !_rtti->IsEmpty());
            const uint32_t global_tid = _typeidLocal2Global->at(arg_typeid);
            DynObjectRef ref = ScriptUserObject::Create(global_tid, arg_size);
            reg1.SetScriptObject(ref.Obj, ref.Mgr);
            break;
        }
        case SCMD_FADD:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const auto arg_lit = codeOp.Arg2i();
            reg1.SetFloat(reg1.FValue + arg_lit); // arg2 was used as int here originally
            break;
        }
        case SCMD_FSUB:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const auto arg_lit = codeOp.Arg2i();
            reg1.SetFloat(reg1.FValue - arg_lit); // arg2 was used as int here originally
            break;
        }
        case SCMD_FMULREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloat(reg1.FValue * reg2.FValue);
            break;
        }
        case SCMD_FDIVREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            if (reg2.FValue == 0.0)
            {
                cc_error("!Floating point divide by zero");
                return kScExecErr_Generic;
            }
            reg1.SetFloat(reg1.FValue / reg2.FValue);
            break;
        }
        case SCMD_FADDREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloat(reg1.FValue + reg2.FValue);
            break;
        }
        case SCMD_FSUBREG:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloat(reg1.FValue - reg2.FValue);
            break;
        }
        case SCMD_FGREATER:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloatAsBool(reg1.FValue > reg2.FValue);
            break;
        }
        case SCMD_FLESSTHAN:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloatAsBool(reg1.FValue < reg2.FValue);
            break;
        }
        case SCMD_FGTE:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloatAsBool(reg1.FValue >= reg2.FValue);
            break;
        }
        case SCMD_FLTE:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            reg1.SetFloatAsBool(reg1.FValue <= reg2.FValue);
            break;
        }
        case SCMD_ZEROMEMORY:
        {
            const auto arg_size = codeOp.Arg1i();
            // Check if we are zeroing at stack tail
            if (_registers[SREG_MAR] == _registers[SREG_SP])
            {
                // creating a local variable -- check the stack to ensure no mem overrun
                ASSERT_STACK_SPACE_BYTES(arg_size);
                // NOTE: according to compiler's logic, this is always followed
                // by SCMD_ADD, and that is where the data is "allocated", here we
                // just clean the place.
                memset(_stackdataPtr, 0, arg_size);
            }
            else
            {
                cc_error("internal error: stack tail address expected on SCMD_ZEROMEMORY instruction, reg[MAR] type is %d",
                    _registers[SREG_MAR].Type);
                return kScExecErr_Generic;
            }
            break;
        }
        case SCMD_CREATESTRING:
        {
            auto &reg1 = _registers[codeOp.Arg1i()];
            const char *ptr = reinterpret_cast<const char*>(reg1.GetDirectPtr());
            DynObjectRef ref = ScriptString::Create(ptr);
            reg1.SetScriptObject(ref.Obj, &myScriptStringImpl);
            break;
        }
        case SCMD_STRINGSEQUAL:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            if ((reg1.IsNull()) || (reg2.IsNull()))
            {
                cc_error("!Null pointer referenced");
                return kScExecErr_Generic;
            }
            else
            {
                const char *ptr1 = reinterpret_cast<const char*>(reg1.GetDirectPtr());
                const char *ptr2 = reinterpret_cast<const char*>(reg2.GetDirectPtr());
                reg1.SetInt32AsBool(strcmp(ptr1, ptr2) == 0);
            }
            break;
        }
        case SCMD_STRINGSNOTEQ:
        {
            auto       &reg1 = _registers[codeOp.Arg1i()];
            const auto &reg2 = _registers[codeOp.Arg2i()];
            if ((reg1.IsNull()) || (reg2.IsNull()))
            {
                cc_error("!Null pointer referenced");
                return kScExecErr_Generic;
            }
            else
            {
                const char *ptr1 = reinterpret_cast<const char*>(reg1.GetDirectPtr());
                const char *ptr2 = reinterpret_cast<const char*>(reg2.GetDirectPtr());
                reg1.SetInt32AsBool(strcmp(ptr1, ptr2) != 0);
            }
            break;
        }
        case SCMD_LOOPCHECKOFF:
            if (loopIterationCheckDisabled == 0)
                loopIterationCheckDisabled++;
            break;
        default:
            cc_error("instruction %d is not implemented", codeOp.Instruction.Code);
            return kScExecErr_Generic;
        }
        /* End perform operation */
        //=====================================================================

        _pc += codeOp.ArgCount + 1;
    }
    return kScExecErr_None;
}

void ScriptExecutor::SetCurrentScript(const RuntimeScript *script)
{
    _current    = script;
    if (_current)
    {
        _code       = _current->GetCode().data();
        _codesize   = _current->GetCode().size();
        _code_fixups = _current->GetCodeFixups().data();
        _strings    = _current->GetStrings().data();
        _stringsize = _current->GetStrings().size();
        _rtti       = RuntimeScript::GetJointRTTI();
        _typeidLocal2Global = &_current->GetLocal2GlobalTypeMap();
    }
    else
    {
        _code       = nullptr;
        _codesize   = 0u;
        _code_fixups = nullptr;
        _strings    = nullptr;
        _stringsize = 0u;
        _rtti       = nullptr;
        _typeidLocal2Global = nullptr;
    }
}

RuntimeScriptValue ScriptExecutor::CallPluginFunction(void *fn_addr, const RuntimeScriptValue *object, const RuntimeScriptValue *params, int param_count)
{
    assert(fn_addr);
    assert(param_count == 0 || params);
    if (!fn_addr)
    {
        cc_error("Null function pointer in CallPluginFunction");
        return {};
    }
    if (param_count > 0 && !params)
    {
        cc_error("Invalid parameters array in CallPluginFunction");
        return {};
    }

    intptr_t parm_value[9];
    if (object)
    {
        parm_value[0] = (intptr_t)object->GetPtrWithOffset();
        param_count++;
    }

    for (int ival = object ? 1 : 0, iparm = 0; ival < param_count; ++ival, ++iparm)
    {
        switch (params[iparm].Type)
        {
        case kScValInteger:
        case kScValFloat:   // AGS passes floats, copying their values into long variable
        case kScValPluginArg:
            parm_value[ival] = (intptr_t)params[iparm].IValue;
            break;
        default:
            parm_value[ival] = (intptr_t)params[iparm].GetPtrWithOffset();
            break;
        }
    }

    //
    // Here we are sending parameters of type intptr_t to a registered function
    // of unknown kind. Intptr_t is 32-bit or 64-bit depending on a build.
    // The exported functions usually have two types of parameters: pointer and
    // 'int' (32-bit). For x32 build those two have the same size, but for x64
    // build pointer has 64-bit size while the 'int' remains 32-bit.
    // In a formal case that would cause 'overflow' - function will receive more
    // data than needed (written to stack), with some values shifted further by
    // 32 bits.
    //
    // Upon testing, however, it was revealed that 64-bit processors usually
    // treat all the function parameters pushed to stack as 64-bit values
    // (few first parameters may be sent via registers, and hence are of least
    // concern anyway). Therefore, no 'overflow' occurs, and 64-bit values are
    // effectively truncated to 32-bit integers in the callee.
    //
    // Formally speaking, this is still unreliable, but we have to live with
    // this for the time being, until a more suitable solution is found and
    // implemented.
    //
    // One basic idea would be to pass array of RuntimeScriptValues and get
    // same RSV as a return result, just like we do with the engine's own exports.
    // Needless to say that such approach would require a breaking change in plugin API.
    //

    typedef intptr_t(*fntype0) ();
    typedef intptr_t(*fntype1) (intptr_t);
    typedef intptr_t(*fntype2) (intptr_t, intptr_t);
    typedef intptr_t(*fntype3) (intptr_t, intptr_t, intptr_t);
    typedef intptr_t(*fntype4) (intptr_t, intptr_t, intptr_t, intptr_t);
    typedef intptr_t(*fntype5) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
    typedef intptr_t(*fntype6) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
    typedef intptr_t(*fntype7) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
    typedef intptr_t(*fntype8) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
    typedef intptr_t(*fntype9) (intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);

    intptr_t result;
    switch (param_count)
    {
    case 0:
        result = reinterpret_cast<fntype0>(fn_addr)();
        break;
    case 1:
        result = reinterpret_cast<fntype1>(fn_addr)(parm_value[0]);
        break;
    case 2:
        result = reinterpret_cast<fntype2>(fn_addr)(parm_value[0], parm_value[1]);
        break;
    case 3:
        result = reinterpret_cast<fntype3>(fn_addr)(parm_value[0], parm_value[1], parm_value[2]);
        break;
    case 4:
        result = reinterpret_cast<fntype4>(fn_addr)(parm_value[0], parm_value[1], parm_value[2], parm_value[3]);
        break;
    case 5:
        result = reinterpret_cast<fntype5>(fn_addr)(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4]);
        break;
    case 6:
        result = reinterpret_cast<fntype6>(fn_addr)(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5]);
        break;
    case 7:
        result = reinterpret_cast<fntype7>(fn_addr)(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5], parm_value[6]);
        break;
    case 8:
        result = reinterpret_cast<fntype8>(fn_addr)(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5], parm_value[6], parm_value[7]);
        break;
    case 9:
        result = reinterpret_cast<fntype9>(fn_addr)(parm_value[0], parm_value[1], parm_value[2], parm_value[3], parm_value[4], parm_value[5], parm_value[6], parm_value[7], parm_value[8]);
        break;
    default:
        cc_error("Too many arguments in call to plugin function");
        return {};
    }

    // Assign as either a numeric value or a pointer, depending on how large the value is.
    // Yes, that's a hack, but that's as much as we can do without having any meta info
    // about the called function's prototype.
    // NOTE: if this is a managed script object, we won't be able to know if we can safely
    // get its manager, unless bytecode instructs us to later down the way.
    return RuntimeScriptValue().SetPluginArgOrPtr(result);
}

void ScriptExecutor::PushValueToStack(const RuntimeScriptValue &rval)
{
    // Write value to the stack tail and advance stack ptr
    _registers[SREG_SP].WriteValue(rval);
    _stackdataPtr += sizeof(int32_t); // formality, to keep data ptr consistent
    _registers[SREG_SP].RValue++;
}

void ScriptExecutor::PushDataToStack(const int32_t num_bytes)
{
    CC_ERROR_IF(_registers[SREG_SP].RValue->IsValid(), "internal error: valid data beyond stack ptr");
    // Assign pointer to data block to the stack tail, advance both stack ptr and stack data ptr
    // NOTE: memory is zeroed by SCMD_ZEROMEMORY
    _registers[SREG_SP].RValue->SetData(_stackdataPtr, num_bytes);
    _stackdataPtr += num_bytes;
    _registers[SREG_SP].RValue++;
}

RuntimeScriptValue ScriptExecutor::PopValueFromStack()
{
    // rewind stack ptr to the last valid value, decrement stack data ptr if needed and invalidate the stack tail
    _registers[SREG_SP].RValue--;
    const RuntimeScriptValue rval = *_registers[SREG_SP].RValue; // save before invalidating
    _stackdataPtr -= sizeof(int32_t); // formality, to keep data ptr consistent
    _registers[SREG_SP].RValue->Invalidate(); // FIXME: bad, this is used to separate PushValue and PushData
    return rval;
}

void ScriptExecutor::PopValuesFromStack(const int32_t num_entries = 1)
{
    for (int i = 0; i < num_entries; ++i)
    {
        // rewind stack ptr to the last valid value, decrement stack data ptr if needed and invalidate the stack tail
        _registers[SREG_SP].RValue--;
        _stackdataPtr -= sizeof(int32_t); // formality, to keep data ptr consistent
        _registers[SREG_SP].RValue->Invalidate(); // FIXME: bad, this is used to separate PushValue and PushData
    }
}

void ScriptExecutor::PopDataFromStack(const int32_t num_bytes)
{
    int32_t total_pop = 0;
    while (total_pop < num_bytes && _registers[SREG_SP].RValue > _stackBegin)
    {
        // rewind stack ptr to the last valid value, decrement stack data ptr if needed and invalidate the stack tail
        _registers[SREG_SP].RValue--;
        _stackdataPtr -= _registers[SREG_SP].RValue->Size;
        // remember popped bytes count
        total_pop += _registers[SREG_SP].RValue->Size;
        _registers[SREG_SP].RValue->Invalidate(); // FIXME: bad, this is used to separate PushValue and PushData
    }
    CC_ERROR_IF(total_pop < num_bytes, "stack underflow");
    CC_ERROR_IF(total_pop > num_bytes, "stack pointer points inside local variable after pop, stack corrupted?");
}

RuntimeScriptValue ScriptExecutor::GetStackPtrOffsetRw(const int32_t rw_offset)
{
    int32_t total_off = 0;
    RuntimeScriptValue *stack_entry = _registers[SREG_SP].RValue;
    while (total_off < rw_offset && stack_entry > _stackBegin)
    {
        stack_entry--;
        total_off += stack_entry->Size;
    }
    CC_ERROR_IF_RETVAL(total_off < rw_offset, RuntimeScriptValue, "accessing address before stack's head");
    RuntimeScriptValue stack_ptr;
    stack_ptr.SetStackPtr(stack_entry);
    stack_ptr.IValue += total_off - rw_offset; // possibly offset to the mid-array
    // Could be accessing array element, so state error only if stack entry does not refer to data array
    CC_ERROR_IF_RETVAL((total_off > rw_offset) && (stack_entry->Type != kScValData), RuntimeScriptValue,
        "stack offset backward: trying to access stack data inside stack entry, stack corrupted?")
    return stack_ptr;
}

void ScriptExecutor::PushToFuncCallStack(FunctionCallStack &func_callstack, const RuntimeScriptValue &rval)
{
    if (func_callstack.Count >= FunctionCallStack::MAX_FUNC_PARAMS)
    {
        cc_error("function callstack overflow");
        return;
    }

    func_callstack.Entries[func_callstack.Head] = rval;
    func_callstack.Head--;
    func_callstack.Count++;
}

void ScriptExecutor::PopFromFuncCallStack(FunctionCallStack &func_callstack, int32_t num_entries)
{
    if (func_callstack.Count == 0)
    {
        cc_error("function callstack underflow");
        return;
    }

    func_callstack.Head += num_entries;
    func_callstack.Count -= num_entries;
}

//-----------------------------------------------------------------------------
//
// Script execution debug dump.
//
// TODO: debug dump that we have now has a very bad performance,
// and is mostly useless for games with big scripts, because it does not print
// any information about the context (which script or function is run, etc).
// It's ported from the older engine code without much change, only for the
// reference. This needs some work to make it any useful with modern games.
//
//-----------------------------------------------------------------------------

#if (DEBUG_CC_EXEC)

void ScriptExecutor::OpenExecLog()
{
    // TODO: let configure file path
    auto s = File::OpenFile("script.log", kFile_Create, kStream_Write);
    _execWriter.reset(new TextStreamWriter(std::move(s)));
}

void ScriptExecutor::CloseExecLog()
{
    _execWriter = {};
}

const char *regnames[] = { "null", "sp", "mar", "ax", "bx", "cx", "op", "dx" };
const char *fixupnames[] = { "null", "fix_gldata", "fix_func", "fix_string", "fix_import", "fix_datadata", "fix_stack" };

void ScriptExecutor::DumpInstruction(const ScriptOperation &op) const
{
    assert(_execWriter);
    if (!_execWriter)
        return;

    // line_num local var should be shared between all the instances
    static int line_num = 0; // FIXME

    if (op.Instruction.Code == SCMD_LINENUM)
    {
        line_num = op.Args[0].IValue;
        return;
    }

    _execWriter->WriteFormat("Line %3d, IP:%8d (SP:%p) ", line_num, _pc, _registers[SREG_SP].RValue);

    const ScriptCommandInfo &cmd_info = sccmd_info[op.Instruction.Code];
    _execWriter->WriteString(cmd_info.CmdName);

    for (int i = 0; i < cmd_info.ArgCount; ++i)
    {
        if (i > 0)
        {
            _execWriter->WriteChar(',');
        }
        if (cmd_info.ArgIsReg[i])
        {
            _execWriter->WriteFormat(" %s", regnames[op.Args[i].IValue]);
        }
        else
        {
            RuntimeScriptValue arg = op.Args[i];
            if (arg.Type == kScValStackPtr || arg.Type == kScValGlobalVar)
            {
                arg = *arg.RValue;
            }
            switch(arg.Type) {
            case kScValInteger:
            case kScValPluginArg:
                _execWriter->WriteFormat(" %d", arg.IValue);
                break;
            case kScValFloat:
                _execWriter->WriteFormat(" %f", arg.FValue);
                break;
            case kScValStringLiteral:
                _execWriter->WriteFormat(" \"%s\"", arg.Ptr);
                break;
            case kScValStackPtr:
            case kScValGlobalVar:
                _execWriter->WriteFormat(" %p", arg.RValue);
                break;
            case kScValData:
            case kScValCodePtr:
                _execWriter->WriteFormat(" %p", arg.GetPtrWithOffset());
                break;
            case kScValStaticArray:
            case kScValScriptObject:
            case kScValStaticFunction:
            case kScValObjectFunction:
            case kScValPluginFunction:
            case kScValPluginObject:
            case kScValPluginArgPtr:
            {
                String name = simp.FindName(arg);
                if (!name.IsEmpty())
                {
                    _execWriter->WriteFormat(" &%s", name.GetCStr());
                }
                else
                {
                    _execWriter->WriteFormat(" %p", arg.GetPtrWithOffset());
                }
             }
                break;
            case kScValUndefined:
				_execWriter->WriteString("undefined");
                break;
             }
        }
    }
    _execWriter->WriteLineBreak();
}

#endif // DEBUG_CC_EXEC

} // namespace Engine
} // namespace AGS
