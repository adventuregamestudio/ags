//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
/*

SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - IMPLEMENTATION

General:
Functions have names of the form 'AaaAaa' or 'AaaAaa_BbbBbb'
where the component parts are camelcased. This means that function 'AaaAaa_BbbBbb' is a
subfunction of function 'AaaAaa' that is exclusively called by function 'AaaAaa'.

The Parser does does NOT get the sequence of tokens in a pipe from the scanning step, i.e.,
it does NOT read the symbols one-by-one. To the contrary, the logic reads back and forth in
the token sequence.

In case of an error, all parser functions call 'UserError()' or 'InternalError()'.
These functions throw an exception that is caught in 'Parser::Parse()'.
If you break on this 'throw' command, the compiler is nicely stopped before the call stack has unwound.

Section and line information: Whenever the parser reads source, e.g. as '_src[...]'
or with 'GetNext()' or 'PeekNext()', it sets a cursor to the position of token read.
Internally, the parser tracks its progress exclusively through this cursor position.
For instance, it tracks at which position in the source code a declaration occurs by
tracking the cursor position where the declaration occurs. Internally, the parser does
not use line numbers or section names, it only uses cursor positions.

Each cursor position can be converted to a line and a section of the source. This is the
line and section info that is used for the benefit of the user when 'UserError()' or
'InternalError()' is called.

However, it's often helpful in debugging to see what line of the source the compiler
is currently at. For this purpose, the compiler keeps a variable 'currentline'. This
variable is _only_ guaranteed to be accurate at the start of a statement or declaration,
and the variable can be clobbered at any time when several compilers run in parallel
because _all_ compilers share the same 'currentline'. So don't let any compier logic
depend on this variable, it is _only_ meant as info during debugging. 

The Parser runs in two phases.
The first phase runs quickly through the tokenized source and collects the headers
of the local functions.

The second phase has the following main components:
    Declaration parsing
    Command parsing
        Functions that process the keyword 'Kkk' are called 'ParseKkk()'

    Code nesting and compound statements
        In 'ParseWhile()' etc., 'HandleEndOf..()', and class 'AGS::Parser::NestingStack'.

    Expression parsing
        In 'ParseExpression()'

    Memory access
        In 'AccessData()'
        In order to read data or write to data, the respective piece of data must
        be located first. This also encompasses literals of the program code.
        '.' and '[...]' are not treated as expression operators (operators like '+')
        but in dedicated functions
        
Notes on how code is emitted:
    The compiler will only add Bytecode to the _end_ of the current codebase.
    But this doesn't mean that all code that has once been emitted will then stay.
    After the compiler has emitted some Bytecode, it will sometimes rip it out again
    and perhaps stash it away in order to re-insert it at a different place later on.
    Also, the compiler will sometimes emit placeholder values that will be patched
    later on.
    So when you halt the compiler inmidst of its run and consider some specific
    cells of the Bytecode, they might have a different content from what they will
    eventually have at the end of the run.
    

'const string's and 'string's
    A '[const] string' is a sequence of bytes in a buffer of unknown length, mainly
    used as a function parameter or a literal. When a variable is declared as a
    'string', which is deprecated, 200 bytes of memory are reserved on the stack
    (local) or in global memory (global). '[const] string's are referred to by the
    address of their first byte. One-dimensional 'char' arrays can be converted to
    '[const] string' and thus be passed to function parameters. 'const string's
    can't be modified, 'string's can. The only way of modifying a 'string' is by
    functions. However, assignments to 'string' are possible and handled with inline
    code. Since the buffer length of 'string's is often unknown, the compiler mostly
    cannot prevent buffer overruns.


MEMORY LAYOUT

Global variables go into their own dedicated memory block and are addressed
    relatively to the beginning of that block. This block is initialized with
    constant values at the start of the game. Thus global variables can start
    out with an initial value. Pointers and Strings can only be initialized to
    null because any other value would entail a runtime computation.

'string' literals go into their own, dedicated memory block and are also
    addressed relatively to the beginning of that block.
	The scanner populates this memory block; for the parser, the whole block is
    treated as constant and read-only.

Imported values: Their exact location is only computed at runtime.
    For the purposes of the parser, imported values are assigned an ordinal number
    #0, #1, #2 etc. and are referenced by their ordinal number.

Local variables go into a memory block, the "local memory block", that is reserved
    on the stack. They are addressed relatively to the start of that block. The
    start of this block can always be determined at compile time by subtracting a
    specific offset from the stack pointer, namely 'OffsetToLocalVarBlock'.
 	This offset changes in the course of the compilation but can always be
    determined at compile time.

A local variable is declared within a nesting of { ... } in the program code;
    It becomes valid at the point of declaration and it becomes invalid when the
    closing '}' to the innermost open '{' is encountered. In the course of reading
    the program from beginning to end, the open '{' that have not yet been closed
    form a stack called the "nesting stack".
    At each point of the compilation run, the count of open braces plus 1 is
    called the "nesting depth" or "scope". 
    Whenever a '{' is encountered, the nesting stack gets an additional level;
    whenever a '}' is encountered, the topmost level is popped from the stack.
        Side Note: Compound statements can have a body that is NOT surrounded with
        braces, e.g., "if (foo) i++;" instead of "if (foo) { i++; }".
        The compiler treats these as if a pair of braces was implied:
        the nesting stack is extended by one level before the compound statement
        body is processed and reduced by one level afterwards.
  
    Each local variable is assigned the scope that is valid at its point of declaration.
    Function parameters have scope 1 by definition; global or imported variables
    have scope 0 by definition.
    When program flow passes a closing '}' then all the variables with higher scope
    are freed. 'continue', 'break' and 'return' statements can break out of several
    '}' at once.In this case, all their respective variables are freed.

Parameters of a function are local variables; they are assigned the nested depth 1.
    Only parameters can have the nested depth 1.
    The first parameter of a function is also the first parameter in the local
    variable block. When 'struct' functions are called then the struct that applies
    to the respective function must be passed to that function: This struct is NOT
    passed as a parameter but in the register OP.


Whilst a function is running, its local variable block is as follows:
    ((lower memory addresses))
		parameter1					<- SP - OffsetToLocalVarBlock
		parameter2
		parameter3
		...
		(return address of the current function)
		variable21 with scope 2
		variable22 with scope 2
		variable23 with scope 2
		...
		variable31 with scope 3
		variable32 with scope 3
		...
		variable41 with scope 4
		...
		(pushed value1)
		(pushed value2)
		...
		(pushed valueN)				<- SP
    ((higher memory addresses))

Classic arrays and Dynarrays, pointers and managed structs:
    Memory that is allocated with "new" is managed by the Engine.
	The compiler must declare the fact that a memory cell shall hold a
    pointer to dynamic memory, by using the opcode MEMWRITEPTR.
    And when a memory cell is no longer reserved for pointers, this
    must be declared as well, using the opcode MEMZEROPTR.
    Whilst a memory cell contains a pointer, it must be manipulated
    through opcodes that end in ...PTR.
		Side note 1: Before a function is called, all its parameters are
        'push'ed to the stack using the PUSHREG opcode.
		So when some parameters are pointers then the fact that the
        respective memory cells contain a pointer isn't declared yet.
		So first thing at the start of the function, all pointer
        parameters must be read with normal non-...PTR opcodes
        and then re-written into the same place with MEMWRITEPTR
		Side note 2: When a '}' is reached and local pointer variables are
        concerned, it isn't enough to just shorten the local memory block.
        MEMZEROPTR must be applied first to all cells that had contained
        pointers in order to tell the engine that those cells won't contain
        pointers any longer.

    Dynarrays: These are sequences of memory blocks that are allocated with 'new'.
        There are two kinds:
        Sequences of pointers ('managed' dynarrays)
        Sequences of a type that is not a pointer ('non-managed' dynarrays)

        A Dynarray of primitives (e.g., int[]) is represented in memory as a
        pointer to a memory sequence (a 'non-managed' sequence) that comprises
        all the elements, one after the other.
        [*]->[][]...[]
        A Dynarray of structs must be a dynarray of 'managed' structs. It is
        represented in memory as a pointer to a block of pointers, each of which
        points to one element.
        [*]->[*][*]...[*]
              |  |     |
              V  V ... V
             [] [] ... []

    "Classic arrays": These are global or local variables, never allocated with 'new', and
        they need not be simple sequences, they can be multi-dimensional.
        A classic array of primitives (e.g., int[12]) or of non-managed structs is represented
        in memory as a block of those elements.
        [][]...[]
        A classic array of managed structs is a classic array of pointers,
        each of which points to a memory block that contains one element.
        [*][*]...[*]
         |  |     |
         V  V ... V
        [] [] ... []

    Pointers are exclusively used for 'managed' memory. If 'managed' structs are manipulated,
        pointers MUST ALWAYS be used; for un-'managed' structs, pointers MAY NEVER be used.
        That means that the compiler can deduce whether a pointer is expected by
        looking at the keyword 'managed' of the struct alone. When the compiler knows that
        a pointer must be used but the source code does not have a '*' then the compiler
        silently implies the '*' and carries on compiling. 
*/


#include <string>
#include <fstream>
#include <cmath>
#include <limits>
#include <memory>
#include <algorithm>

#include "util/string.h"

#include "script/cc_common.h"

#include "cc_internallist.h"
#include "cc_symboltable.h"

#include "cs_parser_common.h"
#include "cs_scanner.h"
#include "cs_parser.h"

// Declared in Common/script/cc_common.h 
// Defined in Common/script/cc_common.cpp
// Note: DO NOT USE this variable in any way except as helper info during debugging
// When compilers run in parallel, they clobber it, this may happen at any time.
// Use '_src.GetPosition()' to track at which point of the source the compiler is.
extern int currentline;

char ccCopyright2[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2026 others";

const std::string  AGS::Parser::_builtinSymbolPrefix = BUILTIN_SYMBOL_PREFIX;
const std::string  AGS::Parser::_builtinDynArrayLength = BUILTIN_DYNAMIC_ARRAY_LENGTH;

// Used when generating Bytecode jump statements where the destination of
// the jump is not yet known. There's nothing special with that number other
// than that it is easy to spot in listings. Don't build logic on that number
int const kDestinationPlaceholder = -77;

std::string const AGS::Parser::TypeQualifierSet2String(TypeQualifierSet tqs) const
{
    std::string ret;
   
    for (auto tq_it = tqs.begin(); tqs.end() != tq_it; tq_it++)
        if (tqs[tq_it->first])
            ret += _sym.GetName(tq_it->second) + " ";
    if (ret.length() > 0)
        ret.pop_back(); // Get rid of the trailing space
    return ret;
}

AGS::Symbol AGS::Parser::MangleStructAndComponent(Symbol stname, Symbol component)
{
    std::string fullname_str = _sym.GetName(stname) + "::" + _sym.GetName(component);
    return _sym.FindOrAdd(fullname_str);
}

void AGS::Parser::SkipNextSymbol(SrcList &src, Symbol expected)
{
    Symbol act = _src.GetNext();
    if (act != expected)
        InternalError(
            "Expected to skip over '%s', found '%s' instead",
            _sym.GetName(expected).c_str(),
            _sym.GetName(act).c_str());
}

void AGS::Parser::Expect(SymbolList const &expected, Symbol const actual, std::string const &custom_msg)
{
    auto found_it = std::find(expected.cbegin(), expected.cend(), actual);
    if (expected.cend() != found_it) // found 'actual' in the list
        return;

    std::string errmsg = custom_msg;
    if (errmsg.empty())
    {
        // Provide a default message
        errmsg = "Expected ";
        for (size_t expected_idx = 0u; expected_idx < expected.size(); expected_idx++)
        {
            errmsg += "'" + _sym.GetName(expected[expected_idx]) + "'";
            if (expected_idx + 2u < expected.size())
                errmsg += ", ";
            else if (expected_idx + 2u == expected.size())
                errmsg += ", or ";
        }
    }
    errmsg += ", found '%s' instead";
    UserError(errmsg.c_str(), _sym.GetName(actual).c_str());
}
            
AGS::Parser::NestingStack::NestingInfo::NestingInfo(NSType stype, ccCompiledScript &scrip)
    : Type(stype)
    , Start(BackwardJumpDest{ scrip })
    , JumpOut(ForwardJump{ scrip })
    , JumpOutLevel(kNoJumpOut)
    , DeadEndWarned(false)
    , BranchJumpOutLevel(0u)
    , SwitchExprVartype(kKW_NoSymbol)
    , SwitchCaseStart(std::vector<BackwardJumpDest>{})
    , SwitchDefaultIdx(kNoDefault)
    , SwitchJumptable(ForwardJump{ scrip })
    , Snippets(std::vector<Snippet>{})
    , OldDefinitions({})
{
}

AGS::Parser::NestingStack::NestingStack(ccCompiledScript &scrip)
    :_scrip(scrip)
{
    // Push first record on stack so that it isn't empty
    Push(NSType::kNone);
}

bool AGS::Parser::NestingStack::AddOldDefinition(Symbol s, SymbolTableEntry const &entry)
{
    if (_stack.back().OldDefinitions.count(s) > 0)
        return true;

    _stack.back().OldDefinitions.emplace(s, entry);
    return false;
}

AGS::Parser::FuncLabelMgr::FuncLabelMgr(ccCompiledScript &scrip, int kind)
    : _scrip(scrip)
    , _kind(kind)
{ }

Symbol AGS::Parser::FuncLabelMgr::GetFirstUnresolvedFunction()
{
    for (auto label_it = _scrip.Labels.begin(); label_it != _scrip.Labels.end(); ++label_it)
        if (_scrip.code[*label_it] % _size == _kind)
            return _scrip.code[*label_it] / _size;
    return kKW_NoSymbol;
}

AGS::Parser::MarMgr::MarMgr(Parser &parser)
    : _parser(parser)
    , _scType (ScT::kNone)
    , _startOffs(0u)
    , _componentOffs (0u)
{ }

AGS::Parser::MarMgr& AGS::Parser::MarMgr::operator=(const MarMgr &other)
{
    if (&this->_parser != &other._parser)
        _parser.InternalError("Illegal assignment");
    this->_componentOffs = other._componentOffs;
    this->_scType = other._scType;
    this->_startOffs = other._startOffs;
    return *this;
}

void AGS::Parser::MarMgr::SetStart(ScopeType type, size_t offset)
{
    if (ScT::kNone != _scType)
        _parser.InternalError("Memory location object doubly initialized ");

    _scType = type;
    _startOffs = offset;
    _componentOffs = 0u;
}

void AGS::Parser::MarMgr::UpdateMAR(size_t lineno, ccCompiledScript &scrip)
{
    switch (_scType)
    {
    default:
        // The start offset is already reached (e.g., when a Dynpointer chain is dereferenced) 
        // but the component offset may need to be processed.
        if (_componentOffs > 0u)
        {
            scrip.WriteCmd(SCMD_ADD, SREG_MAR, _componentOffs);
            _parser._reg_track.SetRegister(SREG_MAR);
        }
        break;

    case ScT::kGlobal:
        scrip.RefreshLineno(lineno);
        scrip.WriteCmd(SCMD_LITTOREG, SREG_MAR, _startOffs + _componentOffs);
        scrip.FixupPrevious(Parser::kFx_GlobalData);
        _parser._reg_track.SetRegister(SREG_MAR);
        break;

    case ScT::kImport:
        // Have to convert the import number into a code offset first.
        // Can only then add the offset to it.
        scrip.RefreshLineno(lineno);
        scrip.WriteCmd(SCMD_LITTOREG, SREG_MAR, _startOffs);
        scrip.FixupPrevious(Parser::kFx_Import);
        if (_componentOffs != 0u)
            scrip.WriteCmd(SCMD_ADD, SREG_MAR, _componentOffs);
        _parser._reg_track.SetRegister(SREG_MAR);
        break;

    case ScT::kLocal:
        scrip.RefreshLineno(lineno);
        CodeCell const offset = scrip.OffsetToLocalVarBlock - _startOffs - _componentOffs;
        if (offset < 0)
            // Must be a bug: That memory is unused.
            _parser.InternalError("Trying to emit the negative offset %d to the top-of-stack", (int) offset);

        scrip.WriteCmd(SCMD_LOADSPOFFS, offset);
        _parser._reg_track.SetRegister(SREG_MAR);
        break;
    }
    Reset();
}

void AGS::Parser::MarMgr::Reset()
{
    _scType = ScT::kNone;
    _startOffs = 0u;
    _componentOffs = 0u;
}

AGS::Parser::RegisterTracking::RegisterTracking(ccCompiledScript &scrip)
    : _scrip(scrip)
    , _tick(0u)
{
    _register_list = std::vector<size_t>{ SREG_AX, SREG_BX, SREG_CX, SREG_DX, SREG_MAR };
    for (auto it = _register_list.begin(); it != _register_list.end(); it++)
        _register[*it] = 0u;
    return;
}

void AGS::Parser::RegisterTracking::SetAllRegisters(void)
{
    for (auto it = _register_list.begin(); it != _register_list.end(); it++)
        SetRegister(*it);
}

size_t AGS::Parser::RegisterTracking::GetGeneralPurposeRegister() const
{
    size_t oldest_reg = INT32_MAX;
    TickT oldest_tick = UINT32_MAX;
    for (auto it = _register_list.begin(); it != _register_list.end(); ++it)
    {
        if (*it == SREG_MAR)
            continue;
        if (_register[*it] >= oldest_tick)
            continue;
        oldest_reg = *it;
        oldest_tick = _register[*it];
    }
    return oldest_reg;
}

AGS::Parser::Parser(SrcList &src, FlagSet options, ccCompiledScript &scrip, SymbolTable &symt, MessageHandler &mh)
    : _nest(scrip)
    , _pp(PP::kPreAnalyze)
    , _reg_track(scrip)
    , _marMgr(*this)
    , _sym(symt)
    , _src(src)
    , _options(options)
    , _scrip(scrip)
    , _msgHandler(mh)
    , _callpointLabels(scrip, 0)
    , _importLabels(scrip, 1)
    , _structRefs({})
{
    _givm.clear();
    _lastEmittedSectionId = 0u;
    _lastEmittedLineno = 0u;
}

void AGS::Parser::SetDynpointerInManagedVartype(Vartype &vartype)
{
    if (_sym.IsManagedVartype(vartype))
        vartype = _sym.VartypeWithDynpointer(vartype);
}

size_t AGS::Parser::StacksizeOfLocals(size_t from_level)
{
    size_t total_size = 0u;
    for (size_t level = from_level; level <= _nest.TopLevel(); level++)
    {
        // We only skim through the old definitions to get their indexes in _sym.
        // We don't use the definitions themselves here but what is current instead.
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.cbegin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            if (_sym.IsVariable(s))
                total_size += _sym.GetSize(s);
        }
    }
    return total_size;
}

bool AGS::Parser::ContainsReleasableDynpointers(Vartype vartype)
{
    if (_sym.IsDynVartype(vartype))
        return true;
    if (_sym.IsArrayVartype(vartype))
        return ContainsReleasableDynpointers(_sym[vartype].VartypeD->BaseVartype);
    if (!_sym.IsStructVartype(vartype))
        return false; // Atomic non-structs cannot have pointers

    SymbolList compo_list;
    _sym.GetComponentsOfStruct(vartype, compo_list);
    for (size_t cl_idx = 0u; cl_idx < compo_list.size(); cl_idx++)
    {
        Symbol const &var = compo_list[cl_idx];
        if (!_sym.IsVariable(var))
            continue;
        if (ContainsReleasableDynpointers(_sym[var].VariableD->Vartype))
            return true;
    }
    return false;
}

void AGS::Parser::FreeDynpointersOfStdArrayOfDynpointer(size_t elements_count)
{
    if (elements_count == 0u)
        return;

    if (elements_count < SIZE_OF_STACK_CELL)
    {
        WriteCmd(SCMD_MEMZEROPTR);
        for (size_t loop = 1u; loop < elements_count; ++loop)
        {
            WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
            _reg_track.SetRegister(SREG_MAR);
            WriteCmd(SCMD_MEMZEROPTR);
        }
        return;
    }

    WriteCmd(SCMD_LITTOREG, SREG_AX, elements_count);
    _reg_track.SetRegister(SREG_AX);

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    WriteCmd(SCMD_MEMZEROPTR);
    WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
    _reg_track.SetRegister(SREG_MAR);
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    _reg_track.SetRegister(SREG_AX);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
}

void AGS::Parser::FreeDynpointersOfStruct(Vartype struct_vtype)
{
    SymbolList compo_list;
    _sym.GetComponentsOfStruct(struct_vtype, compo_list);
    for (int cl_idx = 0; cl_idx < static_cast<int>(compo_list.size()); cl_idx++) // note "int" !
    {
        Symbol const component = compo_list[cl_idx];
        if (_sym.IsVariable(component) && ContainsReleasableDynpointers(_sym[component].VariableD->Vartype))
            continue;
        // Get rid of this component
        compo_list[cl_idx] = compo_list.back();
        compo_list.pop_back();
        cl_idx--; // this might make the var negative so it needs to be int
    }

    // Note, at this point, the components' offsets might no longer be ascending

    int offset_so_far = 0;
    for (auto compo_it = compo_list.begin(); compo_it != compo_list.end(); ++compo_it)
    {
        Symbol const &component = *compo_it;
        int const offset = static_cast<int>(_sym[component].ComponentD->Offset);
        Vartype const vartype = _sym[component].VariableD->Vartype;
        
        // Let MAR point to the component
        int const diff = offset - offset_so_far;
        if (diff != 0)
        {
            WriteCmd(SCMD_ADD, SREG_MAR, diff);
            _reg_track.SetRegister(SREG_MAR);
        }
        offset_so_far = offset;

        if (_sym.IsDynVartype(vartype))
        {
            WriteCmd(SCMD_MEMZEROPTR);
            continue;
        }

        if (compo_list.cend() != compo_it + 1)
            PushReg(SREG_MAR);
        if (_sym.IsArrayVartype(vartype))
            FreeDynpointersOfStdArray(vartype);
        else if (_sym.IsStructVartype(vartype))
            FreeDynpointersOfStruct(vartype);
        if (compo_list.cend() != compo_it + 1)
            PopReg(SREG_MAR);
    }
}

void AGS::Parser::FreeDynpointersOfStdArrayOfStruct(Vartype element_vtype, size_t elements_count)
{

    // AX will be the index of the current element
    WriteCmd(SCMD_LITTOREG, SREG_AX, elements_count);

    BackwardJumpDest loop_start(_scrip);
    loop_start.Set();
    RegisterGuard(RegisterList { SREG_AX, SREG_MAR, },
        [&]
        {
            FreeDynpointersOfStruct(element_vtype);
        });
    
    WriteCmd(SCMD_ADD, SREG_MAR, _sym.GetSize(element_vtype));
    _reg_track.SetRegister(SREG_MAR);
    WriteCmd(SCMD_SUB, SREG_AX, 1);
    _reg_track.SetRegister(SREG_AX);
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
}

void AGS::Parser::FreeDynpointersOfStdArray(Symbol the_array)
{
    Vartype const array_vartype =
        _sym.IsVartype(the_array) ? the_array : _sym.GetVartype(the_array);
    size_t const elements_count = _sym.ArrayElementsCount(array_vartype);
    if (elements_count < 1u)
        return; // nothing to do
    Vartype const element_vartype =
        _sym[array_vartype].VartypeD->BaseVartype;
    if (_sym.IsDynpointerVartype(element_vartype))
    {
        FreeDynpointersOfStdArrayOfDynpointer(elements_count);
        return;
    }

    if (_sym.IsStructVartype(element_vartype))
        FreeDynpointersOfStdArrayOfStruct(element_vartype, elements_count);
}

void AGS::Parser::FreeDynpointersOfLocals(size_t from_level)
{
    // Note: here we only release the pointers contained directly in script's "pointer"
    // variables and PODs (plain structs). Any pointers contained within another dynamically
    // allocated object (struct or array) cannot be released at compile time.
    // Instead, the engine is responsible to track and release these, then cleanup any loose refs
    // using its garbage collector.

    for (size_t level = from_level; level <= _nest.TopLevel(); level++)
    {
        std::map<Symbol, SymbolTableEntry> const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.cbegin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            if (!_sym.IsVariable(s))
                continue; 
            Vartype const s_vartype = _sym.GetVartype(s);
            if (!ContainsReleasableDynpointers(s_vartype))
                continue;

            // Set MAR to the start of the construct that contains releasable pointers
            WriteCmd(SCMD_LOADSPOFFS, _scrip.OffsetToLocalVarBlock - _sym[s].VariableD->Offset);
            _reg_track.SetRegister(SREG_MAR);
            if (_sym.IsDynVartype(s_vartype))
                WriteCmd(SCMD_MEMZEROPTR);
            else if (_sym.IsArrayVartype(s_vartype))
                FreeDynpointersOfStdArray(s);
            else if (_sym.IsStructVartype(s_vartype))
                FreeDynpointersOfStruct(s_vartype);
        }
    }
}

void AGS::Parser::FreeDynpointersOfAllLocals_DynResult(void)
{
    // The return value AX might point to a local dynamic object. So if we
    // now free the dynamic references and we don't take precautions,
    // this dynamic memory might drop its last reference and get
    // garbage collected in consequence. AX would have a dangling pointer.
    // We only need these precautions if there are local dynamic objects.
    RestorePoint rp_before_precautions(_scrip);

    // Allocate a local dynamic pointer to hold the return value.
    PushReg(SREG_AX);
    WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
    _reg_track.SetRegister(SREG_MAR);
    WriteCmd(SCMD_MEMINITPTR, SREG_AX);
    _reg_track.SetRegister(SREG_AX);

    RestorePoint rp_before_freeing(_scrip);
    auto const tick_before_freeing = _reg_track.GetTick();
    FreeDynpointersOfLocals(0u);
	bool const mar_clobbered = !_reg_track.IsValid(SREG_MAR, tick_before_freeing);
    bool const no_precautions_were_necessary = rp_before_freeing.IsEmpty();

    // Now release the dynamic pointer with a special opcode that prevents 
    // memory de-allocation as long as AX still has this pointer, too
    if (mar_clobbered)
    {
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_DYNPOINTER);
        _reg_track.SetRegister(SREG_MAR);
    }
    WriteCmd(SCMD_MEMREADPTR, SREG_AX);
    _reg_track.SetRegister(SREG_AX);
    WriteCmd(SCMD_MEMZEROPTRND); // special opcode
    PopReg(SREG_BX); // do NOT pop AX here
    _reg_track.SetRegister(SREG_BX);

    if (no_precautions_were_necessary)
        rp_before_precautions.Restore();
}

void AGS::Parser::FreeDynpointersOfAllLocals_KeepAX(void)
{
    RestorePoint rp_before_free(_scrip);
    RegisterGuard(SREG_AX,
        [&]
        {
            return FreeDynpointersOfLocals(0u);
        });
}

void AGS::Parser::RestoreLocalsFromSymtable(size_t from_level)
{
    size_t const last_level = _nest.TopLevel();
    for (size_t level = from_level; level <= last_level; level++)
    {
        // Restore the old definition that we've stashed
        auto const &od = _nest.GetOldDefinitions(level);
        for (auto symbols_it = od.begin(); symbols_it != od.end(); symbols_it++)
        {
            Symbol const s = symbols_it->first;
            // Save this local symbol's end of lifescope, and make a full copy for the future record
            _sym[s].LifeScope.second = _scrip.Codesize_i32();
            _sym.localEntries.push_back(_sym[s]);
            // Note, it's important we use deep copy because the vector elements will be destroyed when the level is popped
            _sym[s] = symbols_it->second;
        }
    }
}

void AGS::Parser::HandleEndOfDo()
{
    Expect(
        kKW_While,
        _src.GetNext(),
        "Expected the 'while' of a 'do ... while(...)' statement");
    EvaluationResult eres;
    ParseDelimitedExpression(_src, kKW_OpenParenthesis, eres);
    if (!_sym.IsAnyIntegerVartype(eres.Vartype) && !_sym.IsDynVartype(eres.Vartype))
        UserError(
            "Expected an integer or dynamic array or dynamic pointer expression after 'while', found type '%s' instead",
            _sym.GetName(eres.Vartype).c_str());
    Expect(kKW_Semicolon, _src.GetNext());

    if (!(eres.kTY_Literal == eres.Type &&
          eres.kLOC_SymbolTable == eres.Location &&
          0 == _sym[eres.Symbol].LiteralD->Value))
    {
        EvaluationResultToAx(eres);
        // Jump back to the start of the loop while the condition is true
        _nest.Start().WriteJump(SCMD_JNZ, _src.GetLineno());
    }
    // Jumps out of the loop should go here
    _nest.JumpOut().Patch(_src.GetLineno());

    size_t const jumpout_level = _nest.JumpOutLevel();
    _nest.Pop();
    if (_nest.JumpOutLevel() > jumpout_level)
        _nest.JumpOutLevel() = jumpout_level;
}

void AGS::Parser::HandleEndOfElse()
{
    _nest.JumpOut().Patch(_src.GetLineno());
    size_t const jumpout_level =
        std::max<size_t>(_nest.BranchJumpOutLevel(), _nest.JumpOutLevel());
    _nest.Pop();
    if (_nest.JumpOutLevel() > jumpout_level)
        _nest.JumpOutLevel() = jumpout_level;
}

void AGS::Parser::HandleEndOfSwitch()
{
    // A branch has just ended; set the jumpout level
    _nest.BranchJumpOutLevel() =
        std::max<size_t>(_nest.BranchJumpOutLevel(), _nest.JumpOutLevel());

    // Unless code execution cannot reach this point, 
    // write a jump to the jumpout point to prevent a fallthrough into the jumptable
    bool const dead_end = _nest.JumpOutLevel() > _nest.TopLevel();
    if (dead_end)
    {
        WriteCmd(SCMD_JMP, kDestinationPlaceholder);
        _nest.JumpOut().AddParam();
    }

    // We begin the jump table
    _nest.SwitchJumptable().Patch(_src.GetLineno());

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    CodeCell const eq_opcode =
        _sym.IsAnyStringVartype(_nest.SwitchExprVartype()) ? SCMD_STRINGSEQUAL : SCMD_ISEQUAL;

    const size_t number_of_cases = _nest.Snippets().size();
    const size_t default_idx = _nest.SwitchDefaultIdx();
    for (size_t cases_idx = 0u; cases_idx < number_of_cases; ++cases_idx)
    {
        if (cases_idx == default_idx)
            continue;

        // Emit the code for the case expression of the current case. Result will be in AX
        _nest.Snippets().at(cases_idx).Paste(_scrip);

        // "If switch expression equals case expression, jump to case"
        // Don't auto-generate a 'linenum' directive here: It will point to the end of the switch
        // construct instead of to the respective 'case'/'default', and that is incorrect.
        _scrip.WriteCmd(eq_opcode, SREG_AX, SREG_BX);
        _nest.SwitchCaseStart().at(cases_idx).WriteJump(SCMD_JNZ, _src.GetLineno());
    }

    if (NestingStack::kNoDefault != _nest.SwitchDefaultIdx())
        // "Jump to the default"
        _nest.SwitchCaseStart()[_nest.SwitchDefaultIdx()].WriteJump(SCMD_JMP, _src.GetLineno());

    _nest.JumpOut().Patch(_src.GetLineno());

     // If there isn't a 'default:' branch then control may perhaps continue
     // after this switch (or at least we cannot guarantee otherwise)
     size_t const overall_jumpout_level = NestingStack::kNoDefault == _nest.SwitchDefaultIdx() ?
         _nest.TopLevel() : _nest.BranchJumpOutLevel();

    _nest.Pop();
    if (_nest.JumpOutLevel() > overall_jumpout_level)
        _nest.JumpOutLevel() = overall_jumpout_level;
}

Symbol AGS::Parser::ParseConstantExpression(SrcList &src, std::string const &msg)
{
    if (!src.Length())
        InternalError("Empty expression");

    Symbol const first_sym = src.PeekNext();

    EvaluationResult eres;
    ParseExpression(src, eres);
    if (eres.kTY_Literal != eres.Type)
        UserError(
        (msg + "Cannot evaluate the expression starting with '%s' at compile time").c_str(),
            _sym.GetName(first_sym).c_str());
    return eres.Symbol;
}

Symbol AGS::Parser::ParseFuncdecl_Parameters_ParamDefaultValue(Vartype const param_vartype, size_t const idx)
{
    if (kKW_Assign != _src.PeekNext())
        return kKW_NoSymbol; // No default value given

    // For giving specifics in error messages
    std::string param_prefix_msg = "In parameter #<idx>: ";
    string_replace(param_prefix_msg, "<idx>", std::to_string(idx));

    SkipNextSymbol(_src, kKW_Assign);

    Symbol const default_symbol = ParseConstantExpression(_src, param_prefix_msg);
    
    if (_sym.IsDynVartype(param_vartype))
    {
        Symbol default_value = kKW_Null;
        if (kKW_Null == default_symbol)
            return default_value;
        if (_sym.Find("0") == default_symbol)
        {
            if (PP::kMain == _pp)
                Warning((param_prefix_msg + "Found '0' as the default for a dynamic object (prefer 'null')").c_str());
            return default_value;
        }
        UserError(
            (param_prefix_msg + "Expected the parameter default 'null', found '%s' instead").c_str(),
            _sym.GetName(default_symbol).c_str());
    }

    if (_sym.IsAnyStringVartype(param_vartype))
    {
        Symbol default_value = default_symbol;
        if (_sym.Find("0") == default_symbol)
        {
            if (PP::kMain == _pp)
                Warning((param_prefix_msg + "Found '0' as the default for a string (prefer '\"\"')").c_str());
            return default_value;
        }
        if (!_sym.IsLiteral(default_value) || kKW_String != _sym[default_value].LiteralD->Vartype)
            UserError (
                (param_prefix_msg + "Expected a constant or literal string as a parameter default, found '%s' instead").c_str(),
                _sym.GetName(default_symbol).c_str());
        return default_value;
    }   

    if (_sym.IsAnyIntegerVartype(param_vartype))
    {
        if (!_sym.IsLiteral(default_symbol) || kKW_Int != _sym[default_symbol].LiteralD->Vartype)
            UserError(
                (param_prefix_msg + "Expected a constant integer expression as a parameter default, found '%s' instead").c_str(),
                _sym.GetName(default_symbol).c_str());
        return default_symbol;
    }

    if (kKW_Float == param_vartype)
    {
        if (_sym.Find("0") == default_symbol)
        {
            if (PP::kMain == _pp)
                Warning((param_prefix_msg + "Found '0' as the default for a float (prefer '0.0')").c_str());
        }
        else if (!_sym.IsLiteral(default_symbol) || kKW_Float != _sym[default_symbol].LiteralD->Vartype)
        {
            UserError(
                (param_prefix_msg + "Expected a constant float expression as a parameter default, found '%s' instead").c_str(),
                _sym.GetName(default_symbol).c_str());
        }
        return default_symbol;
    }

    UserError((param_prefix_msg + "Parameter cannot have any default value").c_str());
    return kKW_NoSymbol; // can't reach
}

void AGS::Parser::ParseDynArrayMarkersIfPresent(SrcList &src, Vartype &vartype)
{
    while (true)
    {
        if (kKW_OpenBracket != src.PeekNext())
            return;
        SkipNextSymbol(src, kKW_OpenBracket);
        Expect(kKW_CloseBracket, _src.GetNext());
        vartype = _sym.VartypeWithDynarray(vartype);
        if (!FlagIsSet(_options, SCOPT_RTTIOPS) && kKW_OpenBracket == src.PeekNext())
            UserError("Cannot have dynamic arrays of dynamic arrays because RTTI is off");
    }
}

// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like 'int func(', we expect 'this' --OR-- 'static' (!)
// We'll accept something like "this Character *"
void AGS::Parser::ParseFuncdecl_ExtenderPreparations(bool is_static_extender, Symbol &strct, Symbol &unqualified_name, TypeQualifierSet &tqs)
{
    if (tqs[TQ::kStatic])
		Expect(kKW_Static, _src.PeekNext());

    if (is_static_extender)
        tqs[TQ::kStatic] = true;

    _src.GetNext(); // Eat "this" or "static"
    strct = _src.GetNext();
    if (!_sym.IsStructVartype(strct))
        UserError("Expected a struct type instead of '%s'", _sym.GetName(strct).c_str());

    // Constructors cannot be extenders
    if (unqualified_name == strct)
        UserError("Struct's constructor cannot be defined as extender function");

    Symbol const qualified_name = MangleStructAndComponent(strct, unqualified_name);

    if (kKW_Dynpointer == _src.PeekNext())
    {
        if (is_static_extender)
            UserError("Unexpected '*' after 'static' in static extender function");
        SkipNextSymbol(_src, kKW_Dynpointer); 
    }

    // If a function is defined with the Extender mechanism, it needn't have a declaration
    // in the struct defn. So pretend that this declaration has happened.
    auto &components = _sym[strct].VartypeD->Components;
    if (0 == components.count(unqualified_name))
        components[unqualified_name] = qualified_name;
    _sym.MakeEntryComponent(qualified_name);
    _sym[qualified_name].ComponentD->Component = unqualified_name;
    _sym[qualified_name].ComponentD->Parent = strct;
    
    Symbol const punctuation = _src.PeekNext();
    Expect(SymbolList{ kKW_Comma, kKW_CloseParenthesis }, punctuation);
    if (kKW_Comma == punctuation)
        SkipNextSymbol(_src, kKW_Comma);

    unqualified_name = qualified_name;
}

void AGS::Parser::ParseVarname0(bool const accept_member_access, Symbol &structname, Symbol &varname)
{
    structname = kKW_NoSymbol;
    varname = _src.GetNext();
    if (!_sym.IsIdentifier(varname))
        UserError("Expected an identifier, found '%s' instead", _sym.GetName(varname).c_str());

    // Note: A varname may be allowed although there already is a vartype with the same name.
    // For instance, as a component of a struct. (Room is a vartype; but Character.Room is allowed)
    if (kKW_ScopeRes != _src.PeekNext())
        return;

    SkipNextSymbol(_src, kKW_ScopeRes);
    if (!accept_member_access)
        UserError("Mustn't use '::' here");

    structname = varname;
    Symbol const unqualified_component = _src.GetNext();
    if (_sym.IsVartype(structname))
    {    
        auto const &components = _sym[structname].VartypeD->Components;
        if (0u == components.count(unqualified_component))
            UserError(
                ReferenceMsgSym(
                    "'%s' isn't a component of '%s'",
                    structname).c_str(),
                _sym.GetName(unqualified_component).c_str(),
                _sym.GetName(structname).c_str());

        varname = components.at(unqualified_component);
    }
    else
    {
        // This can happen and be legal for struct component functions
        varname = MangleStructAndComponent(structname, unqualified_component);
    }
}

void AGS::Parser::ParseFuncdecl_Parameters_Param(Symbol const name_of_func, bool const body_follows, bool const read_only, size_t const param_idx)
{
    size_t const declared = _src.GetCursor();

    bool const is_format_param = (kKW_Format == _src.PeekNext());
    if (is_format_param)
    {
        SkipNextSymbol(_src, kKW_Format);
        if (_sym[name_of_func].FunctionD->IsFormat)
            UserError(
                "Parameter #%u: Can only declare at most one parameter per function as '%s'",
                param_idx,
                _sym.GetName(kKW_Format).c_str());
        _sym[name_of_func].FunctionD->IsFormat = true;
    }

    Vartype param_vartype = ParseVartype(_src);

    if (is_format_param && !_sym.IsAnyStringVartype(param_vartype))
        UserError(
            "Expected a string type for the '%s' parameter #%u, found '%s' instead",
            _sym.GetName(kKW_Format).c_str(),
            param_idx,
            _sym.GetName(param_vartype).c_str());

    if (kKW_Void == param_vartype)
        UserError("Parameter #%u: Cannot use the type 'void' in a parameter list", param_idx);
    if (_sym.IsStructVartype(param_vartype) && !_sym.IsManagedVartype(param_vartype))
        UserError(
            ReferenceMsgSym("Parameter #%u: Cannot use the non-managed struct type '%s' in a parameter list", param_vartype).c_str(),
            param_idx,
            _sym.GetName(param_vartype).c_str());

    Symbol const param_name =
        _sym.IsIdentifier(_src.PeekNext()) ? ParseVarname() : kKW_NoSymbol;

    if (body_follows)
    {
        if (kKW_NoSymbol == param_name)
            UserError(
                "Parameter #%u: Expected a parameter name, found '%s' instead",
                param_idx,
                _sym.GetName(_src.PeekNext()).c_str());
        if (name_of_func == param_name)
            // We need access to the data of the current function as long as it is defined,
            // and so we cannot allow a parameter to hide the function in the symbol table
            UserError(
                "Parameter #%u: '%s' is already in use as the name of this function",
                param_idx,
                _sym.GetName(param_name).c_str());
    }
    
    ParseDynArrayMarkersIfPresent(_src, param_vartype);
    Symbol const param_default =
        ParseFuncdecl_Parameters_ParamDefaultValue(param_vartype, param_idx);

    auto &parameters = _sym[name_of_func].FunctionD->Parameters;

    if (!body_follows &&
        kKW_NoSymbol == param_default &&
        !parameters.empty() &&
        kKW_NoSymbol != parameters.back().Default)
        UserError(
            "Parameter #%u follows an optional parameter and so must have a default, too",
            param_idx);

    FuncParameterDesc fpd = {};
    fpd.Vartype = param_vartype;
    fpd.Name = param_name;
    fpd.Default = param_default;
    fpd.Declared = declared;
    fpd.IsFormatParam = is_format_param;
    parameters.push_back(fpd);

    if (PP::kMain != _pp || !body_follows)
        return;

    // Enter parameter as a local variable
    ParseVardecl_CheckAndStashOldDefn(param_name);
    ParseVardecl_Var2SymTable(param_vartype, param_name);

    auto &variable_def = _sym[param_name].VariableD;
    variable_def->TypeQualifiers[TQ::kReadonly] = read_only;
    variable_def->Offset =
        _scrip.OffsetToLocalVarBlock - param_idx * SIZE_OF_STACK_CELL;
    _sym.SetDeclared(param_name, declared);
}

void AGS::Parser::ParseFuncdecl_Parameters(Symbol name_of_func, bool body_follows)
{
    _sym[name_of_func].FunctionD->IsVariadic = false;
    _sym[name_of_func].FunctionD->Parameters.resize(1u); // [0u] is the return type; leave that

    if (PP::kMain == _pp && body_follows)
        _nest.Push(NSType::kParameters);

    if (kKW_Void == _src.PeekNext())
    {
        SkipNextSymbol(_src, kKW_Void);
        return Expect(kKW_CloseParenthesis, _src.GetNext());
    }

    size_t param_idx = 0u;
    while (!_src.ReachedEOF())
    {
        bool read_only = (kKW_Readonly == _src.PeekNext());
        if (read_only)
            SkipNextSymbol(_src, kKW_Readonly);

        Symbol const leading_sym = _src.PeekNext();
        
        if (kKW_CloseParenthesis == leading_sym)
            return SkipNextSymbol(_src, kKW_CloseParenthesis);

        if (kKW_DotDotDot == leading_sym)
        {
            _sym[name_of_func].FunctionD->IsVariadic = true;
            SkipNextSymbol(_src, kKW_DotDotDot);
            return Expect(kKW_CloseParenthesis, _src.GetNext(), "Expected ')' following the '...'");
        }

        if ((++param_idx) >= MAX_FUNCTION_PARAMETERS)
            UserError("Too many parameters defined for function (max. allowed: %u)", MAX_FUNCTION_PARAMETERS);

        ParseFuncdecl_Parameters_Param(name_of_func, body_follows, read_only, _sym.FuncParamsCount(name_of_func) + 1u);

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseParenthesis }, punctuation);
        if (kKW_CloseParenthesis == punctuation)
        {
            if (_sym[name_of_func].FunctionD->IsFormat)
                UserError(
                    "Function '%s' has a '%s' parameter and thus must be variadic",
                    _sym.GetName(name_of_func).c_str(),
                    _sym.GetName(kKW_Format).c_str());
            return;
        }
        continue;
    } // while

    InternalError("End of input when processing parameter list"); // Cannot happen
}

void AGS::Parser::ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype const return_vartype, Symbol struct_of_func, Symbol name_of_func, bool body_follows)
{
    _sym.MakeEntryFunction(name_of_func);
    SymbolTableEntry &entry = _sym[name_of_func];

    entry.LifeScope = std::make_pair(_scrip.Codesize_i32(), _scrip.Codesize_i32());
    
    entry.FunctionD->IsConstructor = (kKW_NoSymbol != struct_of_func) &&
        entry.ComponentD && (entry.ComponentD->Component == entry.ComponentD->Parent);
    entry.FunctionD->Parameters.resize(1u);
    // Function return type (entered as [0u])
    entry.FunctionD->Parameters[0u].Vartype = return_vartype;
    entry.FunctionD->Parameters[0u].Name = kKW_NoSymbol;
    entry.FunctionD->Parameters[0u].Default = kKW_NoSymbol;
    auto &entry_tqs = entry.FunctionD->TypeQualifiers;
    entry_tqs[TQ::kImport] = tqs[TQ::kImport];
    entry_tqs[TQ::kProtected] = tqs[TQ::kProtected];
    entry_tqs[TQ::kReadonly] = tqs[TQ::kReadonly];
    entry_tqs[TQ::kStatic] = tqs[TQ::kStatic];
    entry_tqs[TQ::kWriteprotected] = tqs[TQ::kWriteprotected];
    // Do not set Extends and the component flag here.
    // They are used to denote functions that were either declared in a struct defn or as extender

    if (PP::kPreAnalyze == _pp)
    {
        // Encode in entry.Offset the type of function declaration
        FunctionType ft = kFT_PureForward;
        if (tqs[TQ::kImport])
            ft = kFT_Import;
        if (body_follows)
            ft = kFT_LocalBody;
        if (_sym[name_of_func].FunctionD->Offset < ft)
            _sym[name_of_func].FunctionD->Offset = ft;
    }
}

void AGS::Parser::ParseFuncdecl_CheckAndAddKnownInfo(Symbol const name_of_func, SymbolTableEntry::FunctionDesc *known_info, size_t known_declared, bool const body_follows)
{
    if (!known_info)
        return; // We don't have any known info
    auto &this_entry = _sym[name_of_func].FunctionD;
    if (!this_entry)
        InternalError("Function record missing");

    // Type qualifiers - must match
    auto known_tq = known_info->TypeQualifiers;
    known_tq[TQ::kImport] = false;
    auto this_tq = this_entry->TypeQualifiers;
    this_tq[TQ::kImport] = false;
    if (known_tq != this_tq)
    {
        std::string const known_tq_str = TypeQualifierSet2String(known_tq);
        std::string const this_tq_str = TypeQualifierSet2String(this_tq);
        std::string const msg = ReferenceMsgLoc("'%s' has the qualifiers '%s' here but '%s' elsewhere", known_declared);
        UserError(msg.c_str(), _sym.GetName(name_of_func).c_str(), this_tq_str.c_str(), known_tq_str.c_str());
    }

    // Return type - must match
    Symbol const known_ret_type = known_info->Parameters[0u].Vartype;
    Symbol const this_ret_type = this_entry->Parameters[0u].Vartype;
    if (known_ret_type != this_ret_type)
        UserError(
            ReferenceMsgLoc(
                "Return type of '%s' is declared as '%s' here, as '%s' elsewhere",
                known_declared).c_str(),
            _sym.GetName(name_of_func).c_str(),
            _sym.GetName(this_ret_type).c_str(),
            _sym.GetName(known_ret_type).c_str());

    // Whether variadic function - must match
    if (known_info->IsVariadic != this_entry->IsVariadic)
    {
        std::string const te =
            this_entry->IsVariadic ?
            "is declared to accept additional parameters here" :
            "is declared to not accept additional parameters here";
        std::string const ki =
            known_info->IsVariadic ?
            "to accepts additional parameters elsewhere" :
            "to not accept additional parameters elsewhere";
        UserError(
            ReferenceMsgLoc("Function '%s' %s, %s", known_declared).c_str(),
            _sym.GetName(name_of_func).c_str(),
            te.c_str(),
            ki.c_str());
    }

    // Number of explicit parameters - must match
    auto const &known_params = known_info->Parameters;
    auto &this_params = this_entry->Parameters;
    if (known_params.size() != this_params.size())
        UserError(
            ReferenceMsgLoc(
                "Function '%s' is declared with %u explicit parameters here, "
                "%u explicit parameters elswehere",
                known_declared).c_str(),
            _sym.GetName(name_of_func).c_str(),
            this_params.size() - 1u,
            known_params.size() - 1u);

    // Vartypes of the explicit parameters - must match
    for (size_t param_idx = 1u; param_idx < this_params.size(); param_idx++)
    {
        Vartype const known_param_vartype = known_params[param_idx].Vartype;
        Vartype const this_param_vartype = this_params[param_idx].Vartype;
        if (known_param_vartype != this_param_vartype)
            UserError(
                ReferenceMsgLoc(
                    "For function '%s': Type of parameter #%u is '%s' here, '%s' in a declaration elsewhere",
                    known_declared).c_str(),
                _sym.GetName(name_of_func).c_str(),
                param_idx,
                _sym.GetName(this_param_vartype).c_str(),
                _sym.GetName(known_param_vartype).c_str());
    }

    // Inconsistency in parameter names - is accumulated
    auto &this_incon = this_entry->ParamNamingInconsistency;
    auto const &known_incon = known_info->ParamNamingInconsistency;
    if (known_incon.Exists)
    {
        this_incon.Exists = known_incon.Exists;
        this_incon.ParamIdx = known_incon.ParamIdx;
        this_incon.Name1 = known_incon.Name1;
        this_incon.Declared1 = known_incon.Declared1;
        this_incon.Name2 = known_incon.Name2;
        this_incon.Declared2 = known_incon.Declared2;
    }

    // Parameter names - are accumulated; check for naming inconsistency
    for (size_t param_idx = 1u; param_idx < this_params.size(); ++param_idx)
    {
        auto &this_name = this_params[param_idx].Name;
        auto const &known_name = known_params[param_idx].Name;

        if (kKW_NoSymbol == this_name)
            this_name = known_name;

        if (kKW_NoSymbol == known_name ||
            kKW_NoSymbol == this_name ||
            this_name == known_name)
            continue;

        // Found a naming inconsistency
        auto &incon = this_entry->ParamNamingInconsistency;
        if (incon.Exists)
            continue; // it's enough to track 1 inconsistency

        incon.Exists = true;
        incon.ParamIdx = param_idx;
        incon.Name1 = this_name;
        incon.Declared1 = this_params[param_idx].Declared;
        incon.Name2 = known_name;
        incon.Declared2 = known_params[param_idx].Declared;
    }

    // Parameter defaults - may be completely omitted when the body follows
    //     (for legacy reasons)
    //     Otherwise - must match
    if (body_follows)
    {
        bool at_least_one_default_exists = false;
        for (size_t param_idx = 1u; param_idx < this_params.size(); ++param_idx)
            if (kKW_NoSymbol != this_params[param_idx].Default)
            {
                at_least_one_default_exists = true;
                break;
            }
        if (!at_least_one_default_exists)
        {
            // Copy the known defaults over
            for (size_t param_idx = 1u; param_idx < this_params.size(); ++param_idx)
                this_params[param_idx].Default = known_params[param_idx].Default;
        }
    }

    // Defaults must now all match
    for (size_t param_idx = 1u; param_idx < this_params.size(); ++param_idx)
    {
        auto const this_default = this_params[param_idx].Default;
        auto const known_default = known_params[param_idx].Default;
        if (this_default == known_default)
            continue;

        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        string_replace(errstr1, "<1>", std::to_string(param_idx));
        if (kKW_NoSymbol == this_default)
            string_replace(errstr1, "<2>", "doesn't have a default value");
        else
            string_replace(errstr1, "<2>", "has the default " + _sym.GetName(this_default));

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>";
        if (kKW_NoSymbol == known_default)
            string_replace(errstr2, "<2>", "doesn't have a default value");
        else
            string_replace(errstr2, "<2>", "has the default " + _sym.GetName(known_default));
        errstr1 += errstr2;
        UserError(ReferenceMsgLoc(errstr1, known_declared).c_str());
    }
}

void AGS::Parser::ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, size_t params_count, CodeLoc &function_soffs)
{
    if (body_follows)
    {
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = _scrip.AddNewFunction(_sym.GetName(name_of_func), params_count);
        if (function_soffs < 0)
            UserError("Max. number of functions exceeded");
		
        _callpointLabels.SetLabelValue(name_of_func, function_soffs);
        return;
    }

    if (!func_is_import)
    {
        function_soffs = -1; // forward decl; callpoint is unknown yet
        return;
    }

    // Index of the function in the ccScript::imports[] array
    function_soffs = _scrip.FindOrAddImport(_sym.GetName(name_of_func));
}

bool AGS::Parser::ParseFuncdecl_DoesBodyFollow()
{
    int const cursor = _src.GetCursor();
    _src.SkipToCloser();
    SkipNextSymbol(_src, kKW_CloseParenthesis);
    bool body_follows = (kKW_OpenBrace == _src.PeekNext());
    _src.SetCursor(cursor);
    return body_follows;
}

void AGS::Parser::ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check)
{
    if (0 == _sym.GetName(name_of_func).compare(0u, _builtinSymbolPrefix.length(), _builtinSymbolPrefix))
        UserError("Function names may not begin with '__Builtin_'");

    SymbolTableEntry const &func_entry = _sym[name_of_func];
    bool const is_constructor = (kKW_NoSymbol != struct_of_func) &&
        func_entry.ComponentD && (func_entry.ComponentD->Component == func_entry.ComponentD->Parent);

    if (tqs[TQ::kImport] && body_follows)
        UserError("Cannot declare this function 'import' and define its body at the same time");

    if (kKW_NoSymbol == struct_of_func && tqs[TQ::kProtected])
        UserError(
            "Function '%s' isn't a struct component and so cannot be 'protected'",
            _sym.GetName(name_of_func).c_str());
    if (is_constructor && !_sym[struct_of_func].VartypeD->Flags[VTF::kManaged])
        UserError(
            "Constructors are not supported in non-managed structs (see '%s')",
            _sym.GetName(name_of_func).c_str());
    if (is_constructor && tqs[TQ::kStatic])
        UserError(
            "Static constructors are not supported (see '%s')",
            _sym.GetName(name_of_func).c_str());
    if (is_constructor && tqs[TQ::kProtected])
        UserError(
            "Struct's constructor '%s' cannot be 'protected'",
            _sym.GetName(name_of_func).c_str());
    if (!body_follows && no_loop_check)
        UserError("Can only use 'noloopcheck' when a function body follows the definition");
    if(!_sym.IsFunction(name_of_func) && _sym.IsInUse(name_of_func))
        UserError(
            ReferenceMsgSym("'%s' is defined elsewhere as a non-function", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
    if (is_constructor && (kKW_Void != return_vartype))
        UserError(
            ReferenceMsgSym("Struct's constructor '%s' must be 'void' but is '%s'", return_vartype).c_str(),
            _sym.GetName(name_of_func).c_str(),
            _sym.GetName(return_vartype).c_str());
    if (!_sym.IsManagedVartype(return_vartype) && _sym.IsStructVartype(return_vartype))
        UserError(
            ReferenceMsgSym("Cannot return the non-managed struct type '%s'", return_vartype).c_str(),
            _sym.GetName(return_vartype).c_str());

    if (PP::kPreAnalyze == _pp &&
        body_follows &&
        _sym.IsFunction(name_of_func) &&
        kFT_LocalBody == func_entry.FunctionD->Offset)
        UserError(
            ReferenceMsgSym("Function '%s' is already defined with body elsewhere", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());

    if (PP::kMain != _pp || kKW_NoSymbol == struct_of_func)
        return;

    if (!_sym.IsComponent(name_of_func) ||
        struct_of_func != func_entry.ComponentD->Parent)
    {
        // Functions only become struct components if they are declared in a struct or as extender
        std::string component = _sym.GetName(name_of_func);
        component.erase(0u, component.rfind(':') + 1u);
        UserError(
            ReferenceMsgSym("Function '%s' has not been declared within struct '%s' as a component", struct_of_func).c_str(),
            component.c_str(), _sym.GetName(struct_of_func).c_str());
    }
}

void AGS::Parser::ParseFuncdecl_HandleFunctionOrImportIndex(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool body_follows)
{
    if (PP::kMain == _pp)
    {
        int func_startoffs;
        ParseFuncdecl_EnterAsImportOrFunc(name_of_func, body_follows, tqs[TQ::kImport], _sym.FuncParamsCount(name_of_func), func_startoffs);
        _sym[name_of_func].FunctionD->Offset = func_startoffs;
    }

    if (!tqs[TQ::kImport])
        return;

    // Imported functions
    _sym[name_of_func].FunctionD->TypeQualifiers[TQ::kImport] = true;
    // Import functions have an index into the imports[] array in lieu of a start offset.
    auto const imports_idx = _sym[name_of_func].FunctionD->Offset;
    
    _sym[name_of_func].FunctionD->TypeQualifiers[TQ::kImport] = true;

    if (PP::kPreAnalyze == _pp)
    {
        _sym[name_of_func].FunctionD->Offset = kFT_Import;
        return;
    }

    if (imports_idx < 0 || static_cast<size_t>(imports_idx) >= _scrip.imports.size())
        InternalError(
            "Imported function '%s' has the imports index '%d' which is invalid",
            _sym.GetName(name_of_func).c_str(),
            imports_idx);

    // Append the number of parameters to the name of the import:
    // this lets the engine to link different implementations depending on API
    // which was used when compiling the script.
    // (Sort of a limited "function overloading")
    if (_scrip.imports[imports_idx].find('^') == std::string::npos)
    {
        char appendage[10];
        snprintf(appendage, sizeof(appendage), "^%zu", _sym.FuncParamsCount(name_of_func) + 100 * _sym[name_of_func].FunctionD->IsVariadic);
        _scrip.imports[imports_idx].append(appendage);
    }

    _importLabels.SetLabelValue(name_of_func, imports_idx);
}

void AGS::Parser::ParseFuncdecl(TypeQualifierSet tqs, Vartype const return_vartype, Symbol const struct_of_func, Symbol const name_of_func, bool const no_loop_check, bool  const body_follows)
{
    ParseFuncdecl_Checks(tqs, struct_of_func, name_of_func, return_vartype, body_follows, no_loop_check);
    
    // A forward decl can be written with the "import" keyword (when allowed in the options).
    // This isn't an import proper, so reset the "import" flag in this case.
    if (tqs[TQ::kImport] &&  // This declaration has 'import'
        _sym.IsFunction(name_of_func) &&
        !_sym[name_of_func].FunctionD->TypeQualifiers[TQ::kImport]) // but symbol table hasn't 'import'
    {
        if (FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
            UserError(ReferenceMsgSym(
                "In here, a function with a local body must not have an \"import\" declaration",
                name_of_func).c_str());
        tqs[TQ::kImport] = false;
    }

    // Stash away the known info about the function so that we can check whether this declaration is compatible
    std::unique_ptr<SymbolTableEntry::FunctionDesc> known_info{ _sym[name_of_func].FunctionD };
    _sym[name_of_func].FunctionD = nullptr;
    size_t const known_declared = _sym.GetDeclared(name_of_func);

    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, struct_of_func, name_of_func, body_follows);
    if (PP::kMain == _pp && body_follows)
        _sym[name_of_func].FunctionD->NoLoopCheck = no_loop_check;
    ParseFuncdecl_Parameters(name_of_func, body_follows);
    
    ParseFuncdecl_CheckAndAddKnownInfo(name_of_func, known_info.get(), known_declared, body_follows);

    ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);

    if (PP::kMain == _pp && body_follows)
        // When this function is called, the return address is pushed to the stack
        _scrip.OffsetToLocalVarBlock += SIZE_OF_STACK_CELL;
}

int AGS::Parser::IndexOfLeastBondingOperator(SrcList &expression)
{
    size_t nesting_depth = 0u;

    int largest_prio_found = INT_MIN; // note: largest number == lowest priority
    bool largest_is_prefix = false;
    int index_of_largest = -1;

    // Tracks whether the preceding iteration had an operand
    bool encountered_operand = false;

    expression.StartRead();
    while (!expression.ReachedEOF())
    {
        Symbol const current_sym = expression.GetNext();

        if (kKW_CloseBracket == current_sym || kKW_CloseParenthesis == current_sym)
        {
            encountered_operand = true;
            if (nesting_depth > 0)
                nesting_depth--;
            continue;
        }

        if (kKW_OpenBracket == current_sym || kKW_OpenParenthesis == current_sym)
        {
            nesting_depth++;
            continue;
        }

        if (!_sym.IsOperator(current_sym))
        {
            encountered_operand = true;
            continue;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (nesting_depth > 0)
            continue;

        bool const is_prefix = !encountered_operand;
        encountered_operand = false;

        if (kKW_Increment == current_sym || kKW_Decrement == current_sym)
        {
            // These can be postfix as well as prefix. Assume they are postfix here
            // This means that they operate on a preceding operand, so in the next
            // loop, we will have encountered an operand.
            encountered_operand = true;
        }

        int const current_prio =
            is_prefix ?
            _sym.PrefixOpPrio(current_sym) : _sym.BinaryOrPostfixOpPrio(current_sym);
        if (current_prio < 0)
            UserError(
                is_prefix ?
                "Cannot use '%s' as a prefix operator" :
                "Cannot use '%s' as a binary or postfix operator",
                _sym.GetName(current_sym).c_str());

        if (current_prio < largest_prio_found)
            continue; // cannot be lowest priority

        largest_prio_found = current_prio;
        // The cursor has already moved to the next symbol, so the index is one less
        index_of_largest = expression.GetCursor() - 1;
        largest_is_prefix = is_prefix;
    } // while (!expression.ReachedEOF())

    // If a prefix operator turns out not to be in first position,
    // it must be the end of a chain of unary operators and the first
    // of those should be evaluated first
    return (largest_is_prefix) ? 0 : index_of_largest;
}

CodeCell AGS::Parser::GetOpcode(Symbol const op_sym, Vartype vartype1, Vartype vartype2)
{
    if (!_sym.IsAssignment(op_sym) && !_sym.IsOperator(op_sym))
        InternalError("'%s' isn't an assignment or operator", _sym.GetName(op_sym).c_str());

    if (kKW_Float == vartype1 || kKW_Float == vartype2)
    {
        if (vartype1 != kKW_Float)
            UserError("Cannot apply the operator '%s' to a non-float and a float", _sym.GetName(op_sym).c_str());
        if (vartype2 != kKW_Float)
            UserError("Cannot apply the operator '%s' to a float and a non-float", _sym.GetName(op_sym).c_str());

        CodeCell opcode = _sym[op_sym].OperatorD->FloatOpcode;

        if (SymbolTable::kNoOpcode == opcode)
            UserError("Cannot apply the operator '%s' to float values", _sym.GetName(op_sym).c_str());
        return opcode;
    }

    bool const iatos1 = _sym.IsAnyStringVartype(vartype1);
    bool const iatos2 = _sym.IsAnyStringVartype(vartype2);

    if (iatos1 || iatos2)
    {
        if (kKW_Null == vartype1 || kKW_Null == vartype2)
            // Don't use strings comparison against NULL: This will provoke a runtime error
            return _sym[op_sym].OperatorD->DynOpcode;
		
        if (!iatos1)
            UserError("Can only compare 'null' or a string to another string");
        if (!iatos2)
            UserError("Can only compare a string to another string or 'null'");

        CodeCell opcode = _sym[op_sym].OperatorD->StringOpcode;

        if (SymbolTable::kNoOpcode == opcode)
            UserError("Cannot apply the operator '%s' to string values", _sym.GetName(op_sym).c_str());
        
        return opcode;
    }

    if (((_sym.IsDynpointerVartype(vartype1) || kKW_Null == vartype1) &&
        (_sym.IsDynpointerVartype(vartype2) || kKW_Null == vartype2)) ||
        ((_sym.IsDynarrayVartype(vartype1) || kKW_Null == vartype1) &&
        (_sym.IsDynarrayVartype(vartype2) || kKW_Null == vartype2)))
    {
        CodeCell opcode = _sym[op_sym].OperatorD->DynOpcode;

        if (SymbolTable::kNoOpcode == opcode)
            UserError("Cannot apply the operator '%s' to managed types", _sym.GetName(op_sym).c_str());
        return opcode;
    }

    // Other combinations of managed types won't mingle
    if (_sym.IsDynpointerVartype(vartype1) || _sym.IsDynpointerVartype(vartype2))
        UserError(
            "Cannot apply the operator '%s' to a type '%s' and a type '%s'",
            _sym.GetName(op_sym).c_str(),
            _sym.GetName(vartype1).c_str(),
            _sym.GetName(vartype2).c_str());

    // Integer types
    CodeCell opcode = _sym[op_sym].OperatorD->IntOpcode;

    std::string msg = "Left-hand side of '<op>' term";
    string_replace(msg, "<op>", _sym.GetName(op_sym));
    CheckVartypeMismatch(vartype1, kKW_Int, true, msg);
    msg = "Right-hand side of '<op>' term";
    string_replace(msg, "<op>", _sym.GetName(op_sym));
    CheckVartypeMismatch(vartype2, kKW_Int, true, msg);
    return opcode;
}

bool AGS::Parser::IsVartypeMismatch_Oneway(Vartype vartype_is, Vartype vartype_wants_to_be) const
{
    // cannot convert 'void' to anything
    if (kKW_Void == vartype_is || kKW_Void == vartype_wants_to_be)
        return true;

    // Don't convert if no conversion is called for
    if (vartype_is == vartype_wants_to_be)
        return false;


    // Can convert 'null' to dynpointer or dynarray or 'const string'
    if (kKW_Null == vartype_is)
        return
            _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be) != kKW_String &&
            !_sym.IsDynpointerVartype(vartype_wants_to_be) &&
            !_sym.IsDynarrayVartype(vartype_wants_to_be);

    // Can only assign dynarray pointers to dynarray pointers.
    if (_sym.IsDynarrayVartype(vartype_is) != _sym.IsDynarrayVartype(vartype_wants_to_be))
        return true;

    // Can convert 'String *' to 'const string'
    if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_is) &&
        kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be))
    {
        return false;
    }

    // Can convert 'string' or 'const string' to 'String *'
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_is) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be))
    {
        return false;
    }

    // Note: CanNOT convert 'String *' or 'const string' to 'string';
    // a function that has a 'string' parameter may modify it,
    // but a 'String' or 'const string' may not be modified.

    if (_sym.IsOldstring(vartype_is) != _sym.IsOldstring(vartype_wants_to_be))
        return true;

    // Note: the position of this test is important.
    // Don't "group" string tests "together" and move this test above or below them.
    // Cannot convert 'const' to non-'const'
    if (_sym.IsConstVartype(vartype_is) && !_sym.IsConstVartype(vartype_wants_to_be))
        return true;

    if (_sym.IsOldstring(vartype_is))
        return false;

    // From here on, don't mind constness
    vartype_is = _sym.VartypeWithout(VTT::kConst, vartype_is);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be);

    // 'float's cannot mingle with other types
    if ((vartype_is == kKW_Float) != (vartype_wants_to_be == kKW_Float))
        return true;

    // Can convert 'short', 'char' etc. into 'int'
    if (_sym.IsAnyIntegerVartype(vartype_is) && kKW_Int == vartype_wants_to_be)
        return false;

    // Checks to do if at least one is dynarray
    if (_sym.IsDynarrayVartype(vartype_is) || _sym.IsDynarrayVartype(vartype_wants_to_be))
    {
        if (_sym.IsDynarrayVartype(vartype_is) != _sym.IsDynarrayVartype(vartype_wants_to_be))
            return true;

        Symbol const core_wants_to_be = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);
        Symbol const core_is = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
        // In unmanaged dynarrays, elements of fixed length follow one after the other,
        // so we cannot allow different element types, e.g. shorts converting to int etc.
        if (!_sym.IsManagedVartype(core_is) && core_is != core_wants_to_be)
            return true;

        return IsVartypeMismatch_Oneway(core_is, core_wants_to_be);
    }

    // Checks to do if at least one is dynpointer
    if (_sym.IsDynpointerVartype(vartype_is) || _sym.IsDynpointerVartype(vartype_wants_to_be))
    {
        if (_sym.IsDynpointerVartype(vartype_is) != _sym.IsDynpointerVartype(vartype_wants_to_be))
            return true;

        // Accept conversion of a type to any of its ancesters
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be);
        Symbol current_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_is);
        while (current_core_vartype != target_core_vartype)
        {
            current_core_vartype = _sym[current_core_vartype].VartypeD->Parent;
            if (kKW_NoSymbol == current_core_vartype)
                return true;
        }
        return false;
    }

    // Checks to do if at least one is a struct or an array
    if (_sym.IsStructVartype(vartype_is) || _sym.IsStructVartype(vartype_wants_to_be) ||
        _sym.IsArrayVartype(vartype_is) || _sym.IsArrayVartype(vartype_wants_to_be))
        return (vartype_is != vartype_wants_to_be);

    return false;
}

void AGS::Parser::CheckVartypeMismatch(Vartype vartype_is, Vartype vartype_wants_to_be, bool orderMatters, std::string const &msg)
{
    if (!IsVartypeMismatch_Oneway(vartype_is, vartype_wants_to_be))
        return;
    if (!orderMatters && !IsVartypeMismatch_Oneway(vartype_wants_to_be, vartype_is))
        return;

    std::string is_vartype_string = "'" + _sym.GetName(vartype_is) + "'";
    std::string wtb_vartype_string = "'" + _sym.GetName(vartype_wants_to_be) + "'";
    if (_sym.IsAnyArrayVartype(vartype_is) != _sym.IsAnyArrayVartype(vartype_wants_to_be))
    {
        if (_sym.IsAnyArrayVartype(vartype_is))
            is_vartype_string = "an array";
        if (_sym.IsAnyArrayVartype(vartype_wants_to_be))
            wtb_vartype_string = "an array";
    }
    if (_sym.IsAnyStringVartype(vartype_is) != _sym.IsAnyStringVartype(vartype_wants_to_be))
    {
        if (_sym.IsAnyStringVartype(vartype_is))
            is_vartype_string = "a kind of string";
        if (_sym.IsAnyStringVartype(vartype_wants_to_be))
            wtb_vartype_string = "a kind of string";
    }
    if (_sym.IsDynpointerVartype(vartype_is) != _sym.IsDynpointerVartype(vartype_wants_to_be))
    {
        if (_sym.IsDynpointerVartype(vartype_is))
            is_vartype_string = "a pointer";
        if (_sym.IsDynpointerVartype(vartype_wants_to_be))
            wtb_vartype_string = "a pointer";
    }

    UserError(
        ((msg.empty()? "Type mismatch" : msg) + ": Cannot convert %s to %s").c_str(),
        is_vartype_string.c_str(),
        wtb_vartype_string.c_str());
}

void AGS::Parser::ConvertAXStringToStringObject(Vartype wanted_vartype, Vartype &current_vartype)
{
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, current_vartype) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, wanted_vartype))
    {
        WriteCmd(SCMD_CREATESTRING, SREG_AX); // convert AX
        current_vartype = _sym.VartypeWithDynpointer(_sym.GetStringStructSym());
    }
}

int AGS::Parser::GetReadCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMREAD;
    case 1:  return SCMD_MEMREADB;
    case 2:  return SCMD_MEMREADW;
    }
}

int AGS::Parser::GetWriteCommandForSize(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMWRITE;
    case 1:  return SCMD_MEMWRITEB;
    case 2:  return SCMD_MEMWRITEW;
    }
}

void AGS::Parser::EvaluationResultToAx(EvaluationResult &eres)
{
    Vartype const vartype = eres.Vartype;

    switch (eres.Type)
    {
    default:
        InternalError("Expression result has type %d, cannot move to AX", eres.Type);

    case EvaluationResult::kTY_FunctionName:
        // Cannot convert a naked function symbol; assume that the coder has forgotten '('
        UserError("Expected '(' after '%s'", _sym.GetName(eres.Symbol).c_str());

    case EvaluationResult::kTY_Literal:
        // Convert to runtime value in AX
        WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[eres.Symbol].LiteralD->Value);
        _reg_track.SetRegister(SREG_AX);
        if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype))
            _scrip.FixupPrevious(kFx_String);
        eres.Type = eres.kTY_RunTimeValue;
        eres.Location = eres.kLOC_AX;
        eres.Symbol = kKW_NoSymbol;
        break;

    case EvaluationResult::kTY_RunTimeValue:
        break;

    case EvaluationResult::kTY_StructName:
        // Cannot convert naked typename; coder has probably forgotten '.'
        UserError("Expected '.' after '%s'", _sym.GetName(eres.Symbol).c_str());
    }

    switch (eres.Location)
    {
    default:
        return InternalError("Cannot move expression result location to AX");

    case EvaluationResult::kLOC_AX:
        return; // Already done

    case EvaluationResult::kLOC_MemoryAtMAR:
        _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
        if (kKW_String == _sym.VartypeWithout(VTT::kConst, eres.Vartype))
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
        else
            WriteCmd(
                _sym.IsDynVartype(vartype) ? SCMD_MEMREADPTR : GetReadCommandForSize(_sym.GetSize(vartype)),
                SREG_AX);
        _reg_track.SetRegister(SREG_AX);
        eres.Location = EvaluationResult::kLOC_AX;
        return;
    }
}

void AGS::Parser::StripOutermostParens(SrcList &expression)
{
    // Note: If 'expression' starts out with '(' and ends with ')' then we can't
    // conclude that those parens belong together. Counter-example: '(a+b) * (a-b)'
    while (true)
    {
        expression.StartRead();
        if (kKW_OpenParenthesis != expression.GetNext())
            return; // done
        expression.SkipToCloser();
        SkipNextSymbol(expression, kKW_CloseParenthesis);
        if (!expression.ReachedEOF())
            return; // done

        expression.EatFirstSymbol();
        expression.EatLastSymbol();
    }
}

void AGS::Parser::ParseExpression_New_CtorFuncCall(Symbol argument_vartype, SrcList &expression)
{
    // No parameter list after "new T" (old-style)
    if (kKW_OpenParenthesis != expression.PeekNext())
    {
        if (!_sym.IsStructVartype(argument_vartype))
            return ParseExpression_CheckUsedUp(expression); // not a struct type

        Symbol const ctor_function =
            _sym.FindConstructorOfTypeOrParent(argument_vartype);
        if (kKW_NoSymbol == ctor_function)
            return ParseExpression_CheckUsedUp(expression); // no suitable constructor declared

        UserError(
            ReferenceMsgSym(
                "Expected the parameter list for function '%s'",
                ctor_function).c_str(),
            _sym.GetName(ctor_function).c_str()
        );
    }

    // Parameter list after "new T" (new-style)
    do // exactly 1 times
    {
        if (!_sym.IsStructVartype(argument_vartype))
            break; // not a struct

        Symbol const ctor_function =
            _sym.FindConstructorOfTypeOrParent(argument_vartype);
        if (kKW_NoSymbol == ctor_function)
            break; // no suitable constructor declared

        PushReg(SREG_AX);
        // Address of the new object is expected in MAR
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        _reg_track.SetRegister(SREG_MAR);
        EvaluationResult eres_dummy;

        AccessData_FunctionCall(ctor_function, expression, eres_dummy);
        ParseExpression_CheckUsedUp(expression);
        PopReg(SREG_AX);
        return;
    } while (false);

    // Here when there isn't any init function: must have '()' following
    SkipNextSymbol(expression, kKW_OpenParenthesis);
    Expect(kKW_CloseParenthesis, expression.GetNext());
    ParseExpression_CheckUsedUp(expression);
}

void AGS::Parser::ParseExpression_New(SrcList &expression, EvaluationResult &eres)
{
    expression.StartRead();
    if (expression.ReachedEOF())
        UserError("Expected a type after 'new' but didn't find any");
    Vartype const argument_vartype = expression.GetNext();

    if (!_sym.IsVartype(argument_vartype))
        UserError(
            "Expected a type after 'new', found '%s' instead",
            _sym.GetName(argument_vartype).c_str());
    
    bool const is_managed = _sym.IsManagedVartype(argument_vartype);
    bool const with_bracket_expr = kKW_OpenBracket == expression.PeekNext(); // "new FOO[BAR]"

    Vartype element_vartype = kKW_NoSymbol;
    if (with_bracket_expr)
    {
        // Note that in AGS, you can write "new Struct[...]" but what you mean then is "new Struct*[...]".
        EatDynpointerSymbolIfPresent(_src, argument_vartype);
        
        // Check for '[' with a handcrafted error message so that the user isn't led to 
        // fix their code by defining a dynamic array when this would be the wrong thing to do
        Symbol const open_bracket = expression.GetNext();
        if (kKW_OpenBracket != open_bracket)
            UserError("Unexpected '%s'", _sym.GetName(open_bracket).c_str());

        EvaluationResult bracketed_eres;
        ParseIntegerExpression(expression, bracketed_eres);
        EvaluationResultToAx(bracketed_eres);
        Expect(kKW_CloseBracket, expression.GetNext());

        element_vartype = is_managed ? _sym.VartypeWithDynpointer(argument_vartype) : argument_vartype;
        eres.Vartype = _sym.VartypeWithDynarray(element_vartype);

        while (kKW_OpenBracket == expression.PeekNext())
        {
            SkipNextSymbol(expression, kKW_OpenBracket);
            Expect(kKW_CloseBracket, expression.GetNext());
            element_vartype = eres.Vartype;
            eres.Vartype = _sym.VartypeWithDynarray(element_vartype);
        }
    }
    else // no bracket expression
    {
        if (_sym[argument_vartype].VartypeD->Flags[VTF::kUndefined])
            UserError(
                ReferenceMsgSym("The struct '%s' hasn't been completely defined yet", argument_vartype).c_str(),
                _sym.GetName(argument_vartype).c_str());
        
        if (_sym.IsBuiltinVartype(argument_vartype))  
            UserError("Expected '[' after the built-in type '%s'", _sym.GetName(argument_vartype).c_str());
        if (!is_managed)
            UserError("Expected '[' after the non-managed type '%s'", _sym.GetName(argument_vartype).c_str());

        // Only do this check for new, not for new[]. 
        if (0u == _sym.GetSize(argument_vartype))
            UserError(
                ReferenceMsgSym(
                    "Struct '%s' doesn't contain any variables, cannot use 'new' with it",
                    argument_vartype).c_str(),
                _sym.GetName(argument_vartype).c_str());

        element_vartype = argument_vartype;
        eres.Vartype = _sym.VartypeWithDynpointer(argument_vartype);
    }

    size_t const element_size = _sym.GetSize(element_vartype);
    if (0u == element_size)
        // The Engine really doesn't like that (division by zero error)
        InternalError("Trying to emit allocation of zero dynamic memory");

    // Choose between "old" and new "new" opcode, depending on RTTI presence
    if (FlagIsSet(_options, SCOPT_RTTIOPS))
    {
        element_vartype = _sym.GetBaseVartypeIfDynptr(element_vartype);
        if (with_bracket_expr)
            WriteCmd(SCMD_NEWARRAY2, SREG_AX, element_vartype, element_size);
        else
            WriteCmd(SCMD_NEWUSEROBJECT2, SREG_AX, element_vartype, element_size);
    }
    else
    {
        if (with_bracket_expr)
            WriteCmd(SCMD_NEWARRAY, SREG_AX, element_size, is_managed);
        else
            WriteCmd(SCMD_NEWUSEROBJECT, SREG_AX, element_size);
    }

    _reg_track.SetRegister(SREG_AX);

    if (!with_bracket_expr)
        ParseExpression_New_CtorFuncCall(argument_vartype, expression);

    ParseExpression_CheckUsedUp(expression);

    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_AX;
    eres.Symbol = kKW_NoSymbol;
    // Vartype has already been set
}

void AGS::Parser::ParseExpression_PrefixMinus(EvaluationResult &eres)
{
    if (eres.kTY_Literal == eres.Type)
    {
        // Do the operation right now
        EvaluationResult eres_lhs;
        eres_lhs.Type = eres.kTY_Literal;
        eres_lhs.Location = eres.kLOC_SymbolTable;
        eres_lhs.Vartype = _sym[eres.Symbol].LiteralD->Vartype;
        eres_lhs.Symbol = kKW_Float == eres_lhs.Vartype ? _sym.Find("0.0") : _sym.Find("0");
        if (ParseExpression_CompileTime(kKW_Minus, eres_lhs, eres, eres))
            return;
    }

    EvaluationResultToAx(eres);

    CodeCell const opcode = GetOpcode(kKW_Minus, eres.Vartype, eres.Vartype);
    
    // Calculate 0 - AX
    // The binary representation of 0.0 is identical to the binary representation of 0
    // so this will work for floats as well as for ints.
    WriteCmd(SCMD_LITTOREG, SREG_BX, 0);
    WriteCmd(opcode, SREG_BX, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    _reg_track.SetRegister(SREG_BX);
    _reg_track.SetRegister(SREG_AX);
    eres.Symbol = kKW_NoSymbol;
    eres.Modifiable = false;
}

void AGS::Parser::ParseExpression_PrefixPlus(EvaluationResult &eres)
{
    
    if (_sym.IsAnyIntegerVartype(eres.Vartype) || kKW_Float == eres.Vartype)
        return;

    UserError("Cannot apply unary '+' to an expression of type '%s'", _sym.GetName(eres.Vartype).c_str());
}

void AGS::Parser::ParseExpression_PrefixNegate(Symbol op_sym, EvaluationResult &eres)
{
    bool const bitwise_negation = kKW_BitNeg == op_sym;

    std::string msg = "Argument of '<op>'";
    string_replace(msg, "<op>", _sym.GetName(op_sym));
    CheckVartypeMismatch(eres.Vartype, kKW_Int, true, msg);
    
    if (eres.kTY_Literal == eres.Type)
    {
        // Try to do the negation now
        EvaluationResult eres_lhs;
        eres_lhs.Type = eres.kTY_Literal;
        eres_lhs.Location = eres.kLOC_SymbolTable;
        eres_lhs.Symbol = _sym.Find("0");
        if (ParseExpression_CompileTime(op_sym, eres_lhs, eres, eres))
            return;
    }

    EvaluationResultToAx(eres);
    
    if (bitwise_negation)
    {
        // There isn't any opcode for this, so calculate -1 - AX
        WriteCmd(SCMD_LITTOREG, SREG_BX, -1);
        WriteCmd(SCMD_SUBREG, SREG_BX, SREG_AX);
        WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
        _reg_track.SetRegister(SREG_BX);
        _reg_track.SetRegister(SREG_AX);
    }
    else
    {
        WriteCmd(SCMD_NOTREG, SREG_AX);
        _reg_track.SetRegister(SREG_AX);
    }

    eres.Vartype = kKW_Int;
    eres.Location = EvaluationResult::kLOC_AX;
}

void AGS::Parser::ParseExpression_PrefixCrement(Symbol op_sym, AGS::SrcList &expression, EvaluationResult &eres)
{
    SrcList inner = SrcList(expression);

    // Strip any enclosing '()' around the term
    // so that you can do '++(foo)' as well as '++foo' 
    if (kKW_OpenParenthesis == inner[0u])
    {
        StripOutermostParens(inner);
        if (kKW_OpenParenthesis == inner[0u])
        {
            // There must be spurious trailing symbols after the closing paren.
            inner.StartRead();
            inner.SkipToCloser();
            SkipNextSymbol(inner, kKW_CloseParenthesis);
            ParseExpression_CheckUsedUp(inner);
        }
    }

    inner.StartRead();

    ParseAssignment_ReadLHSForModification(inner, eres);
    
    std::string msg = "Argument of '<op>'";
    string_replace(msg, "<op>", _sym.GetName(op_sym).c_str());
    CheckVartypeMismatch(eres.Vartype, kKW_Int, true, msg); 
    
    WriteCmd((kKW_Increment == op_sym) ? SCMD_ADD : SCMD_SUB, SREG_AX, 1);
    _reg_track.SetRegister(SREG_AX);

    // Really do the assignment the long way so that all the checks and safeguards will run.
    // If a shortcut is possible then undo this and generate the shortcut instead.
    RestorePoint before_long_way_modification = RestorePoint(_scrip);
    AccessData_AssignTo(inner, eres);
    
    if (EvaluationResult::kLOC_MemoryAtMAR == eres.Location)
    {
        before_long_way_modification.Restore();
        CodeCell memwrite = GetWriteCommandForSize(_sym.GetSize(eres.Vartype));
        WriteCmd(memwrite, SREG_AX);
        _reg_track.SetRegister(SREG_AX);
    }
    eres.SideEffects = true;
    ParseExpression_CheckUsedUp(inner);
}


void AGS::Parser::ParseExpression_Prefix(SrcList &expression, EvaluationResult &eres)
{
    // The least binding operator is the first thing in the expression
    // This means that the op must be an unary op.
    Symbol const op_sym = expression[0u];

    if (expression.Length() < 2u)
        UserError(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(op_sym).c_str());

    SrcList inner = SrcList(expression);
    inner.EatFirstSymbol();

    if (kKW_New == op_sym)
        return ParseExpression_New(inner, eres);

    if (kKW_Decrement == op_sym || kKW_Increment == op_sym)
        return ParseExpression_PrefixCrement(op_sym, inner, eres);
        
    // Special case: Lowest integer literal, written in decimal notation.
    // We treat this here in the parser because the scanner doesn't know
    // whether a minus symbol stands for a unary minus.
    if (op_sym == kKW_Minus &&
        inner.Length() == 1u &&
        inner[0u] == kKW_OnePastLongMax)
    {
        eres.Type = eres.kTY_Literal;
        eres.Location = eres.kLOC_SymbolTable;
        eres.Symbol = _sym.Find("-2147483648");
        eres.Vartype = kKW_Int;
        ParseExpression_CheckUsedUp(inner);
        return;
    }

    ParseExpression_Term(inner, eres);
    
    switch (op_sym)
    {
    case kKW_BitNeg:
    case kKW_Not:
        return ParseExpression_PrefixNegate(op_sym, eres);

    case kKW_Minus:
        return ParseExpression_PrefixMinus( eres);

    case kKW_Plus:
        return ParseExpression_PrefixPlus(eres);
    }

    InternalError("Illegal prefix op '%s'", _sym.GetName(op_sym).c_str());
}

void AGS::Parser::ParseExpression_PostfixCrement(Symbol const op_sym, SrcList &expression, EvaluationResult &eres)
{
    bool const op_is_inc = kKW_Increment == op_sym;

    SrcList inner = SrcList(expression);

    // Strip any enclosing '()' around the term
    // so that you can do '(foo)++' as well as 'foo++' 
    if (kKW_OpenParenthesis == inner[0u])
    {
        StripOutermostParens(inner);
        if (kKW_OpenParenthesis == inner[0u])
        {
            // There must be spurious trailing symbols after the closing paren.
            inner.StartRead();
            inner.SkipToCloser();
            SkipNextSymbol(inner, kKW_CloseParenthesis);
            ParseExpression_CheckUsedUp(inner);
        }
    }

    inner.StartRead();
    ParseAssignment_ReadLHSForModification(inner, eres);
    ParseExpression_CheckUsedUp(inner);
    
    std::string msg = "Argument of '<op>'";
    string_replace(msg, "<op>", _sym.GetName(op_sym).c_str());
    CheckVartypeMismatch(eres.Vartype, kKW_Int, true, msg);
    
    // Really do the assignment the long way so that all the checks and safeguards will run.
    // If a shortcut is possible then undo this and generate the shortcut instead.
    RestorePoint before_long_way_modification{ _scrip };

    PushReg(SREG_AX);
    WriteCmd((op_is_inc ? SCMD_ADD : SCMD_SUB), SREG_AX, 1);
    AccessData_AssignTo(inner, eres);
    PopReg(SREG_AX);

    if (EvaluationResult::kLOC_MemoryAtMAR == eres.Location)
    {   // We know the memory where the var resides. Do modify this memory directly.
        before_long_way_modification.Restore();
        WriteCmd((op_is_inc ? SCMD_ADD : SCMD_SUB), SREG_AX, 1);
        CodeCell memwrite = GetWriteCommandForSize(_sym.GetSize(eres.Vartype));
        WriteCmd(memwrite, SREG_AX);
        WriteCmd((!op_is_inc ? SCMD_ADD : SCMD_SUB), SREG_AX, 1);
        _reg_track.SetRegister(SREG_AX);
    }
    eres.Location = EvaluationResult::kLOC_AX;
    eres.SideEffects = true;
}

void AGS::Parser::ParseExpression_Postfix(SrcList &expression, EvaluationResult &eres, bool const result_used)
{
    size_t const expr_len = expression.Length();

    if (0u == expr_len)
        InternalError("Empty expression");

    Symbol const op_sym = expression[expr_len - 1u];
    if (1u == expr_len)
        UserError("'%s' must either precede or follow some term to be modified", _sym.GetName(op_sym).c_str());

    SrcList inner = SrcList(expression);
    inner.EatLastSymbol();

    if (op_sym != kKW_Decrement && op_sym != kKW_Increment)
        UserError("Expected a term following the '%s', didn't find it", _sym.GetName(op_sym).c_str());

    // If the result isn't used then take the more efficient version of increment / decrement
    if (!result_used)
        return ParseExpression_PrefixCrement(op_sym, inner, eres);

    ParseExpression_PostfixCrement(op_sym, inner, eres);
}

void AGS::Parser::ParseExpression_Ternary_Term2(EvaluationResult &eres_term1, bool term1_has_been_ripped_out, SrcList &term2, EvaluationResult &eres, bool const result_used)
{
    bool const second_term_exists = (term2.Length() > 0);
    if (second_term_exists)
    {
        ParseExpression_Term(term2, eres, result_used);

        EvaluationResult eres_dummy = eres;
        EvaluationResultToAx(eres_dummy); // don't clobber eres
    }
    else
    {
        // Take the first expression as the result of the missing second expression
        eres = eres_term1;
        if (term1_has_been_ripped_out)
        {   // Still needs to be moved to AX
            EvaluationResult eres_dummy = eres;
            EvaluationResultToAx(eres_dummy); // don't clobber eres
        }
    }

    // If term2 and term3 evaluate to a 'string', we might in theory make this
    //  a 'string' expression. However, at this point we don't know yet whether
    // term3 will evaluate to a 'string', and we need to generate code here,
    // so to be on the safe side, convert any 'string' into 'String'.
    // Note that the result of term2 has already been moved to AX
    ConvertAXStringToStringObject(_sym.GetStringStructPtrSym(), eres.Vartype);
}

void AGS::Parser::ParseExpression_Ternary(size_t const tern_idx, SrcList &expression, bool const result_used, EvaluationResult &eres)
{
    // First term ends before the '?'
    SrcList term1 = SrcList(expression, 0u, tern_idx);

    // Second term begins after the '?', we don't know how long it is yet
    SrcList after_term1 = SrcList(expression, tern_idx + 1u, expression.Length() - (tern_idx + 1u));

    // Find beginning of third term
    after_term1.StartRead();
    after_term1.SkipTo(kKW_Colon);
    if (after_term1.ReachedEOF() || kKW_Colon != after_term1.PeekNext())
    {
        expression.SetCursor(tern_idx);
        UserError("Didn't find the matching ':' to '?'");
    }
    size_t const term3_start = after_term1.GetCursor() + 1u;
    SrcList term3 = SrcList(after_term1, term3_start, after_term1.Length() - term3_start);
    SrcList term2 = SrcList(after_term1, 0u, after_term1.GetCursor());
    if (!term3.Length())
    {
        expression.SetCursor(tern_idx);
        UserError("The third expression of this ternary is empty");
    }

    bool const second_term_exists = (term2.Length() > 0);

    EvaluationResult eres_term1, eres_term2, eres_term3;

    // Note: Cannot use the same jump-collector for the end of term1
    // and the end of term2 although both jump to the same destination,
    // i.e., out of the ternary. Reason is, term2 might be ripped out 
    // of the codebase. In this case its jump out of the ternary would 
    // be ripped out too, and when afterwards the jumps are patched,
    // a wrong, random location would get patched.
    ForwardJump jumpdest_out_of_ternary(_scrip);
    ForwardJump jumpdest_after_term2(_scrip);
    ForwardJump jumpdest_to_term3(_scrip);

    RestorePoint start_of_term1(_scrip);


    // First term of ternary (i.e, the test of the ternary)
    // We basically let through anything that can be compared to
    // some kind of zero or null so that 'a ?: whatever' can be evaluated
    ParseExpression_Term(term1, eres_term1);
    
    bool const term1_known = eres.kTY_Literal == eres_term1.Type;
    CodeCell const term1_value = term1_known ? _sym[eres_term1.Symbol].LiteralD->Value : false;
    // Put the result into AX, but don't clobber 'vloc_term1'
    // So we can use results at compile time that are known at compile time
    // and also be sure that all the other results can be found in AX.
    EvaluationResult eres_dummy = eres_term1;
    EvaluationResultToAx(eres_dummy);
   
    bool term1_has_been_ripped_out = false;
    if (term1_known)
    {   // Don't need to do the test at runtime
        start_of_term1.Restore();
        term1_has_been_ripped_out = true;
    }
    else 
    {
        // Note, here we generate a conditional jump and add its destination to a
        // ForwardJump object. Once we have done that, we mustn't rip out this
        // conditional jump afterwards because when we rip the code out, the jump
        // destination still stays in the ForwardJump object and will be patched when
        // the ForwardJump object is resolved although the corresponding code isn't
        // there any longer.
        // This means that it is wrong to generate this conditional jump as
        // part of the code of the first term of the ternary and then decide whether
        // this code should be kept or ripped out.
        WriteCmd(
            second_term_exists ? SCMD_JZ : SCMD_JNZ,
            kDestinationPlaceholder);
        if (second_term_exists)
            jumpdest_to_term3.AddParam();
        else
            jumpdest_out_of_ternary.AddParam();
    }

    // Second term of the ternary
    RestorePoint start_of_term2(_scrip);
    ParseExpression_Ternary_Term2(
        eres_term1, term1_has_been_ripped_out,
        term2, eres_term2,
        result_used);
    
    // Needs to be here so that the jump after term2 is ripped out whenever
    // term3 is ripped out so there isn't any term that would need to be jumped over.
    RestorePoint start_of_term3 (_scrip);
    if (second_term_exists)
    {
        WriteCmd(SCMD_JMP, kDestinationPlaceholder);
        jumpdest_after_term2.AddParam();
    }

    bool term2_has_been_ripped_out = false;
    if (term1_known && !term1_value)
    {
        start_of_term2.Restore(); // Don't need term2, it will never be evaluated
        term2_has_been_ripped_out = true;
    }

    // Third term of ternary
    // All jumps to the start of the third term should go to _here_.
    jumpdest_to_term3.Patch(_src.GetLineno());

    ParseExpression_Term(term3, eres_term3, result_used);
    // Put the result into AX, but don't clobber 'vloc_term3'
    // So we can use results at compile time that are known at compile time
    // and also be sure that all the other results can be found in AX.
    eres_dummy = eres_term3;
    EvaluationResultToAx(eres_dummy);
    ConvertAXStringToStringObject(_sym.GetStringStructPtrSym(), eres_term3.Vartype);

    bool term3_has_been_ripped_out = false;
    if (term1_known && term1_value)
    {
        // Don't need term3, will never be evaluated
        start_of_term3.Restore(); 
        term3_has_been_ripped_out = true;
    }

    if (!term2_has_been_ripped_out && !term3_has_been_ripped_out)
        jumpdest_after_term2.Patch(_src.GetLineno());
    jumpdest_out_of_ternary.Patch(_src.GetLineno());

    eres.LocalNonParameter = 
        eres_term2.LocalNonParameter || eres_term3.LocalNonParameter;

    eres.Vartype = eres_term2.Vartype;
    if (IsVartypeMismatch_Oneway(eres_term3.Vartype, eres_term2.Vartype))
    {
        if (IsVartypeMismatch_Oneway(eres_term2.Vartype, eres_term3.Vartype))
        {
            expression.SetCursor(tern_idx);
            UserError("An expression of type '%s' is incompatible with an expression of type '%s'",
                _sym.GetName(eres_term2.Vartype).c_str(), _sym.GetName(eres_term3.Vartype).c_str());
        }
        eres.Vartype = eres_term3.Vartype;
    }

    if (term1_known)
    {
        if (term1_value && EvaluationResult::kTY_Literal == eres_term2.Type)
        {
            start_of_term1.Restore(); // Don't need the ternary at all
            eres = eres_term2;
            return;
        }
        if (!term1_value && EvaluationResult::kTY_Literal == eres_term3.Type)
        {
            start_of_term1.Restore(); // Don't need the ternary at all
            eres = eres_term3;
            return;
        }
    }

    // Each branch has been putting the result into AX so that's where it's now
    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = EvaluationResult::kLOC_AX;
    eres.Modifiable = false;
}

void AGS::Parser::ParseExpression_As(SrcList &vartype_list, EvaluationResult &eres)
{
    vartype_list.StartRead();
    Vartype new_vartype = ParseVartype(vartype_list);
    ParseDynArrayMarkersIfPresent(vartype_list, new_vartype);
    ParseExpression_CheckUsedUp(vartype_list);

    Vartype const curr_vartype = eres.Vartype;

    if (_sym.IsDynpointerVartype(curr_vartype))
    {
        if (!_sym.IsDynpointerVartype(new_vartype))
            new_vartype = _sym.VartypeWithDynpointer(new_vartype);
    }

    // Handle cases where 'curr_vartype' is convertable to 'new_vartype'
    // even without any 'as' clause.
    if (!IsVartypeMismatch_Oneway(curr_vartype, new_vartype))
    {
        eres.Vartype = new_vartype;
        return;
    }

    if (_sym.IsDynarrayVartype(curr_vartype))
        // Here, when we try to convert a (managed) dynarray type
        // 'A*[]' to type 'B*[]' where 'B' is not an ancester of 'A':
        // If we'd do this, then the compiler would "forget"
        // that the array used to have the type 'A*[]' originally.
        // At this point in time, this array might already contain
        // non-null elements. We will never run checks whether these
        // actually _are_ of type 'B*'.
        // This is unsafe. So do NOT allow this.
        UserError(    
            ReferenceMsgSym(
                "Cannot convert the dynamic array '%s' to type '%s'",
                curr_vartype).c_str(),
            _sym.GetName(curr_vartype).c_str(),
            _sym.GetName(new_vartype).c_str());

    if (_sym.IsAtomicVartype(curr_vartype))
        // Here, we might implement an int-to-float or float-to-int
        // conversion in future. There are several possibilities
        // for the float-to-int conversion (to nearest, lower,
        // upper int, towards zero, away from zero), so which one?
        UserError(
            "Cannot convert type '%s' to type '%s' through an 'as' clause",
            _sym.GetName(curr_vartype).c_str(),
            _sym.GetName(new_vartype).c_str());

    if (!_sym.IsDynpointerVartype(curr_vartype))
        InternalError(
            "Expected to convert a dynpointer, found '%s'",
            _sym.GetName(curr_vartype).c_str());

    Vartype const element_vartype = _sym.GetFirstBaseVartype(new_vartype);
    if (!_sym.IsManagedVartype(element_vartype))
        UserError(
            ReferenceMsgSym(
                "Cannot convert to the non-managed type '%s'",
                new_vartype).c_str(),
            _sym.GetName(new_vartype).c_str());

    if (FlagIsSet(_options, SCOPT_RTTIOPS))
    {
        EvaluationResultToAx(eres);
        WriteCmd(SCMD_DYNAMICCAST, element_vartype);
    }
    else
    {
        if (IsVartypeMismatch_Oneway(new_vartype, curr_vartype))
            UserError(
                "Cannot convert type '%s', it isn't type '%s' or an ancester of it (RTTI is disabled)",
                _sym.GetName(curr_vartype).c_str(),
                _sym.GetName(new_vartype).c_str());
    }
    eres.Vartype = new_vartype;
}

void AGS::Parser::ParseExpression_Binary(size_t const op_idx, SrcList &expression, EvaluationResult &eres)
{
    RestorePoint start_of_term(_scrip);
    Symbol const operator_sym = expression[op_idx];

    // Process the left hand side
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    SrcList lhs = SrcList(expression, 0u, op_idx);
    ParseExpression_Term(lhs, eres);

    EvaluationResult eres_lhs = eres;
    EvaluationResultToAx(eres);
   
    ForwardJump to_exit(_scrip);

    bool lazy_evaluation = false;
    if (kKW_And == operator_sym)
    {
        // if AX is 0 then the AND has failed, so just jump directly to the end of the term
        // AX will still be 0 so that will do as the result of the calculation
        lazy_evaluation = true;
        expression.SetCursor(op_idx + 1u);
        WriteCmd(SCMD_JZ, kDestinationPlaceholder);
        to_exit.AddParam();
    }
    else if (kKW_Or == operator_sym)
    {
        // If AX is non-zero then the OR has succeeded, so just jump directly to the end of the term; 
        // AX will still be non-zero so that will do as the result of the calculation
        lazy_evaluation = true;
        expression.SetCursor(op_idx + 1u);
        WriteCmd(SCMD_JNZ, kDestinationPlaceholder);
        to_exit.AddParam();
    }
    else if (kKW_Increment == operator_sym || kKW_Decrement == operator_sym)
    {
        UserError("Cannot use '%s' as a binary operator", _sym.GetName(operator_sym).c_str());
    }
    else if (kKW_As == operator_sym)
    {
        SrcList rhs = SrcList(expression, op_idx + 1u, expression.Length());
        return ParseExpression_As( rhs, eres);
    }
    else
    {
        // Hang on to the intermediate result
        PushReg(SREG_AX);
    }

    SrcList rhs = SrcList(expression, op_idx + 1u, expression.Length());
    if (0u == rhs.Length())
    {
        // there is no right hand side for the expression
        expression.SetCursor(op_idx + 1u);
        UserError("Binary operator '%s' doesn't have a right hand side", _sym.GetName(operator_sym).c_str());
    }

    ParseExpression_Term(rhs, eres);
    size_t const expression_end_idx = expression.GetCursor();

    EvaluationResult eres_rhs = eres;
    EvaluationResultToAx(eres);

    if (!lazy_evaluation)
    {
        expression.SetCursor(op_idx + 1u);
        PopReg(SREG_BX); // Note, we pop to BX although we have pushed AX
        _reg_track.SetRegister(SREG_BX);
        // now the result of the left side is in BX, of the right side is in AX
        CodeCell const opcode = GetOpcode(operator_sym, eres_lhs.Vartype, eres_rhs.Vartype);
        WriteCmd(opcode, SREG_BX, SREG_AX);
        WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_AX);
    }

    _reg_track.SetRegister(SREG_AX);
    eres.Location = EvaluationResult::kLOC_AX;
    expression.SetCursor(expression_end_idx);
    to_exit.Patch(_src.GetLineno());

    if (_sym.IsBooleanOperator(operator_sym))
        eres.Vartype = kKW_Int;

    if (eres.kTY_Literal != eres_lhs.Type || eres.kTY_Literal != eres_rhs.Type)
        return;

    // Attempt to do this at compile-time

    if (kKW_And == operator_sym || kKW_Or == operator_sym)
    {
        bool const condition = (0 != _sym[eres_lhs.Symbol].LiteralD->Value);
        if (kKW_And == operator_sym)
            eres = condition ? eres_rhs : eres_lhs;
        else // kKW_Or
            eres = condition ? eres_lhs : eres_rhs;

        if (!_sym.IsAnyIntegerVartype(_sym[eres.Symbol].LiteralD->Vartype))
        {   // Swap an int literal in (note: Don't change the vartype of the pre-existing literal)
            bool const result = (0 != _sym[eres.Symbol].LiteralD->Value);
            eres.Symbol = result ? _sym.Find("1") : _sym.Find("0");
        }

        start_of_term.Restore();
        return;
    }
 
    if (ParseExpression_CompileTime(operator_sym, eres_lhs, eres_rhs, eres))
        start_of_term.Restore();
}

void AGS::Parser::ParseExpression_CheckUsedUp(SrcList &expression)
{
    if (expression.ReachedEOF())
        return;

    // e.g. "4 3" or "(5) 3".

    // Some calls don't promise to use up all its expression, so we need a check
    // whether some spurious symbols follow. These spurious symbols cannot be operators
    // or else the operators would have been caught in the term segmentation.
    // So this happens when several identifiers or literals follow each other.
    // The best guess for this case is that an operator is missing in between.

    UserError(
        "Expected an operator, found '%s' instead",
        _sym.GetName(expression.GetNext()).c_str());
}

void AGS::Parser::ParseExpression_InParens(SrcList &expression, EvaluationResult &eres, bool const result_used)
{
    // Check for spurious symbols after the closing paren.
    expression.SetCursor(1u);
    expression.SkipToCloser();
    SkipNextSymbol(expression, kKW_CloseParenthesis);
    ParseExpression_CheckUsedUp(expression);

    SrcList inner(expression);
    StripOutermostParens(inner);
    return ParseExpression_Term(inner, eres, result_used);
}

std::string const AGS::Parser::ReferenceMsgLoc(std::string const &msg, size_t declared)
{
    if (SymbolTable::kNoSrcLocation == declared)
        return msg;

    int const section_id = _src.GetSectionIdAt(declared);
    std::string const &section = _src.SectionId2Section(section_id);

    int const line = _src.GetLinenoAt(declared);
    if (line <= 0 || (!section.empty() && '_' == section[0u]))
        return msg;

    std::string tpl;
    if (_src.GetSectionId() != section_id)
        tpl = ". See <1> line <2>";
    else if (_src.GetLineno() != line)
        tpl = ". See line <2>";
    else
        tpl = ". See the current line";
    size_t const loc1 = tpl.find("<1>");
    if (std::string::npos != loc1)
        string_replace(tpl, "<1>", section);
    size_t const loc2 = tpl.find("<2>");
    if (std::string::npos != loc2)
        string_replace(tpl, "<2>", std::to_string(line));
    return msg + tpl;
}

std::string const AGS::Parser::ReferenceMsgSym(std::string const &msg,Symbol symb)
{
    return ReferenceMsgLoc(msg, _sym.GetDeclared(symb));
}

void AGS::Parser::AccessData_FunctionCall_EmitCall(Symbol const name_of_func, size_t const args_count, bool const func_is_import)
{
    if (func_is_import)
    {
        // tell it how many args for this call (nested imported functions cause stack problems otherwise)
        WriteCmd(SCMD_NUMFUNCARGS, args_count);
    }

    // Load function address into AX
    WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[name_of_func].FunctionD->Offset);
    _reg_track.SetRegister(SREG_AX);

    if (func_is_import)
    {   
        _scrip.FixupPrevious(kFx_Import);
        if (!_scrip.IsImport(_sym.GetName(name_of_func)))
        {
            // We don't know the import number of this function yet, so put a label here
            // and keep track of the location in order to patch in the proper import number later on
            _scrip.code.back() = _importLabels.Function2Label(name_of_func);
            _importLabels.TrackLabelLoc(_scrip.Codesize_i32() - 1);
        }

        WriteCmd(SCMD_CALLEXT, SREG_AX); // Do the call
        _reg_track.SetAllRegisters();
        // At runtime, we will arrive here when the function call has returned: Restore the stack
        if (args_count > 0u)
            WriteCmd(SCMD_SUBREALSTACK, args_count);
        return;
    }

    // Func is non-import
    _scrip.FixupPrevious(kFx_Code);
    if (_sym[name_of_func].FunctionD->Offset < 0)
    {
        // We don't know yet at which address the function is going to start, so put a label here
        // and keep track of the location in order to patch in the correct address later on
        _scrip.code.back() = _callpointLabels.Function2Label(name_of_func);
        _callpointLabels.TrackLabelLoc(_scrip.Codesize_i32() - 1);
    }
    WriteCmd(SCMD_CALL, SREG_AX);  // Do the call
    _reg_track.SetAllRegisters();

    // At runtime, we will arrive here when the function call has returned: Restore the stack
    if (args_count > 0u)
    {
        size_t const size_of_passed_params = args_count * SIZE_OF_STACK_CELL;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_passed_params);
        _scrip.OffsetToLocalVarBlock -= size_of_passed_params;
    }
}

void AGS::Parser::AccessData_GenerateDynarrayLengthAttrib(EvaluationResult &eres)
{
    // eres.Vartype should be a "dynamic array of T"
    Symbol const array_struct = eres.Vartype;
    std::string const &struct_name = _sym[array_struct].Name;
    std::string const unqualified_attrib_name = "Length"; // fixme, use global constant
    std::string const qualified_attrib_name = struct_name + "::" + unqualified_attrib_name;
    if (_sym.Find(qualified_attrib_name))
        return; // pseudo-attribute exists

    // Get or generate the universal function that would substitute getters
    // for each dynamic array of T's Length attribute
    Symbol const dynarray_len_func = _sym.FindOrAdd(_builtinDynArrayLength);
    if (!_sym.IsFunction(dynarray_len_func))
    {
        TypeQualifierSet tqs;
        tqs[TQ::kImport] = true;
        Symbol const no_struct = kKW_NoSymbol;
        bool const body_follows = false;
        ParseFuncdecl_MasterData2Sym(tqs, kKW_Int, no_struct, dynarray_len_func, body_follows);
        _sym[dynarray_len_func].FunctionD->Parameters.push_back({});
        _sym[dynarray_len_func].FunctionD->Parameters[1u].Vartype = eres.Vartype;
        _sym[dynarray_len_func].FunctionD->Offset = _scrip.FindOrAddImport(_sym.GetName(dynarray_len_func));
        _sym.SetDeclared(dynarray_len_func, _src.GetCursor());
    }

    // Generate pseudo-attribute beloging to this type
    Symbol const sym_attrib = _sym.FindOrAdd(unqualified_attrib_name);
    TypeQualifierSet tqs;
    tqs[TQ::kImport] = true;
    tqs[TQ::kReadonly] = true;
    // Cheat and replace getter's symbol with our universal array length getter
    ParseStruct_MasterAttribute2SymbolTable(tqs, kKW_Int, array_struct, sym_attrib, false, dynarray_len_func);
}

                                                        
void AGS::Parser::AccessData_FunctionCall_Arguments_Named(Symbol const name_of_func, std::vector<FuncParameterDesc> const &param_desc, SrcList &arg_srcs, std::vector<SrcList> &arg_exprs)
{
    if (_sym[name_of_func].FunctionD->ParamNamingInconsistency.Exists)
    {
        // We only track ONE name per parameter. Whenever a function has
        // two declarations where a parameter has differing names,
        // it is no longer possible to call that function using named arguments.
        auto const &inconsistency = _sym[name_of_func].FunctionD->ParamNamingInconsistency;
        UserError(
            ReferenceMsgLoc(
                ReferenceMsgLoc(
                    "Call to function '%s': "
                    "Cannot use named arguments because parameter #%u "
                    "is named inconsistently in declarations: '%s' and '%s'",
                    inconsistency.Declared2).c_str(),
                inconsistency.Declared1).c_str(),
            _sym.GetName(name_of_func).c_str(),
            inconsistency.ParamIdx,
            _sym.GetName(inconsistency.Name1).c_str(),
            _sym.GetName(inconsistency.Name2).c_str());
    }

    SrcList const empty_arg = SrcList{ arg_srcs, 0u, 0u, };
    arg_exprs.assign(param_desc.size(), empty_arg);

    arg_srcs.StartRead();

    // For error messages
    std::string ctf = "Call to function '<func>': ";
    string_replace(ctf, "<func>", _sym.GetName(name_of_func));

    while (true)
    {
        Symbol const divider = arg_srcs.GetNext();
        if (kKW_CloseParenthesis == divider)
            return;

        // Get parameter name
        Symbol const param_name = arg_srcs.GetNext();

        if (!_sym.IsIdentifier(param_name))
            UserError(
                (ctf + "Expected a parameter name, found '%s' "
                    "(cannot mix positional and named arguments in one call)").c_str(),
                _sym.GetName(param_name).c_str());

        // Find the parameter by name
        size_t param_idx = 0u;
        for (size_t idx = 1u; idx < param_desc.size(); ++idx)
            if (param_desc[idx].Name == param_name)
            {
                param_idx = idx;
                break;
            }
        if (!param_idx)
            UserError(
                ReferenceMsgSym(
                    (ctf + "Function doesn't have a parameter named '%s'").c_str(),
                    name_of_func).c_str(),
                _sym.GetName(param_name).c_str());
        if (arg_exprs[param_idx].Length())
            UserError(
                (ctf + "An argument for parameter '%s' has already been specified").c_str(),
                _sym.GetName(param_name).c_str());

        Expect(kKW_Colon, arg_srcs.GetNext(),
            ctf + "Expected ':'");

        // Isolate the argument expression
        size_t const current_arg_start = arg_srcs.GetCursor();
        arg_srcs.SkipTo(SymbolList{ kKW_Comma, kKW_CloseParenthesis, });
        size_t const current_arg_end = arg_srcs.GetCursor();
        SrcList arg_expr(arg_srcs, current_arg_start, current_arg_end - current_arg_start);

        arg_exprs[param_idx] = arg_expr;
    }
}

void AGS::Parser::AccessData_FunctionCall_Arguments_Sequence(Symbol const name_of_func, std::vector<FuncParameterDesc> const &param_desc, SrcList &arg_list, std::vector<SrcList> &arg_exprs)
{
    // [0u] is unused (reserved for the return value)
    arg_exprs.push_back(SrcList{ arg_list, 0u, 0u });

    if (arg_list.Length() == 3u && kKW_Void == arg_list[1u]) // '(void)'
        return;

    arg_list.StartRead();

    for (size_t arg_idx = 1u; true; ++arg_idx)
    {
        Symbol const divider = arg_list.GetNext();
        if (kKW_CloseParenthesis == divider)
            return;

        if (kKW_OpenParenthesis != divider && kKW_Comma != divider)
            InternalError("Expected one of '(,)', found '%s' instead", _sym.GetName(divider).c_str());

        // Isolate the current argument
        size_t const current_arg_start = arg_list.GetCursor();
        arg_list.SkipTo(SymbolList{ kKW_Comma, kKW_CloseParenthesis, });
        size_t const current_arg_end = arg_list.GetCursor();
        SrcList current_arg(arg_list, current_arg_start, current_arg_end - current_arg_start);

        if (current_arg.Length() > 2 && kKW_Colon == current_arg[1u] && _sym.IsIdentifier(current_arg[0u]))
            UserError(
                "Call to function %s, argument #%u: "
                "Expected an expression "
                "(Cannot mix positional and named arguments in one call)",
                _sym.GetName(name_of_func).c_str(),
                arg_idx);
        // 'current_arg[...]' has moved the cursor; undo that
        arg_list.SetCursor(current_arg_end);

        if (current_arg.Length() || kKW_CloseParenthesis != arg_list.PeekNext())
            arg_exprs.push_back(current_arg);
    }
}

void AGS::Parser::AccessData_FunctionCall_AnalyseFormatString(std::vector<FuncParameterDesc> const &param_descs, std::vector<SrcList> &arg_exprs, bool &check_vartypes_by_format, std::vector<std::string> &format_strings)
{
    check_vartypes_by_format = false;

    // Find the index of the format parameter
    auto it = std::find_if(
        param_descs.begin(), param_descs.end(),
        [](FuncParameterDesc const &fpd) -> bool { return fpd.IsFormatParam; });
    if (it == param_descs.end())
        InternalError("Cannot find the format string");
    size_t param_idx = it - param_descs.begin();

    // Find the corresponding argument and make sure that it's a string literal
    auto &arg_expr = arg_exprs[param_idx];
    if (arg_expr.Length() != 1)
        return;
    Symbol format_sym = arg_expr[0u];
    if (!_sym.IsLiteral(format_sym) ||
        !_sym.IsAnyStringVartype(_sym[format_sym].LiteralD->Vartype))
        return; // Can only check literals

    check_vartypes_by_format = true;

    // Get the actual characters of the format string
    auto format_cstr = &(_scrip.strings[_sym[format_sym].LiteralD->Value]);
    std::string format{ format_cstr };

    // Extract the specifiers
    for (size_t format_idx1 = 0; format_idx1 < format.length(); format_idx1++)
    {
        if (format[format_idx1] != '%')
            continue;

        if (format_idx1 + 1u < format.length() && format[format_idx1 + 1u] == '%')
        {   
            format_idx1++; // Skip "%%"
            continue;
        }

        // Letters that define a format  to be printed
        static std::string const format_letters = "AEFGXacdefgiopsux";
        // Modifiers that can be in beween '%' and the respective format letter
        static std::string const modifiers = "hjLltz";
        // Symbols that can be in beween '%' and the respective format letter
        static std::string const legal_symbols = " +-.#*";

        // Extract the format specifications
        for (size_t format_idx2 = format_idx1 + 1u; format_idx2 < format.length(); format_idx2++)
        {
            char const ch = format[format_idx2];
            if ('0' <= ch && ch <= '9')
                continue;
            if (std::string::npos != legal_symbols.find(ch))
                continue;
            if (std::string::npos != modifiers.find(ch))
                continue;

            auto const spec = format.substr(format_idx1, format_idx2 - format_idx1 + 1u);
            if (std::string::npos != format_letters.find(ch))
            {
                format_strings.push_back(spec);
                format_idx1 = format_idx2;
                break;
            }

            UserError(
                "Argument #%u: Cannot process '%s' in the format string literal",
                param_idx,
                spec.c_str());
        }
    }
}

// Note, we _must_ pass 'args_size' to this function, we can't deduce that from 'arg_exprs',
// or else things will get awry when the last parameter has a default and an argument is not supplied for it.
void AGS::Parser::AccessData_FunctionCall_Arguments_Push(Symbol name_of_func, bool func_is_import, size_t args_size,
    std::vector<SrcList> arg_exprs, bool use_named_args, std::vector<AGS::FuncParameterDesc> const &param_descs,
    bool check_format, std::vector<std::string> const formats)
{
    auto const no_desc = FuncParameterDesc{};
    size_t const first_variadic_idx = param_descs.size();
    size_t const variadic_args_count = args_size - param_descs.size();

    // Check the type specification of the format if applicable
    if (check_format && (formats.size() > args_size - first_variadic_idx))
        UserError(
            "Call to function '%s': "
            "The format string provides specifications for %u variadic argument(s) "
            " but only %u variadic argument(s) are passed",
            _sym.GetName(name_of_func).c_str(),
            formats.size(),
            variadic_args_count);
    if (check_format && (formats.size() < args_size - first_variadic_idx))
        UserError(
            "Call to function '%s': "
            "%u variadic argument(s) are passed, but "
            " the format string provides specifications for only %u variadic argument(s)",
            _sym.GetName(name_of_func).c_str(),
            variadic_args_count,
            formats.size());

    // Push the arguments
    // In reverse order because the engine expects them this way
    for (size_t idx = args_size - 1u; idx >= 1u; --idx)
    {
        // For error messages
        std::string ctfp = "Call to function '<func>', <name>: ";
        string_replace(ctfp, "<func>", _sym.GetName(name_of_func));
        std::string argument_name = "";
        if (use_named_args &&
            idx < param_descs.size() &&
            !_sym[name_of_func].FunctionD->ParamNamingInconsistency.Exists)
        {
            argument_name = "parameter '<param>'";
            string_replace(argument_name, "<param>", _sym.GetName(param_descs[idx].Name));
            if (argument_name == "parameter ''")
                argument_name = "";
        }

        if ("" == argument_name)
        {
            argument_name = "argument #<arg>";
            string_replace(argument_name, "<arg>", std::to_string(idx));
        }

        string_replace(ctfp, "<name>", argument_name);

        FuncParameterDesc const &param_desc =
            (idx < param_descs.size()) ? param_descs[idx] : no_desc;

        Symbol arg_vartype = kKW_NoSymbol;

        if (idx < arg_exprs.size() && arg_exprs[idx].Length() > 0u) // Argument passed
        {
            auto &arg_expr = arg_exprs[idx];

            EvaluationResult eres;
            ParseExpression_Term(arg_expr, eres, true, true);
            EvaluationResultToAx(eres);
            arg_vartype = eres.Vartype;
            if (idx < param_descs.size())
            {
                if (IsVartypeMismatch_Oneway(eres.Vartype, param_desc.Vartype))
                    UserError(
                        ReferenceMsgSym(
                            (ctfp + "Cannot convert '%s' to '%s'").c_str(),
                            name_of_func).c_str(),
                        _sym.GetName(eres.Vartype).c_str(),
                        _sym.GetName(param_desc.Vartype).c_str());

                ConvertAXStringToStringObject(param_desc.Vartype, eres.Vartype);
                if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, eres.Vartype) &&
                    kKW_String == _sym.VartypeWithout(VTT::kConst, param_desc.Vartype))
                    // Must make sure that NULL isn't passed at runtime
                    WriteCmd(SCMD_CHECKNULLREG, SREG_AX);
            }

        }
        else // No argument or empty argument passed
        {
            // Provide default when it exists
            Symbol const deflt = param_desc.Default;
            if (kKW_NoSymbol == deflt)
                UserError(
                    ReferenceMsgLoc(
                        (ctfp + "Argument isn't passed and parameter doesn't have a default").c_str(),
                        param_desc.Declared).c_str());

            if (!_sym.IsLiteral(deflt))
                InternalError("Parameter default symbol isn't literal");
            arg_vartype = _sym[deflt].LiteralD->Vartype;
            CodeCell value = _sym[deflt].LiteralD->Value;
            WriteCmd(SCMD_LITTOREG, SREG_AX, value);
        }

        if (check_format && idx >= param_descs.size())
        {
            size_t const format_idx = idx - first_variadic_idx;
            auto &format = formats[format_idx];
            int const fletter = format.back();
            switch (fletter)
            {
            default:
                InternalError("Cannot process the format letter '%c'", fletter);

            // characters
            case 'c':
            // (signed) integers
            case 'd':
            case 'i':
            // unsigned integers (currently treat them like integers)
            case 'o':
            case 'u':
            case 'X':
            case 'x':
                if (!_sym.IsAnyIntegerVartype(arg_vartype))
                    UserError(
                        (ctfp + "Argument has type '%s' and doesn't match the format specifier '%s' for integer types").c_str(),
                        _sym.GetName(arg_vartype).c_str(),
                        format.c_str());
                break;

            case 'A':
            case 'a':
            case 'E':
            case 'e':
            case 'F':
            case 'f':
            case 'G':
            case 'g':
                if (kKW_Float != arg_vartype)
                    UserError(
                        (ctfp + "Argument has type '%s' and doesn't match the format specifier '%s' for type 'float'").c_str(),
                        _sym.GetName(arg_vartype).c_str(),
                        format.c_str());
                break;

            case 'p':
                // Also let through 'null'
                if (arg_vartype != kKW_Null &&
                    !_sym.IsDynVartype(arg_vartype))
                    UserError(
                        (ctfp + "Argument has type '%s' and doesn't match the format specifier '%s' for dynamic types").c_str(),
                        _sym.GetName(arg_vartype).c_str(),
                        format.c_str());
                break;

            case 's':
                // Also let through 'null'
                // Also let through one-dimensional classic arrays of 'char'
                if (arg_vartype != kKW_Null &&
                    !_sym.IsAnyStringVartype(arg_vartype) &&
                    !(  _sym[arg_vartype].VartypeD->Type == VTT::kArray &&
                        _sym[arg_vartype].VartypeD->Dims.size() == 1u &&
                        _sym[arg_vartype].VartypeD->BaseVartype == kKW_Char))
                    UserError(
                        (ctfp + "Argument has type '%s' and doesn't match the format specifier '%s' for string types").c_str(),
                        _sym.GetName(arg_vartype).c_str(),
                        format.c_str());
            }
        }

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
    }
}

void AGS::Parser::AccessData_FunctionCall_Arguments(Symbol const name_of_func, bool const func_is_import, std::vector<FuncParameterDesc> const &param_descs, bool const is_variadic, SrcList &arguments, size_t &args_size)
{
    // Parse the arguments into expressions that match 'param_descs'
    std::vector<SrcList> arg_exprs = {};

    bool const use_named_args =
        arguments.Length() >= 3u &&
        _sym.IsIdentifier(arguments[1u]) &&
        kKW_Colon == arguments[2u];
    if (use_named_args)
        AccessData_FunctionCall_Arguments_Named(name_of_func, param_descs, arguments, arg_exprs);
    else
        AccessData_FunctionCall_Arguments_Sequence(name_of_func, param_descs, arguments, arg_exprs);

    args_size = std::max(param_descs.size(), arg_exprs.size()); 
    if (args_size > param_descs.size() && !is_variadic)
        UserError(
            ReferenceMsgSym(
                "Call to function '%s': Expected at most %u arguments, found more",
                name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str(),
            param_descs.size() - 1u); // '-1u' because 'param_descs[0]' is the return parameter

    bool check_vartypes_by_format = false;
    std::vector<std::string> format_strings = {};
    if (_sym[name_of_func].FunctionD->IsFormat)
        AccessData_FunctionCall_AnalyseFormatString(param_descs, arg_exprs, check_vartypes_by_format, format_strings);
    AccessData_FunctionCall_Arguments_Push(name_of_func, func_is_import, args_size, arg_exprs, use_named_args, param_descs,
        check_vartypes_by_format, format_strings);
    
    // Move cursor to the end of the arguments, they have been used up
    arguments.SetCursor(arguments.Length());
}

void AGS::Parser::AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, EvaluationResult &eres)
{
    // Get the arguments of the call
    size_t const arguments_begin = expression.GetCursor();
    SkipNextSymbol(expression, kKW_OpenParenthesis);
    expression.SkipToCloser();
    SkipNextSymbol(expression, kKW_CloseParenthesis);
    size_t const arguments_end = expression.GetCursor();
    SrcList arguments = SrcList(expression, arguments_begin, arguments_end - arguments_begin);

    auto const function_tqs = _sym[name_of_func].FunctionD->TypeQualifiers;
    bool const func_is_import = function_tqs[TQ::kImport];
    // If function uses normal stack, we need to do stack calculations to get at certain elements
    bool const func_uses_normal_stack = !func_is_import;
    bool const calling_func_uses_this = (kKW_NoSymbol != _sym.GetVartype(kKW_This));
    bool op_pushed = false;

    if (calling_func_uses_this)
    {
        // Save OP since we need it after the func call
        // We must do this no matter whether the callED function itself uses "this"
        // because a called function that doesn't might call a function that does.
        PushReg(SREG_OP);
        op_pushed = true;
    }

    bool const called_func_uses_this =
        std::string::npos != _sym.GetName(name_of_func).find("::") &&
        !function_tqs[TQ::kStatic];

    bool mar_pushed = false;
    if (called_func_uses_this)
    {
        // MAR contains the address of "outer"; this is what will be used for "this" in the called function.
        _marMgr.UpdateMAR(_src.GetLineno(), _scrip);

        // Parameter processing might entail calling yet other functions, e.g., in "f(...g(x)...)".
        // So we cannot emit SCMD_CALLOBJ here, before parameters have been processed.
        // Save MAR because parameter processing might clobber it 
        PushReg(SREG_MAR);
        mar_pushed = true;
    }

    size_t args_size;
    AccessData_FunctionCall_Arguments(
        name_of_func,
        func_is_import,
        _sym[name_of_func].FunctionD->Parameters,
        _sym.IsVariadicFunc(name_of_func),
        arguments,
        args_size);

    // Get the 'net' number of arguments, without the return parameter in '[0]'
    size_t const args_count = args_size - 1u; 
    
      if (called_func_uses_this)
      {
        if (0u == args_count)
        {   // MAR must still be current, so undo the unneeded PUSH above.
            _scrip.OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;
            size_t const size_of_pushreg = 2u;
            _scrip.code.resize(_scrip.code.size() - size_of_pushreg);
            mar_pushed = false;
        }
        else
        {   // Recover the value of MAR from the stack. It's in front of the parameters.
            WriteCmd(
                SCMD_LOADSPOFFS,
                (1 + (func_uses_normal_stack ? args_count : 0)) * SIZE_OF_STACK_CELL);
            WriteCmd(SCMD_MEMREAD, SREG_MAR);
            _reg_track.SetRegister(SREG_MAR);
        }
        WriteCmd(SCMD_CALLOBJ, SREG_MAR);
    }

    AccessData_FunctionCall_EmitCall(name_of_func, args_count, func_is_import);

    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_AX;
    eres.Symbol = kKW_NoSymbol;
    eres.Vartype = _sym.FuncReturnVartype(name_of_func);
    eres.SideEffects = true; // A function call is a side effect
    eres.Modifiable = false;

    if (mar_pushed)
    {
        PopReg(SREG_MAR);
        _reg_track.SetRegister(SREG_MAR);
    }
    if (op_pushed)
        PopReg(SREG_OP);

    MarkAcessed(name_of_func);
    expression.SetCursor(arguments_end);
}

bool AGS::Parser::ParseExpression_CompileTime(Symbol const op_sym, EvaluationResult const &eres_lhs, EvaluationResult const &eres_rhs, EvaluationResult &eres)
{
    Vartype const vartype_lhs = _sym[eres_lhs.Symbol].LiteralD->Vartype;
    Vartype const vartype_rhs = _sym[eres_rhs.Symbol].LiteralD->Vartype;

    CompileTimeFunc *ctf;
    if ((kKW_Float == vartype_lhs) && (kKW_Float == vartype_rhs))
        ctf = _sym[op_sym].OperatorD->FloatCTFunc;
    else if (_sym.IsAnyIntegerVartype(vartype_lhs) && _sym.IsAnyIntegerVartype(vartype_rhs))
        ctf = _sym[op_sym].OperatorD->IntCTFunc;
    else
        return false;

    if (nullptr == ctf)
        return false;

    Symbol symbol;
    try
    {
        ctf->Evaluate(eres_lhs.Symbol, eres_rhs.Symbol, symbol);
    }
    catch (CompileTimeFunc::CompileTimeError &e)
    {
        UserError(e.what());
    }

    eres.Type = eres.kTY_Literal;
    eres.Location = eres.kLOC_SymbolTable;
    eres.Symbol = symbol;
    eres.Vartype = _sym.IsBooleanOperator(op_sym) ? kKW_Int : vartype_lhs;
    return true;
}

void AGS::Parser::ParseExpression_NoOps(SrcList &expression, EvaluationResult &eres, bool const result_used)
{
    if (kKW_OpenParenthesis == expression[0u])
        return ParseExpression_InParens(expression, eres, result_used);

    AccessData(VAC::kReading, expression, eres);
    ParseExpression_CheckUsedUp(expression);
}

void AGS::Parser::ParseExpression_Term(SrcList &expression, EvaluationResult &eres, bool const result_used, bool const classic_array_ok)
{
    size_t const exp_length = expression.Length();
    if (0u == exp_length)
        InternalError("Cannot parse empty subexpression");

    int const least_binding_op_idx = IndexOfLeastBondingOperator(expression);  // can be < 0
    
    if (0 > least_binding_op_idx)
        ParseExpression_NoOps(expression, eres, result_used);
    else if (0 == least_binding_op_idx)
        ParseExpression_Prefix(expression, eres);
    else if (expression.Length() - 1u == least_binding_op_idx)
        ParseExpression_Postfix(expression, eres, result_used);
    else if (kKW_Tern == expression[least_binding_op_idx])
        ParseExpression_Ternary(least_binding_op_idx, expression, result_used, eres);
    else
        ParseExpression_Binary(least_binding_op_idx, expression, eres);        

    expression.SetCursor(std::max(least_binding_op_idx, 1));

    if (_sym.IsArrayVartype(eres.Vartype))
    {
        if (!classic_array_ok)
            UserError("Cannot access this array as a whole (did you forget to add \"[0]\"?)");

        if (eres.kLOC_MemoryAtMAR == eres.Location)
        {
            // Must move to AX or else 'array[0u]' will be returned
            _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            _reg_track.SetRegister(SREG_AX);
            eres.Location = eres.kLOC_AX;
        }
    }

    if (_sym.IsAtomicVartype(eres.Vartype) && _sym.IsStructVartype(eres.Vartype))
    {
        if (_sym.IsManagedVartype(eres.Vartype))
        {
            // Interpret the memory address as the result
            // We don't have a way of saying, "MAR _is_ the value"
            // so we move the value to AX, we _can_ say "AX _is_ the value".
            eres.Vartype = _sym.VartypeWithDynpointer(eres.Vartype);
            _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            _reg_track.SetRegister(SREG_AX);
            eres.Location = eres.kLOC_AX;
            return;
        }

        UserError("Cannot access this non-managed struct as a whole");
    }
}

// We access a component of a struct in order to read or write it.
void AGS::Parser::AccessData_StructMember(Symbol component, VariableAccess access_type, bool access_via_this, SrcList &expression, EvaluationResult &eres)
{
    expression.GetNext(); // Eat component
    SymbolTableEntry &entry = _sym[component];
    auto const compo_tqs = entry.VariableD->TypeQualifiers;

    if (VAC::kReading != access_type && compo_tqs[TQ::kWriteprotected] && !access_via_this)
        UserError(
            "Writeprotected component '%s' must not be modified from outside",
            _sym.GetName(component).c_str());
    if (compo_tqs[TQ::kProtected] && !access_via_this)
        UserError(
            "Protected component '%s' must not be accessed from outside",
            _sym.GetName(component).c_str());

    _marMgr.AddComponentOffset(entry.ComponentD->Offset);
    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_MemoryAtMAR;
    eres.Vartype = _sym.GetVartype(component);
    eres.Modifiable =
        eres.Modifiable &&
        !compo_tqs[TQ::kReadonly] &&
        (access_via_this || (!compo_tqs[TQ::kWriteprotected] && !compo_tqs[TQ::kProtected]));

}

Symbol  AGS::Parser::ConstructAttributeFuncName(Symbol attribsym, bool is_setter, bool is_indexed)
{
    std::string member_str = _sym.GetName(attribsym);
    // If "::" in the name, take the part after the last "::"
    size_t const member_access_pos = member_str.rfind("::");
    if (std::string::npos != member_access_pos)
        member_str = member_str.substr(member_access_pos + 2u);
    char const *stem_str = is_setter ? "set" : "get";
    char const *indx_str = is_indexed ? "i_" : "_";
    std::string func_str = stem_str + (indx_str + member_str);
    return _sym.FindOrAdd(func_str);
}

void AGS::Parser::AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, Vartype vartype)
{
    // Search for the attribute: It might be in an ancestor of 'vartype' instead of in 'vartype'.
    Symbol const unqualified_component = expression.GetNext();
    Symbol const struct_of_component =
        FindStructOfComponent(vartype, unqualified_component);
    if (kKW_NoSymbol == struct_of_component)
        UserError(
            ReferenceMsgSym(
                "Struct '%s' doesn't have an attribute named '%s'",
                struct_of_component).c_str(),
            _sym.GetName(vartype).c_str(),
            _sym.GetName(unqualified_component).c_str());

    auto const &struct_components = _sym[struct_of_component].VartypeD->Components;
    Symbol const name_of_attribute = struct_components.at(unqualified_component);

    bool const attrib_uses_this =
        !_sym[name_of_attribute].AttributeD->IsStatic;
    bool const call_is_indexed =
        (kKW_OpenBracket == expression.PeekNext());
    bool const attrib_is_indexed =
        _sym[name_of_attribute].AttributeD->IsIndexed;

    if (call_is_indexed && !attrib_is_indexed)
        UserError("Unexpected '[' after non-indexed attribute '%s'", _sym.GetName(name_of_attribute).c_str());
    else if (!call_is_indexed && attrib_is_indexed)
        UserError("Expected '[' after indexed attribute '%s'", _sym.GetName(name_of_attribute).c_str());

    if (is_setter && kKW_NoSymbol == _sym[name_of_attribute].AttributeD->Setter)
        UserError(
            ReferenceMsgSym(
                "Cannot assign a value to readonly attribute '%s'",
                name_of_attribute).c_str(),
            _sym[name_of_attribute].Name.c_str());

    // Get the appropriate access function (as a symbol)
    Symbol const qualified_func_name = is_setter ?
        _sym[name_of_attribute].AttributeD->Setter : _sym[name_of_attribute].AttributeD->Getter;
    bool const func_is_import = _sym[qualified_func_name].FunctionD->TypeQualifiers[TQ::kImport];

    if (attrib_uses_this)
        PushReg(SREG_OP); // is the current this ptr, must be restored after call

    size_t args_count = 0u;
    if (is_setter)
    {
        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++args_count;
    }

    if (call_is_indexed)
    {
        // The index to be set is in the [...] clause; push it as the first parameter
        if (attrib_uses_this)
            PushReg(SREG_MAR); // must not be clobbered
        EvaluationResult eres;
        Expect(kKW_OpenBracket, _src.GetNext());
        ParseIntegerExpression(expression, eres);
        Expect(kKW_CloseBracket, _src.GetNext());
        EvaluationResultToAx(eres);

        if (attrib_uses_this)
            PopReg(SREG_MAR);

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++args_count;
    }

    if (attrib_uses_this)
        WriteCmd(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    AccessData_FunctionCall_EmitCall(qualified_func_name, args_count, func_is_import);

    if (attrib_uses_this)
        PopReg(SREG_OP); // restore old this ptr after the func call

    MarkAcessed(qualified_func_name);
}


void AGS::Parser::AccessData_Dereference(EvaluationResult &eres)
{
    if (EvaluationResult::kLOC_AX == eres.Location)
    {
        WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
        _reg_track.SetRegister(SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
        eres.Location = EvaluationResult::kLOC_MemoryAtMAR;
        _marMgr.Reset();
    }
    else
    {
        _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
        // We need to check here whether m[MAR] == 0, but CHECKNULL
        // checks whether MAR == 0. So we need to do MAR := m[MAR] first.
        WriteCmd(SCMD_MEMREADPTR, SREG_MAR);
        _reg_track.SetRegister(SREG_MAR);
        WriteCmd(SCMD_CHECKNULL);
    }
}

void AGS::Parser::AccessData_ProcessArrayIndex(size_t const dim, size_t const factor, bool const is_dynarray, SrcList &index_expr, EvaluationResult &eres)
{
    index_expr.StartRead();
    // If all ops are pending on the MAR register, it hasn't been set yet at all.
    // So then we don't need to protect MAR against being clobbered, 
    bool const guard_mar_register = !_marMgr.AreAllOpsPending();
    MarMgr orig_mar_state(_marMgr);
    RegisterGuard(guard_mar_register ? RegisterList{ SREG_MAR } : RegisterList{},
        [&]
        {
            ParseIntegerExpression(index_expr, eres);
            // We need the result of the array index expression (i.e., the offset to
            // add to the location of the base variable) in AX unless it is a literal.
            // That's the only location we can use. We can't use the address that MAR
            // points to as the location because MAR will be clobbered by the 'POP(MAR)'
            // that will be generated at the end of 'RegisterGuard()'. 
            if (eres.kTY_Literal != eres.Type)
                EvaluationResultToAx(eres);
        });

    // Restore the state that the MAR register has been in
    // This is important because we compute the correct MAR value lazily,
    // directly before the value is needed. The state contains accumulated
    // instructions that haven't been emitted yet.
    _marMgr = orig_mar_state;

    if (!index_expr.ReachedEOF())
        UserError(
            "Unexpected '%s' after array index",
            _sym.GetName(index_expr.GetNext()).c_str()); 

    if (eres.kTY_Literal == eres.Type)
    {
        // The arrax index is known at compile time, so check it as far as possible
        int const index_value = _sym[eres.Symbol].LiteralD->Value;
        if (index_value < 0)
            UserError(
                "Array index is %d, thus too low (minimum is 0)", index_value);
        if (dim > 0u && static_cast<size_t>(index_value) >= dim)
            UserError(
                "Array index is %d, thus too high (maximum is %u)", index_value, dim - 1u);

        if (is_dynarray && index_value > 0)
        {
            // We need to check the offset at runtime because we can't know the
            // array size that has been allocated.
            // Make sure that a _whole_ array element fits, so 'index_value * factor' isn't enough
            WriteCmd(SCMD_LITTOREG, SREG_AX, (index_value + 1) * factor - 1);
            _reg_track.SetRegister(SREG_AX);
            WriteCmd(SCMD_DYNAMICBOUNDS, SREG_AX);
        }
        
        _marMgr.AddComponentOffset(index_value * factor);
        return;
    }
    
    // DYNAMICBOUNDS compares the offset into the memory block:
    // it mustn't be larger than the size of the allocated memory. 
    // On the other hand, CHECKBOUNDS checks the index: it mustn't be
    // larger than the maximum given. So dynamic bounds must be checked
    // _after_ the multiplication; static bounds _before_ the multiplication.
    // For better error messages at runtime, do CHECKBOUNDS first, then multiply.
    if (!is_dynarray)
        WriteCmd(SCMD_CHECKBOUNDS, SREG_AX, dim);
    if (factor != 1u)
    {
        WriteCmd(SCMD_MUL, SREG_AX, factor);
        _reg_track.SetRegister(SREG_AX);
    }
    if (is_dynarray)
        WriteCmd(SCMD_DYNAMICBOUNDS, SREG_AX);

    _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
    WriteCmd(SCMD_ADDREG, SREG_MAR, SREG_AX);
    _reg_track.SetRegister(SREG_MAR);
}

void AGS::Parser::AccessData_ProcessArrayIndexes(SrcList &expression, EvaluationResult &eres)
{
    while (kKW_OpenBracket == expression.PeekNext())
    {
        if (!_sym.IsAnyArrayVartype(eres.Vartype))
            UserError(
                "An array index cannot follow an expression of type '%s'",
                _sym.GetName(eres.Vartype).c_str());

        Vartype const array_vartype = eres.Vartype;
        Vartype const element_vartype = _sym[array_vartype].VartypeD->BaseVartype;
        bool const is_dynarray = _sym.IsDynarrayVartype(array_vartype);

        if (is_dynarray)
            AccessData_Dereference(eres);

        std::vector<size_t> dynarray_dims = { 0u, }; // Dynarrays have just 1 dimension
        std::vector<size_t> &dims = is_dynarray ? dynarray_dims : _sym[array_vartype].VartypeD->Dims;
        std::vector<size_t> factors;
        factors.resize(dims.size());
        size_t factor = _sym.GetSize(element_vartype);
        for (int idx = dims.size() - 1; idx >= 0; idx--) // yes, 'int'
        {
            factors[idx] = factor;
            factor *= dims[idx];
        }

        Expect(kKW_OpenBracket, expression.GetNext());

        for (size_t dim_idx = 0u; dim_idx < dims.size(); dim_idx++)
        {
            // Get the current index
            size_t const index_start = expression.GetCursor();
            expression.SkipTo(SymbolList{ kKW_Comma, kKW_CloseBracket });
            size_t const index_end = expression.GetCursor();
            SrcList index_expr = SrcList(expression, index_start, index_end - index_start);
            if (0u == index_expr.Length())
                UserError("Array index must not be empty");

            EvaluationResult eres_index;
            AccessData_ProcessArrayIndex(dims[dim_idx], factors[dim_idx], is_dynarray, index_expr, eres_index);
            if (eres_index.SideEffects)
                eres.SideEffects = true;

            Symbol const divider = expression.GetNext();
            if (dim_idx + 1u == dims.size())
            {
                Expect(kKW_CloseBracket, divider);
                break;
            }
            Expect(SymbolList{ kKW_CloseBracket, kKW_Comma }, divider);
            if (kKW_CloseBracket == divider)
            {
                Expect(
                    kKW_OpenBracket,
                    expression.GetNext(),
                    "Another index should follow: Expected");
            }
        }

        eres.Vartype = element_vartype;
    }
}

void AGS::Parser::AccessData_Variable(VariableAccess access_type, SrcList &expression, EvaluationResult &eres)
{
    Symbol varname = expression.GetNext();
    SymbolTableEntry &entry = _sym[varname];
    CodeCell const soffs = entry.VariableD->Offset;
    auto const var_tqs = entry.VariableD->TypeQualifiers;
    auto const scope_type = _sym.GetScopeType(varname);

    if (var_tqs[TQ::kImport])
        MarkAcessed(varname);

    if (VAC::kReading != access_type && var_tqs[TQ::kReadonly])
        UserError("Cannot modify the readonly '%s'", _sym.GetName(varname).c_str());

    _marMgr.Reset();
    _marMgr.SetStart(scope_type, soffs);
    // Note, no instructions are generated, so MAR is not set here

    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_MemoryAtMAR;
    eres.Symbol = kKW_NoSymbol;
    eres.Vartype = _sym.GetVartype(varname);
    eres.LocalNonParameter = (ScT::kLocal == scope_type && entry.Scope != _sym.kParameterScope);
    eres.Modifiable = !var_tqs[TQ::kReadonly];

    if (_sym.kParameterScope == _sym[varname].Scope &&
        _sym.VartypeWithout(VTT::kConst, eres.Vartype) == kKW_String)
    {
        // "string" parameters are passed as _pointers_ to the
        // actual string, so we need to dereference here.
        // The parameter cell doesn't contain a pointer to
        // managed memory space, so don't use 'SCMD_MEMREADPTR'
        _marMgr.UpdateMAR(expression.GetLineno(), _scrip);
        WriteCmd(SCMD_MEMREAD, SREG_MAR);
        _reg_track.SetRegister(SREG_MAR);
    }

    return AccessData_ProcessArrayIndexes(expression, eres);
}

void AGS::Parser::AccessData_This(EvaluationResult &eres)
{
    // The expression returned is "this"
    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_MemoryAtMAR;
    eres.Symbol = kKW_NoSymbol;
    eres.Vartype = _sym.GetVartype(kKW_This);
    eres.Modifiable = true;

    WriteCmd(SCMD_REGTOREG, SREG_OP, SREG_MAR);
    _reg_track.SetRegister(SREG_MAR);
    WriteCmd(SCMD_CHECKNULL);
    _marMgr.Reset();
}

void AGS::Parser::AccessData_StructVartype(Vartype const vartype, EvaluationResult &eres)
{
    // Return the struct itself, static access
    eres.Type = eres.kTY_StructName;
    eres.Location = eres.kLOC_SymbolTable;
    eres.Symbol = vartype;
    eres.Vartype = kKW_NoSymbol;
    _marMgr.Reset();
}

void AGS::Parser::AccessData_FirstClause(VariableAccess access_type, SrcList &expression, EvaluationResult &eres, bool &inside_access, bool &implied_dot)
{
    implied_dot = false;

    eres.SideEffects = false; // This is almost alwways correct

    Symbol const first_sym = expression.PeekNext();

    if (kKW_Null == first_sym ||
        _sym.IsConstant(first_sym) ||
        _sym.IsLiteral(first_sym))
    {
        if (VAC::kReading != access_type)
            UserError("Cannot modify '%s'", _sym.GetName(first_sym).c_str());

        SkipNextSymbol(expression, first_sym);
        Symbol lit = first_sym;
        while (_sym.IsConstant(lit))
            lit = _sym[lit].ConstantD->ValueSym;
        SetCompileTimeLiteral(lit, eres);
        return;
    }

    if (_sym.IsFunction(first_sym))
    {
        SkipNextSymbol(expression, first_sym); 
        if (kKW_OpenParenthesis != expression.PeekNext())
        {
            // Return the function symbol as-is
            eres.Type = eres.kTY_FunctionName;
            eres.Location = eres.kLOC_SymbolTable;
            eres.Symbol = first_sym;
            eres.Vartype = kKW_NoSymbol;
            eres.Modifiable = false;
            return;
        }

        AccessData_FunctionCall(first_sym, expression, eres);
        if (_sym.IsDynarrayVartype(eres.Vartype))
            AccessData_ProcessArrayIndexes(expression, eres);
        return;
    }

    if (kKW_This == first_sym)
    {
        if (kKW_NoSymbol == _sym.GetVartype(kKW_This))
            UserError("'this' is only legal in a struct function");
        if (_sym[kKW_This].VariableD->TypeQualifiers[TQ::kStatic])
            UserError("'this' is only legal in a non-static function");

        SkipNextSymbol(expression, kKW_This);

        AccessData_This(eres);
        inside_access = true;
        return;
    }

    if (_sym.IsVariable(first_sym))
    {
        AccessData_Variable(access_type, expression, eres);
        return;
    }

    if (_sym.IsStructVartype(first_sym))
    {
        SkipNextSymbol(expression, first_sym);
        inside_access = true;
        return AccessData_StructVartype(first_sym, eres);
    }

    // Can this unknown symbol be interpreted as a component of 'this'
    // or as a static component of the current class?
    Vartype const this_vartype = _sym.GetVartype(kKW_This);
    if (_sym.IsVartype(this_vartype))
    {
        Symbol const potential_component = _sym.FindStructComponent(this_vartype, first_sym);
        if (kKW_NoSymbol != potential_component)
        {
            // Fake a "this." or "Vartype." here:
            // Eat the component, pretend that it is 'this' or 'Vartype'.
            // We need to do this in order to force the code that will be emitted
            // to be connected to the proper place in the source.
            expression.GetNext();

            if (_sym[kKW_This].VariableD->TypeQualifiers[TQ::kStatic])
                AccessData_StructVartype(this_vartype, eres);
            else
                AccessData_This(eres);
            inside_access = true;

            // Going forward, the code should imply the '.' already read in.
            implied_dot = true;
            // Then back up so that the component will be read again as the next symbol.
            expression.BackUp();
            return;
        }
    }

    if (kKW_OnePastLongMax == first_sym)
        UserError("Integer literal is out of bounds (maximum is %d)", INT32_MAX);

    UserError("Unexpected '%s' in expression", _sym.GetName(first_sym).c_str());
}

void AGS::Parser::AccessData_SubsequentClause(VariableAccess access_type, bool access_via_this, SrcList &expression, EvaluationResult &eres)
{
    bool const static_access = (eres.kTY_StructName == eres.Type);
    Vartype const vartype = static_access ? eres.Symbol : eres.Vartype;
    Symbol const unqualified_component = expression.PeekNext();
    Symbol const qualified_component = FindComponentInStruct(vartype, unqualified_component);

    // Note: Don't unconditionally reset 'eres.LocalNonParameter', 'eres.SideEffects'
    // if they are true, the true should stick

    if (kKW_NoSymbol == qualified_component)
        UserError(
            ReferenceMsgSym(
                "Expected a component of '%s', found '%s' instead",
                vartype).c_str(),
            _sym.GetName(vartype).c_str(),
            _sym.GetName(unqualified_component).c_str());

    if (_sym.IsAttribute(qualified_component))
    {
        if (static_access && !_sym[qualified_component].AttributeD->IsStatic)
            UserError(
                ReferenceMsgSym(
                    "Must specify a specific object for the non-static attribute '%s'",
                    qualified_component).c_str(),
                _sym.GetName(qualified_component).c_str());

        // make MAR point to the struct of the attribute
        _marMgr.UpdateMAR(_src.GetLineno(), _scrip);

        if (VAC::kWriting == access_type)
        {
            // We cannot process the attribute here so return the attribute itself
            eres.Type = eres.kTY_AttributeName;
            eres.Location = eres.kLOC_SymbolTable;
            eres.Symbol = qualified_component;
            eres.Vartype = _sym[qualified_component].AttributeD->Vartype;
            if (kKW_NoSymbol == _sym[qualified_component].AttributeD->Setter)
                UserError(
                    ReferenceMsgSym("Attribute '%s' is readonly", qualified_component).c_str(),
                    _sym.GetName(unqualified_component).c_str());
            return;
        }
        bool const is_setter = false;
        AccessData_CallAttributeFunc(is_setter, expression, vartype);
        eres.Type = eres.kTY_RunTimeValue;
        eres.Location = eres.kLOC_AX;
        eres.Symbol = kKW_NoSymbol;
        eres.Vartype = _sym[qualified_component].AttributeD->Vartype;
        eres.SideEffects = true; // Calling an attribute is a side-effect
        return;
    }

    if (_sym.IsConstant(qualified_component))
    {
        SkipNextSymbol(expression, unqualified_component); // Eat the constant symbol

        eres.Type = eres.kTY_Literal;
        eres.Location = eres.kLOC_SymbolTable;
        eres.Symbol = _sym[qualified_component].ConstantD->ValueSym;
        eres.Vartype = _sym[eres.Symbol].LiteralD->Vartype;
        eres.Modifiable = false;
        return;
    }

    if (_sym.IsFunction(qualified_component))
    {
        if (static_access && !_sym[qualified_component].FunctionD->TypeQualifiers[TQ::kStatic])
            UserError(
                ReferenceMsgSym(
                    "Must specify a specific object for the non-static function %s",
                    qualified_component).c_str(),
                _sym.GetName(qualified_component).c_str());

        if (_sym.IsConstructor(qualified_component))
        {
            // Cannot call a constructor directly as a function.
            // As an exception, when within a constructor
            // you may call the constructors of ancestors of 'this'.
            bool is_call_forbidden = true;
            if (_sym.IsConstructor(_currentlyCompiledFunction))
            {
                Symbol const struct_of_constructor = _sym[qualified_component].ComponentD ?
                    _sym[qualified_component].ComponentD->Parent : kKW_NoSymbol;
                Symbol const vartype_of_this = _sym.GetVartype(kKW_This);
                
                for (int vartype = _sym[vartype_of_this].VartypeD->Parent;
                    vartype != kKW_NoSymbol && _sym.IsVartype(vartype);
                    vartype = _sym[vartype].VartypeD->Parent)
                    if (struct_of_constructor == vartype)
                    {
                        is_call_forbidden = false;
                        break;
                    }
            }

            if (is_call_forbidden)
                UserError(
                    ReferenceMsgSym(
                        "Cannot call this constructor directly as a function",
                        qualified_component).c_str());
        }

        SkipNextSymbol(expression, unqualified_component); // Eat function symbol
        if (kKW_OpenParenthesis != expression.PeekNext())
        {
            // Return the function symbol as-is
            eres.Type = eres.kTY_FunctionName;
            eres.Location = eres.kLOC_SymbolTable;
            eres.Symbol = qualified_component;
            eres.Vartype = kKW_NoSymbol;
            eres.Modifiable = false;
            return;
        }

        AccessData_FunctionCall(qualified_component, expression, eres);
        if (_sym.IsDynarrayVartype(vartype))
            return AccessData_ProcessArrayIndexes(expression, eres);
        return;
    }

    if (_sym.IsVariable(qualified_component))
    {
        if (static_access && !_sym[qualified_component].VariableD->TypeQualifiers[TQ::kStatic])
            UserError(
                ReferenceMsgSym(
                    "Must specify a specific object for non-static component %s",
                    qualified_component).c_str(),
                _sym.GetName(qualified_component).c_str());

        AccessData_StructMember(qualified_component, access_type, access_via_this, expression, eres);
        return AccessData_ProcessArrayIndexes(expression, eres);
    }

    InternalError("Unknown kind of component of '%s'", _sym.GetName(vartype).c_str());
}

AGS::Symbol AGS::Parser::FindStructOfComponent(Vartype strct, Symbol unqualified_component)
{
    while (strct > 0 && _sym.IsVartype(strct))
    {
        auto const &components = _sym[strct].VartypeD->Components;
        if (0u < components.count(unqualified_component))
            return strct;
        strct = _sym[strct].VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

AGS::Symbol AGS::Parser::FindComponentInStruct(Vartype strct, Symbol unqualified_component)
{
    while (strct > 0 && _sym.IsVartype(strct))
    {
        auto const &components = _sym[strct].VartypeD->Components;
        if (0u < components.count(unqualified_component))
            return components.at(unqualified_component);
        strct = _sym[strct].VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

bool AGS::Parser::AccessData_IsClauseLast(SrcList &expression)
{
    size_t const cursor = expression.GetCursor();
    expression.SkipTo(kKW_Dot);
    bool is_last = (kKW_Dot != expression.PeekNext());
    expression.SetCursor(cursor);
    return is_last;
}

void AGS::Parser::AccessData(VariableAccess access_type, SrcList &expression, EvaluationResult &eres)
{
    expression.StartRead();
    if (0u == expression.Length())
        InternalError("Empty expression");
   
    bool implied_dot = false; // only true when "this." or "TYPENAME." is implied
    bool inside_access = false; // true when inside a struct function body
    
    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    AccessData_FirstClause(
        AccessData_IsClauseLast(expression) ? access_type : VAC::kReading,
        expression,
        eres,
        inside_access,
        implied_dot);
    
    Vartype outer_vartype = kKW_NoSymbol;

    // If the previous function has assumed a 'this.' or a 'VARTYPE.' that isn't there,
    // then a '.' won't be coming up but the while body must be executed anyway.
    while (kKW_Dot == expression.PeekNext() || implied_dot)
    {
        if (!implied_dot)
            SkipNextSymbol(expression, kKW_Dot);
        implied_dot = false;

        // Here, if EvaluationResult::kLOC_MemoryAtMAR == eres.location then the first byte of outer is at m[MAR + mar_offset].
        outer_vartype = eres.kTY_StructName == eres.Type? eres.Symbol : eres.Vartype;
        if (eres.kTY_StructName == eres.Type)
        {
            outer_vartype = eres.Symbol; // Static access
        }
        else
        {
            if (_sym.IsDynpointerVartype(eres.Vartype))
            {
                AccessData_Dereference(eres);
                eres.Vartype = _sym.VartypeWithout(VTT::kDynpointer, eres.Vartype);
            }

            bool array_attribute_handled = false;
            if (_sym.IsDynarrayVartype(eres.Vartype) && _sym.FindOrAdd("Length") == expression.PeekNext())
            {
                // Pseudo attribute 'Length' will get the length of the dynarray
                AccessData_Dereference(eres);

                AccessData_GenerateDynarrayLengthAttrib(eres);
                array_attribute_handled = true;
            }

            if (_sym.IsAnyArrayVartype(eres.Vartype))
            {
                if (!array_attribute_handled)
                    UserError("Expected a struct in front of '.' but found an array instead");
            }
            else if (!_sym.IsStructVartype(eres.Vartype))
            {
                UserError(
                    "Expected a struct in front of '.' but found an expression of type '%s' instead",
                    _sym.GetName(outer_vartype).c_str());
            }
        }
        if (expression.ReachedEOF())
            UserError("Expected struct component after '.' but did not find it");

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        AccessData_SubsequentClause(AccessData_IsClauseLast(expression) ? access_type : VAC::kReading, inside_access, expression, eres);

        // Next component access, if there is any, is dependent on the current access, no longer on "this".
        inside_access = false;
    }
}

void AGS::Parser::AccessData_StrCpy(size_t count)
{
    BackwardJumpDest loop_start(_scrip);
    ForwardJump out_of_loop(_scrip);

    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_CX); // CX = dest
    WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_BX); // BX = src
    WriteCmd(SCMD_LITTOREG, SREG_DX, count - 1); // DX = counter
    loop_start.Set();   // Label LOOP_START
    WriteCmd(SCMD_REGTOREG, SREG_BX, SREG_MAR); // AX = m[BX]
    WriteCmd(SCMD_MEMREAD, SREG_AX);
    WriteCmd(SCMD_REGTOREG, SREG_CX, SREG_MAR); // m[CX] = AX
    WriteCmd(SCMD_MEMWRITE, SREG_AX);
    WriteCmd(SCMD_JZ, kDestinationPlaceholder);  // if (AX == 0) jumpto LOOP_END
    out_of_loop.AddParam();
    WriteCmd(SCMD_ADD, SREG_BX, 1); // BX++, CX++, DX--
    WriteCmd(SCMD_ADD, SREG_CX, 1);
    WriteCmd(SCMD_SUB, SREG_DX, 1);
    WriteCmd(SCMD_REGTOREG, SREG_DX, SREG_AX); // if (DX != 0) jumpto LOOP_START
    loop_start.WriteJump(SCMD_JNZ, _src.GetLineno());
    WriteCmd(SCMD_ADD, SREG_CX, 1); // Force a 0-terminated dest string
    WriteCmd(SCMD_REGTOREG, SREG_CX, SREG_MAR);
    WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
    WriteCmd(SCMD_MEMWRITE, SREG_AX);
    out_of_loop.Patch(_src.GetLineno());
    _reg_track.SetAllRegisters();
}

void AGS::Parser::AccessData_AssignTo(SrcList &expression, EvaluationResult const &eres)
{
    // We'll evaluate 'expression' later on, which will
    // move the cursor, so save it here and restore it later
    size_t const end_of_rhs_cursor = _src.GetCursor();

    EvaluationResult rhs_eres = eres;
    if (EvaluationResult::kTY_Literal != rhs_eres.Type)
        EvaluationResultToAx(rhs_eres);

    // Get the LHS of the assignment for writing.
    // Protect the register from being clobbered that contains the result of the RHS.
    EvaluationResult lhs_eres;
    RegisterGuard(
        EvaluationResult::kLOC_AX == rhs_eres.Location ? RegisterList{ SREG_AX } :
        EvaluationResult::kLOC_MemoryAtMAR == rhs_eres.Location ? RegisterList{ SREG_MAR } :
        RegisterList{},
        [&]
        {
            AccessData(VAC::kWriting, expression, lhs_eres);
            if (!expression.ReachedEOF() && lhs_eres.kTY_AttributeName != lhs_eres.Type)
                // Spurious characters follow the LHS in front of the assignment symbol, e.g., 'var 77 = 9;'
                UserError("Unexpected '%s'", _sym.GetName(expression.PeekNext()).c_str());
            
            if (EvaluationResult::kLOC_AX == lhs_eres.Location)
            {
                if (!_sym.IsManagedVartype(lhs_eres.Vartype))
                    UserError("Cannot modify this value");
				
                WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_MAR);
                _reg_track.SetRegister(SREG_MAR);
                WriteCmd(SCMD_CHECKNULL);
                rhs_eres.Location = EvaluationResult::kLOC_MemoryAtMAR;
            }
        });

    EvaluationResultToAx(rhs_eres);

    if (EvaluationResult::kTY_AttributeName == lhs_eres.Type) 
    {
        // We need to call the attribute setter
        Symbol const attribute = lhs_eres.Symbol;

        ConvertAXStringToStringObject(lhs_eres.Vartype, rhs_eres.Vartype);        
        if (IsVartypeMismatch_Oneway(rhs_eres.Vartype, lhs_eres.Vartype))
            UserError(
                ReferenceMsgSym(
                    "Attribute '%s' has type '%s'; cannot assign a type '%s' value to it",
                    attribute).c_str(),
                _sym.GetName(attribute).c_str(),
                _sym.GetName(lhs_eres.Vartype).c_str(),
                _sym.GetName(rhs_eres.Vartype).c_str());

        Vartype struct_of_attribute = _sym[attribute].ComponentD->Parent;
        bool const is_setter = true;
        AccessData_CallAttributeFunc(is_setter, expression, struct_of_attribute);
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return;
    }

    // At this point, the result of the RHS must be moved to the place MAR points to
    _marMgr.UpdateMAR(_src.GetLineno(), _scrip);

    if (kKW_String == lhs_eres.Vartype && kKW_String == _sym.VartypeWithout(VTT::kConst, rhs_eres.Vartype))
    {
        // copy the string contents over.
        AccessData_StrCpy(STRINGBUFFER_LENGTH);
        _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
        return;
    }

    ConvertAXStringToStringObject(lhs_eres.Vartype, rhs_eres.Vartype);
    if (IsVartypeMismatch_Oneway(rhs_eres.Vartype, lhs_eres.Vartype))
        UserError(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhs_eres.Vartype).c_str(),
            _sym.GetName(lhs_eres.Vartype).c_str());

    CodeCell const opcode =
        _sym.IsDynVartype(lhs_eres.Vartype) ?
        SCMD_MEMWRITEPTR : GetWriteCommandForSize(_sym.GetSize(lhs_eres.Vartype));
    WriteCmd(opcode, SREG_AX);
    _reg_track.SetRegister(SREG_AX);
    _src.SetCursor(end_of_rhs_cursor); // move cursor back to end of RHS
}

void AGS::Parser::SkipToEndOfExpression(SrcList &src)
{
    int nesting_depth = 0;

    Symbol const vartype_of_this = _sym.GetVartype(kKW_This);

    // The ':' in an "a ? b : c" construct can also be the end of a label, and in AGS,
    // expressions are allowed for labels. So we must take care that label ends aren't
    // mistaken for expression parts. For this, tern_depth counts the number of
    // unmatched '?' on the outer level. If this is non-zero, then any arriving 
    // ':' will be interpreted as part of a ternary.
    int tern_depth = 0;

    Symbol peeksym;
    while (0 <= (peeksym = src.PeekNext())) // note assignment in while condition
    {
        // Skip over parts that are enclosed in braces, brackets, or parens
        if (kKW_OpenParenthesis == peeksym || kKW_OpenBracket == peeksym || kKW_OpenBrace == peeksym)
            ++nesting_depth;
        else if (kKW_CloseParenthesis == peeksym || kKW_CloseBracket == peeksym || kKW_CloseBrace == peeksym)
            if (--nesting_depth < 0)
                break; // this symbol cannot be part of the current expression
        if (nesting_depth > 0)
        {
            src.GetNext();
            continue;
        }

        if (kKW_As == peeksym || kKW_New == peeksym)
        {   // Only allowed if a type follows   
            SkipNextSymbol(src, peeksym);
            Symbol const following_sym = src.PeekNext();
            if (_sym.IsVartype(following_sym))
            {
                SkipNextSymbol(src, following_sym);
                continue;
            }
            UserError(
                "Expected a type after '%s', found '%s' instead",
                _sym.GetName(peeksym).c_str(),
                _sym.GetName(following_sym).c_str());
        }

        if (kKW_Colon == peeksym)
        {
            // This is only allowed if it can be matched to an open tern
            if (--tern_depth < 0)
                break;

            SkipNextSymbol(src, kKW_Colon);
            continue;
        }

        if (kKW_Dot == peeksym)
        {
            SkipNextSymbol(src, kKW_Dot); 
            src.GetNext(); // Eat following symbol
            continue;
        }

        if (kKW_Null == peeksym)
        {   // Allowed.
            SkipNextSymbol(src, kKW_Null); 
            continue;
        }

        if (kKW_Tern == peeksym)
        {
            tern_depth++;
            SkipNextSymbol(src, kKW_Tern); 
            continue;
        }

        if (_sym.IsVartype(peeksym))
        {   // Only allowed if a dot follows
            SkipNextSymbol(src, peeksym); // Eat the vartype
            Symbol const nextsym = src.PeekNext();
            if (kKW_Dot == nextsym)
                continue; // Do not eat the dot.
            src.BackUp(); // spit out the vartype
            break;
        }

        // Let a symbol through if it can be considered a component of 'this'.
        if (kKW_NoSymbol != vartype_of_this) 
        {
            Symbol const potential_component = _sym.FindStructComponent(vartype_of_this, peeksym);
            if (kKW_NoSymbol != potential_component)
            {
                SkipNextSymbol(src, peeksym);
                continue;
            }
        }

        if (!_sym.CanBePartOfAnExpression(peeksym))
            break;
        SkipNextSymbol(src, peeksym); 
    }

    if (nesting_depth > 0)
        InternalError("Nesting corrupted");
}

void AGS::Parser::ParseExpression(SrcList &src, EvaluationResult &eres)
{
    size_t const expr_start = src.GetCursor();
    SkipToEndOfExpression(src);
    SrcList expression = SrcList(src, expr_start, src.GetCursor() - expr_start);
    Symbol const next_sym = src.PeekNext();
    if (_sym.IsIdentifier(next_sym) &&
        !_sym.IsPredefined(next_sym) &&
        _sym.kNoSrcLocation == _sym.GetDeclared(next_sym))
    {
        UserError("Identifier '%s' is undeclared (did you mis-spell it?)", _sym.GetName(next_sym).c_str());
    }

    if (0u == expression.Length())
        UserError("Expected an expression, found '%s' instead", _sym.GetName(next_sym).c_str());

    size_t const expr_end = src.GetCursor();
    ParseExpression_Term(expression, eres);
    src.SetCursor(expr_end);
    return;
}

void AGS::Parser::ParseIntegerExpression(SrcList &src, EvaluationResult &eres, std::string const &msg)
{
    ParseExpression(src, eres);
    
    return CheckVartypeMismatch(eres.Vartype, kKW_Int, true, msg);
}

void AGS::Parser::ParseDelimitedExpression(SrcList &src, Symbol const opener, EvaluationResult &eres)
{
    Expect(opener, src.GetNext());
    ParseExpression(src, eres);
    Symbol const closer = _sym[opener].DelimeterD->Partner;
    return Expect(closer, src.GetNext());
}

void AGS::Parser::ParseAssignment_ReadLHSForModification(SrcList &expression, EvaluationResult &eres)
{
    AccessData(VAC::kReadingForLaterWriting, expression, eres);
    ParseExpression_CheckUsedUp(expression);
    
    // Also put the value into AX so that it can be read/modified as well as written
    EvaluationResult eres_dummy = eres;
    EvaluationResultToAx(eres_dummy); // Don't clobber eres
}

void AGS::Parser::ParseAssignment_Assign(SrcList &lhs)
{
    SkipNextSymbol(_src, kKW_Assign);
    EvaluationResult eres;
    ParseExpression(_src, eres); // RHS of the assignment
        
    return AccessData_AssignTo(lhs, eres);
}

void AGS::Parser::ParseAssignment_MAssign(Symbol const ass_symbol, SrcList &lhs)
{
    SkipNextSymbol(_src, ass_symbol);

    // Parse RHS
    EvaluationResult rhs_eres;
    ParseExpression(_src, rhs_eres);
    EvaluationResultToAx(rhs_eres);
    PushReg(SREG_AX);

    // Parse LHS (moves the cursor to end of LHS, so save it and restore it afterwards)
    size_t const end_of_rhs_cursor = _src.GetCursor();
    EvaluationResult lhs_eres;
    ParseAssignment_ReadLHSForModification(lhs, lhs_eres);
    _src.SetCursor(end_of_rhs_cursor);

    // Use the operator on LHS and RHS
    CodeCell const opcode = GetOpcode(ass_symbol, lhs_eres.Vartype, rhs_eres.Vartype);
    PopReg(SREG_BX); // Note, we've pushed AX but we're popping BX
    _reg_track.SetRegister(SREG_BX);
    WriteCmd(opcode, SREG_AX, SREG_BX);
    _reg_track.SetRegister(SREG_AX);

    if (EvaluationResult::kLOC_MemoryAtMAR == lhs_eres.Location)
    {
        // Shortcut: Write the result directly back to memory
        CodeCell memwrite = GetWriteCommandForSize(_sym.GetSize(lhs_eres.Vartype));
        WriteCmd(memwrite, SREG_AX);
        return;
    }

    AccessData_AssignTo(lhs, rhs_eres);   
}

void AGS::Parser::ParseVardecl_InitialValAssignment_IntOrFloatVartype(Vartype const wanted_vartype, std::vector<char> &initial_val)
{
    EvaluationResult eres;
    // Parse an expression making sure that no code is generated,
    // not even LINUM directives (we are outside of a function body)
    RestorePoint rp(_scrip);
    ParseExpression(_src, eres);
    rp.Restore(false);
    
    if (eres.kTY_Literal != eres.Type)
        UserError("Cannot evaluate this expression at compile time, it cannot be used as initializer");

    CodeCell const litval = _sym[eres.Symbol].LiteralD->Value;

    if ((kKW_Float == wanted_vartype) != (kKW_Float == eres.Vartype))
        UserError(
            "Expected a '%s' value as an initializer but found a '%s' value instead",
            _sym.GetName(wanted_vartype).c_str(),
            _sym.GetName(eres.Vartype).c_str());
    
    size_t const wanted_size = _sym.GetSize(wanted_vartype);
    initial_val.resize(wanted_size);
    switch (wanted_size)
    {
    default:
        UserError("Cannot give an initial value to a variable of type '%s' here", _sym.GetName(wanted_vartype).c_str());
        return;
    case 1u:
        initial_val[0u] = litval;
        return;
    case 2u:
        (reinterpret_cast<int16_t *> (initial_val.data()))[0] = litval;
        return;
    case 4u:
        (reinterpret_cast<int32_t *> (initial_val.data()))[0] = litval;
        return;
    }
}

void AGS::Parser::ParseVardecl_InitialValAssignment_ArrayOrStringBuf_Literal(Symbol string_lit, size_t const available_space, std::vector<char> &initial_val)
{
    // Get the relevant characters from the strings table
    std::string const lit_value = &(_scrip.strings[_sym[string_lit].LiteralD->Value]);

    // '- 1u' to leave space for the terminating '\0'
    if (lit_value.length() > available_space - 1u)
        UserError(
            "Initializing string literal has %u chars and is too long "
            "(available space: %u chars)",
            lit_value.length(),
            available_space - 1u);

    initial_val.assign(lit_value.begin(), lit_value.end());
    initial_val.resize(available_space);
}

void AGS::Parser::ParseVardecl_InitialValAssignment_Struct(Vartype const vartype, std::vector<char> &initial_val)
{
    // Data that we need to track for each (direct or indirect) component of 'vartype'
    struct ComponentFields
    {
        Symbol Component = kKW_NoSymbol; // without the qualifier
        Symbol QualifiedName = kKW_NoSymbol;
        size_t Offset = 0u;
        AGS::Vartype Vartype = kKW_NoSymbol;
        bool IsFunction = false;
        bool IsAttribute = false;
        std::vector<char> InitialVal = {};
        size_t InitialValDeclared = SIZE_MAX;
    };
    std::vector<ComponentFields> fields = {};

    SymbolList components;
    _sym.GetComponentsOfStruct(vartype, components);

    for (size_t idx = 0u; idx < components.size(); idx++)
    {
        auto &compo = components[idx];
        ComponentFields cf;
        cf.Component = _sym[compo].ComponentD->Component;
        cf.QualifiedName = compo;
        cf.Offset = _sym[compo].ComponentD->Offset;
        cf.Vartype = _sym[compo].VariableD ? _sym[compo].VariableD->Vartype : kKW_NoSymbol;
        cf.InitialVal = {};
        cf.InitialValDeclared = 0u;
        cf.IsAttribute = (nullptr != _sym[compo].AttributeD);
        cf.IsFunction = (nullptr != _sym[compo].FunctionD);
        fields.push_back(cf);
    }

    Expect(kKW_OpenBrace, _src.GetNext());
    Symbol sym = _src.GetNext();
    while (sym != kKW_CloseBrace)
    {
        // Find the entry in 'fields' where 'Component' equals 'sym'
        auto found_it = std::find_if(
            fields.begin(),
            fields.end(),
            [&](ComponentFields const &cf) { return cf.Component == sym; });

        if (fields.end() == found_it) // didn't find in the list
        {
            if (!_sym.IsIdentifier(sym))
                UserError(
                    "Expected an identifier as a component of '%s' but found '%s",
                    _sym.GetName(vartype).c_str(),
                    _sym.GetName(sym).c_str());
            UserError(
                ReferenceMsgSym(
                    "Cannot find '%s' in struct '%s'", vartype).c_str(),
                _sym.GetName(sym).c_str(),
                _sym.GetName(vartype).c_str());
        }
        if (found_it->IsAttribute)
            UserError(
                ReferenceMsgSym(
                    "Cannot initialize the attribute '%s'", vartype).c_str(),
                _sym.GetName(vartype).c_str(),
                _sym.GetName(found_it->QualifiedName).c_str());
        if (found_it->IsFunction)
            UserError(
                ReferenceMsgSym(
                    "Cannot initialize the function '%s'", vartype).c_str(),
                _sym.GetName(vartype).c_str(),
                _sym.GetName(found_it->QualifiedName).c_str());
        if (!found_it->InitialVal.empty())
            UserError(
                ReferenceMsgLoc(
                    "Component '%s' has already been initialized",
                    found_it->InitialValDeclared).c_str(),
                _sym.GetName(found_it->QualifiedName).c_str());

        Expect(kKW_Colon, _src.GetNext());
        found_it->InitialValDeclared = _src.GetCursor();
        ParseVardecl_InitialValAssignment(found_it->Vartype, found_it->InitialVal);
        sym = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseBrace }, sym);
        if (kKW_Comma == sym)
            sym = _src.GetNext();
    }

    // Sort the fields in ascending order of 'Offset'
    std::sort(
        fields.begin(),
        fields.end(),
        [](ComponentFields const &cf1, ComponentFields const &cf2) { return cf1.Offset < cf2.Offset; });

    initial_val.clear();

    for (auto it = fields.begin(); it != fields.end(); it++)
    {
        if (it->IsAttribute || it->IsFunction)
            continue; // Nothing to initialize
        if (it->InitialVal.empty())
            continue; // Skip fields that don't have an initialization
        // Everything between the end of the preceding initialization and the current offset
        // hasn't been specified, so set it to zeros.
        initial_val.resize(it->Offset);
        
        // Append the current initial value here
        initial_val.insert(
            initial_val.end(),
            it->InitialVal.begin(),
            it->InitialVal.end());
    }
    // Fields at the end of this struct might not be initialized yet,
    // so fill up to the size of this struct.
    initial_val.resize(_sym.GetSize(vartype));
}

bool AGS::Parser::ParseVardecl_InitialValAssignment_PeekArrayNamed()
{
    // Get the index 'N:'
    EvaluationResult eres;
    size_t const array_index_start = _src.GetCursor();
    // Parse an integer expression making sure that no code is generated
    // not even LINUM directives (we are outside of a function body)
    RestorePoint rp(_scrip);
    ParseExpression(_src, eres);
    rp.Restore(false);
    const bool is_named_assignment = (EvaluationResult::kLOC_SymbolTable == eres.Location
        && _sym.IsLiteral(eres.Symbol)
        && _src.PeekNext() == kKW_Colon);
    _src.SetCursor(array_index_start);
    return is_named_assignment;
}

void AGS::Parser::ParseVardecl_InitialValAssignment_Array_Named(size_t const first_dim_size, Vartype const el_vartype, std::vector<char> &initial_val)
{
    struct init_record
    {
        size_t Declared = SIZE_MAX; // Where in _src the initial value was specified
        std::vector<char> InitialVal = {};
    };
    std::unordered_map<size_t, init_record> inits;

    while (kKW_CloseBrace != _src.PeekNext())
    {
        // Get the index 'N:'
        EvaluationResult eres;
        size_t const array_index_start = _src.GetCursor();
        // Parse an integer expression making sure that no code is generated
        // not even LINUM directives (we are outside of a function body)
        RestorePoint rp(_scrip);
        ParseIntegerExpression(_src, eres, "Expected an array index");
        rp.Restore(false);
        if (EvaluationResult::kLOC_SymbolTable != eres.Location)
        {
            _src.SetCursor(array_index_start);
            UserError(
                "Cannot evaluate the array index expression at compile time that starts with '%s'",
                _sym.GetName(_src.PeekNext()).c_str());
        }
        if (!_sym.IsLiteral(eres.Symbol))
            InternalError("Cannot retrieve literal '%s'", _sym.GetName(eres.Symbol).c_str());
        int const idx_as_int = _sym[eres.Symbol].LiteralD->Value;
        if (idx_as_int < 0)
            UserError("Array index '%d' is too low (minimum is '0')", idx_as_int);
        size_t const idx = static_cast<size_t>(idx_as_int);
        if (idx >= first_dim_size)
            UserError("Array index '%u' is too high (maximum is '%u')", idx, first_dim_size - 1u);
        if (inits.count(idx))
            UserError(
                ReferenceMsgLoc(
                    "The value of field '%u' has already been specified",
                    inits[idx].Declared).c_str(),
                idx);
        
        Expect(kKW_Colon, _src.GetNext());

        // Get the value and write a record to 'inits'
        init_record current_record;
        current_record.Declared = array_index_start;
        ParseVardecl_InitialValAssignment(el_vartype, current_record.InitialVal);
        inits[idx] = current_record;

        // Handle the separators
        Symbol const comma_or_close = _src.PeekNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseBrace, }, comma_or_close);
        if (kKW_Comma == comma_or_close)
            SkipNextSymbol(_src, kKW_Comma);
    }
    SkipNextSymbol(_src, kKW_CloseBrace);

    // Stitch together the fields,
    // using binary zeros wherever a field hasn't been specified.
    size_t const el_size = _sym.GetSize(el_vartype);
    auto const all_binary_zeros = std::vector<char>(el_size, '\0');
    initial_val.clear();
    initial_val.reserve(el_size * first_dim_size);
    for (size_t dim_idx = 0u; dim_idx < first_dim_size; ++dim_idx)
    {
        auto const &the_field_to_append =
            inits.count(dim_idx) ? inits[dim_idx].InitialVal : all_binary_zeros;
        initial_val.insert(
            initial_val.end(), 
            the_field_to_append.cbegin(), the_field_to_append.cend());
    }
}

void AGS::Parser::ParseVardecl_InitialValAssignment_Array_Sequence(size_t const first_dim_size, Vartype const el_vartype, std::vector<char> &initial_val)
{
    initial_val.clear();

    for (size_t dim_idx = 0u; kKW_CloseBrace != _src.PeekNext(); ++dim_idx)
    {
        if (first_dim_size == dim_idx)
            UserError("Expected at most %u array fields, found more", first_dim_size);

        if (kKW_OpenBracket == _src.PeekNext())
            UserError(
                "Expected a value, found '[' instead "
                "(cannot mix named and unnamed array fields within the same '{...}'");
        std::vector<char> el_initial_val;
        ParseVardecl_InitialValAssignment(el_vartype, el_initial_val);
        initial_val.insert(
            initial_val.end(),
            el_initial_val.begin(), el_initial_val.end());

        Symbol const comma_or_close = _src.PeekNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseBrace, }, comma_or_close);
        if (kKW_Comma == comma_or_close)
            SkipNextSymbol(_src, kKW_Comma);
    }
    SkipNextSymbol(_src, kKW_CloseBrace);

    // Initialize all following elements with binary zeros
    initial_val.resize(first_dim_size * _sym.GetSize(el_vartype));
}

void AGS::Parser::ParseVardecl_InitialValAssignment_Array(Vartype const vartype, std::vector<char> &initial_val)
{
    Symbol const next_sym = _src.GetNext();
    if (_sym.IsLiteral(next_sym) && _sym.VartypeWithConst(kKW_String) == _sym[next_sym].LiteralD->Vartype)
    {
        Vartype const base_vartype = _sym[vartype].VartypeD->BaseVartype;
        if (kKW_Char != base_vartype)
            UserError(
                "Cannot initialize an array of '%s' with a string literal",
                _sym.GetName(base_vartype).c_str());
        size_t const dims_count = _sym.ArrayDimensionsCount(vartype);
        if (dims_count > 1u)
            UserError(
                "Cannot initialize a multi-dimensional array with a string literal");

        ParseVardecl_InitialValAssignment_ArrayOrStringBuf_Literal(
            next_sym,
            _sym.ArrayElementsCount(vartype),
            initial_val);
        return;
    }
    Expect(kKW_OpenBrace, next_sym, "Expected '{' or a string literal");

    // Split the first dimension off the array vartype
    size_t const first_dim_size = _sym[vartype].VartypeD->Dims.at(0u);
    Vartype const el_vartype = _sym.ArrayVartypeWithoutFirstDim(vartype);

    if (ParseVardecl_InitialValAssignment_PeekArrayNamed())
        return ParseVardecl_InitialValAssignment_Array_Named(first_dim_size, el_vartype, initial_val);
    return ParseVardecl_InitialValAssignment_Array_Sequence(first_dim_size, el_vartype, initial_val);
}

void AGS::Parser::ParseVardecl_InitialValAssignment_StringBuf(std::vector<char> &initial_val)
{
    Symbol string_lit = _src.GetNext();
    if (_sym.IsConstant(string_lit))
        string_lit = _sym[string_lit].ConstantD->ValueSym;
    
    if (!_sym.IsLiteral(string_lit) ||
        _sym.VartypeWithConst(kKW_String) != _sym[string_lit].LiteralD->Vartype)
        UserError("Expected a string literal after '=', found '%s' instead", _sym.GetName(_src.PeekNext()).c_str());

    ParseVardecl_InitialValAssignment_ArrayOrStringBuf_Literal(string_lit, STRINGBUFFER_LENGTH, initial_val);
}

void AGS::Parser::ParseVardecl_InitialValAssignment(Vartype vartype, std::vector<char> &initial_val)
{
    if (_sym.IsDynarrayVartype(vartype))
    {
        // We only reserve the space for the pointer.
        // The space for the actual object will be reserved with an AGS 'new' expression.
        initial_val.resize (SIZE_OF_DYNPOINTER);

        if (kKW_Null != _src.GetNext())
            UserError("Can only initialize this global dynamic array as 'null'");
        return;
    }

    if (_sym.IsDynpointerVartype(vartype))
    {
        // We only reserve the space for the pointer.
        // The space for the actual object will be reserved with an AGS 'new' expression.
        initial_val.resize (SIZE_OF_DYNPOINTER);

        if (kKW_Null != _src.GetNext())
            UserError("Can only initialize this global managed variable as 'null'");
        return;
    }

    if (_sym.IsStructVartype(vartype))
        return ParseVardecl_InitialValAssignment_Struct(vartype, initial_val);

    if (_sym.IsArrayVartype(vartype))
        return ParseVardecl_InitialValAssignment_Array(vartype, initial_val);

    if (kKW_String == vartype)
        return ParseVardecl_InitialValAssignment_StringBuf(initial_val);

    if (_sym.IsAnyIntegerVartype(vartype) || kKW_Float == vartype)
        return ParseVardecl_InitialValAssignment_IntOrFloatVartype(vartype, initial_val);

    UserError(
        "Type '%s' cannot be initialized",
        _sym.GetName(vartype).c_str());
}

void AGS::Parser::ParseVardecl_Var2SymTable(Vartype const vartype, Symbol const var_name)
{
    SymbolTableEntry &var_entry = _sym[var_name];
    _sym.MakeEntryVariable(var_name);
    var_entry.VariableD->Vartype = vartype;
    var_entry.Scope = _nest.TopLevel();
    var_entry.LifeScope = std::make_pair(_scrip.Codesize_i32(), _scrip.Codesize_i32());
    _sym.SetDeclared(var_name, _src.GetCursor());
}

void AGS::Parser::ParseStruct_ConstantDefn(Symbol const name_of_struct)
{
    if (PP::kMain != _pp)
    {
        _src.SkipTo(kKW_Semicolon);
        Expect(kKW_Semicolon, _src.GetNext());
        return;
    }

    Vartype const vartype = ParseVartype(_src);
    if (kKW_Int != vartype && kKW_Float != vartype)
        UserError("Can only handle compile-time constants of type 'int' or 'float' but found '%s' instead",
            _sym.GetName(vartype).c_str());

    bool const in_struct = (kKW_NoSymbol != name_of_struct);

    while (true)
    {
        Symbol const unqualified_component = ParseVarname();
        Symbol const qualified_component =
            in_struct ? 
                MangleStructAndComponent(name_of_struct, unqualified_component) : unqualified_component;

        if (PP::kMain == _pp && in_struct)
        {
            if (_sym.IsInUse(qualified_component))
                UserError(
                    ReferenceMsgSym("'%s' is already defined", qualified_component).c_str(),
                    _sym.GetName(qualified_component).c_str());
        
            // Mustn't be in any ancester
            Symbol const parent = FindStructOfComponent(name_of_struct, unqualified_component);
            if (kKW_NoSymbol != parent)
                UserError(
                    ReferenceMsgSym(
                        "The struct '%s' extends '%s', and '%s' is already defined",
                        parent).c_str(),
                    _sym.GetName(name_of_struct).c_str(),
                    _sym.GetName(parent).c_str(),
                    _sym.GetName(qualified_component).c_str());
        }
        if (!in_struct && _nest.TopLevel() > 1u)
            ParseVardecl_CheckAndStashOldDefn(qualified_component);

        if (_src.PeekNext() == kKW_OpenBracket)
            UserError("Cannot define a compile-time constant array");
        if (kKW_OpenParenthesis == _src.PeekNext())
            UserError("Cannot define a compile-time constant function (did you mean 'readonly'?)");

            Expect(kKW_Assign, _src.GetNext());

        Symbol const lit = ParseConstantExpression(_src);
        CheckVartypeMismatch(_sym[lit].LiteralD->Vartype, vartype, true, "");

        _sym.MakeEntryConstant(qualified_component);
        SymbolTableEntry &entry = _sym[qualified_component];
        entry.ConstantD->ValueSym = lit;
        _sym.SetDeclared(qualified_component, _src.GetCursor());

        if (in_struct)
        {
            _sym.MakeEntryComponent(qualified_component);
            entry.ComponentD->Component = unqualified_component;
            entry.ComponentD->Parent = name_of_struct;
            _sym[name_of_struct].VartypeD->Components[unqualified_component] = qualified_component;
        }
        else
        {
            entry.Scope = _nest.TopLevel();
        }

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Semicolon == punctuation)
            return;
    }
}

void AGS::Parser::ParseConstantDefn()
{
    ParseStruct_ConstantDefn(kKW_NoSymbol);
}

void AGS::Parser::ParseVardecl_CheckIllegalCombis(Vartype vartype, ScopeType scope_type)
{
    if (vartype == kKW_String && !FlagIsSet(_options, SCOPT_OLDSTRINGS))
        UserError("Variables of type 'string' aren't supported any longer (use the type 'String' instead)");
    if (vartype == kKW_String && ScT::kImport == scope_type)
        // cannot import because string is really char *, and the pointer won't resolve properly
        UserError("Cannot import a 'string' variable; use 'char[]' instead");
    if (vartype == kKW_Void)
        UserError("'void' is not a valid type in this context");
}

void AGS::Parser::ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info, bool body_follows)
{
    // Note, a variable cannot be declared if it is in use as a constant.
    if (nullptr != known_info->FunctionD)
        UserError(
            ReferenceMsgLoc(
				"The name '%s' is declared as a function elsewhere, as a variable here", 
				known_info->Declared).c_str(),
            known_info->Name.c_str());
    if (nullptr != known_info->VartypeD)
        UserError(
            ReferenceMsgLoc(
				"The name '%s' is declared as a type elsewhere, as a variable here", 
				known_info->Declared).c_str(),
            known_info->Name.c_str());
    
    if (nullptr == known_info->VariableD)
        return; // We don't have any known info

    if (known_info->VariableD->Vartype != this_entry->VariableD->Vartype)
        // This will check the array lengths, too
        UserError(
            ReferenceMsgLoc(
                "The variable '%s' is declared as '%s' here, as '%s' elsewhere",
                known_info->Declared).c_str(),
            known_info->Name.c_str(),
            _sym.GetName(this_entry->VariableD->Vartype).c_str(),
            _sym.GetName(known_info->VariableD->Vartype).c_str());

    // Note, if the variables have the same vartype, they must also have the same size because size is a vartype property.

    TypeQualifierSet known_tq = known_info->VariableD->TypeQualifiers;
    known_tq[TQ::kImport] = false;
    TypeQualifierSet this_tq = this_entry->VariableD->TypeQualifiers;
    this_tq[TQ::kImport] = false;
    if (known_tq != this_tq)
    {
        std::string const ki_tq = TypeQualifierSet2String(known_tq);
        std::string const te_tq = TypeQualifierSet2String(this_tq);
        std::string msg = ReferenceMsgLoc(
            "The variable '%s' has the qualifiers '%s' here, but '%s' elsewhere",
            known_info->Declared);
        UserError(msg.c_str(), known_info->Name.c_str(), te_tq.c_str(), ki_tq.c_str());
    }
}

void AGS::Parser::ParseVardecl_Import(Symbol var_name)
{
    if (kKW_Assign == _src.PeekNext())
        UserError("Imported variables cannot have any initial assignment");

    if (_givm[var_name])
    {
        // This isn't really an import, so reset the flag and don't mark it for import
        _sym[var_name].VariableD->TypeQualifiers[TQ::kImport] = false;
        return;
    }

    _sym[var_name].VariableD->TypeQualifiers[TQ::kImport] = true;
    int const import_offset = _scrip.FindOrAddImport(_sym.GetName(var_name));
    if (import_offset < 0)
        InternalError("Import table overflow");

    _sym[var_name].VariableD->Offset = static_cast<size_t>(import_offset);
}

void AGS::Parser::ParseVardecl_Global(Symbol var_name, Vartype vartype)
{
    size_t const vartype_size = _sym.GetSize(vartype);
    std::vector<char> initial_val;

    if (kKW_Assign == _src.PeekNext())
    {
        SkipNextSymbol(_src, kKW_Assign);
        ParseVardecl_InitialValAssignment(vartype, initial_val);
    }
    else
    {
        initial_val.insert(initial_val.begin(), _sym.GetSize(vartype), '\0');
    }
    
    SymbolTableEntry &entry = _sym[var_name];
    entry.VariableD->Vartype = vartype;
    int const global_offset = _scrip.AddGlobal(vartype_size, initial_val.data());
    if (global_offset < 0)
        InternalError("Cannot allocate global variable");

    entry.VariableD->Offset = static_cast<size_t>(global_offset);
}

void AGS::Parser::ParseVardecl_Local(Symbol var_name, Vartype vartype)
{
    if (!_nest.DeadEndWarned() && _nest.JumpOutLevel() < _nest.TopLevel())
    {
        Warning("Code execution cannot reach this point");
        _nest.DeadEndWarned() = true;
    }

    size_t const var_size = _sym.GetSize(vartype);
    bool const is_dyn = _sym.IsDynVartype(vartype);

    _sym[var_name].VariableD->Offset = _scrip.OffsetToLocalVarBlock;

    if (kKW_Assign != _src.PeekNext())
    {
        if (0u == var_size)
            return;

        // Initialize the variable with binary zeroes.
        if (SIZE_OF_STACK_CELL == var_size && !is_dyn)
        {
            WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
            _reg_track.SetRegister(SREG_AX);
            PushReg(SREG_AX);
            return;
        }

        WriteCmd(SCMD_LOADSPOFFS, 0);
        _reg_track.SetRegister(SREG_MAR);
        if (is_dyn)
            WriteCmd(SCMD_MEMZEROPTR);
        else 
            WriteCmd(SCMD_ZEROMEMORY, var_size);
        WriteCmd(SCMD_ADD, SREG_SP, var_size);
        _scrip.OffsetToLocalVarBlock += var_size;
        return;
    }

    // "readonly" vars cannot be assigned to, so don't use standard assignment function here.
    SkipNextSymbol(_src, kKW_Assign); 
    EvaluationResult rhs_eres;
    ParseExpression(_src, rhs_eres);
    EvaluationResultToAx(rhs_eres);
    
    // Vartypes must match. This is true even if the lhs is readonly.
    // As a special case, a string may be assigned a const string
    //  because the const string will be copied, not modified.
    Vartype rhsvartype = rhs_eres.Vartype;
    Vartype const lhsvartype = vartype;

    if (IsVartypeMismatch_Oneway(rhsvartype, lhsvartype) &&
        !(kKW_String == _sym.VartypeWithout(VTT::kConst, rhsvartype) &&
          kKW_String == _sym.VartypeWithout(VTT::kConst, lhsvartype)))
    {
        UserError(
            "Cannot assign a type '%s' value to a type '%s' variable",
            _sym.GetName(rhsvartype).c_str(),
            _sym.GetName(lhsvartype).c_str());
    }

    if (SIZE_OF_INT == var_size && !is_dyn)
    {
        // This PUSH moves the result of the initializing expression into the
        // new variable and reserves space for this variable on the stack.
        PushReg(SREG_AX);
        return;
    }

    ConvertAXStringToStringObject(vartype, rhsvartype);
    WriteCmd(SCMD_LOADSPOFFS, 0);
    _reg_track.SetRegister(SREG_MAR);
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, lhsvartype))
        AccessData_StrCpy(STRINGBUFFER_LENGTH);
    else
        WriteCmd(
            is_dyn ? SCMD_MEMWRITEPTR : GetWriteCommandForSize(var_size),
            SREG_AX);
    WriteCmd(SCMD_ADD, SREG_SP, var_size);
    _scrip.OffsetToLocalVarBlock += var_size;
}

void AGS::Parser::ParseVardecl0(Symbol var_name, Vartype vartype, ScopeType scope_type, TypeQualifierSet tqs)
{
    if (kKW_OpenBracket == _src.PeekNext())
		ParseArrayDecl(var_name, vartype);

    // Don't warn for builtins or imports, they might have been predefined
    if (!_sym.IsBuiltinVartype(vartype) && ScT::kImport != scope_type && 0u == _sym.GetSize(vartype))
        Warning(
            ReferenceMsgSym("Variable '%s' has zero size", vartype).c_str(),
            _sym.GetName(var_name).c_str());

    // Enter the variable into the symbol table
    ParseVardecl_Var2SymTable(vartype, var_name);
    _sym[var_name].VariableD->TypeQualifiers = tqs;

    switch (scope_type)
    {
    default:
        return InternalError("Wrong scope type");

    case ScT::kGlobal:
        return ParseVardecl_Global(var_name, vartype);

    case ScT::kImport:
        return ParseVardecl_Import(var_name);

    case ScT::kLocal:
        ParseVardecl_Local(var_name, vartype);
        // Assign local's lifescope to the post stack allocation
        _sym[var_name].LifeScope = std::make_pair(_scrip.Codesize_i32(), _scrip.Codesize_i32());
        return;
    }
}

void AGS::Parser::ParseVardecl_CheckAndStashOldDefn(Symbol var_name)
{
    do // exactly 1 times
    {
        if (_sym.IsPredefined(var_name))
            UserError("Cannot redefine the predefined '%s'", _sym.GetName(var_name).c_str());

        if (_sym.IsVartype(var_name))
            UserError(
                ReferenceMsgSym("'%s' is already in use as a type", var_name).c_str(),
                _sym.GetName(var_name).c_str());

        if (_sym.IsFunction(var_name))
        {
            Warning(
                ReferenceMsgSym("This hides the function '%s()'", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            break;
        }

        if (_sym.IsVariable(var_name))
            break;
        
        // Local compile-time constants can be overridden, but global constants can't
        if (_sym.IsConstant(var_name) && _sym[var_name].Scope > 0u)
            break;

        if (!_sym.IsInUse(var_name))
            break;

        UserError(
            ReferenceMsgSym("'%s' is already in use elsewhere", var_name).c_str(),
            _sym.GetName(var_name).c_str());
    }
    while (false);

    size_t const declared_var_scope = _sym[var_name].Scope;
    if (SymbolTable::kParameterScope == declared_var_scope &&
        (_nest.TopLevel() == SymbolTable::kParameterScope || _nest.TopLevel() == SymbolTable::kFunctionScope))
        UserError(
            ReferenceMsgSym("'%s' has already been declared as a parameter", var_name).c_str(),
            _sym.GetName(var_name).c_str());
    size_t const top_level = _nest.TopLevel();
    if (top_level == declared_var_scope)
        UserError(
            ReferenceMsgSym("'%s' has already been declared in this scope", var_name).c_str(),
            _sym.GetName(var_name).c_str());
    // Can't have 'for(int x;…) { int x; }'
    // It's a bit cumbersome to theck for this:
    // The 'for' nesting itself has an internal 'while' nesting.
    // This 'while' nesting must have a '{}' body because a declaration can't be the sole content of a 'for'.
    // So offending 'for' header declarations must be 2 levels higher up.
    if (top_level >= 2 &&
        top_level - 2 == declared_var_scope &&
        NSType::kFor == _nest.Type(declared_var_scope) &&
        NSType::kBraces == _nest.Type())
    {
        UserError(
            ReferenceMsgSym("'%s' has already been declared in the 'for' clause of this scope", var_name).c_str(),
            _sym.GetName(var_name).c_str());
    }

    if (_nest.AddOldDefinition(var_name, _sym[var_name]))
        InternalError("AddOldDefinition: Storage place occupied");
    _sym[var_name].Clear();
}

void AGS::Parser::ParseVardecl(TypeQualifierSet tqs, Vartype vartype, Symbol var_name, ScopeType scope_type)
{
    ParseVardecl_CheckIllegalCombis(vartype, scope_type);
    
    if (ScT::kLocal == scope_type)
		ParseVardecl_CheckAndStashOldDefn(var_name);

    SymbolTableEntry known_info = _sym[var_name];
    ParseVardecl0(var_name, vartype, scope_type, tqs);
    if (ScT::kLocal != scope_type)
        return ParseVardecl_CheckThatKnownInfoMatches(&_sym[var_name], &known_info, false);
}

void AGS::Parser::ParseFuncBodyStart(Symbol const struct_of_func, Symbol const name_of_func)
{
    _currentlyCompiledFunction = name_of_func;

    _nest.Push(NSType::kFunction);

    // write base address of function for any relocation needed later
    WriteCmd(SCMD_THISBASE, _scrip.Codesize_i32());
    SymbolTableEntry &entry = _sym[name_of_func];
    if (entry.FunctionD->NoLoopCheck)
        WriteCmd(SCMD_LOOPCHECKOFF);

    // If there are dynpointer parameters, then the caller has simply "pushed" them onto the stack.
    // We catch up here by reading each dynpointer and writing it again using MEMINITPTR
    // to declare that the respective cells will from now on be used for dynpointers.
    size_t const params_count = _sym.FuncParamsCount(name_of_func);
    for (size_t param_idx = 1u; param_idx <= params_count; param_idx++) // skip return value param_idx == 0u
    {
        Vartype const param_vartype = _sym[name_of_func].FunctionD->Parameters[param_idx].Vartype;
        if (!_sym.IsDynVartype(param_vartype))
            continue;

        // The return address is on top of the stack, so the nth param is at (n+1)th position
        WriteCmd(SCMD_LOADSPOFFS, SIZE_OF_STACK_CELL * (param_idx + 1));
        _reg_track.SetRegister(SREG_MAR);
        WriteCmd(SCMD_MEMREAD, SREG_AX); // Read the address that is stored there
        _reg_track.SetRegister(SREG_AX);
        // Create a dynpointer that points to the same object as m[AX] and store it in m[MAR]
        WriteCmd(SCMD_MEMINITPTR, SREG_AX);
    }

    SymbolTableEntry &this_entry = _sym[kKW_This];
    this_entry.VariableD->Vartype = kKW_NoSymbol;
    if (struct_of_func > 0)
    {
        // Declare "this" but do not allocate memory for it
        this_entry.Scope = 0u;
        this_entry.Accessed = true; 
        this_entry.VariableD->Vartype = struct_of_func; // Don't declare this as dynpointer
        this_entry.VariableD->TypeQualifiers = {};
        this_entry.VariableD->TypeQualifiers[TQ::kReadonly] = true;
        this_entry.VariableD->TypeQualifiers[TQ::kStatic] = _sym[name_of_func].FunctionD->TypeQualifiers[TQ::kStatic];
        this_entry.VariableD->Offset = 0u;
    }
}

void AGS::Parser::HandleEndOfFuncBody(Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    _currentlyCompiledFunction = kKW_NoSymbol;

    bool const dead_end = _nest.JumpOutLevel() <= _sym.kParameterScope;

    if (!dead_end)
    {
        // Free all the dynpointers in parameters and locals. 
        FreeDynpointersOfLocals(1u);
        // Pop the local variables proper from the stack but leave the parameters.
        // This is important because the return address is directly above the parameters;
        // we need the return address to return. (The caller will pop the parameters later.)
        RemoveLocalsFromStack(_sym.kFunctionScope);
    }
    // All the function variables, _including_ the parameters, become invalid.
    RestoreLocalsFromSymtable(_sym.kParameterScope);

    if (!dead_end)
    {
        // Function has ended. Set AX to 0 unless the function doesn't return any value.
        Vartype const return_vartype = _sym[name_of_current_func].FunctionD->Parameters.at(0u).Vartype;
        if (kKW_Void != return_vartype)
        {
            WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
            _reg_track.SetRegister(SREG_AX);
        }

        if (kKW_Void != return_vartype &&
            !_sym.IsAnyIntegerVartype(return_vartype))
            // This function needs to return a value, the default '0' isn't suitable
            Warning("Code execution may reach this point and the default '0' return isn't suitable (did you forget a 'return' statement?)");
        WriteCmd(SCMD_RET);
    }

    // Save the end of current function's lifescope
    _sym[name_of_current_func].LifeScope.second = _scrip.Codesize_i32();

    // We've just finished the body of the current function.
    name_of_current_func = kKW_NoSymbol;
    struct_of_current_func = _sym[kKW_This].VariableD->Vartype = kKW_NoSymbol;
    
    _nest.Pop();    // End function variables nesting
    _nest.Pop();    // End function parameters nesting
   
    // This has popped the return address from the stack, 
    // so adjust the offset to the start of the parameters.
    _scrip.OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;
}

void AGS::Parser::ParseStruct_GenerateForwardDeclError(Symbol stname, TypeQualifierSet tqs, TypeQualifier tq, VartypeFlag vtf)
{
    // Name of the type qualifier, e.g. 'builtin'
    std::string const tq_name =
        _sym.GetName(tqs.TQ2Symbol(tq));

    std::string const struct_name =
        _sym.GetName(stname).c_str();

    std::string const msg =
        tqs[tq] ?
        "Struct '%s' is '%s' here, not '%s' in a declaration elsewhere" :
        "Struct '%s' is not '%s' here, '%s' in a declaration elsewhere";

    UserError(
        ReferenceMsgSym(msg.c_str(), stname).c_str(),
        struct_name.c_str(),
        tq_name.c_str(),
        tq_name.c_str());
}

void AGS::Parser::ParseStruct_CheckForwardDecls(Symbol stname, TypeQualifierSet tqs)
{
    if (!_sym.IsVartype(stname))
        return;

    SymbolTableEntry &entry = _sym[stname];
    auto &flags = _sym[stname].VartypeD->Flags;
    if (flags[VTF::kAutoptr] != tqs[TQ::kAutoptr])
        return ParseStruct_GenerateForwardDeclError(stname, tqs, TQ::kAutoptr, VTF::kAutoptr);
    if (flags[VTF::kBuiltin] != tqs[TQ::kBuiltin])
        return ParseStruct_GenerateForwardDeclError(stname, tqs, TQ::kBuiltin, VTF::kBuiltin);
    if (!tqs[TQ::kManaged])
        UserError(
            ReferenceMsgSym(
                "The struct '%s' has been forward-declared, so it must be 'managed'",
                stname).c_str(),
            _sym.GetName(stname).c_str());
}

void AGS::Parser::ParseStruct_SetTypeInSymboltable(Symbol stname, TypeQualifierSet tqs)
{
    SymbolTableEntry &entry = _sym[stname];
    _sym.MakeEntryVartype(stname);

    entry.VartypeD->Parent = kKW_NoSymbol;
    entry.VartypeD->Size = 0u;
    entry.Declared = _src.GetCursor();

    auto &flags = entry.VartypeD->Flags;
    flags[VTF::kUndefined] = true; // Not completely defined yet
    flags[VTF::kStruct] = true;
    if (tqs[TQ::kManaged])
        flags[VTF::kManaged] = true;
    if (tqs[TQ::kBuiltin])
        flags[VTF::kBuiltin] = true;
    if (tqs[TQ::kAutoptr])
        flags[VTF::kAutoptr] = true;

    _sym.SetDeclared(stname, _src.GetCursor());
}

void AGS::Parser::ParseStruct_ExtendsClause(Symbol stname)
{
    SkipNextSymbol(_src, kKW_Extends);
    Symbol const parent = _src.GetNext(); // name of the extended struct

    if (PP::kPreAnalyze == _pp)
        return; // No further analysis necessary in first phase

    if (!_sym.IsStructVartype(parent))
        UserError(
			ReferenceMsgSym("Expected a struct type, found '%s' instead", parent).c_str(), 
			_sym.GetName(parent).c_str());
    if (!_sym.IsManagedVartype(parent) && _sym.IsManagedVartype(stname))
        UserError("Managed struct cannot extend the unmanaged struct '%s'", _sym.GetName(parent).c_str());
    if (_sym.IsManagedVartype(parent) && !_sym.IsManagedVartype(stname))
        UserError("Unmanaged struct cannot extend the managed struct '%s'", _sym.GetName(parent).c_str());
    if (_sym.IsBuiltinVartype(parent) && !_sym.IsBuiltinVartype(stname))
        UserError("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", _sym.GetName(parent).c_str());
    _sym[stname].VartypeD->Size = _sym.GetSize(parent);
    _sym[stname].VartypeD->Parent = parent;
}

void AGS::Parser::Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl)
{
    if (in_struct_decl)
    {
        TypeQualifier error_tq;
        if (tqs[(error_tq = TQ::kAutoptr)] ||
            tqs[(error_tq = TQ::kBuiltin)] ||
            tqs[(error_tq = TQ::kInternalstring)])
            UserError("Cannot use '%s' within a struct declaration", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
    }
    else // !in_struct_decl
    {
        TypeQualifier error_tq;
        if (tqs[(error_tq = TQ::kProtected)] ||
            tqs[(error_tq = TQ::kStatic)] ||
            tqs[(error_tq = TQ::kWriteprotected)])
        {
            UserError("Can only use '%s' within a struct declaration", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
        }
    }

    if (in_func_body)
    {
        TypeQualifier error_tq;
        if (tqs[(error_tq = TQ::kAutoptr)] ||
            tqs[(error_tq = TQ::kBuiltin)] ||
            tqs[(error_tq = TQ::kImport)] ||
            tqs[(error_tq = TQ::kInternalstring)] ||
            tqs[(error_tq = TQ::kManaged)] ||
            tqs[(error_tq = TQ::kStatic)])
        {
            UserError("Cannot use '%s' within a function body", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
        }
    }

    if (1 < tqs[TQ::kProtected] + tqs[TQ::kWriteprotected] + tqs[TQ::kReadonly])
        UserError("Can only use one out of 'protected', 'readonly', and 'writeprotected'");

    if (tqs[TQ::kAutoptr])
    {
        if (!tqs[TQ::kManaged])
            UserError("'autoptr' must be combined with 'managed'");
    }

    // Note: 'builtin' does not always presuppose 'managed'
    if (tqs[TQ::kInternalstring] && (!tqs[TQ::kAutoptr]))
        UserError("'internalstring' must be combined with 'autoptr'");
    if (tqs[TQ::kImport] && tqs[TQ::kInternalstring])
        UserError("Cannot combine 'import' and 'internalstring'");
}

void AGS::Parser::Parse_CheckTQSIsEmpty(TypeQualifierSet tqs)
{
    for (auto it = tqs.begin(); it != tqs.end(); it++)
    {
        if (!tqs[it->first])
            continue;
        UserError("Unexpected '%s' before a command", _sym.GetName(it->second).c_str());
    }
}

void AGS::Parser::ParseQualifiers(TypeQualifierSet &tqs)
{
    bool istd_found = false;
    bool itry_found = false;
    tqs = {};
    while (!_src.ReachedEOF())
    {
        Symbol peeksym = _src.PeekNext();
        switch (peeksym)
        {
        default: return;
        case kKW_Autoptr:        tqs[TQ::kAutoptr] = true; break;
        case kKW_Builtin:        tqs[TQ::kBuiltin] = true; break;
        case kKW_ImportStd:      tqs[TQ::kImport] = true; istd_found = true;  break;
        case kKW_ImportTry:      tqs[TQ::kImport] = true; itry_found = true;  break;
        case kKW_Internalstring: tqs[TQ::kInternalstring] = true; break;
        case kKW_Managed:        tqs[TQ::kManaged] = true; break;
        case kKW_Protected:      tqs[TQ::kProtected] = true; break;
        case kKW_Readonly:       tqs[TQ::kReadonly] = true; break;
        case kKW_Static:         tqs[TQ::kStatic] = true; break;
        case kKW_Writeprotected: tqs[TQ::kWriteprotected] = true; break;
        } // switch (_sym.GetSymbolType(peeksym))

        _src.GetNext();
        if (istd_found && itry_found)
            UserError("Cannot use both 'import' and '_tryimport'");
    };
}

void AGS::Parser::ParseStruct_FuncDecl(TypeQualifierSet tqs, Symbol struct_of_func, Vartype vartype, Symbol name_of_func)
{
    if (tqs[TQ::kWriteprotected])
        UserError("Cannot apply 'writeprotected' to this function declaration");

    SkipNextSymbol(_src, kKW_OpenParenthesis);
    ParseFuncdecl(tqs, vartype, struct_of_func, name_of_func, false, false);

    // Can't code a body behind the function, so the next symbol must be ';'
    return Expect(kKW_Semicolon, _src.PeekNext());
}

void AGS::Parser::ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const params_wanted_count = (is_indexed ? 1u : 0u) + (is_setter ? 1u : 0u);
    if (params_wanted_count != _sym.FuncParamsCount(name_of_func))
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' should have %u parameter(s) but is declared with %u parameter(s) instead",
            name_of_func);
        UserError(msg.c_str(), entry.Name.c_str(), params_wanted_count, _sym.FuncParamsCount(name_of_func));
    }

    Vartype const ret_vartype = is_setter ? kKW_Void : vartype;
    if (ret_vartype != _sym.FuncReturnVartype(name_of_func))
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' must return type '%s' but returns '%s' instead",
            name_of_func);
        UserError(msg.c_str(),
            entry.Name.c_str(),
            _sym.GetName(ret_vartype).c_str(),
            _sym.GetName(_sym.FuncReturnVartype(name_of_func)).c_str());
    }

    size_t p_idx = 1u;
    if (is_indexed)
    {
        auto actual_vartype = entry.FunctionD->Parameters[p_idx].Vartype;
        if (kKW_Int != actual_vartype)
        {
            std::string const msg = ReferenceMsgSym(
                "Parameter #%d of attribute function '%s' must have type 'int' but has type '%s' instead",
                name_of_func);
            UserError(msg.c_str(), p_idx, entry.Name.c_str(), _sym.GetName(actual_vartype).c_str());
        }
        p_idx++;
    }

    if (!is_setter)
        return;

    auto const actual_vartype = entry.FunctionD->Parameters[p_idx].Vartype;
    if (vartype != actual_vartype)
        UserError(
			ReferenceMsgSym(
				"Parameter #%d of attribute function '%s' must have type '%s' but has type '%s' instead",
				name_of_func).c_str(), 
			p_idx, 
			entry.Name.c_str(), 
			_sym.GetName(vartype).c_str(), 
			_sym.GetName(actual_vartype).c_str());
}

void AGS::Parser::ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    auto &param_exprs = _sym[name_of_func].FunctionD->Parameters;
    FuncParameterDesc fpd = {};
    if (is_indexed)
    {
        fpd.Vartype = kKW_Int;
        param_exprs.push_back(fpd);
    }
    if (is_setter)
    {
        fpd.Vartype = vartype;
        param_exprs.push_back(fpd);
    }
}

void AGS::Parser::ParseStruct_Attribute_DeclareFunc(TypeQualifierSet tqs, Symbol strct, Symbol qualified_name, Symbol unqualified_name, bool is_setter, bool is_indexed, Vartype vartype)
{
    // If this symbol has been defined before, check whether the definitions clash
    if (_sym.IsInUse(qualified_name) && !_sym.IsFunction(qualified_name))
        UserError(
			ReferenceMsgSym(
				"Attribute uses '%s' as a function, this clashes with a declaration elsewhere",
				qualified_name).c_str(), 
			_sym[qualified_name].Name.c_str());
    if (_sym.IsFunction(qualified_name))
		ParseStruct_Attribute_CheckFunc(qualified_name, is_setter, is_indexed, vartype);
    
    tqs[TQ::kImport] = true; // Assume that attribute functions are imported
    if (tqs[TQ::kImport] &&
        _sym.IsFunction(qualified_name) &&
        !_sym[qualified_name].FunctionD->TypeQualifiers[TQ::kImport])
    {
        if (FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
            UserError(
				ReferenceMsgSym(
                "In here, attribute functions may not be defined locally", qualified_name).c_str());
        tqs[TQ::kImport] = false;
    }

    // Store the fact that this function has been declared within the struct declaration
    _sym.MakeEntryComponent(qualified_name);
    _sym[qualified_name].ComponentD->Parent = strct;
    _sym[qualified_name].ComponentD->Component = unqualified_name;
    _sym[strct].VartypeD->Components[unqualified_name] = qualified_name;

    Vartype const return_vartype = is_setter ? kKW_Void : vartype;
    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, strct, qualified_name, false);

    ParseStruct_Attribute_ParamList(strct, qualified_name, is_setter, is_indexed, vartype);
    
    bool const body_follows = false; // we are within a struct definition
    return ParseFuncdecl_HandleFunctionOrImportIndex(tqs, strct, qualified_name, body_follows);
}

void AGS::Parser::ParseStruct_Attribute2SymbolTable(TypeQualifierSet tqs, Vartype const vartype, Symbol const name_of_struct, Symbol const unqualified_attribute, bool const is_indexed)
{
    ParseStruct_MasterAttribute2SymbolTable(tqs, vartype, name_of_struct, unqualified_attribute, is_indexed);
}

void AGS::Parser::ParseStruct_MasterAttribute2SymbolTable(TypeQualifierSet tqs, Vartype const vartype, Symbol const name_of_struct, Symbol const unqualified_attribute, bool const is_indexed,
        Symbol getter_func, Symbol setter_func)
{
    Symbol const qualified_attribute = MangleStructAndComponent(name_of_struct, unqualified_attribute);
    size_t const declaration_start = _src.GetCursor();

    // Not a proper function qualifier: Only signifies that a setter won't be defined
    bool const is_readonly = tqs[TQ::kReadonly];
    tqs[TQ::kReadonly] = false; 


    // Mustn't be in struct already
    if (_sym.IsInUse(qualified_attribute))
        UserError(
            ReferenceMsgSym("'%s' is already defined", qualified_attribute).c_str(),
            _sym.GetName(qualified_attribute).c_str());

    // Mustn't be in any ancester
    Symbol const parent = FindStructOfComponent(name_of_struct, unqualified_attribute);
    if (kKW_NoSymbol != parent)
        UserError(
            ReferenceMsgSym("The struct '%s' extends '%s', and '%s' is already defined", parent).c_str(),
            _sym.GetName(name_of_struct).c_str(),
            _sym.GetName(parent).c_str(),
            _sym.GetName(unqualified_attribute).c_str());

    _sym.MakeEntryComponent(qualified_attribute);
    _sym[qualified_attribute].ComponentD->Component = unqualified_attribute;
    _sym[qualified_attribute].ComponentD->Parent = name_of_struct;
    _sym[name_of_struct].VartypeD->Components[unqualified_attribute] = qualified_attribute;
    _sym.SetDeclared(qualified_attribute, declaration_start);

    _sym.MakeEntryAttribute(qualified_attribute);
    _sym[qualified_attribute].AttributeD->IsIndexed = is_indexed;
    _sym[qualified_attribute].AttributeD->IsStatic = tqs[TQ::kStatic];
    _sym[qualified_attribute].AttributeD->Vartype = vartype;

    // Declare attribute getter, e.g. get_ATTRIB()
    Symbol const unqualified_getter =
        ConstructAttributeFuncName(qualified_attribute, false, is_indexed);
    if (getter_func == kKW_NoSymbol)
    {
        Symbol const qualified_getter = MangleStructAndComponent(name_of_struct, unqualified_getter);
        _sym[qualified_attribute].AttributeD->Getter = qualified_getter;
        ParseStruct_Attribute_DeclareFunc(tqs, name_of_struct, qualified_getter, unqualified_getter, false, is_indexed, vartype);
        _sym.SetDeclared(qualified_getter, declaration_start);
    }
    else
    {
        _sym[qualified_attribute].AttributeD->Getter = getter_func;
        _sym[name_of_struct].VartypeD->Components[unqualified_getter] = getter_func;
    }

    if (!is_readonly)
    {
        // Declare attribute setter, e.g. set_ATTRIB(value)
        Symbol const unqualified_setter =
            ConstructAttributeFuncName(qualified_attribute, true, is_indexed);
        if (setter_func == kKW_NoSymbol)
        {
            Symbol const qualified_setter = MangleStructAndComponent(name_of_struct, unqualified_setter);
            _sym[qualified_attribute].AttributeD->Setter = qualified_setter;
            ParseStruct_Attribute_DeclareFunc(tqs, name_of_struct, qualified_setter, unqualified_setter, true, is_indexed, vartype);
            _sym.SetDeclared(qualified_setter, declaration_start);
        }
        else
        {
            _sym[qualified_attribute].AttributeD->Setter = setter_func;
            _sym[name_of_struct].VartypeD->Components[unqualified_setter] = setter_func;
        }
    }
}

void AGS::Parser::ParseStruct_Attribute(TypeQualifierSet tqs, Symbol const name_of_struct)
{
    TypeQualifier error_tq;
    if (tqs[error_tq = TQ::kManaged] ||
        tqs[error_tq = TQ::kProtected] ||
        tqs[error_tq = TQ::kWriteprotected])
        UserError("Cannot use '%s' in front of 'attribute'", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
    
    bool const is_const_vartype = kKW_Const == _src.PeekNext();
    if (is_const_vartype)
        SkipNextSymbol(_src, kKW_Const); 

    Vartype vartype = _src.GetNext();

    if (!_sym.IsVartype(vartype))
        UserError("Expected a type, found '%s' instead", _sym.GetName(vartype).c_str());
    if (is_const_vartype && kKW_String != vartype)
        UserError("The only allowed type that starts with 'const' is 'const string' (did you mean 'readonly attribute'?)");

    SetDynpointerInManagedVartype(vartype);
    EatDynpointerSymbolIfPresent(_src, vartype);

    while (true)
    {
        Symbol const attribute = ParseVarname();
        
        bool const is_indexed = (kKW_OpenBracket == _src.PeekNext());
        if (is_indexed)
        {
            SkipNextSymbol(_src, kKW_OpenBracket);
            Expect(kKW_CloseBracket, _src.GetNext());
        }

        ParseStruct_Attribute2SymbolTable(tqs, vartype, name_of_struct, attribute, is_indexed);

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Semicolon == punctuation)
            return;
    }
}


void AGS::Parser::ParseArrayDecl(Symbol vname, Vartype &vartype)
{
    // To the end of the '[…]' clause
    SkipNextSymbol(_src, kKW_OpenBracket);
    size_t const array_expr_start = _src.GetCursor();

    _src.SkipToCloser();
    SkipNextSymbol(_src, kKW_CloseBracket);

    // When more '[…]' clauses follow, they need to be processed first, so call this func recursively
    if (kKW_OpenBracket == _src.PeekNext())
        ParseArrayDecl(vname, vartype);

    if (PP::kPreAnalyze == _pp)
        return; // No need to analyse it further in this phase

    size_t const array_expr_end = _src.GetCursor();
    _src.SetCursor(array_expr_start);

    if (kKW_CloseBracket == _src.PeekNext())
    {
        // Dynamic array
        SkipNextSymbol(_src, kKW_CloseBracket);
        if (vartype == kKW_String)
            UserError("Cannot have a dynamic array of old-style strings");
        if (_sym.IsArrayVartype(vartype))
            UserError("Cannot have a dynamic array of a classic array");
        vartype = _sym.VartypeWithDynarray(vartype);
        _src.SetCursor(array_expr_end);
        return;
    }

    // Static array
    std::vector<size_t> dims;
    while (true)
    {
        EvaluationResult eres;
        int const dim_start = _src.GetCursor();
        _src.SkipTo(kKW_Comma);
        SrcList expression = SrcList(_src, dim_start, _src.GetCursor() - dim_start);
        if (0u == expression.Length())
            UserError(
                "Expected an integer expression for array dimension #%u, did not find any",
                dims.size() + 1);
        expression.StartRead();
        ParseIntegerExpression(expression, eres);
        if (eres.kTY_Literal != eres.Type)
            UserError(
                "Cannot evaluate the integer expression for array dimension #%u at compile time",
                dims.size() + 1u);
        if (!expression.ReachedEOF())
            UserError(
                "Unexpected '%s' after the integer expression for array dimension #%u",
                _sym.GetName(expression.GetNext()).c_str(),
                dims.size() + 1u);
            
        CodeCell const dimension_as_int = _sym[eres.Symbol].LiteralD->Value;
        if (dimension_as_int < 1)
            UserError(
                "Array dimension #%u must be at least 1 but is %d instead",
                dims.size() + 1u,
                dimension_as_int);

        dims.push_back(static_cast<size_t>(dimension_as_int));

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseBracket }, punctuation);
        if (kKW_Comma == punctuation)
            continue;
        // Successive static arrays will be joined within 'VartypeWithArray()', below.
        break; 
    }

    vartype = _sym.VartypeWithArray(dims, vartype);
    _src.SetCursor(array_expr_end);
}

void AGS::Parser::ParseStruct_VariableDefn(TypeQualifierSet tqs, Vartype vartype, Symbol name_of_struct, Symbol vname)
{
    if (PP::kMain != _pp)
        return _src.SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon });

    if (_sym.IsDynarrayVartype(vartype)) // e.g., 'int [] zonk;', disallowed
        Expect(kKW_OpenParenthesis, _src.PeekNext());
    if (tqs[TQ::kStatic]) // e.g. 'static int zonk;': No static struct variables in AGS
        UserError("Static variables are not supported");
    if (tqs[TQ::kImport])
        UserError("Cannot import struct component variables; import the whole struct instead");

    if (!FlagIsSet(_options, SCOPT_RTTIOPS) &&
        (_sym.IsManagedVartype(vartype) && _sym.IsManagedVartype(name_of_struct)))
        // Cannot allow nested managed pointers without RTTI support (will cause memory leaks at runtime)
        UserError("Cannot have managed variable components in managed struct (RTTI is not enabled)");

    if (_sym.IsBuiltinVartype(vartype) && !_sym.IsManagedVartype(vartype))
        // Non-managed built-in vartypes do exist
        UserError(
			"May not have a component variable of the non-managed built-in type '%s'", 
			_sym.GetName(vartype).c_str());
        
    SymbolTableEntry &entry = _sym[vname];
    entry.ComponentD->Offset = _sym[name_of_struct].VartypeD->Size;

    _sym.MakeEntryVariable(vname);
    entry.VariableD->Vartype = vartype;
    entry.VariableD->TypeQualifiers = tqs.WithoutTypedefQualifiers();

    if (_src.PeekNext() == kKW_OpenBracket)
    {
        Vartype vartype = _sym[vname].VariableD->Vartype;
        ParseArrayDecl(vname, vartype);
        _sym[vname].VariableD->Vartype = vartype;
    }

    _sym[name_of_struct].VartypeD->Size += _sym.GetSize(vname);
}

void AGS::Parser::ParseStruct_VariableOrFunctionDefn(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype)
{
    size_t const declaration_start = _src.GetCursor();

    // Get the variable or function name.
    Symbol const unqualified_component = ParseVarname();
    Symbol const qualified_component = MangleStructAndComponent(name_of_struct, unqualified_component);

    bool const is_function = (kKW_OpenParenthesis == _src.PeekNext());

    if (PP::kMain == _pp)
    {
        if (!is_function && _sym.IsInUse(qualified_component))
            UserError(
				ReferenceMsgSym("'%s' is already defined", qualified_component).c_str(), 
				_sym.GetName(qualified_component).c_str());

        // Mustn't be in any ancester
        Symbol const parent = FindStructOfComponent(name_of_struct, unqualified_component);
        if (kKW_NoSymbol != parent)
            UserError(
                ReferenceMsgSym(
                    "The struct '%s' extends '%s', and '%s' is already defined",
                    parent).c_str(),
                _sym.GetName(name_of_struct).c_str(),
                _sym.GetName(parent).c_str(),
                _sym.GetName(qualified_component).c_str());
    }

    _sym.MakeEntryComponent(qualified_component);
    _sym[qualified_component].ComponentD->Component = unqualified_component;
    _sym[qualified_component].ComponentD->Parent = name_of_struct;
    _sym[name_of_struct].VartypeD->Components[unqualified_component] = qualified_component;

    if (is_function)
        ParseStruct_FuncDecl(tqs, name_of_struct, vartype, qualified_component);
    else 
        ParseStruct_VariableDefn(tqs, vartype, name_of_struct, qualified_component);

    if (_sym.IsConstructor(qualified_component))
        _sym[name_of_struct].VartypeD->Constructor = qualified_component;
    _sym.SetDeclared(qualified_component, declaration_start);
 }

void AGS::Parser::EatDynpointerSymbolIfPresent(SrcList src, Vartype vartype)
{
    if (kKW_Dynpointer != src.PeekNext())
        return;

    if (PP::kPreAnalyze == _pp || _sym.IsManagedVartype(vartype))
    {
        SkipNextSymbol(src, kKW_Dynpointer);
        return;
    }

    UserError("Cannot use '*' on the non-managed type '%s'", _sym.GetName(vartype).c_str());
}

void AGS::Parser::ParseStruct_Vartype_MemberList(TypeQualifierSet tqs, Symbol name_of_struct, Vartype vartype)
{
    while (true)
    {
        ParseStruct_VariableOrFunctionDefn(name_of_struct, tqs, vartype);

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Semicolon == punctuation)
            return;
    }
}

void AGS::Parser::ParseStruct_CheckComponentVartype(Symbol const stname, Vartype const vartype)
{
    if (!_sym.IsVartype(vartype))
        UserError(
            ReferenceMsgSym("Expected a type, found '%s' instead", vartype).c_str(),
            _sym.GetName(vartype).c_str());

    if (!_sym.IsManagedVartype(vartype))
    {
        if (stname == vartype)
            UserError(
                "Cannot include a component of type '%s' into this struct of type '%s'",
                _sym.GetName(vartype).c_str(),
                _sym.GetName(stname).c_str());

        for (Vartype parent = stname; kKW_NoSymbol != parent; parent = _sym[parent].VartypeD->Parent)
            if (vartype == parent)
                UserError(
                    "This struct extends '%s'; cannot include a component of type '%s' into this struct of type '%s'",
                    _sym.GetName(vartype).c_str(),
                    _sym.GetName(vartype).c_str(),
                    _sym.GetName(stname).c_str());
    }
}

void AGS::Parser::ParseStruct_Vartype(Symbol name_of_struct, TypeQualifierSet tqs)
{
    _src.BackUp();
    Vartype vartype = ParseVartype(_src);

    if (PP::kMain == _pp)
        // Check for illegal struct member types
		ParseStruct_CheckComponentVartype(name_of_struct, vartype);

    // "int [] func(...)"
    ParseDynArrayMarkersIfPresent(_src, vartype);
    
    // "TYPE noloopcheck foo(...)"
    if (kKW_Noloopcheck == _src.PeekNext())
		UserError("Cannot use 'noloopcheck' here");

    // We've accepted a type expression and are now reading vars or one func that should have this type.
    ParseStruct_Vartype_MemberList(tqs, name_of_struct, vartype);
}

void AGS::Parser::ParseStruct(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    size_t const start_of_struct_decl = _src.GetCursor();

    Symbol const stname = _src.GetNext(); // get token for name of struct

    if (!(_sym.IsVartype(stname) && _sym[stname].VartypeD->Flags[VTF::kUndefined]) && _sym.IsInUse(stname))
        UserError(
            ReferenceMsgSym("'%s' is already defined", stname).c_str(),
            _sym.GetName(stname).c_str());

    ParseStruct_CheckForwardDecls(stname, tqs);
    
    if (name_of_current_func > 0)
        UserError("Cannot define a struct type within a function");

    ParseStruct_SetTypeInSymboltable(stname, tqs);

    // Declare the struct type that implements new strings
    if (tqs[TQ::kInternalstring])
    {
        if (_sym.GetStringStructSym() > 0 && stname != _sym.GetStringStructSym())
            UserError("The stringstruct type is already defined to be %s", _sym.GetName(_sym.GetStringStructSym()).c_str());
        _sym.SetStringStructSym(stname);
    }

    if (kKW_Extends == _src.PeekNext())
		ParseStruct_ExtendsClause(stname);

    // forward-declaration of struct type
    if (kKW_Semicolon == _src.PeekNext())
    {
        if (!tqs[TQ::kManaged])
            UserError("Forward-declared 'struct's must be 'managed'");
        SkipNextSymbol(_src, kKW_Semicolon); 
        return;
    }

    Expect(kKW_OpenBrace, _src.GetNext());
    
    // Declaration of the components
    while (kKW_CloseBrace != _src.PeekNext())
    {
        currentline = _src.GetLinenoAt(_src.GetCursor());
        TypeQualifierSet tqs = {};
        ParseQualifiers(tqs);
        bool const in_func_body = false;
        bool const in_struct_decl = true;
        Parse_CheckTQ(tqs, in_func_body, in_struct_decl);

        Symbol const leading_sym = _src.GetNext();
        switch (leading_sym)
        {
        case kKW_Attribute:
            ParseStruct_Attribute(tqs, stname);
            continue;

        case kKW_Const:
        {
            if (kKW_String == _src.PeekNext())
                break; // Vartype, treated below

            TypeQualifierSet tqs_without_static = tqs;
            tqs_without_static[TQ::kStatic] = false;
            Parse_CheckTQSIsEmpty(tqs_without_static);
            ParseStruct_ConstantDefn(stname);
            continue;
        }

        } // switch (leading_sym)

        ParseStruct_Vartype(stname, tqs);
    }

    if (PP::kMain == _pp)
    {
        // round up size to nearest multiple of STRUCT_ALIGNTO
        size_t &struct_size = _sym[stname].VartypeD->Size;
        if (0u != (struct_size % STRUCT_ALIGNTO))
            struct_size += STRUCT_ALIGNTO - (struct_size % STRUCT_ALIGNTO);
    }

    SkipNextSymbol(_src, kKW_CloseBrace);

    // Struct has now been completely defined
    _sym[stname].VartypeD->Flags[VTF::kUndefined] = false;
    _structRefs.erase(stname);

    Symbol const nextsym = _src.PeekNext();
    if (kKW_Semicolon == nextsym)
    {
        if (tqs[TQ::kReadonly])
        {
            // Only now do we find out that there isn't any following declaration
            // so "readonly" was incorrect. 
            // Back up the cursor for the error message.
            _src.SetCursor(start_of_struct_decl);
            UserError("'readonly' can only be used in a variable declaration");
        }
        SkipNextSymbol(_src, kKW_Semicolon); 
        return;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    if (_src.ReachedEOF())
        UserError("Unexpected end of input (did you forget a ';'?)");

    if (!(_sym.IsIdentifier(nextsym) && !_sym.IsVartype(nextsym)) &&
        kKW_Dynpointer != nextsym &&
        kKW_Noloopcheck != nextsym &&
        kKW_OpenBracket != nextsym)
    {
        UserError("Unexpected '%s' (did you forget a ';'?)", _sym.GetName(nextsym).c_str());
    }

    // Take struct that has just been defined as the vartype of a declaration
    // Those type qualifiers that are used to define the struct type
    // have been used up, so reset them
    TypeQualifierSet vardecl_tqs = tqs;
    vardecl_tqs[TQ::kAutoptr] = false;
    vardecl_tqs[TQ::kBuiltin] = false;
    vardecl_tqs[TQ::kManaged] = false;
    vardecl_tqs[TQ::kInternalstring] = false;

    Vartype vartype = stname;
    SetDynpointerInManagedVartype(vartype);
    EatDynpointerSymbolIfPresent(_src, vartype);
    ParseDynArrayMarkersIfPresent(_src, vartype);
    ScopeType const scope_type = (vardecl_tqs[TQ::kImport]) ? ScT::kImport : ScT::kGlobal;

    ParseVartype_MemberList(vardecl_tqs, vartype, scope_type, false, struct_of_current_func, name_of_current_func);
}

void AGS::Parser::ParseEnum_AssignedValue(Symbol vname, CodeCell &value)
{
    SkipNextSymbol(_src, kKW_Assign);

    std::string msg = "In the assignment to <name>: ";
    string_replace(msg, "<name>", _sym.GetName(vname));
    Symbol const lit = ParseConstantExpression(_src, msg);
    
    value = _sym[lit].LiteralD->Value;
}

void AGS::Parser::ParseEnum_Item2Symtable(Symbol enum_name,Symbol item_name, int value)
{
    Symbol value_sym;
    FindOrAddIntLiteral(value, value_sym);
    
    SymbolTableEntry &entry = _sym[item_name];
    _sym.MakeEntryConstant(item_name);

    entry.ConstantD->ValueSym = value_sym;
    entry.Scope = 0u;

    // AGS implements C-style enums, so their qualified name is identical to their unqualified name.
    _sym[enum_name].VartypeD->Components[item_name] = item_name;

    _sym.SetDeclared(item_name, _src.GetCursor());
}

void AGS::Parser::ParseEnum_Name2Symtable(Symbol enum_name)
{
    SymbolTableEntry &entry = _sym[enum_name];

    if (_sym.IsPredefined(enum_name))
        UserError("Expected an identifier, found the predefined symbol '%s' instead", _sym.GetName(enum_name).c_str());
    if (_sym.IsFunction(enum_name) || _sym.IsVartype(enum_name))
        UserError(
			ReferenceMsgLoc("'%s' is already defined", entry.Declared).c_str(), 
			_sym.GetName(enum_name).c_str());
    _sym.MakeEntryVartype(enum_name);

    entry.VartypeD->Size = SIZE_OF_INT;
    entry.VartypeD->BaseVartype = kKW_Int;
    entry.VartypeD->Flags[VTF::kEnum] = true;
}

AGS::Symbol AGS::Parser::ParseVartype(SrcList &src, bool const with_dynpointer_handling)
{
    bool const leading_const = (kKW_Const == src.PeekNext());
    if (leading_const)
        SkipNextSymbol(src, kKW_Const); 

    Vartype vartype = src.GetNext();
    if (!_sym.IsVartype(vartype))
        UserError("Expected a type, found '%s' instead", _sym.GetName(vartype).c_str());
    if (_sym[vartype].VartypeD->Flags[VTF::kUndefined])
        _structRefs[vartype] = src.GetCursor();

    if (leading_const)
    {
        if (kKW_String != vartype)
            UserError("The only allowed type beginning with 'const' is 'const string' (did you want to use 'readonly'?)");
        vartype = _sym.VartypeWithConst(vartype);
    }

    if (with_dynpointer_handling)
    {
        SetDynpointerInManagedVartype(vartype);
        EatDynpointerSymbolIfPresent(src, vartype);
    }
    return vartype;
}

void AGS::Parser::ParseEnum(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    size_t const start_of_enum_decl = _src.GetCursor();
    if (kKW_NoSymbol != name_of_current_func)
        UserError("Cannot define an enum type within a function");
    if (tqs[TQ::kBuiltin])
        UserError("Can only use 'builtin' when declaring a struct");

    // Get name of the enum, enter it into the symbol table
    Symbol enum_name = _src.GetNext();
    ParseEnum_Name2Symtable(enum_name);

    Expect(kKW_OpenBrace, _src.GetNext());

    CodeCell current_constant_value = 0;

    while (true)
    {
        Symbol item_name = _src.GetNext();
        if (kKW_CloseBrace == item_name)
            break; // item list empty or ends with trailing ','

        if (PP::kMain == _pp)
        {
            if (_sym.IsConstant(item_name))
                UserError(
                    ReferenceMsgSym("'%s' is already defined as a constant or enum value", item_name).c_str(),
                    _sym.GetName(item_name).c_str());
            if (_sym.IsPredefined(item_name) || _sym.IsVariable(item_name) || _sym.IsFunction(item_name))
                UserError("Expected '}' or an unused identifier, found '%s' instead", _sym.GetName(item_name).c_str());
        }

        Symbol const punctuation = _src.PeekNext();
        Expect(SymbolList{ kKW_Comma, kKW_Assign, kKW_CloseBrace }, punctuation);

        if (kKW_Assign == punctuation)
        {
            // the value of this entry is specified explicitly
            ParseEnum_AssignedValue(item_name, current_constant_value);
        }
        else
        {
            if (std::numeric_limits<CodeCell>::max() == current_constant_value)
                UserError(
                    "Cannot assign an enum value higher that %d to %s",
                    std::numeric_limits<CodeCell>::max(),
                    _sym.GetName(item_name).c_str());
            current_constant_value++;
        }

        // Enter this enum item as a constant int into the _sym table
        ParseEnum_Item2Symtable(enum_name, item_name, current_constant_value);

        Symbol const comma_or_brace = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseBrace }, comma_or_brace);
        if (kKW_Comma == comma_or_brace)
            continue;
        break;
    }

    Symbol const nextsym = _src.PeekNext();
    if (kKW_Semicolon == nextsym)
    {
        SkipNextSymbol(_src, kKW_Semicolon); 
        if (tqs[TQ::kReadonly])
        {
            // Only now do we find out that there isn't any following declaration
            // so "readonly" was incorrect. 
            // Back up the cursor for the error message.
            _src.SetCursor(start_of_enum_decl);
            UserError("Can only use 'readonly' when declaring a variable or attribute");
        }
        return;
    }

    // If this doesn't seem to be a declaration at first glance,
    // warn that the user might have forgotten a ';'.
    if (_src.ReachedEOF())
        UserError("Unexpected end of input (did you forget a ';'?)");

    if (!(_sym.IsIdentifier(nextsym) && !_sym.IsVartype(nextsym)) &&
        kKW_Dynpointer != nextsym &&
        kKW_Noloopcheck != nextsym &&
        kKW_OpenBracket != nextsym)
        UserError("Unexpected '%s' (did you forget a ';'?)", _sym.GetName(nextsym).c_str());

    // Take enum that has just been defined as the vartype of a declaration
    Vartype vartype = enum_name;
    ParseDynArrayMarkersIfPresent(_src, vartype);
    ScopeType const scope_type = tqs[TQ::kImport] ? ScT::kImport : ScT::kGlobal;
    ParseVartype_MemberList(tqs, vartype, scope_type, false, struct_of_current_func, name_of_current_func);
}

void AGS::Parser::ParseAttribute(TypeQualifierSet tqs, Symbol const name_of_current_func)
{
    if (kKW_NoSymbol != name_of_current_func)
        UserError("Cannot define an attribute within a function body");

    TypeQualifier error_tq;
    if (tqs[error_tq = TQ::kAutoptr] ||
        tqs[error_tq = TQ::kBuiltin] ||
        tqs[error_tq = TQ::kInternalstring] ||
        tqs[error_tq = TQ::kManaged] ||
        tqs[error_tq = TQ::kProtected] ||
        tqs[error_tq = TQ::kWriteprotected])
        UserError("Cannot use '%s' in front of 'attribute'", _sym.GetName(tqs.TQ2Symbol(error_tq)).c_str());
    if (tqs[TQ::kStatic])
        UserError("Can only declare 'static attribute' within a 'struct' declaration (use extender syntax 'attribute ... (static STRUCT)')");

    Vartype vartype = ParseVartype(_src);

    while (true)
    {
        Symbol const attribute = ParseVarname();
        
        bool const is_indexed = (kKW_OpenBracket == _src.PeekNext());
        if (is_indexed)
        {
            SkipNextSymbol(_src, kKW_OpenBracket);
            Expect(kKW_CloseBracket, _src.GetNext());
        }

        Expect(kKW_OpenParenthesis, _src.GetNext());
        Symbol const static_or_this = _src.GetNext();
        Expect(SymbolList{ kKW_Static, kKW_This }, static_or_this);
        tqs[TQ::kStatic] = static_or_this == kKW_Static;
        Symbol const name_of_struct = _src.GetNext();
        if (!_sym.IsStructVartype(name_of_struct))
            UserError("Expected a struct type instead of '%s'", _sym.GetName(name_of_struct).c_str());
        if (!tqs[TQ::kStatic])
        {
            if (!_sym.IsManagedVartype(name_of_struct))
                UserError(
                    ReferenceMsgSym("Cannot use 'this' with the unmanaged struct '%s'", name_of_struct).c_str(),
                    _sym.GetName(name_of_struct).c_str());
            if (kKW_Dynpointer == _src.PeekNext())
                SkipNextSymbol(_src, kKW_Dynpointer); // Optional here
        }
        Expect(kKW_CloseParenthesis, _src.GetNext());

        ParseStruct_Attribute2SymbolTable(tqs, vartype, name_of_struct, attribute, is_indexed);

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Semicolon == punctuation)
            return;
    }
}

void AGS::Parser::ParseExport_Function(Symbol func)
{
    // If all functions will be exported anyway, skip this here.
    if (FlagIsSet(_options, SCOPT_EXPORTALL))
        return;

    if (_sym[func].FunctionD->TypeQualifiers[TQ::kImport])
        UserError(
            ReferenceMsgSym("Function '%s' is imported, so it cannot be exported", func).c_str(),
            _sym.GetName(func).c_str());

    int retval = _scrip.AddExport(
        _sym.GetName(func),
        _sym[func].FunctionD->Offset,
        _sym.FuncParamsCount(func) + 100u * _sym[func].FunctionD->IsVariadic);
    if (retval < 0)
        InternalError("Could not export function '%s', exports table overflow?", _sym.GetName(func).c_str());
}

void AGS::Parser::ParseExport_Variable(Symbol var)
{
    ScopeType const var_sct =_sym.GetScopeType(var);
    if (ScT::kImport == var_sct)
        UserError(
            ReferenceMsgSym("Cannot export the imported variable '%s'", var).c_str(),
            _sym.GetName(var).c_str());
    if (ScT::kGlobal != var_sct)
        UserError(
            ReferenceMsgSym("Cannot export the non-global variable '%s'", var).c_str(),
            _sym.GetName(var).c_str());

    // Note, if this is a string then the compiler keeps track of it by its first byte.
    // AFAICS, this _is_ exportable.
    
    int retval = _scrip.AddExport(
        _sym.GetName(var),
        _sym[var].VariableD->Offset);
    if (retval < 0)
        InternalError("Could not export variable '%s', exports table overflow?", _sym.GetName(var).c_str());
}

void AGS::Parser::ParseExport()
{
    if (PP::kPreAnalyze == _pp)
    {
        _src.SkipTo(kKW_Semicolon);
        SkipNextSymbol(_src, kKW_Semicolon);
        return;
    }

    // export specified symbols
    while (true) 
    {
        Symbol const export_sym = _src.GetNext();
        if (_sym.IsFunction(export_sym))
            ParseExport_Function(export_sym);
        else if (_sym.IsVariable(export_sym))
            ParseExport_Variable(export_sym);
        else
            UserError("Expected a function or global variable but found '%s' instead", _sym.GetName(export_sym).c_str());    

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon, }, punctuation);
        if (kKW_Semicolon == punctuation)
            break;
    }
}

void AGS::Parser::ParseVartype_CheckForIllegalContext()
{
    NSType const ns_type = _nest.Type();
    if (NSType::kSwitch == ns_type)
		UserError("Cannot use declarations directly within a 'switch' body. (Put \"{ ... }\" around the 'case' statements)");
        

    if (NSType::kBraces == ns_type || NSType::kFunction == ns_type || NSType::kNone == ns_type)
        return;

    UserError("A declaration cannot be the sole body of an 'if', 'else' or loop clause");
}

void AGS::Parser::ParseVartype_CheckIllegalCombis(bool is_function,TypeQualifierSet tqs)
{
    if (tqs[TQ::kStatic] && !is_function)
        UserError("Outside of a 'struct' declaration, 'static' can only be applied to functions");
        
    // Note: 'protected' is valid for struct functions; those can be defined directly,
    // as in 'int strct::function(){}' or extender, as int 'function(this strct *){}'// We cannot know at this point whether the function is extender, so we cannot
    // check  at this point whether 'protected' is allowed.

    if (tqs[TQ::kReadonly] && is_function)
        UserError("Cannot apply 'readonly' to a function");
    if (tqs[TQ::kWriteprotected] && is_function)
        UserError("Cannot apply 'writeprotected' to a function");
}

void AGS::Parser::ParseVartype_FuncDecl(TypeQualifierSet tqs, Vartype vartype, Symbol struct_name, Symbol func_name, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func, bool &body_follows)
{
    size_t const declaration_start = _src.GetCursor();
    SkipNextSymbol(_src, kKW_OpenParenthesis);

    bool const func_is_static_extender = (kKW_Static == _src.PeekNext());
    bool const func_is_extender = func_is_static_extender || (kKW_This == _src.PeekNext());
    
    if (func_is_extender)
    {
        if (struct_name > 0)
            UserError("Cannot use extender syntax with a function name that follows '::'");
            
        // Rewrite extender function as a component function of the corresponding struct.
        ParseFuncdecl_ExtenderPreparations(func_is_static_extender, struct_name, func_name, tqs);
    }

    // Do not set .Extends or the Component flag here. These denote that the
    // func has been either declared within the struct definition or as extender.

    body_follows = ParseFuncdecl_DoesBodyFollow();
    ParseFuncdecl(tqs, vartype, struct_name, func_name, no_loop_check, body_follows);
    _sym.SetDeclared(func_name, declaration_start);

    if (!body_follows)
        return;

    if (0 < name_of_current_func)
        UserError(
            ReferenceMsgSym("Function bodies cannot nest, but the body of function %s is still open. (Did you forget a '}'?)", func_name).c_str(),
            _sym.GetName(name_of_current_func).c_str());

    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    struct_of_current_func = struct_name;
}

void AGS::Parser::ParseVartype_VarDecl_PreAnalyze(Symbol var_name, ScopeType scope_type)
{
    if (0u != _givm.count(var_name))
    {
        if (_givm[var_name])
            UserError("'%s' is already defined as a global non-import variable", _sym.GetName(var_name).c_str());
        else if (ScT::kGlobal == scope_type && FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
            UserError("'%s' is defined as an import variable; that cannot be overridden here", _sym.GetName(var_name).c_str());
    }
    _givm[var_name] = (ScT::kGlobal == scope_type);

    // Apart from this, we aren't interested in var defns at this stage, so skip this defn
    _src.SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon });
}

void AGS::Parser::ParseVartype_VariableDefn(TypeQualifierSet tqs, Vartype vartype, Symbol vname, ScopeType scope_type)
{
    if (PP::kPreAnalyze == _pp)
        return ParseVartype_VarDecl_PreAnalyze(vname, scope_type);

    Parse_CheckTQ(tqs, (_nest.TopLevel() > _sym.kParameterScope), _sym.IsComponent(vname));
    
   // Note: Don't make a variable here yet; we haven't checked yet whether we may do so.

    TypeQualifierSet variable_tqs = tqs;
    // "autoptr", "managed" and "builtin" are aspects of the vartype, not of the variable having the vartype.
    variable_tqs[TQ::kAutoptr] = false;
    variable_tqs[TQ::kManaged] = false;
    variable_tqs[TQ::kBuiltin] = false;

    return ParseVardecl(variable_tqs, vartype, vname, scope_type);
}

void AGS::Parser::ParseVartype_MemberList(TypeQualifierSet tqs, Vartype vartype, ScopeType scope_type, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    while (true)
    {
        // Get the variable or function name.
        Symbol var_or_func_name = kKW_NoSymbol;
        Symbol struct_name = kKW_NoSymbol;
        ParseVarname(struct_name, var_or_func_name);
        
        bool const is_function = (kKW_OpenParenthesis == _src.PeekNext());

        // certain qualifiers, such as "static" only go with certain kinds of definitions.
        ParseVartype_CheckIllegalCombis(is_function, tqs);

        if (is_function)
        {
            // Do not set .Extends or the Component flag here. These denote that the
            // func has been either declared within the struct definition or as extender,
            // so they are NOT set unconditionally
            bool body_follows = false;
            ParseVartype_FuncDecl(tqs, vartype, struct_name, var_or_func_name, no_loop_check, struct_of_current_func, name_of_current_func, body_follows);
            if (body_follows)
                return;
        }
        else if (_sym.IsDynarrayVartype(vartype) || no_loop_check)
        {
            // Those are only allowed with functions
            UserError("Expected '('");
        }
        else
        {
            if (kKW_NoSymbol != struct_name)
                UserError("Variable may not contain '::'");

            ParseVartype_VariableDefn(tqs, vartype, var_or_func_name, scope_type);
        }

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Semicolon == punctuation)
            return;
    }
}

void AGS::Parser::ParseVartypeClause(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    if (_src.ReachedEOF())
        UserError("Unexpected end of input (did you forget ';'?)");
    if (tqs[TQ::kBuiltin])
        UserError("Can only use 'builtin' when declaring a 'struct'");
    ParseVartype_CheckForIllegalContext();

    ScopeType const scope_type = 
        (kKW_NoSymbol != name_of_current_func) ? ScT::kLocal :
        (tqs[TQ::kImport]) ? ScT::kImport : ScT::kGlobal;
    
    _src.BackUp();
    Vartype vartype = ParseVartype(_src, false);

    // A pointer symbol is generally implied for managed vartypes
    bool managed_vartype_has_implied_pointer = _sym.IsManagedVartype(vartype);
    // However, when the option is set then don't do this in import statements
    if (ScT::kImport == scope_type && FlagIsSet(_options, SCOPT_NOAUTOPTRIMPORT))
        managed_vartype_has_implied_pointer = false;        

    if (kKW_Dynpointer == _src.PeekNext() ||
        _sym.IsAutoptrVartype(vartype) ||
        managed_vartype_has_implied_pointer)
    {
        vartype = _sym.VartypeWithDynpointer(vartype);
    }
    EatDynpointerSymbolIfPresent(_src, vartype);
    
    // "int [] func(...)"
    ParseDynArrayMarkersIfPresent(_src, vartype);
    
    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    bool const no_loop_check = (kKW_Noloopcheck == _src.PeekNext());
    if (no_loop_check)
        _src.GetNext();

    // We've accepted a vartype expression and are now reading vars or one func that should have this type.
    ParseVartype_MemberList(tqs, vartype, scope_type, no_loop_check, struct_of_current_func, name_of_current_func);
}

void AGS::Parser::HandleEndOfCompoundStmts()
{
    while (_nest.TopLevel() > _sym.kFunctionScope)
        switch (_nest.Type())
        {
        default:
            InternalError("Nesting of unknown type ends");
            break; // can't be reached

        case NSType::kBraces:
        case NSType::kSwitch:
            // The body of those statements can only be closed by an explicit '}'.
            // So that means that there cannot be any more non-braced compound statements to close here.
            return;

        case NSType::kDo:
            HandleEndOfDo();
            break;

        case NSType::kElse:
            HandleEndOfElse();
            break;

        case NSType::kIf:
        {
            bool else_follows;
            HandleEndOfIf(else_follows);
            if (else_follows)
                return;
            break;
        }

        case NSType::kWhile:
            HandleEndOfWhile();
            break;
        } // switch (nesting_stack->Type())
}

void AGS::Parser::ParseReturn(Symbol name_of_current_func)
{
    Symbol const functionReturnType = _sym.FuncReturnVartype(name_of_current_func);

    if (kKW_Semicolon != _src.PeekNext())
    {
        if (functionReturnType == kKW_Void)
            UserError("Cannot return a value from a 'void' function");

        // parse what is being returned
        EvaluationResult eres;
        ParseExpression(_src, eres);
        EvaluationResultToAx(eres);
        ConvertAXStringToStringObject(functionReturnType, eres.Vartype);

        // check whether the return type is correct
        CheckVartypeMismatch(eres.Vartype, functionReturnType, true, "");
        if (_sym.IsOldstring(eres.Vartype) && eres.LocalNonParameter)
            UserError("Cannot return a local 'string' variable");
    }
    else if (_sym.IsAnyIntegerVartype(functionReturnType))
    {
        WriteCmd(SCMD_LITTOREG, SREG_AX, 0);
        _reg_track.SetRegister(SREG_AX);
    }
    else if (kKW_Void != functionReturnType)
    {
        UserError("Must return a '%s' value from function", _sym.GetName(functionReturnType).c_str());
	}
	
    Expect(kKW_Semicolon, _src.GetNext());
    
    _nest.JumpOutLevel() =
        std::min(_nest.JumpOutLevel(), _sym.kParameterScope);

    // If locals contain pointers, free them
    if (_sym.IsDynVartype(functionReturnType))
        FreeDynpointersOfAllLocals_DynResult(); // Special protection for result needed
    else if (kKW_Void != functionReturnType)
        FreeDynpointersOfAllLocals_KeepAX();
    else 
        FreeDynpointersOfLocals(0u);

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    // Pop the local variables proper from the stack but leave the parameters.
    // This is important because the return address is directly above the parameters;
    // we need the return address to return. (The caller will pop the parameters later.)
    RemoveLocalsFromStack(_sym.kFunctionScope);
  
    WriteCmd(SCMD_RET);

    // The locals only disappear if control flow actually follows the "return"
    // statement. Otherwise, below the statement, the locals remain on the stack.
    // So restore the OffsetToLocalVarBlock.
    _scrip.OffsetToLocalVarBlock = save_offset;
}

void AGS::Parser::ParseIf()
{
    EvaluationResult eres;
    ParseDelimitedExpression(_src, kKW_OpenParenthesis, eres);
    if (!_sym.IsAnyIntegerVartype(eres.Vartype) && !_sym.IsDynVartype(eres.Vartype))
        UserError(
            "Expected an integer or dynamic array or dynamic pointer expression after 'if', found type '%s' instead",
            _sym.GetName(eres.Vartype).c_str());
    EvaluationResultToAx(eres);

    _nest.Push(NSType::kIf);

    // The code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    WriteCmd(SCMD_JZ, kDestinationPlaceholder);
    _nest.JumpOut().AddParam();
}

void AGS::Parser::HandleEndOfIf(bool &else_follows)
{
    if (kKW_Else != _src.PeekNext())
    {
        else_follows = false;
        _nest.JumpOut().Patch(_src.GetLineno());
        _nest.Pop(); 
        return;
    }

    else_follows = true;
    SkipNextSymbol(_src, kKW_Else);
    _nest.BranchJumpOutLevel() = _nest.JumpOutLevel();
    _nest.JumpOutLevel() = _nest.kNoJumpOut;

    // Match the 'else' clause that is following to this 'if' stmt:
    // So we're at the end of the "then" branch. Jump out.
    _scrip.WriteCmd(SCMD_JMP, kDestinationPlaceholder);
    // So now, we're at the beginning of the "else" branch.
    // The jump after the "if" condition should go here.
    _nest.JumpOut().Patch(_src.GetLineno());
    // Mark the  out jump after the "then" branch, above, for patching.
    _nest.JumpOut().AddParam();
    // To prevent matching multiple else clauses to one if
    _nest.SetType(NSType::kElse);
}

void AGS::Parser::ParseWhile()
{
    // point to the start of the code that evaluates the condition
    CodeLoc const condition_eval_loc = _scrip.Codesize_i32();

    EvaluationResult eres;
    ParseDelimitedExpression(_src, kKW_OpenParenthesis, eres);
    if (!_sym.IsAnyIntegerVartype(eres.Vartype) && !_sym.IsDynVartype(eres.Vartype))
        UserError(
            "Expected an integer or dynamic array or dynamic pointer expression after 'while', found type '%s' instead",
            _sym.GetName(eres.Vartype).c_str());
    
    _nest.Push(NSType::kWhile);

    if (!(eres.kTY_Literal == eres.Type &&
        eres.kLOC_SymbolTable == eres.Location &&
        0 != _sym[eres.Symbol].LiteralD->Value))
    {
        EvaluationResultToAx(eres);
        // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
        WriteCmd(SCMD_JZ, kDestinationPlaceholder);
        _nest.JumpOut().AddParam();
    }
    _nest.Start().Set(condition_eval_loc);
}

void AGS::Parser::HandleEndOfWhile()
{
    // jump back to the start location
    _nest.Start().WriteJump(SCMD_JMP, _src.GetLineno());

    // This ends the loop.
    // Don't emit a 'linenum' directive: We can guarantee that we will only reach this place
    // from the 'while' clause line of the 'for', which has been emitted in the last 'linenum'.
    _nest.JumpOut().Patch(_src.GetLineno(), true);
    _nest.Pop();

    if (NSType::kFor != _nest.Type())
        return;

    // This is the outer level of the FOR loop.
    // It can contain defns, e.g., "for (int i = 0;...)".
    // (as if it were surrounded in braces). Free these definitions
    return HandleEndOfBraceCommand();
}

void AGS::Parser::ParseDo()
{
    _nest.Push(NSType::kDo);
    _nest.Start().Set();
}

void AGS::Parser::HandleEndOfBraceCommand()
{
    size_t const depth = _nest.TopLevel();
    FreeDynpointersOfLocals(depth);
    RemoveLocalsFromStack(depth);
    RestoreLocalsFromSymtable(depth);
    size_t const jumpout_level = _nest.JumpOutLevel();
    _nest.Pop();
    if (_nest.JumpOutLevel() > jumpout_level)
        _nest.JumpOutLevel() = jumpout_level;
}

void AGS::Parser::ParseAssignmentOrExpression()
{    
    // Get expression
    size_t const expr_start = _src.GetCursor();
    SkipToEndOfExpression(_src);
    SrcList expression = SrcList(_src, expr_start, _src.GetCursor() - expr_start);
    Symbol const next_sym = _src.PeekNext();
    if (_sym.IsIdentifier(next_sym) &&
        !_sym.IsPredefined(next_sym) &&
        _sym.kNoSrcLocation == _sym.GetDeclared(next_sym))
    {
        SkipNextSymbol(_src, next_sym);
        UserError("Identifier '%s' is undeclared (did you mis-spell it?)", _sym.GetName(next_sym).c_str());
    }
    if (expression.Length() == 0u)
        UserError("Expected an assignment or expression, found '%s' instead", _sym.GetName(next_sym).c_str());

    switch (next_sym)
    {
    default:
    {
        // No assignment symbol following: This is an isolated expression, e.g., a function call
        EvaluationResult eres;
        size_t const expr_end = _src.GetCursor();
        ParseExpression_Term(expression, eres, false);
        _src.SetCursor(expr_end);
        if (eres.kTY_FunctionName == eres.Type)
            Expect(kKW_OpenParenthesis, next_sym);
        if (eres.kTY_StructName == eres.Type)
            Expect(kKW_Dot, next_sym);
        if (!eres.SideEffects)
            Warning("This expression doesn't have any effect");
       
        return;
    }
	
    case kKW_Assign:
        return ParseAssignment_Assign(expression);

    case kKW_AssignBitAnd:
    case kKW_AssignBitOr:
    case kKW_AssignBitXor:
    case kKW_AssignDivide:
    case kKW_AssignMinus:
    case kKW_AssignModulo:
    case kKW_AssignMultiply:
    case kKW_AssignPlus:
    case kKW_AssignShiftLeft:
    case kKW_AssignShiftRight:
        return ParseAssignment_MAssign(next_sym, expression);
    }
}

void AGS::Parser::ParseFor_InitClauseVardecl()
{
    Vartype vartype = _src.GetNext();
    SetDynpointerInManagedVartype(vartype);
    EatDynpointerSymbolIfPresent(_src, vartype);
    
    while (true)
    {
        Symbol varname = _src.GetNext();
        Symbol const nextsym = _src.PeekNext();
        if (kKW_ScopeRes == nextsym || kKW_OpenParenthesis == nextsym)
            UserError("Function definition not allowed in 'for' loop initialiser");
        ParseVardecl(TypeQualifierSet{}, vartype, varname, ScT::kLocal);
        
        Symbol const punctuation = _src.PeekNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Comma == punctuation)
            SkipNextSymbol(_src, kKW_Comma);
        if (kKW_Semicolon == punctuation)
            return;
    }
}

// The first clause of a 'for' header
void AGS::Parser::ParseFor_InitClause(Symbol peeksym)
{
    if (kKW_Semicolon == peeksym)
        return; // Empty init clause
    if (_sym.IsVartype(peeksym))
        return ParseFor_InitClauseVardecl();
    return ParseAssignmentOrExpression();
}

void AGS::Parser::ParseFor_WhileClause()
{
    if (kKW_Semicolon == _src.PeekNext())
        // Not having a while clause means no check
        return;

    // Make sure that a linenumber bytecode is emitted
    _scrip.InvalidateLastEmittedLineno();
    EvaluationResult eres;
    ParseExpression(_src, eres);
    EvaluationResultToAx(eres);
    CheckVartypeMismatch(
        eres.Vartype,
        kKW_Int,
        true,
        "Second clause in 'for' statement");
}

void AGS::Parser::ParseFor_IterateClause()
{
    if (kKW_CloseParenthesis == _src.PeekNext())
        return; // iterate clause is empty

    // Make sure that a linenum pseudo-directive is emitted
    _scrip.InvalidateLastEmittedLineno();
    return ParseAssignmentOrExpression();
}

void AGS::Parser::ParseFor()
{
    // "for (I; E; C) {...}" is equivalent to "{ I; while (E) {...; C} }"
    // We implement this with TWO levels of the nesting stack: an outer and an inner level.
    // The outer level contains "I"
    // Then execution jumps to E
    // The inner level starts with C E
    // If E fails, execution exits the inner level to the rest of the outer level.
    // At the end of the inner level, execution jumps back up to C
    // The rest of the outer level frees I.

    // Outer nesting level
    _nest.Push(NSType::kFor);

    Expect(kKW_OpenParenthesis, _src.GetNext());
    
    Symbol const peeksym = _src.PeekNext();
    if (kKW_CloseParenthesis == peeksym)
        UserError("Empty parentheses '()' aren't allowed after 'for' (write 'for(;;)' instead");

    // Initialization clause (I)
    ParseFor_InitClause(peeksym);
    Expect(
        kKW_Semicolon,
        _src.GetNext(),
        "Expected ';' after the initializer clause of the 'for' loop ");

    // Inner nesting level
    _nest.Push(NSType::kWhile);
    _nest.Start().Set();

    // Parse the 'while' clause, then cut out its code and save it
    RestorePoint while_clause_start(_scrip);
    Snippet while_clause_snippet;
    ParseFor_WhileClause();
    Expect(
        kKW_Semicolon,
        _src.GetNext(),
        "Expected ';' after the condition clause of the 'for' loop");
    while_clause_start.Cut(while_clause_snippet, false);

    // Parse the 'iterate' clause, then cut out its code and save it
    RestorePoint iterate_clause_start(_scrip);
    Snippet iterate_clause_snippet;
    ParseFor_IterateClause();
    Expect(
        kKW_CloseParenthesis,
        _src.GetNext(),
        "Expected ')' after the iterator clause of the 'for' loop ");
    iterate_clause_start.Cut(iterate_clause_snippet, false);

    if (iterate_clause_snippet.IsEmpty())
    {
        // At the end of the 'for' construct or at a 'continue', 
        // we need to jump directly to the code of the 'while' clause (which will be inserted _here_).
        _nest.Start().Set();
    }
    else
    {
        // We need to insert a jump over the code for the 'iterate' clause
        // so that it won't be executed the first time that the 'for' construct runs.
        ForwardJump after_iterate_clause(_scrip);
        WriteCmd(SCMD_JMP, kDestinationPlaceholder);
        after_iterate_clause.AddParam();

        // At the end of the 'for' construct or at a 'continue',
        // we need to jump to the start of the 'iterate' clause (which will be inserted _here_)
        _nest.Start().Set();
        iterate_clause_snippet.Paste(_scrip);
        after_iterate_clause.Patch(_src.GetCursor()); 
    }

    // An empty 'while' clause is tantamount to a no-op, so only do things if it isn't empty.
    if (!while_clause_snippet.IsEmpty())
    {
        while_clause_snippet.Paste(_scrip);
        _scrip.WriteCmd(SCMD_JZ, kDestinationPlaceholder); // Don't emit a 'linenum' here
        _nest.JumpOut().AddParam();
    }
}

void AGS::Parser::ParseSwitch()
{
    // Get the switch expression
    EvaluationResult eres;
    ParseDelimitedExpression(_src, kKW_OpenParenthesis, eres);

    Expect(kKW_OpenBrace, _src.GetNext());
    if (kKW_CloseBrace == _src.PeekNext())
    {
        // A switch without any clauses, tantamount to a NOP
        // Don't throw away the code that was generated for the switch expression:
        // It might have side effects!
        SkipNextSymbol(_src, kKW_CloseBrace); 
        return;
    }

    // Copy the result to the BX register, ready for case statements
    EvaluationResultToAx(eres);
    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_BX);
    _reg_track.SetRegister(SREG_BX);

    _nest.Push(NSType::kSwitch);
    _nest.SetSwitchExprVartype(eres.Vartype);

    // Jump to the jump table
    _scrip.WriteCmd(SCMD_JMP, kDestinationPlaceholder);
    _nest.SwitchJumptable().AddParam();

    return Expect(SymbolList{ kKW_Case, kKW_Default, }, _src.PeekNext());
}

void AGS::Parser::ParseSwitchFallThrough()
{
    if (NSType::kSwitch != _nest.Type())
        UserError("'%s' is only allowed directly within a 'switch' block", _sym.GetName(kKW_FallThrough).c_str());
    Expect(kKW_Semicolon, _src.GetNext());
    return Expect(SymbolList{ kKW_Case, kKW_Default }, _src.PeekNext());
}

void AGS::Parser::ParseSwitchLabel(Symbol case_or_default)
{
    RestorePoint switch_label_start(_scrip);
    CodeLoc const start_of_code_loc = _scrip.Codesize_i32();
    size_t const start_of_fixups = _scrip.fixups.size();
    size_t const start_of_code_lineno = _src.GetLineno();

    if (NSType::kSwitch != _nest.Type())
        UserError("'%s' is only allowed directly within a 'switch' block", _sym.GetName(case_or_default).c_str());

    if (!_nest.SwitchCaseStart().empty())
    {
        if (_nest.SwitchCaseStart().back().Get() != start_of_code_loc &&
            _nest.JumpOutLevel() > _nest.TopLevel())
        {
            // Don't warn if 'fallthrough;' immediately precedes the 'case' or 'default'
            int const codeloc = _src.GetCursor();
            if (kKW_Semicolon != _src[codeloc - 2] || kKW_FallThrough != _src[codeloc - 3])
                Warning("Code execution may fall through to the next case (did you forget a 'break;'?)");
            _src.SetCursor(codeloc);
        }

        _nest.BranchJumpOutLevel() =
            std::max(_nest.BranchJumpOutLevel(), _nest.JumpOutLevel());
    }
    _nest.JumpOutLevel() = _nest.kNoJumpOut;

    BackwardJumpDest case_code_start(_scrip);
    case_code_start.Set();
    _nest.SwitchCaseStart().push_back(case_code_start);

    if (kKW_Default == case_or_default)
    {
        if (NestingStack::kNoDefault != _nest.SwitchDefaultIdx())
            UserError("This switch block already has a 'default:' label");
        _nest.SwitchDefaultIdx() = _nest.SwitchCaseStart().size() - 1u;
    }
    else // "case"
    {
        // Compile a comparison of the switch expression result (which is in SREG_BX)
        // to the current case

        EvaluationResult eres;
        RegisterGuard(SREG_BX,
            [&]
            {
                ParseExpression(_src, eres);
                EvaluationResultToAx(eres);
            });
                        
        // Vartypes of the "case" expression and the "switch" expression must match
        CheckVartypeMismatch(eres.Vartype, _nest.SwitchExprVartype(), false, "");
    }

    // Rip out the already generated code for the case expression and store it with the switch
    // In order to process sequences of 'case'/'default' statements without intervening code,
    //      don't keep the starting 'linenum' directive in the code
    //          that the 'case'/'default' may have generated,
    //      but do invalidate line numbers so that a 'linenum' directive
    //          will be generated as soon as "real" code comes up.
    Snippet snippet;
    switch_label_start.Cut(snippet, false);
    _nest.Snippets().push_back(snippet);
    _scrip.InvalidateLastEmittedLineno();
    return Expect(kKW_Colon, _src.GetNext());
}

void AGS::Parser::RemoveLocalsFromStack(size_t nesting_level)
{
    size_t const size_of_local_vars = StacksizeOfLocals(nesting_level);
    if (size_of_local_vars > 0u)
    {
        _scrip.OffsetToLocalVarBlock -= size_of_local_vars;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_local_vars);
    }
}

void AGS::Parser::SetCompileTimeLiteral(Symbol const lit, EvaluationResult &eres)
{
    if (!_sym.IsLiteral(lit))
        InternalError("'%s' isn't literal", _sym.GetName(lit).c_str());

    eres.Type = eres.kTY_Literal;
    eres.Location = eres.kLOC_SymbolTable;
    eres.Symbol = lit;
    eres.Vartype = _sym[lit].LiteralD->Vartype;
    eres.Modifiable = false;
    eres.LocalNonParameter = false;

    if (kKW_String == _sym.VartypeWithout(VTT::kConst, eres.Vartype))
        EvaluationResultToAx(eres); // Cannot handle string literals
}

void AGS::Parser::FindOrAddIntLiteral(CodeCell value, Symbol &symb)
{
    std::string const valstr = std::to_string(value);
    symb = _sym.Find(valstr);
    if (kKW_NoSymbol != symb)
    {
        if (_sym.IsLiteral(symb))
            return;
        InternalError("'%s' should be an integer literal but isn't.", valstr.c_str());
    }

    symb = _sym.Add(valstr);
    _sym.MakeEntryLiteral(symb);
    _sym[symb].LiteralD->Vartype = kKW_Int;
    _sym[symb].LiteralD->Value = value;
}

void AGS::Parser::ParseBreak()
{
    Expect(kKW_Semicolon, _src.GetNext());
    
    // Find the (level of the) looping construct to which the break applies
    // Note that this is similar, but _different_ from what happens at 'continue'.
    size_t nesting_level;
    for (nesting_level = _nest.TopLevel(); nesting_level > 0u; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kSwitch == ltype || NSType::kWhile == ltype)
            break;
    }

    if (0u == nesting_level)
        UserError("Can only use 'break' inside a loop or a 'switch' statement block");

    _nest.JumpOutLevel() = std::min(_nest.JumpOutLevel(), nesting_level);

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    FreeDynpointersOfLocals(nesting_level + 1u);
    RemoveLocalsFromStack(nesting_level + 1u);
    
    // Jump out of the loop or switch
    WriteCmd(SCMD_JMP, kDestinationPlaceholder);
    _nest.JumpOut(nesting_level).AddParam();

    // The locals only disappear if control flow actually follows the 'break' statement. 
    // Otherwise, i.e., below the statement, the locals remain on the stack.
    // So restore the OffsetToLocalVarBlock.
    _scrip.OffsetToLocalVarBlock = save_offset;
}

void AGS::Parser::ParseContinue()
{
    Expect(kKW_Semicolon, _src.GetNext());
    
    // Find the level of the looping construct to which the 'continue' applies
    // Note that this is similar, but _different_ from what happens at 'break'.
    size_t nesting_level;
    for (nesting_level = _nest.TopLevel(); nesting_level > 0u; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kWhile == ltype)
            break;
    }

    if (nesting_level == 0u)
        UserError("Can only use 'continue' inside a loop");

    _nest.JumpOutLevel() = std::min(_nest.JumpOutLevel(), nesting_level);

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    FreeDynpointersOfLocals(nesting_level + 1u);
    RemoveLocalsFromStack(nesting_level + 1u);

    // Jump to the start of the loop
    _nest.Start(nesting_level).WriteJump(SCMD_JMP, _src.GetLineno());

    // The locals only disappear if control flow actually follows the 'continue' statement. 
    // Otherwise, i.e., below the statement, the locals remain on the stack.
    // So restore the OffsetToLocalVarBlock.
    _scrip.OffsetToLocalVarBlock = save_offset;
}

void AGS::Parser::ParseOpenBrace(Symbol struct_of_current_func, Symbol name_of_current_func)
{
    if (_sym.kParameterScope == _nest.TopLevel())
        return ParseFuncBodyStart(struct_of_current_func, name_of_current_func);
    _nest.Push(NSType::kBraces);
}

void AGS::Parser::ParseCommand(Symbol leading_sym, Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
    if (kKW_CloseBrace != leading_sym &&
        kKW_Case != leading_sym &&
        kKW_Default != leading_sym)
    {
        if (!_nest.DeadEndWarned() && _nest.JumpOutLevel() < _nest.TopLevel())
        {
            Warning("Code execution cannot reach this point");
            _nest.DeadEndWarned() = true;
        }
    }

    // NOTE that some branches of this switch will leave
    // the whole function, others will continue after the switch.
    switch (leading_sym)
    {
    default:
    {
        // No keyword, so it should be an assignment or an isolated expression
        _src.BackUp();
        ParseAssignmentOrExpression();
        Expect(kKW_Semicolon, _src.GetNext());
        break;
    }

    case kKW_Break:
        ParseBreak();
        break;

    case kKW_Case:
        ParseSwitchLabel(leading_sym);
        break;

    case kKW_CloseBrace:
        // Note that the scanner has already made sure that every close brace has an open brace
        if (_sym.kFunctionScope >= _nest.TopLevel())
            return HandleEndOfFuncBody(struct_of_current_func, name_of_current_func);

        if (NSType::kSwitch == _nest.Type())
            HandleEndOfSwitch();
		else
			HandleEndOfBraceCommand();
        break;

    case kKW_Continue:
        ParseContinue();
        break;

    case kKW_Default:
        ParseSwitchLabel(leading_sym);
        break;

    case kKW_Do:
        return ParseDo();

    case kKW_Else:
        UserError("Cannot find any 'if' clause that matches this 'else'");

    case kKW_FallThrough:
        ParseSwitchFallThrough();
        break;

    case kKW_For:
        return ParseFor();

    case kKW_If:
        return ParseIf();

    case kKW_OpenBrace:
        if (PP::kPreAnalyze == _pp)
        {
            struct_of_current_func = name_of_current_func = kKW_NoSymbol;
            _src.SkipToCloser();
            SkipNextSymbol(_src, kKW_CloseBrace);
            return;
        }
        return ParseOpenBrace(struct_of_current_func, name_of_current_func);

    case kKW_Return:
        ParseReturn(name_of_current_func);
        break;

    case kKW_Switch:
        ParseSwitch();
        break;

    case kKW_While:
        // This cannot be the end of a do...while() statement
        // because that would have been handled in HandleEndOfDo()
        return ParseWhile();
    }

    // This statement may be the end of some unbraced
    // compound statements, e.g. "while (...) if (...) i++";
    // Pop the nesting levels of such statements and handle
    // the associated jumps.
    return HandleEndOfCompoundStmts();
}

void AGS::Parser::RegisterGuard(RegisterList const &guarded_registers, std::function<void(void)> block)
{
    RestorePoint rp(_scrip);
    RegisterTracking::TickT const tick_at_start = _reg_track.GetTick();
    size_t const cursor_at_start = _src.GetCursor();

    // Save the current MAR manager in case it gets clobbered and needs to be restored
    MarMgr save_mar_state(_marMgr);

    RegisterTracking::TickT register_set_point[CC_NUM_REGISTERS] = {};
    for (auto it = guarded_registers.begin(); it != guarded_registers.end(); ++it)
        register_set_point[*it] = _reg_track.GetRegister(*it);

    // Tentatively evaluate the block to find out what it clobbers
    block();
    
    // Find out what guarded registers have been clobbered since start of block
    std::vector<size_t> pushes;
    for (auto it = guarded_registers.begin(); it != guarded_registers.end(); ++it)
        if (!_reg_track.IsValid(*it, tick_at_start))
            pushes.push_back(*it);
    if (pushes.empty())
        return;

    // Need to redo this, some registers are clobbered that should not be
    // Note, we cannot simply rip out the code, insert the 'push'es
    // and put the code in again: The 'push'es alter the stack size,
    // and the ripped code may depend on the stack size.
    rp.Restore();

    for (auto it = pushes.begin(); it != pushes.end(); ++it)
    {
        PushReg(*it);
        _reg_track.SetRegister(*it, register_set_point[*it]);
    }
    _src.SetCursor(cursor_at_start);
    block();
    for (auto it = pushes.rbegin(); it != pushes.rend(); ++it)
    {
        PopReg(*it);
        if (*it == SREG_MAR)
            _marMgr = save_mar_state; // Restore potentially clobbered MAR manager
        // We know that we're popping the same register that we've pushed,
        // so it is safe to reset the set point to the point that was
        // valid at the time of that push.
        _reg_track.SetRegister(*it, register_set_point[*it]);
    }
}

void AGS::Parser::HandleSrcSectionChangeAt(size_t pos)
{
    size_t const src_section_id = _src.GetSectionIdAt(pos);
    if (src_section_id == _lastEmittedSectionId)
        return;

    if (PP::kMain == _pp)
    {
        if (_scrip.StartNewSection(_src.SectionId2Section(src_section_id)) < 0)
        {
            // If there's not enough memory to allocate a string, then there sure won't be enough
            // memory for the error message either. Still let's try and hope for the best
            InternalError("Cannot allocate memory for the section name");
        }
    }
    _lastEmittedSectionId = src_section_id;
}

void AGS::Parser::ParseInput()
{
    Parser::NestingStack nesting_stack(_scrip);
    size_t nesting_level = 0u;

    // We start off in the global data part - no code is allowed until a function definition is started
    Symbol struct_of_current_func = kKW_NoSymbol; // non-zero only when a struct member function is open
    Symbol name_of_current_func = kKW_NoSymbol;

    // Collects vartype qualifiers such as 'readonly'
    TypeQualifierSet tqs = {};

    while (!_src.ReachedEOF())
    {
        size_t const next_pos = _src.GetCursor();
        HandleSrcSectionChangeAt(next_pos);
        currentline = _src.GetLinenoAt(next_pos);

        ParseQualifiers(tqs);
        
        Symbol const leading_sym = _src.GetNext();

        // Vartype clauses

        switch (leading_sym)
        {
        case kKW_Attribute:
            ParseAttribute(tqs, name_of_current_func);
            continue;
        
        case kKW_Const:
            if (kKW_String == _src.PeekNext())
                break; // Vartype, treated below

            Parse_CheckTQSIsEmpty(tqs);
            ParseConstantDefn();
            continue;

        case kKW_Enum:
            Parse_CheckTQ(tqs, (name_of_current_func > 0), false);
            ParseEnum(tqs, struct_of_current_func, name_of_current_func);
            continue;

        case kKW_Export:
            Parse_CheckTQSIsEmpty(tqs);
            ParseExport();
            continue;

        case kKW_Struct:
            Parse_CheckTQ(tqs, (name_of_current_func > 0), false);
            ParseStruct(tqs, struct_of_current_func, name_of_current_func);
            continue;
        } // switch (leading_sym)

        if (kKW_Const == leading_sym || (_sym.IsVartype(leading_sym) && kKW_Dot != _src.PeekNext()))
        {
            // Note: We cannot check yet whether the TQS are legal because we don't know whether the
            // var / func names that will be defined will be composite.
            ParseVartypeClause(tqs, struct_of_current_func, name_of_current_func);
            continue;
        }

        // Command clauses

        if (kKW_NoSymbol == name_of_current_func)
            UserError("Cannot start a definition or declaration with '%s'", _sym.GetName(leading_sym).c_str());

        Parse_CheckTQSIsEmpty(tqs);
        ParseCommand(leading_sym, struct_of_current_func, name_of_current_func);
    } // while (!targ.reached_eof())
}

void AGS::Parser::Parse_ReinitSymTable(size_t size_after_scanning)
{
    for (size_t sym_idx = _sym.GetLastAllocated() + 1; sym_idx < _sym.entries.size(); sym_idx++)
    {
        SymbolTableEntry &s_entry = _sym[sym_idx];

        if (_sym.IsFunction(sym_idx))
        {
            s_entry.FunctionD->TypeQualifiers[TQ::kImport] = (kFT_Import == s_entry.FunctionD->Offset);
            s_entry.FunctionD->Offset = kDestinationPlaceholder;
            continue;
        }

        if (_sym.IsLiteral(sym_idx))
            continue; 

        s_entry.Clear(); // note, won't (and shouldn't) clear the Name field
    }

    // This has invalidated the symbol table caches, so kill them
    _sym.ResetCaches();
}

void AGS::Parser::Parse_BlankOutUnusedImports()
{
    for (size_t entries_idx = 0u; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].Accessed)
            continue;

        // Don't "compact" the entries in '_scrip.imports[]'. They are referenced by index, and if you
        // change the indexes of the entries then you get dangling "references". So the only thing allowed is
        // setting unused import entries to "".
        if (_sym.IsFunction(entries_idx))
        {
            if(_sym[entries_idx].FunctionD->TypeQualifiers[TQ::kImport])
                _scrip.imports[_sym[entries_idx].FunctionD->Offset].clear();
            continue;
        }
        if (_sym.IsVariable(entries_idx))
        {
            if (_sym[entries_idx].VariableD->TypeQualifiers[TQ::kImport])
                _scrip.imports[_sym[entries_idx].VariableD->Offset].clear();
            continue;
        }
    }
}

void AGS::Parser::Error(bool is_internal, std::string const &message)
{
   _msgHandler.AddMessage(
        is_internal? MessageHandler::kSV_InternalError : MessageHandler::kSV_UserError,
        _src.SectionId2Section(_src.GetSectionId()),
        _src.GetLineno(),
        is_internal ? ("Internal error: " + message) : message);

    // Set a breakpoint here to stop the compiler as soon as an error happens,
    // before the call stack has unwound:
    throw CompilingError(message);
}

void AGS::Parser::UserError(char const *msg, ...)
{
    va_list vlist1, vlist2;
    va_start(vlist1, msg);
    va_copy(vlist2, vlist1);
    size_t const needed_len = vsnprintf(nullptr, 0u, msg, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(message.data(), msg, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    Error(false, message.data());
}

void AGS::Parser::InternalError(char const *descr, ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    // '+ 1u' for the trailing '\0'
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(message.data(), descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    Error(true, message.data());
}

void AGS::Parser::Warning(char const *descr, ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    // '+ 1u' for the trailing '\0'
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(message.data(), descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msgHandler.AddMessage(
        MessageHandler::kSV_Warning,
        _src.SectionId2Section(_src.GetSectionId()),
        _src.GetLineno(),
        &message[0u]);
}

void AGS::Parser::Parse_PreAnalyzePhase()
{
    size_t const sym_size_after_scanning  = _sym.entries.size();

    _pp = PP::kPreAnalyze;
    ParseInput();

    // Keep (just) the headers of functions that have a body to the main symbol table
    // Reset everything else in the symbol table,
    // but keep the entries so that they are guaranteed to have
    // the same index when parsed in phase 2
    return Parse_ReinitSymTable(sym_size_after_scanning);
}

void AGS::Parser::Parse_MainPhase()
{
    _pp = PP::kMain;
    return ParseInput();
}

void AGS::Parser::Parse_CheckForUnresolvedStructForwardDecls()
{
    for (auto it = _structRefs.cbegin(); it != _structRefs.cend(); ++it)
    {
        auto &stname = it->first;
        auto &src_location = it->second;
        if (_sym[stname].VartypeD->Flags[VTF::kUndefined])
        {
            _src.SetCursor(src_location);
            UserError(
                ReferenceMsgSym("Struct '%s' is used but never completely defined", stname).c_str(),
                _sym.GetName(stname).c_str());
        }
    }
}

void AGS::Parser::Parse_CheckFixupSanity()
{
    for (size_t fixup_idx = 0u; fixup_idx < _scrip.fixups.size(); fixup_idx++)
    {
        if (FIXUP_IMPORT != _scrip.fixuptypes[fixup_idx])
            continue;
        int const code_idx = _scrip.fixups[fixup_idx];
        if (code_idx < 0 || static_cast<size_t>(code_idx) >= _scrip.code.size())
            InternalError(
                "!Fixup #%d references non-existent code offset #%d",
                fixup_idx,
                code_idx);
        int const cv = _scrip.code[code_idx];
        if (cv < 0 || static_cast<size_t>(cv) >= _scrip.imports.size() || _scrip.imports[cv].empty())
            InternalError(
                "Fixup #%d references non-existent import #%d",
                fixup_idx,
                cv);
    }
}

void AGS::Parser::Parse_ExportAllFunctions()
{
    for (size_t func_idx = 0u; func_idx < _scrip.Functions.size(); func_idx++)
    {
        if (0 > _scrip.AddExport(
            _scrip.Functions[func_idx].Name,
            _scrip.Functions[func_idx].CodeOffs,
            _scrip.Functions[func_idx].ParamsCount))
            InternalError("Could not export function '%s', exports table overflow?", _scrip.Functions[func_idx].Name.c_str());
    }
}

void AGS::Parser::Parse()
{
    try
    {
        CodeLoc const start_of_input = _src.GetCursor();

        Parse_PreAnalyzePhase();
        
        _src.SetCursor(start_of_input);
        Parse_MainPhase();

        _scrip.ReplaceLabels();
        Symbol first_unresolved_function = _callpointLabels.GetFirstUnresolvedFunction();
        if (kKW_NoSymbol != first_unresolved_function)
        {
            // We're at the end of the file, so set the cursor to the loc of the function
            size_t const error_loc = _sym[first_unresolved_function].Declared;
            if (error_loc != SymbolTable::kNoSrcLocation)
                _src.SetCursor(error_loc);
            UserError(
                "The local function '%s' is never defined with body (did you forget 'import'?)",
                _sym.GetName(first_unresolved_function).c_str());
        }

        first_unresolved_function = _importLabels.GetFirstUnresolvedFunction();
        if (kKW_NoSymbol != first_unresolved_function)
            // This shouldn't be possible.
            InternalError(
                "The 'import' function '%s' has been referenced in this file but never defined",
                _sym.GetName(first_unresolved_function).c_str());

        Parse_CheckForUnresolvedStructForwardDecls();

        if (FlagIsSet(_options, SCOPT_EXPORTALL))
			Parse_ExportAllFunctions();
        Parse_BlankOutUnusedImports();
        return Parse_CheckFixupSanity();
    }
    catch (CompilingError &)
    {
        // Message handler already has the error, can simply continue
    }
    catch (std::exception const &e)
    {
        std::string msg = "Exception encountered at currentline = <line>: ";
        string_replace(msg, "<line>", std::to_string(currentline));
        msg.append(e.what());

        _msgHandler.AddMessage(
            MessageHandler::kSV_InternalError,
            _src.SectionId2Section(_src.GetSectionId()),
            _src.GetLineno(),
            msg);
    }
}

int cc_compile(std::string const &inpl, AGS::FlagSet options, AGS::ccCompiledScript &scrip, AGS::MessageHandler &mh)
{
    AGS::SymbolTable local_symt;
    AGS::SectionList local_secs;
    return cc_compile(inpl, options, scrip, local_symt, local_secs, mh);
}

int cc_compile(std::string const &inpl, AGS::FlagSet options, AGS::ccCompiledScript &scrip,
    AGS::SymbolTable &symt, AGS::SectionList &sections, AGS::MessageHandler &mh)
{
    std::vector<AGS::Symbol> symbols;
    AGS::LineHandler lh;
    size_t cursor = 0u;
    AGS::SrcList src = AGS::SrcList(symbols, lh, cursor);
    src.NewSection("UnnamedSection");
    src.NewLine(1u);

    AGS::Scanner scanner = { inpl, FlagIsSet(options, SCOPT_UTF8), src, scrip, symt, mh };
    scanner.Scan();
    if (mh.HasError())
        return -1;
    
    AGS::Parser parser = { src, options, scrip, symt, mh };
    parser.Parse();
    if (mh.HasError())
        return -2;

    sections = lh.CreateSectionList();
    return 0;
}
