/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - IMPLEMENTATION

General:
Functions have names of the form AaaAaa or AaaAaa_BbbBbb
where the component parts are camelcased. This means that function AaaAaa_BbbBbb is a
subfunction of function AaaAaa that is exclusively called by function AaaAaa.

The Parser does does NOT get the sequence of tokens in a pipe from the scanning step, i.e.,
it does NOT read the symbols one-by-one. To the contrary, the logic reads back and forth in
the token sequence.

In case of an error, all parser functions call UserError() or InternalError().
These functions throw an exception that is caught in Parser::Parse().
If you break on this 'throw' command, the compiler is nicely stopped before the call stack has unwound.

The Parser runs in two phases.
The first phase runs quickly through the tokenized source and collects the headers
of the local functions.

The second phase has the following main components:
    Declaration parsing
    Command parsing
        Functions that process the keyword Kkk are called ParseKkk()

    Code nesting and compound statements
        In ParseWhile() etc., HandleEndOf..(), and class AGS::Parser::NestingStack.

    Expression parsing
        In ParseExpression()

    Memory access
        In AccessData()
        In order to read data or write to data, the respective piece of data must
        be located first. This also encompasses literals of the program code.
        Note that "." and "[]" are not treated as normal operators (operators like +).
        The memory offset of struct components in relation to the
        location of the respective struct is calculated at compile time, whereas array
        offsets are calculated at run time.

Notes on how nested statements are handled:
    When handling nested constructs, the parser sometimes generates and emits some code,
    then rips it out of the codebase and stores it internally, then later on, retrieves
    it and emits it into the codebase again.

Oldstyle strings, string literals, string buffers:
    If a "string" is declared, 200 bytes of memory are reserved on the stack (local) or in
    global memory (global). This is called a "string buffer". Whenever oldstyle strings or
    literal strings are used, they are referred to by the address of their first byte.
    The only way of modifying a string buffer is by functions. However, string buffer
    assignments are possible and handled with inline code. The compiler doesn't attempt in
    any way to prevent buffer underruns or overruns.


MEMORY LAYOUT

Global variables go into their own dedicated memory block and are addressed relatively to the beginning of that block.
	This block is initialized with constant values at the start of the game. So it is possible to set the start value of
    globals to some constant value, but it is not possible to _calculate_ it at the start of the runtime.
    In particular, initial pointer values and initial String values can only be given as null because any
    other value would entail a runtime computation.

Literal strings go into their own, dedicated memory block and are also addressed relatively to the beginning of that block.
	The scanner populates this memory block; for the parser, the whole block is treated as constant and read-only.

Imported values are treated as if they were global values. However, their exact location is only computed at runtime by the
    linker. For the purposes of the parser, imported values are assigned an ordinal number #0, #1, #2 etc. and are referenced
    by their ordinal number.

Local variables go into a memory block, the "local memory block", that is reserved on the stack.
 	They are addressed relatively to the start of that block. The start of this block can always be determined at
 	compile time by subtracting a specific offset from the stack pointer, namely OffsetToLocalVarBlock.
 	This offset changes in the course of the compilation but can always be determined at compile time.
 	The space for local variables is reserved on the stack at runtime when the respective function is called.
    Therefore, local variables can be initialized to any value that can be computed,
    they aren't restricted to compile time constants as the global variables are.

A local variable is declared within a nesting of { ... } in the program code;
    It becomes valid at the point of declaration and it becomes invalid when the closing '}' to the
    innermost open '{' is encountered. In the course of reading the program from beginning to end,
    the open '{' that have not yet been closed form a stack called the "nesting stack".
    Whenever a '{' is encountered, the nesting stack gets an additional level; whenever a '}' is encountered,
    the topmost level is popped from the stack.
        Side Note: Compound statements can have a body that is NOT surrounded with braces, e.g.,
        "if (foo) i++;" instead of "if (foo) { i++; }". In this case the nesting stack is
        still extended by one level before the compound statement body is processed and
        reduced by one level afterwards.
  
    The depth of the stack plus 1 is called the nested depth or scope. Each local variable is assigned
 	the nested depth of its point of declaration.

    When program flow passes a closing '}' then all the variables with higher nested depth can be freed.
    This shortens the local memory block from the end; its start remains unchanged.
    "continue", "break" and "return" statements can break out of several '}' at once. In this case,
    all their respective variables must be freed.

    Class NestingStack keeps information on the nested level of code.
    For each nested level, the class keeps, amongst others, the location in the bytecode
    of the start of the construct and the location of a Bytecode jump to its end.

Parameters of a function are local variables; they are assigned the nested depth 1.
    Only parameters can have the nested depth 1.
    The first parameter of a function is also the first parameter in the local variable block. To make this happen,
    the parameters must be pushed back-to-front onto the stack when the function is called,
    i.e. the last function parameter must be pushed first.

    Global, imported variables, literal constants and strings are valid from the point of declaration onwards
    until the end of the compilation unit; their assigned nested depth is 0.

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
    Memory that is allocated with "new" is allocated dynamically by the Engine. The compiler need not be concerned how it is done.
	The compiler also needn't concern itself with freeing the dynamic memory itself; this is the Engine's job, too.
    However, the compiler must declare that a memory cell shall hold a pointer to dynamic memory, by using the opcode MEMWRITEPTR.
    And when a memory cell is no longer reserved for pointers, this must be declared as well, using the opcode MEMZEROPTR.
		Side note: Before a function is called, all its parameters are "pushed" to the stack using the PUSHREG opcode.
		So when some parameters are pointers then the fact that the respective memory cells contain a pointer isn't declared yet.
		So first thing at the start of the function, all pointer parameters must be read with normal non-...PTR opcodes
        and then written into the same place as before using the special opcode MEMWRITEPTR.
		Side note: When a '}' is reached and local pointer variables are concerned, it isn't enough to just shorten the
		local memory block. On all such pointer variables, MEMZEROPTR must be applied first to declare that the respective memory
	    cells won't necessarily contain pointers any more.

    Any address that should hold a pointer must be manipulated using the SCMD_...PTR form of the opcodes

    There are only two kinds of dynamic memory blocks:
        memory blocks that do not contain any pointers to dynamic memory whatsoever
        memory blocks that completely consist of pointers to dynamic memory ONLY.
    (This is an engine restriction pertaining to memory serialization, not a compiler restriction.)

    A Dynarray of primitives (e.g., int[]) is represented in memory as a pointer to a memory
    block that comprises all the elements, one after the other.
    [*]->[][]...[]
    A Dynarray of structs must be a dynarray of managed structs. It is represented in
    memory as a pointer to a block of pointers, each of which points to one element.
    [*]->[*][*]...[*]
          |  |     |
          V  V ... V
         [] [] ... []
    In contrast to a dynamic array, a classic array is never managed.
    A classic array of primitives (e.g., int[12]) or of non-managed structs is represented
    in memory as a block of those elements.
    [][]...[]
    A classic array of managed structs is a classic array of pointers,
    each of which points to a memory block that contains one element.
    [*][*]...[*]
     |  |     |
     V  V ... V
    [] [] ... []

Pointers are exclusively used for managed memory. If managed structs are manipulated,
    pointers MUST ALWAYS be used; for un-managed structs, pointers MAY NEVER be used.
    However as an exception, in import statements non-pointed managed structs can be used, too.
    That means that the compiler can deduce whether a pointer is expected by
    looking at the keyword "managed" of the struct alone -- except in global import declarations.
    Blocks of primitive vartypes can be allocated as managed memory, in which case pointers
    MUST be used. Again, the compiler can deduce from the declaration that a pointer MUST be
    used in this case.
*/


#include <string>
#include <fstream>
#include <cmath>
#include <climits>
#include <memory>

#include "util/string.h"

#include "script/cc_common.h"

#include "cc_internallist.h"
#include "cc_symboltable.h"

#include "cs_parser_common.h"
#include "cs_scanner.h"
#include "cs_parser.h"

// Declared in Common/script/cc_common.h 
// Defined in Common/script/cc_common.cpp
extern int currentline;

char ccCopyright2[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2020 others";

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

void AGS::Parser::SkipTo(SymbolList const &stoplist, SrcList &source)
{
    int delimeter_nesting_depth = 0;
    for (; !source.ReachedEOF(); source.GetNext())
    {
        // Note that the scanner/tokenizer has already verified
        // that all opening symbols get closed and 
        // that we don't have (...] or similar in the input
        Symbol const next_sym = _src.PeekNext();
        switch (next_sym)
        {
        case kKW_OpenBrace:
        case kKW_OpenBracket:
        case kKW_OpenParenthesis:
            ++delimeter_nesting_depth;
            continue;
        
        case kKW_CloseBrace:
        case kKW_CloseBracket:
        case kKW_CloseParenthesis:
            if (--delimeter_nesting_depth < 0)
                return;
            continue;
        
        }
        if (0 < delimeter_nesting_depth)
            continue;

        for (auto it = stoplist.begin(); it != stoplist.end(); ++it)
            if (next_sym == *it)
                return;
    }
}

void AGS::Parser::SkipToClose(Predefined closer)
{
    SkipTo(SymbolList{}, _src);
    Symbol const punctuation = _src.GetNext();
    if (closer != punctuation)    
		InternalError("Unexpected closing symbol %s", _sym.GetName(punctuation).c_str());
}

void AGS::Parser::Expect(SymbolList const &expected, Symbol actual, std::string const &custom_msg)
{
    for (size_t expected_idx = 0; expected_idx < expected.size(); expected_idx++)
        if (actual == expected[expected_idx])
            return;

    std::string errmsg = custom_msg;
    if (errmsg == "")
    {
        // Provide a default message
        errmsg = "Expected ";
        for (size_t expected_idx = 0; expected_idx < expected.size(); expected_idx++)
        {
            errmsg += "'" + _sym.GetName(expected[expected_idx]) + "'";
            if (expected_idx + 2 < expected.size())
                errmsg += ", ";
            else if (expected_idx + 2 == expected.size())
                errmsg += " or ";
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
    _componentOffs = 0;
}

void AGS::Parser::MarMgr::UpdateMAR(size_t lineno, ccCompiledScript &scrip)
{
    switch (_scType)
    {
    default:
        // The start offset is already reached (e.g., when a Dynpointer chain is dereferenced) 
        // but the component offset may need to be processed.
        if (_componentOffs > 0)
            scrip.WriteCmd(SCMD_ADD, SREG_MAR, _componentOffs);
        break;

    case ScT::kGlobal:
        scrip.RefreshLineno(lineno);
        scrip.WriteCmd(SCMD_LITTOREG, SREG_MAR, _startOffs + _componentOffs);
        scrip.FixupPrevious(Parser::kFx_GlobalData);
        break;

    case ScT::kImport:
        // Have to convert the import number into a code offset first.
        // Can only then add the offset to it.
        scrip.RefreshLineno(lineno);
        scrip.WriteCmd(SCMD_LITTOREG, SREG_MAR, _startOffs);
        scrip.FixupPrevious(Parser::kFx_Import);
        if (_componentOffs != 0)
            scrip.WriteCmd(SCMD_ADD, SREG_MAR, _componentOffs);
        break;

    case ScT::kLocal:
        scrip.RefreshLineno(lineno);
        CodeCell const offset = scrip.OffsetToLocalVarBlock - _startOffs - _componentOffs;
        if (offset < 0)
            // Must be a bug: That memory is unused.
            _parser.InternalError("Trying to emit the negative offset %d to the top-of-stack", (int) offset);

        scrip.WriteCmd(SCMD_LOADSPOFFS, offset);
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
    size_t oldest_reg = INT_MAX;
    TickT oldest_tick = ULONG_MAX;
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
    _lastEmittedSectionId = 0;
    _lastEmittedLineno = 0;
}

void AGS::Parser::SetDynpointerInManagedVartype(Vartype &vartype)
{
    if (_sym.IsManagedVartype(vartype))
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
}

size_t AGS::Parser::StacksizeOfLocals(size_t from_level)
{
    size_t total_size = 0;
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

// Does vartype v contain releasable pointers?
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
    for (size_t cl_idx = 0; cl_idx < compo_list.size(); cl_idx++)
    {
        Symbol const &var = compo_list[cl_idx];
        if (!_sym.IsVariable(var))
            continue;
        if (ContainsReleasableDynpointers(_sym[var].VariableD->Vartype))
            return true;
    }
    return false;
}

// We're at the end of a block and releasing a standard array of pointers.
// MAR points to the array start. Release each array element (pointer).
void AGS::Parser::FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements)
{
    if (num_of_elements == 0)
        return;

    if (num_of_elements < 4)
    {
        WriteCmd(SCMD_MEMZEROPTR);
        for (size_t loop = 1; loop < num_of_elements; ++loop)
        {
            WriteCmd(SCMD_ADD, SREG_MAR, SIZE_OF_DYNPOINTER);
            _reg_track.SetRegister(SREG_MAR);
            WriteCmd(SCMD_MEMZEROPTR);
        }
        return;
    }

    WriteCmd(SCMD_LITTOREG, SREG_AX, num_of_elements);
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

// We're at the end of a block and releasing all the pointers in a struct.
// MAR already points to the start of the struct.
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

// We're at the end of a block and we're releasing a standard array of struct.
// MAR points to the start of the array. Release all the pointers in the array.
void AGS::Parser::FreeDynpointersOfStdArrayOfStruct(Vartype element_vtype, size_t num_of_elements)
{

    // AX will be the index of the current element
    WriteCmd(SCMD_LITTOREG, SREG_AX, num_of_elements);

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

// We're at the end of a block and releasing a standard array. MAR points to the start.
// Release the pointers that the array contains.
void AGS::Parser::FreeDynpointersOfStdArray(Symbol the_array)
{
    Vartype const array_vartype =
        _sym.IsVartype(the_array) ? the_array : _sym.GetVartype(the_array);
    size_t const num_of_elements = _sym.NumArrayElements(array_vartype);
    if (num_of_elements < 1)
        return; // nothing to do
    Vartype const element_vartype =
        _sym[array_vartype].VartypeD->BaseVartype;
    if (_sym.IsDynpointerVartype(element_vartype))
    {
        FreeDynpointersOfStdArrayOfDynpointer(num_of_elements);
        return;
    }

    if (_sym.IsStructVartype(element_vartype))
        FreeDynpointersOfStdArrayOfStruct(element_vartype, num_of_elements);
}

// Note: Currently, the structs/arrays that are pointed to cannot contain
// pointers in their turn.
// If they do, we need a solution at runtime to chase the pointers to release;
// we cannot do it at compile time. Also, the pointers might form "rings"
// (e.g., A contains a field that points to B; B contains a field that
// points to A), so we cannot rely on reference counting for identifying
// _all_ the unreachable memory chunks. (If nothing else points to A or B,
// both are unreachable so _could_ be released, but they still point to each
// other and so have a reference count of 1; the reference count will never reach 0).

void AGS::Parser::FreeDynpointersOfLocals(size_t from_level)
{
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

// Free all local Dynpointers taking care to not clobber AX
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
            // Note, it's important we use deep copy because the vector elements will be destroyed when the level is popped
            _sym[s] = symbols_it->second;
                continue;
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
    for (size_t cases_idx = 0; cases_idx < number_of_cases; ++cases_idx)
    {
        if (cases_idx == default_idx)
            continue;

        CodeLoc const codesize = _scrip.codesize;
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

// Must return a symbol that is a literal.
Symbol AGS::Parser::ParseParamlist_Param_DefaultValue(size_t idx, Vartype const param_vartype)
{
    if (kKW_Assign != _src.PeekNext())
        return kKW_NoSymbol; // No default value given

    // For giving specifics in error messages
    std::string msg = "In parameter #<idx>: ";
    msg.replace(msg.find("<idx>"), 5u, std::to_string(idx));

    _src.GetNext();   // Eat '='

    Symbol const default_symbol = ParseConstantExpression(_src, msg);
    
    if (_sym.IsDynVartype(param_vartype))
    {
        Symbol default_value = kKW_Null;
        if (kKW_Null == default_symbol)
            return default_value;
        if (_sym.Find("0") == default_symbol)
        {
            if (PP::kMain == _pp)
                Warning("Found '0' as the default for a dynamic object (prefer 'null')");
            return default_value;
        }
        UserError("Expected the parameter default 'null', found '%s' instead", _sym.GetName(default_symbol).c_str());
    }

    if (_sym.IsAnyStringVartype(param_vartype))
    {
        Symbol default_value = default_symbol;
        if (_sym.Find("0") == default_symbol)
        {
            if (PP::kMain == _pp)
                Warning("Found '0' as the default for a string (prefer '\"\"')");
            return default_value;
        }
        if (!_sym.IsLiteral(default_value) || kKW_String != _sym[default_value].LiteralD->Vartype)
            Error (
                "Expected a constant or literal string as a parameter default, found '%s' instead",
                _sym.GetName(default_symbol).c_str());
        return default_value;
    }   

    if (_sym.IsAnyIntegerVartype(param_vartype))
    {
        if (!_sym.IsLiteral(default_symbol) || kKW_Int != _sym[default_symbol].LiteralD->Vartype)
            UserError(
                "Expected a constant integer expression as a parameter default, found '%s' instead",
                _sym.GetName(default_symbol).c_str());
        return default_symbol;
    }

    if (kKW_Float == param_vartype)
    {
        if (_sym.Find("0") == default_symbol)
        {
            if (PP::kMain == _pp)
                Warning("Found '0' as the default for a float (prefer '0.0')");
        }
        else if (!_sym.IsLiteral(default_symbol) || kKW_Float != _sym[default_symbol].LiteralD->Vartype)
        {
            UserError(
                "Expected a constant float expression as a parameter default, found '%s' instead",
                _sym.GetName(default_symbol).c_str());
        }
        return default_symbol;
    }

    UserError("Parameter cannot have any default value");
    return kKW_NoSymbol; // can't reach
}

void AGS::Parser::ParseDynArrayMarkerIfPresent(Vartype &vartype)
{
    if (kKW_OpenBracket != _src.PeekNext())
        return;
    _src.GetNext(); // Eat '['
    Expect(kKW_CloseBracket, _src.GetNext());
    vartype = _sym.VartypeWith(VTT::kDynarray, vartype);
}

// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
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

    Symbol const qualified_name = MangleStructAndComponent(strct, unqualified_name);

    if (kKW_Dynpointer == _src.PeekNext())
    {
        if (is_static_extender)
            UserError("Unexpected '*' after 'static' in static extender function");
        _src.GetNext(); // Eat '*'
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
        _src.GetNext(); // Eat ','

    unqualified_name = qualified_name;
}

void AGS::Parser::ParseVarname0(bool accept_member_access, Symbol &structname, Symbol &varname)
{
    structname = kKW_NoSymbol;
    varname = _src.GetNext();
    if (varname <= kKW_LastPredefined)
        UserError("Expected an identifier, found '%s' instead", _sym.GetName(varname).c_str());

    // Note: A varname may be allowed although there already is a vartype with the same name.
    // For instance, as a component of a struct. (Room is a vartype; but Character.Room is allowed)
    if (kKW_ScopeRes != _src.PeekNext())
        return;

    _src.GetNext(); // Eat '::'
    if (!accept_member_access)
        UserError("May not use '::' here");

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

// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present
Symbol AGS::Parser::ParseParamlist_Param_Name(bool body_follows)
{

    if (PP::kPreAnalyze == _pp || !body_follows)
    {
        // Ignore the parameter name when present, it won't be used later on (in this phase)
        Symbol const nextsym = _src.PeekNext();
        if (_sym.IsIdentifier(nextsym))
            _src.GetNext();
        return kKW_NoSymbol;
    }

    Symbol const param_name = ParseVarname();
    if (_sym.IsFunction(param_name))
    {
        Warning(
            ReferenceMsgSym("This hides the function '%s()'", param_name).c_str(),
            _sym.GetName(param_name).c_str());
        return param_name;
    }

    if (_sym.IsVariable(param_name))
    {
        if (ScT::kLocal != _sym.GetScopeType(param_name))
            return param_name;

        UserError(
            ReferenceMsgSym("The name '%s' is already in use as a parameter", param_name).c_str(),
            _sym.GetName(param_name).c_str());
    }

    if (_sym.IsVartype(param_name))
        Warning(
            ReferenceMsgSym("This hides the type '%s'", param_name).c_str(),
            _sym.GetName(param_name).c_str());

    return param_name;
}

void AGS::Parser::ParseParamlist_Param_AsVar2Sym(Symbol param_name, TypeQualifierSet tqs, Vartype param_vartype, int param_idx)
{
    SymbolTableEntry &param_entry = _sym[param_name];
    
    if (tqs[TQ::kReadonly])
        param_entry.VariableD->TypeQualifiers[TQ::kReadonly] = true;

    // the parameters are pushed backwards, so the top of the
    // stack has the first parameter. The + 1 is because the
    // call will push the return address onto the stack as well
    param_entry.VariableD->Offset =
        _scrip.OffsetToLocalVarBlock - (param_idx + 1) * SIZE_OF_STACK_CELL;
    _sym.SetDeclared(param_name, _src.GetCursor());
}

void AGS::Parser::ParseParamlist_Param(Symbol const name_of_func, bool const body_follows, TypeQualifierSet tqs, Vartype param_vartype, size_t const param_idx)
{
    if (kKW_Void == param_vartype)
        UserError("Cannot use the type 'void' in a parameter list");
    if (_sym.IsStructVartype(param_vartype) && !_sym.IsManagedVartype(param_vartype))
        UserError(
            ReferenceMsgSym("Cannot use the non-managed struct type '%s' in a parameter list", param_vartype).c_str(),
            _sym.GetName(param_vartype).c_str());

    Symbol const param_name = ParseParamlist_Param_Name(body_follows);
    ParseDynArrayMarkerIfPresent(param_vartype);
    Symbol const param_default = ParseParamlist_Param_DefaultValue(param_idx, param_vartype);
    
    if (!body_follows &&
        kKW_NoSymbol == param_default &&
        !_sym[name_of_func].FunctionD->Parameters.empty() &&
        kKW_NoSymbol != _sym[name_of_func].FunctionD->Parameters.back().Default)
        UserError(
            "Parameter #%u of function '%s' follows an optional parameter and so must have a default, too",
            param_idx, _sym.GetName(name_of_func).c_str());
    
    _sym[name_of_func].FunctionD->Parameters.push_back({});
    auto &back = _sym[name_of_func].FunctionD->Parameters.back();
    back.Vartype = param_vartype; 
    back.Name = param_name;
    back.Default = param_default;
    
    if (PP::kMain != _pp || !body_follows)
        return;

    // All function parameters correspond to local variables.
    // A body will follow, so we need to enter this parameter as a variable into the symbol table
    ParseVardecl_CheckAndStashOldDefn(param_name);
    ParseVardecl_Var2SymTable(param_name, param_vartype);
    // Set the offset
    return ParseParamlist_Param_AsVar2Sym(param_name, tqs, param_vartype, param_idx);
}

void AGS::Parser::ParseFuncdecl_Paramlist(Symbol funcsym, bool body_follows)
{
    _sym[funcsym].FunctionD->IsVariadic = false;
    _sym[funcsym].FunctionD->Parameters.resize(1u); // [0] is the return type; leave that

    TypeQualifierSet tqs = {};

    size_t param_idx = 0;
    while (!_src.ReachedEOF())
    {
        ParseQualifiers(tqs);
        
        // Only certain qualifiers allowed
        for (auto tq_it = tqs.begin(); tq_it != tqs.end(); tq_it++)
        {
            if (!tqs[tq_it->first] || TQ::kReadonly == tq_it->first || TQ::kStatic == tq_it->first)
                continue;
            UserError("Unexpected '%s' in parameter list", _sym.GetName(tqs.TQ2Symbol(tq_it->first)).c_str());
        }
        
        Symbol const leading_sym = _src.PeekNext();
        if (param_idx == 0 && kKW_Void == leading_sym)
        {
            _src.GetNext(); // Eat 'void'
            return Expect(kKW_CloseParenthesis, _src.GetNext());
        }

        if (kKW_CloseParenthesis == leading_sym)
            return (void) _src.GetNext(); // Eat ')' 

        if (kKW_DotDotDot == leading_sym)
        {
            _sym[funcsym].FunctionD->IsVariadic = true;
            _src.GetNext(); // Eat '...'
            return Expect(kKW_CloseParenthesis, _src.GetNext(), "Expected ')' following the '...'");
        }

        if ((++param_idx) >= MAX_FUNCTION_PARAMETERS)
            UserError("Too many parameters defined for function (max. allowed: %u)", MAX_FUNCTION_PARAMETERS - 1u);

        ParseParamlist_Param(funcsym, body_follows, tqs, ParseVartype(), _sym.NumOfFuncParams(funcsym) + 1);
            
        tqs = {}; // type qualifiers have been used up

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseParenthesis }, punctuation);
        if (kKW_CloseParenthesis == punctuation)
            return;
        continue;       
    } // while
    
    InternalError("End of input when processing parameter list"); // Cannot happen
}

void AGS::Parser::ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_function, Symbol name_of_function, bool body_follows)
{
    _sym.MakeEntryFunction(name_of_function);
    SymbolTableEntry &entry = _sym[name_of_function];
    
    entry.FunctionD->Parameters.resize(1u);
    entry.FunctionD->Parameters[0].Vartype = return_vartype;
    entry.FunctionD->Parameters[0].Name = kKW_NoSymbol;
    entry.FunctionD->Parameters[0].Default = kKW_NoSymbol;
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
        if (_sym[name_of_function].FunctionD->Offset < ft)
            _sym[name_of_function].FunctionD->Offset = ft;
    }
}

// there was a forward declaration -- check that the real declaration matches it
void AGS::Parser::ParseFuncdecl_CheckThatKnownInfoMatches(std::string const &func_name, SymbolTableEntry::FunctionDesc const *this_entry, SymbolTableEntry::FunctionDesc const *known_info, size_t const known_declared, bool body_follows)
{
    if (!known_info)
        return; // We don't have any known info
    if (!this_entry)
        InternalError("Function record missing");

    auto known_tq = known_info->TypeQualifiers;
    known_tq[TQ::kImport] = false;
    auto this_tq = this_entry->TypeQualifiers;
    this_tq[TQ::kImport]  = false;
    if (known_tq != this_tq)
    {
        std::string const known_tq_str = TypeQualifierSet2String(known_tq);
        std::string const this_tq_str = TypeQualifierSet2String(this_tq);
        std::string const msg = ReferenceMsgLoc("'%s' has the qualifiers '%s' here but '%s' elsewhere", known_declared);
        UserError(msg.c_str(), func_name.c_str(), this_tq_str.c_str(), known_tq_str.c_str());
    }

    size_t const known_num_parameters = known_info->Parameters.size() - 1;
    size_t const this_num_parameters = this_entry->Parameters.size() - 1;
    if (known_num_parameters != this_num_parameters)
        UserError(
			ReferenceMsgLoc(
				"Function '%s' is declared with %d mandatory parameters here, %d mandatory parameters elswehere",
				known_declared).c_str(), 
			func_name.c_str(), 
			this_num_parameters, 
			known_num_parameters);

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
        std::string const msg =
            ReferenceMsgLoc("Function '%s' %s, %s", known_declared);
        UserError(msg.c_str(), func_name.c_str(), te.c_str(), ki.c_str());
    }

    Symbol const known_ret_type = known_info->Parameters[0u].Vartype;
    Symbol const this_ret_type = this_entry->Parameters[0u].Vartype;
    if (known_ret_type != this_ret_type)
        UserError(
            ReferenceMsgLoc(
				"Return type of '%s' is declared as '%s' here, as '%s' elsewhere",
				known_declared).c_str(),
            func_name.c_str(),
            _sym.GetName(this_ret_type).c_str(),
            _sym.GetName(known_ret_type).c_str());

    auto const &known_params = known_info->Parameters;
    auto const &this_params = this_entry->Parameters;
    for (size_t param_idx = 1; param_idx <= this_num_parameters; param_idx++)
    {
        Vartype const known_param_vartype = known_params[param_idx].Vartype;
        Vartype const this_param_vartype = this_params[param_idx].Vartype;
        if (known_param_vartype != this_param_vartype)
            UserError(
                ReferenceMsgLoc(
					"For function '%s': Type of parameter #%d is '%s' here, '%s' in a declaration elsewhere",
					known_declared).c_str(),
                func_name.c_str(),
                param_idx,
                _sym.GetName(this_param_vartype).c_str(),
                _sym.GetName(known_param_vartype).c_str());
    }

    if (body_follows)
    {
        // If none of the parameters have a default, we'll let this through.
        bool has_default = false;
        for (size_t param_idx = 1; param_idx < this_params.size(); ++param_idx)
            if (kKW_NoSymbol != this_params[param_idx].Default)
            {
                has_default = true;
                break;
            }
        if (!has_default)
            return;
    }

    for (size_t param_idx = 1; param_idx < this_params.size(); ++param_idx)
    {
        auto const this_default = this_params[param_idx].Default;
        auto const known_default = known_params[param_idx].Default;
        if (this_default == known_default)
            continue;

        std::string errstr1 = "In this declaration, parameter #<1> <2>; ";
        errstr1.replace(errstr1.find("<1>"), 3, std::to_string(param_idx));
        if (kKW_NoSymbol == this_default)
            errstr1.replace(errstr1.find("<2>"), 3, "doesn't have a default value");
        else
            errstr1.replace(errstr1.find("<2>"), 3, "has the default " + _sym.GetName(this_default));

        std::string errstr2 = "in a declaration elsewhere, that parameter <2>";
        if (kKW_NoSymbol == known_default)
            errstr2.replace(errstr2.find("<2>"), 3, "doesn't have a default value");
        else
            errstr2.replace(errstr2.find("<2>"), 3, "has the default " + _sym.GetName(known_default));
        errstr1 += errstr2;
        UserError(ReferenceMsgLoc(errstr1, known_declared).c_str());
    }
}

// Enter the function in the imports[] or functions[] array; get its index   
void AGS::Parser::ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, size_t num_of_parameters, CodeLoc &function_soffs)
{
    if (body_follows)
    {
        // Index of the function in the ccCompiledScript::functions[] array
        function_soffs = _scrip.AddNewFunction(_sym.GetName(name_of_func), num_of_parameters);
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

// We're at something like "int foo(", directly before the "("
// Get the symbol after the corresponding ")"
bool AGS::Parser::ParseFuncdecl_DoesBodyFollow()
{
    int const cursor = _src.GetCursor();
    SkipToClose(kKW_CloseParenthesis);
    bool body_follows = (kKW_OpenBrace == _src.PeekNext());
    _src.SetCursor(cursor);
    return body_follows;
}

void AGS::Parser::ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check)
{
    if (kKW_NoSymbol == struct_of_func && tqs[TQ::kProtected])
        UserError(
            "Function '%s' isn't a struct component and so cannot be 'protected'",
            _sym.GetName(name_of_func).c_str());
    if (!body_follows && no_loop_check)
        UserError("Can only use 'noloopcheck' when a function body follows the definition");
    if(!_sym.IsFunction(name_of_func) && _sym.IsInUse(name_of_func))
        UserError(
            ReferenceMsgSym("'%s' is defined elsewhere as a non-function", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());
    if (!_sym.IsManagedVartype(return_vartype) && _sym.IsStructVartype(return_vartype))
        UserError(
            ReferenceMsgSym("Cannot return the non-managed struct type '%s'", return_vartype).c_str(),
            _sym.GetName(return_vartype).c_str());

    if (PP::kPreAnalyze == _pp &&
        body_follows &&
        _sym.IsFunction(name_of_func) &&
        kFT_LocalBody == _sym[name_of_func].FunctionD->Offset)
        UserError(
            ReferenceMsgSym("Function '%s' is already defined with body elsewhere", name_of_func).c_str(),
            _sym.GetName(name_of_func).c_str());

    if (PP::kMain != _pp || kKW_NoSymbol == struct_of_func)
        return;

    if (!_sym.IsComponent(name_of_func) ||
        struct_of_func != _sym[name_of_func].ComponentD->Parent)
    {
        // Functions only become struct components if they are declared in a struct or as extender
        std::string component = _sym.GetName(name_of_func);
        component.erase(0, component.rfind(':') + 1);
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
        ParseFuncdecl_EnterAsImportOrFunc(name_of_func, body_follows, tqs[TQ::kImport], _sym.NumOfFuncParams(name_of_func), func_startoffs);
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

    if (struct_of_func > 0)
    {
        char appendage[10];
        
        sprintf(appendage, "^%d", _sym.NumOfFuncParams(name_of_func) + 100 * _sym[name_of_func].FunctionD->IsVariadic);
        strcat(_scrip.imports[imports_idx], appendage);
    }

    _importLabels.SetLabelValue(name_of_func, imports_idx);
}

void AGS::Parser::ParseFuncdecl(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_func, Symbol name_of_func, bool no_loop_check, bool body_follows)
{
    //                                                           123456789a
    if (0 == _sym.GetName(name_of_func).substr(0u, 10u).compare("__Builtin_"))
        UserError("Function names may not begin with '__Builtin_'");

    // If the parameter list begins with an extender parameter then this has already been resolved at this point
    // and we're behind this parameter. Otherwise, we're directly behind the opening '('.
    body_follows = ParseFuncdecl_DoesBodyFollow();

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

    if (PP::kMain == _pp && body_follows)
    {
        // All the parameters that will be defined as local variables go on nesting level 1.
        _nest.Push(NSType::kParameters);
        // When this function is called, first all the parameters are pushed on the stack
        // and then the address to which the function should return after it has finished.
        // So the first parameter isn't on top of the stack but one address below that
        _scrip.OffsetToLocalVarBlock += SIZE_OF_STACK_CELL;
    }

    // Stash away the known info about the function so that we can check whether this declaration is compatible
    std::unique_ptr<SymbolTableEntry::FunctionDesc> known_info{ _sym[name_of_func].FunctionD };
    _sym[name_of_func].FunctionD = nullptr;
    size_t const known_declared = _sym.GetDeclared(name_of_func);

    ParseFuncdecl_MasterData2Sym(tqs, return_vartype, struct_of_func, name_of_func, body_follows);
    ParseFuncdecl_Paramlist(name_of_func, body_follows);
    
    ParseFuncdecl_CheckThatKnownInfoMatches(_sym.GetName(name_of_func), _sym[name_of_func].FunctionD, known_info.get(), known_declared, body_follows);
    
    // copy the default values from the function prototype into the symbol table
    if (known_info.get())
    {
        auto &func_parameters = _sym[name_of_func].FunctionD->Parameters;
        auto const &known_parameters = known_info->Parameters;
        for (size_t parameters_idx = 0; parameters_idx < func_parameters.size(); ++parameters_idx)
            func_parameters[parameters_idx].Default = known_parameters[parameters_idx].Default;
    }

    ParseFuncdecl_HandleFunctionOrImportIndex(tqs, struct_of_func, name_of_func, body_follows);
}

int AGS::Parser::IndexOfLeastBondingOperator(SrcList &expression)
{
    size_t nesting_depth = 0;

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

// Change the generic opcode to the one that is correct for the vartypes
// Also check whether the operator can handle the types at all
CodeCell AGS::Parser::GetOpcode(Symbol const op_sym, Vartype vartype1, Vartype vartype2)
{
    if (!_sym.IsOperator(op_sym))
        InternalError("'%s' isn't an operator", _sym.GetName(op_sym).c_str());

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
    msg.replace(msg.find("<op>"), 4, _sym.GetName(op_sym));
    CheckVartypeMismatch(vartype1, kKW_Int, true, msg);
    msg = "Right-hand side of '<op>' term";
    msg.replace(msg.find("<op>"), 4, _sym.GetName(op_sym));
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


    // Can convert null to dynpointer or dynarray
    if (kKW_Null == vartype_is)
        return
            !_sym.IsDynpointerVartype(vartype_wants_to_be) &&
            !_sym.IsDynarrayVartype(vartype_wants_to_be);

    // Can only assign dynarray pointers to dynarray pointers.
    if (_sym.IsDynarrayVartype(vartype_is) != _sym.IsDynarrayVartype(vartype_wants_to_be))
        return true;

    // can convert String * to const string
    if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_is) &&
        kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be))
    {
        return false;
    }

    // can convert string or const string to String *
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype_is) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be))
    {
        return false;
    }

    // Note: CanNOT convert String * or const string to string;
    // a function that has a string parameter may modify it, but a String or const string may not be modified.

    if (_sym.IsOldstring(vartype_is) != _sym.IsOldstring(vartype_wants_to_be))
        return true;

    // Note: the position of this test is important.
    // Don't "group" string tests "together" and move this test above or below them.
    // cannot convert const to non-const
    if (_sym.IsConstVartype(vartype_is) && !_sym.IsConstVartype(vartype_wants_to_be))
        return true;

    if (_sym.IsOldstring(vartype_is))
        return false;

    // From here on, don't mind constness or dynarray-ness
    vartype_is = _sym.VartypeWithout(VTT::kConst, vartype_is);
    vartype_is = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kConst, vartype_wants_to_be);
    vartype_wants_to_be = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);

    // floats cannot mingle with other types
    if ((vartype_is == kKW_Float) != (vartype_wants_to_be == kKW_Float))
        return true;

    // Can convert short, char etc. into int
    if (_sym.IsAnyIntegerVartype(vartype_is) && kKW_Int == vartype_wants_to_be)
        return false;

    // Checks to do if at least one is dynarray
    if (_sym.IsDynarrayVartype(vartype_is) || _sym.IsDynarrayVartype(vartype_wants_to_be))
    {
        // BOTH sides must be dynarray 
        if (_sym.IsDynarrayVartype(vartype_is) != _sym.IsDynarrayVartype(vartype_wants_to_be))
            return false;

        // The underlying core vartypes must be identical:
        // A dynarray contains a sequence of elements whose size are used
        // to index the individual element, so no extending elements
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynarray, vartype_wants_to_be);
        Symbol const current_core_vartype = _sym.VartypeWithout(VTT::kDynarray, vartype_is);
        return current_core_vartype != target_core_vartype;
    }

    // Checks to do if at least one is dynpointer
    if (_sym.IsDynpointerVartype(vartype_is) || _sym.IsDynpointerVartype(vartype_wants_to_be))
    {
        // BOTH sides must be dynpointer
        if (_sym.IsDynpointerVartype(vartype_is) != _sym.IsDynpointerVartype(vartype_wants_to_be))
            return true;

        // Core vartypes need not be identical here: check against extensions
        Symbol const target_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_wants_to_be);
        Symbol current_core_vartype = _sym.VartypeWithout(VTT::kDynpointer, vartype_is);
        while (current_core_vartype != target_core_vartype)
        {
            current_core_vartype = _sym[current_core_vartype].VartypeD->Parent;
            if (current_core_vartype == 0)
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

// If we need a String but AX contains a string, 
// then convert AX into a String object and set its type accordingly
void AGS::Parser::ConvertAXStringToStringObject(Vartype wanted_vartype, Vartype &current_vartype)
{
    if (kKW_String == _sym.VartypeWithout(VTT::kConst, current_vartype) &&
        _sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, wanted_vartype))
    {
        WriteCmd(SCMD_CREATESTRING, SREG_AX); // convert AX
        current_vartype = _sym.VartypeWith(VTT::kDynpointer, _sym.GetStringStructSym());
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

void AGS::Parser::HandleStructOrArrayResult(EvaluationResult &eres)
{
    Vartype const vartype = eres.Vartype;

    if (_sym.IsArrayVartype(vartype))
        UserError("Cannot access this array as a whole (did you forget to add \"[0]\"?)");

    if (_sym.IsAtomicVartype(vartype) && _sym.IsStructVartype(vartype))
    {
        if (_sym.IsManagedVartype(vartype))
        {
            // Interpret the memory address as the result
            // We don't have a way of saying, "MAR _is_ the value"
            // so we move the value to AX, we _can_ say "AX _is_ the value".
            eres.Vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
            _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
            WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_AX);
            _reg_track.SetRegister(SREG_AX);
            eres.Location = eres.kLOC_AX;
            return;
        }

        UserError("Cannot access this non-managed struct as a whole");
    }
}

void AGS::Parser::EvaluationResultToAx(EvaluationResult &eres)
{
    Vartype const vartype = eres.Vartype;

    switch (eres.Type)
    {
    default:
        InternalError("Expression result has type %d, cannot move to AX", eres.Type);

    case eres.kTY_FunctionName:
        // Cannot convert a naked function symbol; assume that the coder has forgotten '('
        UserError("Expected '(' after '%s'", _sym.GetName(eres.Symbol).c_str());

    case eres.kTY_Literal:
        // Convert to runtime value in AX
        WriteCmd(SCMD_LITTOREG, SREG_AX, _sym[eres.Symbol].LiteralD->Value);
        _reg_track.SetRegister(SREG_AX);
        if (kKW_String == _sym.VartypeWithout(VTT::kConst, vartype))
            _scrip.FixupPrevious(kFx_String);
        eres.Type = eres.kTY_RunTimeValue;
        eres.Location = eres.kLOC_AX;
        eres.Symbol = kKW_NoSymbol;
        break;

    case eres.kTY_RunTimeValue:
        break;

    case eres.kTY_StructName:
        // Cannot convert naked typename; coder has probably forgotten '.'
        UserError("Expected '.' after '%s'", _sym.GetName(eres.Symbol).c_str());
    }

    switch (eres.Location)
    {
    default:
        return InternalError("Cannot move expression result location to AX");

    case eres.kLOC_AX:
        return; // Already done

    case eres.kLOC_MemoryAtMAR:
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

void AGS::Parser::ParseExpression_CheckArgOfNew(Vartype argument_vartype)
{
    if (!_sym.IsVartype(argument_vartype))
        UserError("Expected a type after 'new', found '%s' instead", _sym.GetName(argument_vartype).c_str());
    if (_sym[argument_vartype].VartypeD->Flags[VTF::kUndefined])
        UserError(
            ReferenceMsgSym("The struct '%s' hasn't been completely defined yet", argument_vartype).c_str(),
            _sym.GetName(argument_vartype).c_str());
    if (!_sym.IsAnyIntegerVartype(argument_vartype) && kKW_Float != argument_vartype && !_sym.IsManagedVartype(argument_vartype))
        UserError("Can only use integer types or 'float' or managed types with 'new'");

    // Note: While it is an error to use a built-in type with new, it is
    // allowed to use a built-in type with new[].
}

void AGS::Parser::ParseExpression_New(SrcList &expression, EvaluationResult &eres)
{
    if (expression.ReachedEOF())
        UserError("Expected a type after 'new' but didn't find any");
    Vartype const argument_vartype = expression.GetNext();

    ParseExpression_CheckArgOfNew(argument_vartype);
    
    bool const is_managed = _sym.IsManagedVartype(argument_vartype);
    bool const with_bracket_expr = kKW_OpenBracket == expression.PeekNext(); // "new FOO[BAR]"

    Vartype element_vartype = kKW_NoSymbol;
    if (with_bracket_expr)
    {
        // Note that in AGS, you can write "new Struct[...]" but what you mean then is "new Struct*[...]".
        EatDynpointerSymbolIfPresent(argument_vartype);
        
        // Check for '[' with a handcrafted error message so that the user isn't led to 
        // fix their code by defining a dynamic array when this would be the wrong thing to do
        Symbol const open_bracket = _src.GetNext();
        if (kKW_OpenBracket != open_bracket)
            UserError("Unexpected '%s'", _sym.GetName(open_bracket).c_str());

        EvaluationResult bracketed_eres;
        ParseIntegerExpression(_src, bracketed_eres);
        EvaluationResultToAx(bracketed_eres);
        Expect(kKW_CloseBracket, _src.GetNext());

        element_vartype = is_managed ? _sym.VartypeWith(VTT::kDynpointer, argument_vartype) : argument_vartype;
        eres.Vartype = _sym.VartypeWith(VTT::kDynarray, element_vartype);
    }
    else
    {
        if (_sym.IsBuiltinVartype(argument_vartype))  
            UserError("Expected '[' after the built-in type '%s'", _sym.GetName(argument_vartype).c_str());
        if (!is_managed)
            UserError("Expected '[' after the integer type '%s'", _sym.GetName(argument_vartype).c_str());

        if (kKW_OpenParenthesis == expression.PeekNext())
        {
            Warning("'()' after 'new' isn't implemented, is currently ignored");
            expression.GetNext();
            SkipTo(SymbolList{}, expression);
            expression.GetNext();
        }

        // Only do this check for new, not for new[]. 
        if (0 == _sym.GetSize(argument_vartype))
            UserError(
                ReferenceMsgSym(
                    "Struct '%s' doesn't contain any variables, cannot use 'new' with it",
                    argument_vartype).c_str(),
                _sym.GetName(argument_vartype).c_str());

        element_vartype = argument_vartype;
        eres.Vartype = _sym.VartypeWith(VTT::kDynpointer, argument_vartype);
    }

    size_t const element_size = _sym.GetSize(element_vartype);
    if (0 == element_size)
        // The Engine really doesn't like that (division by zero error)
        InternalError("Trying to emit allocation of zero dynamic memory");

    // Choose between "old" and new "new" opcode, depending on RTTI presence
    if (FlagIsSet(_options, SCOPT_RTTIOPS))
    {
        element_vartype = _sym.GetFirstBaseVartype(element_vartype);
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

    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_AX;
    eres.Symbol = kKW_NoSymbol;
    // Vartype has already been set
}

// We're parsing an expression that starts with '-' (unary minus)
void AGS::Parser::ParseExpression_PrefixMinus(SrcList &expression, EvaluationResult &eres)
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

// We're parsing an expression that starts with '+' (unary plus)
void AGS::Parser::ParseExpression_PrefixPlus(SrcList &expression, EvaluationResult &eres)
{
    expression.StartRead();
    
    ParseExpression_Term(expression, eres);
    
    if (_sym.IsAnyIntegerVartype(eres.Vartype) || kKW_Float == eres.Vartype)
        return;

    UserError("Cannot apply unary '+' to an expression of type '%s'", _sym.GetName(eres.Vartype));
}

// We're parsing an expression that starts with '!' (boolean NOT) or '~' (bitwise Negate)
void AGS::Parser::ParseExpression_PrefixNegate(Symbol op_sym, SrcList &expression, EvaluationResult &eres)
{
    bool const bitwise_negation = kKW_BitNeg == op_sym;

    std::string msg = "Argument of '<op>'";
    msg.replace(msg.find("<op>"), 4, _sym.GetName(op_sym));
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
    bool const op_is_inc = (kKW_Increment == op_sym);

    expression.StartRead();

    ParseAssignment_ReadLHSForModification(expression, eres);
    
    std::string msg = "Argument of '<op>'";
    msg.replace(msg.find("<op>"), 4, _sym.GetName(op_sym).c_str());
    CheckVartypeMismatch(eres.Vartype, kKW_Int, true, msg); 
    
    WriteCmd((op_is_inc ? SCMD_ADD : SCMD_SUB), SREG_AX, 1);
    _reg_track.SetRegister(SREG_AX);

    // Really do the assignment the long way so that all the checks and safeguards will run.
    // If a shortcut is possible then undo this and generate the shortcut instead.
    RestorePoint before_long_way_modification = RestorePoint(_scrip);

    AccessData_AssignTo(expression, eres);
    
    if (EvaluationResult::kLOC_MemoryAtMAR == eres.Location)
    {
        before_long_way_modification.Restore();
        CodeCell memwrite = GetWriteCommandForSize(_sym.GetSize(eres.Vartype));
        WriteCmd(memwrite, SREG_AX);
        _reg_track.SetRegister(SREG_AX);
    }
    eres.SideEffects = true;
}

void AGS::Parser::ParseExpression_LongMin(EvaluationResult &eres)
{
    eres.Type = eres.kTY_Literal;
    eres.Location = eres.kLOC_SymbolTable;
    eres.Symbol = _sym.Find("-2147483648");
    eres.Vartype = kKW_Int;
}


// The least binding operator is the first thing in the expression
// This means that the op must be an unary op.
void AGS::Parser::ParseExpression_Prefix(SrcList &expression, EvaluationResult &eres)
{
    Symbol const op_sym = expression[0];

    if (expression.Length() < 2)
        UserError(
            "Expected a term after '%s' but didn't find any",
            _sym.GetName(op_sym).c_str());

    expression.EatFirstSymbol();

    if (kKW_New == op_sym)
        return ParseExpression_New(expression, eres);

    if (kKW_Decrement == op_sym || kKW_Increment == op_sym)
    {
        StripOutermostParens(expression);
        return ParseExpression_PrefixCrement(op_sym, expression, eres);
    }

    // Special case: Lowest integer literal, written in decimal notation.
    // We treat this here in the parser because the scanner doesn't know
    // whether a minus symbol stands for a unary minus.
    if (op_sym == kKW_Minus &&
        expression.Length() == 1 &&
        expression[0] == kKW_OnePastLongMax)
        return ParseExpression_LongMin(eres);

    ParseExpression_Term(expression, eres);
    
    switch (op_sym)
    {
    case kKW_BitNeg:
    case kKW_Not:
        return ParseExpression_PrefixNegate(op_sym, expression, eres);

    case kKW_Minus:
        return ParseExpression_PrefixMinus(expression, eres);

    case kKW_Plus:
        return ParseExpression_PrefixPlus(expression, eres);
    }

    InternalError("Illegal prefix op '%s'", _sym.GetName(op_sym).c_str());
}

void AGS::Parser::StripOutermostParens(SrcList &expression)
{
    while (expression[0] == kKW_OpenParenthesis)
    {
        size_t const last = expression.Length() - 1;
        if (kKW_CloseParenthesis != expression[last])
            return;
        expression.SetCursor(1u);
        SkipTo(SymbolList{}, expression);
        if (expression.GetCursor() != last)
            return;
        expression.EatFirstSymbol();
        expression.EatLastSymbol();
    }
}

void AGS::Parser::ParseExpression_PostfixCrement(Symbol const op_sym, SrcList &expression, EvaluationResult &eres)
{
    bool const op_is_inc = kKW_Increment == op_sym;

    StripOutermostParens(expression);
    expression.StartRead();

    ParseAssignment_ReadLHSForModification(expression, eres);
    
    std::string msg = "Argument of '<op>'";
    msg.replace(msg.find("<op>"), 4, _sym.GetName(op_sym).c_str());
    CheckVartypeMismatch(eres.Vartype, kKW_Int, true, msg);
    
    // Really do the assignment the long way so that all the checks and safeguards will run.
    // If a shortcut is possible then undo this and generate the shortcut instead.
    RestorePoint before_long_way_modification{ _scrip };

    PushReg(SREG_AX);
    WriteCmd((op_is_inc ? SCMD_ADD : SCMD_SUB), SREG_AX, 1);
    AccessData_AssignTo(expression, eres);
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

void AGS::Parser::ParseExpression_Postfix(SrcList &expression, EvaluationResult &eres, bool result_used)
{
    size_t const len = expression.Length();

    if (0u == len)
        InternalError("Empty expression");

    Symbol const op_sym = expression[len - 1u];
    if (1u == len)
        UserError("'%s' must either precede or follow some term to be modified", _sym.GetName(op_sym).c_str());

    expression.EatLastSymbol();

    switch (op_sym)
    {
    case kKW_Decrement:
    case kKW_Increment:
        // If the result isn't used then take the more efficient version of increment / decrement
        return result_used ?
            ParseExpression_PostfixCrement(op_sym, expression, eres) : ParseExpression_PrefixCrement(op_sym, expression, eres);
    }

    UserError("Expected a term following the '%s', didn't find it", _sym.GetName(op_sym).c_str());
}

void AGS::Parser::ParseExpression_Ternary_Term2(EvaluationResult &eres_term1, bool term1_has_been_ripped_out, SrcList &term2, EvaluationResult &eres, bool result_used)
{
    bool const second_term_exists = (term2.Length() > 0);
    if (second_term_exists)
    {
        ParseExpression_Term(term2, eres, result_used);
        if (!term2.ReachedEOF())
            InternalError("Unexpected '%s' after 1st term of ternary", _sym.GetName(term2.GetNext()).c_str());

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

void AGS::Parser::ParseExpression_Ternary(size_t tern_idx, SrcList &expression, EvaluationResult &eres, bool result_used)
{
    // First term ends before the '?'
    SrcList term1 = SrcList(expression, 0, tern_idx);

    // Second term begins after the '?', we don't know how long it is yet
    SrcList after_term1 = SrcList(expression, tern_idx + 1, expression.Length() - (tern_idx + 1));

    // Find beginning of third term
    after_term1.StartRead();
    SkipTo(kKW_Colon, after_term1);
    if (after_term1.ReachedEOF() || kKW_Colon != after_term1.PeekNext())
    {
        expression.SetCursor(tern_idx);
        UserError("Didn't find the matching ':' to '?'");
    }
    size_t const term3_start = after_term1.GetCursor() + 1;
    SrcList term3 = SrcList(after_term1, term3_start, after_term1.Length() - term3_start);
    SrcList term2 = SrcList(after_term1, 0u, after_term1.GetCursor());
    if (0 == term3.Length())
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
    // Result is used, that's the point of the ternary
    ParseExpression_Term(term1, eres_term1);
    
    bool const term1_known =
        eres.kTY_Literal ==  eres_term1.Type &&
        (eres_term1.Vartype == kKW_Float || _sym.IsAnyIntegerVartype(eres_term1.Vartype));
    CodeCell const term1_value = term1_known ? _sym[eres_term1.Symbol].LiteralD->Value : false;
    EvaluationResult eres_dummy = eres_term1;
    EvaluationResultToAx(eres_dummy); // Don't clobber vloc_term1
   
    if (!term1.ReachedEOF())
        InternalError("Unexpected '%s' after 1st term of ternary", _sym.GetName(term1.GetNext()).c_str());

    // Jump either to the start of the third term or to the end of the ternary expression.
    WriteCmd(
        second_term_exists ? SCMD_JZ : SCMD_JNZ,
        kDestinationPlaceholder);
    if (second_term_exists)
        jumpdest_to_term3.AddParam();
    else
        jumpdest_out_of_ternary.AddParam();

    bool term1_has_been_ripped_out = false;
    if (term1_known)
    {   // Don't need to do the test at runtime
        start_of_term1.Restore();
        term1_has_been_ripped_out = true;
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
    jumpdest_to_term3.Patch(_src.GetLineno());

    ParseExpression_Term(term3, eres_term3, result_used);
    eres_dummy = eres_term3;
    EvaluationResultToAx(eres_dummy); // don't clobber vloc_term3
    ConvertAXStringToStringObject(_sym.GetStringStructPtrSym(), eres_term3.Vartype);

    bool term3_has_been_ripped_out = false;
    if (term1_known && term1_value)
    {
        start_of_term3.Restore(); // Don't need term3, will never be evaluated
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
        expression.SetCursor(op_idx + 1);
        WriteCmd(SCMD_JZ, kDestinationPlaceholder);
        to_exit.AddParam();
    }
    else if (kKW_Or == operator_sym)
    {
        // If AX is non-zero then the OR has succeeded, so just jump directly to the end of the term; 
        // AX will still be non-zero so that will do as the result of the calculation
        lazy_evaluation = true;
        expression.SetCursor(op_idx + 1);
        WriteCmd(SCMD_JNZ, kDestinationPlaceholder);
        to_exit.AddParam();
    }
    else if (kKW_Increment == operator_sym || kKW_Decrement == operator_sym)
    {
        UserError("Cannot use '%s' as a binary operator", _sym.GetName(operator_sym).c_str());
    }
    else
    {
        // Hang on to the intermediate result
        PushReg(SREG_AX);
    }

    SrcList rhs = SrcList(expression, op_idx + 1, expression.Length());
    if (0 == rhs.Length())
    {
        // there is no right hand side for the expression
        expression.SetCursor(op_idx + 1);
        UserError("Binary operator '%s' doesn't have a right hand side", _sym.GetName(operator_sym).c_str());
    }

    ParseExpression_Term(rhs, eres);
    size_t const expression_end_idx = expression.GetCursor();

    EvaluationResult eres_rhs = eres;
    EvaluationResultToAx(eres);

    if (!lazy_evaluation)
    {
        expression.SetCursor(op_idx + 1);
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

void AGS::Parser::ParseExpression_InParens(SrcList &expression, EvaluationResult &eres, bool result_used)
{
    // Check for spurious symbols after the closing paren.
    expression.SetCursor(1u);
    SkipTo(SymbolList{}, expression);
    expression.GetNext(); // Eat the closing parenthesis
    ParseExpression_CheckUsedUp(expression);
    
    StripOutermostParens(expression);
    return ParseExpression_Term(expression, eres, result_used);
}

// We're in the parameter list of a function call, and we have less parameters than declared.
// Provide defaults for the missing values
void AGS::Parser::AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args,Symbol funcSymbol, bool func_is_import)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        Symbol const param_default = _sym[funcSymbol].FunctionD->Parameters[arg_idx].Default;
        if (kKW_NoSymbol == param_default)
            UserError("Function call parameter #%d isn't provided and doesn't have any default value", arg_idx);
        if (!_sym.IsLiteral(param_default))
            InternalError("Parameter default symbol isn't literal");

        // push the default value onto the stack
        WriteCmd(
            SCMD_LITTOREG,
            SREG_AX,
            _sym[param_default].LiteralD->Value);
        _reg_track.SetRegister(SREG_AX);

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
    }
}

std::string const AGS::Parser::ReferenceMsgLoc(std::string const &msg, size_t declared)
{
    if (SymbolTable::kNoSrcLocation == declared)
        return msg;

    int const section_id = _src.GetSectionIdAt(declared);
    std::string const &section = _src.SectionId2Section(section_id);

    int const line = _src.GetLinenoAt(declared);
    if (line <= 0 || (!section.empty() && '_' == section[0]))
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
        tpl.replace(tpl.find("<1>"), 3, section);
    size_t const loc2 = tpl.find("<2>");
    if (std::string::npos != loc2)
        tpl.replace(tpl.find("<2>"), 3, std::to_string(line));
    return msg + tpl;
}

std::string const AGS::Parser::ReferenceMsgSym(std::string const &msg,Symbol symb)
{
    return ReferenceMsgLoc(msg, _sym.GetDeclared(symb));
}

void AGS::Parser::AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args,Symbol funcSymbol, bool func_is_import)
{
    size_t param_num = num_supplied_args + 1;
    size_t start_of_current_param = 0;
    int end_of_current_param = closed_paren_idx;  // can become < 0, points to (last symbol of parameter + 1)
    // Go backwards through the parameters since they must be pushed that way
    do
    {
        // Find the start of the next parameter
        param_num--;
        int delimiter_nesting_depth = 0;
        for (size_t paramListIdx = end_of_current_param - 1; true; paramListIdx--)
        {
            Symbol const &symb = parameters[paramListIdx];
            if (_sym.IsDelimeter(symb))
            {
                bool const is_opener = _sym[symb].DelimeterD->Opening;
                // Going backwards so closers increase the nesting depth, openers decrease it
                if (is_opener)
                    delimiter_nesting_depth--;
                else
                    delimiter_nesting_depth++;
            }
            if ((delimiter_nesting_depth == 0 && kKW_Comma == symb) ||
                (delimiter_nesting_depth < 0 && kKW_OpenParenthesis == symb))
            {
                start_of_current_param = paramListIdx + 1;
                break;
            }
            if (paramListIdx == 0)
                break; // Don't put this into the for header!
        }

        if (end_of_current_param < 0 || static_cast<size_t>(end_of_current_param) < start_of_current_param)  
            InternalError("Parameter length is negative");

        // Compile the parameter
        EvaluationResult eres;

        SrcList current_param = SrcList(parameters, start_of_current_param, end_of_current_param - start_of_current_param);
        // Note, don't use 'ParseExpression_Term()' here; that function doesn't check
        // whether its parameter is nothing but an expression.
        ParseExpression(current_param, eres);
        if (!current_param.ReachedEOF())
            Expect(SymbolList{ kKW_Comma, kKW_CloseParenthesis }, current_param.PeekNext());
        EvaluationResultToAx(eres);

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            Vartype const param_vartype = _sym[funcSymbol].FunctionD->Parameters[param_num].Vartype;
            ConvertAXStringToStringObject(param_vartype, eres.Vartype);
            // If we need a normal string but AX contains a string object ptr, 
            // check that this ptr isn't null
            if (_sym.GetStringStructSym() == _sym.VartypeWithout(VTT::kDynpointer, eres.Vartype) &&
                kKW_String == _sym.VartypeWithout(VTT::kConst, param_vartype))
                WriteCmd(SCMD_CHECKNULLREG, SREG_AX);

            std::string msg = "Parameter #<num> of call to function <func>";
            msg.replace(msg.find("<num>"), 5, std::to_string(param_num));
            msg.replace(msg.find("<func>"), 6, _sym.GetName(funcSymbol));
            CheckVartypeMismatch(eres.Vartype, param_vartype, true, msg);
        }

        // Note: We push the parameters, which is tantamount to writing them
        // into memory with SCMD_MEMWRITE. The called function will use them
        // as local variables. However, if a parameter is managed, then its 
        // memory must be written with SCMD_MEMWRITEPTR, not SCMD_MEMWRITE 
        // as we do here. So to compensate, the called function will have to 
        // read each pointer variable with SCMD_MEMREAD and then write it
        // back with SCMD_MEMWRITEPTR.

        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);

        end_of_current_param = start_of_current_param - 1;
    }
    while (end_of_current_param > 0);
}


// Count parameters, check that all the parameters are non-empty; find closing paren
void AGS::Parser::AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters,Symbol name_of_func, size_t &index_of_close_paren, size_t &num_supplied_args)
{
    size_t delimeter_nesting_depth = 1;
    num_supplied_args = 1;
    size_t param_idx;
    bool found_param_symbol = false;

    for (param_idx = 1; param_idx < parameters.Length(); param_idx++)
    {
        Symbol const &symb = parameters[param_idx];

        if (_sym.IsDelimeter(symb))
        {
            bool const is_opener = _sym[symb].DelimeterD->Opening;
            if (is_opener)
            {
                delimeter_nesting_depth++;
            }
            else
            {
                delimeter_nesting_depth--;
                if (0 == delimeter_nesting_depth)
                    break;
            }
        }

        if (1 ==delimeter_nesting_depth && kKW_Comma == symb)
        {
            num_supplied_args++;
            if (found_param_symbol)
                continue;

            UserError("Argument %d in function call is empty", num_supplied_args - 1);
        }
        found_param_symbol = true;
    }

    // Special case: "()" means 0 arguments
    if (num_supplied_args == 1 &&
        parameters.Length() > 1 &&
        kKW_CloseParenthesis == parameters[1])
    {
        num_supplied_args = 0;
    }

    index_of_close_paren = param_idx;

    if (kKW_CloseParenthesis != parameters[index_of_close_paren])
        InternalError("Missing ')' at the end of the parameter list");
    if (index_of_close_paren > 0 && kKW_Comma == parameters[index_of_close_paren - 1])
        UserError("Last argument in function call is empty");
    if (delimeter_nesting_depth > 0)
        InternalError("Parser confused near '%s'", _sym.GetName(name_of_func).c_str());
}

// We are processing a function call. General the actual function call
void AGS::Parser::AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import)
{
    if (func_is_import)
    {
        // tell it how many args for this call (nested imported functions cause stack problems otherwise)
        WriteCmd(SCMD_NUMFUNCARGS, num_args);
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
            _scrip.code[_scrip.codesize - 1] = _importLabels.Function2Label(name_of_func);
            _importLabels.TrackLabelLoc(_scrip.codesize - 1);
        }

        WriteCmd(SCMD_CALLEXT, SREG_AX); // Do the call
        _reg_track.SetAllRegisters();
        // At runtime, we will arrive here when the function call has returned: Restore the stack
        if (num_args > 0)
            WriteCmd(SCMD_SUBREALSTACK, num_args);
        return;
    }

    // Func is non-import
    _scrip.FixupPrevious(kFx_Code);
    if (_sym[name_of_func].FunctionD->Offset < 0)
    {
        // We don't know yet at which address the function is going to start, so put a label here
        // and keep track of the location in order to patch in the correct address later on
        _scrip.code[_scrip.codesize - 1] = _callpointLabels.Function2Label(name_of_func);
        _callpointLabels.TrackLabelLoc(_scrip.codesize - 1);
    }
    WriteCmd(SCMD_CALL, SREG_AX);  // Do the call
    _reg_track.SetAllRegisters();

    // At runtime, we will arrive here when the function call has returned: Restore the stack
    if (num_args > 0)
    {
        size_t const size_of_passed_args = num_args * SIZE_OF_STACK_CELL;
        WriteCmd(SCMD_SUB, SREG_SP, size_of_passed_args);
        _scrip.OffsetToLocalVarBlock -= size_of_passed_args;
    }
}

void AGS::Parser::AccessData_GenerateDynarrayLengthFuncCall(EvaluationResult &eres)
{
    // Load MAR with the address of the dynarray. Will provoke a runtime error when NULL
    AccessData_Dereference(eres);

    // We calculate the length of the dynarray by calling an external function.
    // Ensure that this function is declared as an import function
    std::string const dynarray_len_func_name = "__Builtin_DynamicArrayLength";
    Symbol const dynarray_len_func = _sym.FindOrAdd(dynarray_len_func_name);
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
        strcat(_scrip.imports[_sym[dynarray_len_func].FunctionD->Offset], "^1");
        _sym.SetDeclared(dynarray_len_func, _src.GetCursor());
    }
    _sym[dynarray_len_func].Accessed = true;

    WriteCmd(SCMD_PUSHREAL, SREG_MAR); // Load the dynarray address onto the far stack
    AccessData_GenerateFunctionCall(dynarray_len_func, 1u, true);

    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_AX;
    eres.Symbol = kKW_NoSymbol;
    eres.Vartype = kKW_Int;
    eres.Modifiable = false;
}

// We are processing a function call.
// Get the parameters of the call and push them onto the stack.
void AGS::Parser::AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args)
{
    size_t const num_func_args = _sym.NumOfFuncParams(name_of_func);

    size_t num_supplied_args = 0;
    size_t closed_paren_idx;
    AccessData_FunctionCall_CountAndCheckParm(parameters, name_of_func, closed_paren_idx, num_supplied_args);
    
    // Push default parameters onto the stack when applicable
    // This will give an error if there aren't enough default parameters
    if (num_supplied_args < num_func_args)
    {
        AccessData_FunctionCall_ProvideDefaults(num_func_args, num_supplied_args, name_of_func, func_is_import);
    }
	
    if (num_supplied_args > num_func_args && !_sym.IsVariadicFunc(name_of_func))
        UserError(
            (1 == num_func_args) ?
                "Expected just %d parameter but found %d" :
                "Expected just %d parameters but found %d",
            num_func_args,
            num_supplied_args);

    // ASSERT at this point, the number of parameters is okay

    // Push the explicit arguments of the function
    if (num_supplied_args > 0)
    {
        AccessData_FunctionCall_PushParams(parameters, closed_paren_idx, num_func_args, num_supplied_args, name_of_func, func_is_import);
    }

    actual_num_args = std::max(num_supplied_args, num_func_args);
    parameters.SetCursor(closed_paren_idx + 1); // Go to the end of the parameter list
}

void AGS::Parser::AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, EvaluationResult &eres)
{
    if (kKW_OpenParenthesis != expression[1])
        UserError("Expected '('");

    expression.EatFirstSymbol();

    auto const function_tqs = _sym[name_of_func].FunctionD->TypeQualifiers;
    bool const func_is_import = function_tqs[TQ::kImport];
    // If function uses normal stack, we need to do stack calculations to get at certain elements
    bool const func_uses_normal_stack = !func_is_import;
    bool const called_func_uses_this =
        std::string::npos != _sym.GetName(name_of_func).find("::") &&
        !function_tqs[TQ::kStatic];
    bool const calling_func_uses_this = (kKW_NoSymbol != _sym.GetVartype(kKW_This));
    bool mar_pushed = false;
    bool op_pushed = false;

    if (calling_func_uses_this)
    {
        // Save OP since we need it after the func call
        // We must do this no matter whether the callED function itself uses "this"
        // because a called function that doesn't might call a function that does.
        PushReg(SREG_OP);
        op_pushed = true;
    }

    if (called_func_uses_this)
    {
        // MAR contains the address of "outer"; this is what will be used for "this" in the called function.
        _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
        _reg_track.SetRegister(SREG_MAR);

        // Parameter processing might entail calling yet other functions, e.g., in "f(...g(x)...)".
        // So we cannot emit SCMD_CALLOBJ here, before parameters have been processed.
        // Save MAR because parameter processing might clobber it 
        PushReg(SREG_MAR);
        mar_pushed = true;
    }

    size_t num_args = 0;
    AccessData_PushFunctionCallParams(name_of_func, func_is_import, expression, num_args);
    
    if (called_func_uses_this)
    {
        if (0 == num_args)
        {   // MAR must still be current, so undo the unneeded PUSH above.
            _scrip.OffsetToLocalVarBlock -= SIZE_OF_STACK_CELL;
            _scrip.codesize -= 2;
            mar_pushed = false;
        }
        else
        {   // Recover the value of MAR from the stack. It's in front of the parameters.
            WriteCmd(
                SCMD_LOADSPOFFS,
                (1 + (func_uses_normal_stack ? num_args : 0)) * SIZE_OF_STACK_CELL);
            WriteCmd(SCMD_MEMREAD, SREG_MAR);
            _reg_track.SetRegister(SREG_MAR);
        }
        WriteCmd(SCMD_CALLOBJ, SREG_MAR);
    }

    AccessData_GenerateFunctionCall(name_of_func, num_args, func_is_import);

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

void AGS::Parser::ParseExpression_NoOps(SrcList &expression, EvaluationResult &eres, bool result_used)
{
    if (kKW_OpenParenthesis == expression[0])
        return ParseExpression_InParens(expression, eres, result_used);

    AccessData(VAC::kReading, expression, eres);
    return ParseExpression_CheckUsedUp(expression);
}

void AGS::Parser::ParseExpression_Term(SrcList &expression, EvaluationResult &eres, bool result_used)
{
    if (expression.Length() == 0)
        InternalError("Cannot parse empty subexpression");

    int const least_binding_op_idx = IndexOfLeastBondingOperator(expression);  // can be < 0
    
    if (0 > least_binding_op_idx)
        ParseExpression_NoOps(expression, eres, result_used);
    else if (0 == least_binding_op_idx)
        ParseExpression_Prefix(expression, eres);
    else if (expression.Length() - 1u == least_binding_op_idx)
        ParseExpression_Postfix(expression, eres, result_used);
    else if (kKW_Tern == expression[least_binding_op_idx])
        ParseExpression_Ternary(least_binding_op_idx, expression, eres, result_used);
    else
        ParseExpression_Binary(least_binding_op_idx, expression, eres);        
    
    return HandleStructOrArrayResult(eres);
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
    size_t const m_access_position = member_str.rfind("::");
    if (std::string::npos != m_access_position)
        member_str = member_str.substr(m_access_position + 2);
    char const *stem_str = is_setter ? "set" : "get";
    char const *indx_str = is_indexed ? "i_" : "_";
    std::string func_str = stem_str + (indx_str + member_str);
    return _sym.FindOrAdd(func_str);
}

// We call the getter or setter of an attribute
void AGS::Parser::AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, Vartype vartype)
{
    // Search for the attribute: It might be in an ancestor of 'vartype' instead of in 'vartype'.
    Symbol const unqualified_component = expression.GetNext();
    Symbol const struct_of_component =
        FindStructOfComponent(vartype, unqualified_component);
    if (kKW_NoSymbol == struct_of_component)
        UserError(
            ReferenceMsgSym(
                "Struct '%s' does not have an attribute named '%s'",
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

    size_t num_of_args = 0;
    if (is_setter)
    {
        if (func_is_import)
            WriteCmd(SCMD_PUSHREAL, SREG_AX);
        else
            PushReg(SREG_AX);
        ++num_of_args;
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
        ++num_of_args;
    }

    if (attrib_uses_this)
        WriteCmd(SCMD_CALLOBJ, SREG_MAR); // make MAR the new this ptr

    AccessData_GenerateFunctionCall(qualified_func_name, num_of_args, func_is_import);

    if (attrib_uses_this)
        PopReg(SREG_OP); // restore old this ptr after the func call

    MarkAcessed(qualified_func_name);
}


// Location contains a pointer to another address. Get that address.
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

void AGS::Parser::AccessData_ProcessCurrentArrayIndex(size_t const idx, size_t const dim, size_t const factor, bool const is_dynarray, SrcList &expression, EvaluationResult &eres)
{
    // Get the index
    size_t const index_start = expression.GetCursor();
    SkipTo(SymbolList{ kKW_Comma, kKW_CloseBracket }, expression);
    size_t const index_end = expression.GetCursor();
    SrcList current_index = SrcList(expression, index_start, index_end - index_start);
    if (0 == current_index.Length())
        UserError("Array index #u is empty, this is not supported here", idx + 1u);

    // If all ops are pending on the MAR register, it hasn't been set yet at all.
    // So then we don't need to protect MAR against being clobbered, 
    // but we do need to keep track of those pending ops in this case.
    bool const all_ops_are_pending = _marMgr.AreAllOpsPending();
    MarMgr save_mar_state(_marMgr);
    RegisterGuard(all_ops_are_pending ? RegisterList{} : RegisterList{ SREG_MAR },
        [&]
        {
            std::string msg = "In array index #<idx>: ";
            msg.replace(msg.find("<idx>"), 5u, std::to_string(idx + 1u));
            current_index.StartRead();
            ParseIntegerExpression(current_index, eres, msg);
            if (eres.kTY_Literal != eres.Type)
                EvaluationResultToAx(eres);
        });

    if (eres.kTY_Literal == eres.Type)
    {
        // The arrax index is known at compile time, so check it as far as possible
        int const index_value = _sym[eres.Symbol].LiteralD->Value;
        if (index_value < 0)
            UserError(
                "Array index #%u is %d, thus too low (minimum is 0)",
                idx + 1u,
                index_value);
        if (dim > 0 && static_cast<size_t>(index_value) >= dim)
            UserError(
                "Array index #%u is %d, thus too high (maximum is %u)",
                idx + 1u,
                index_value,
                dim - 1u);

        if (is_dynarray && index_value > 0)
        {
            // We need to check the offset at runtime because we can't know the
            // array size that has been allocated.
            WriteCmd(SCMD_LITTOREG, SREG_AX, index_value * factor);
            _reg_track.SetRegister(SREG_AX);
            WriteCmd(SCMD_DYNAMICBOUNDS, SREG_AX);
        }
        
        _marMgr.AddComponentOffset(index_value * factor);
        return;
    }

    if (all_ops_are_pending)
        _marMgr = save_mar_state;

    // DYNAMICBOUNDS compares the offset into the memory block:
    // it mustn't be larger than the size of the allocated memory. 
    // On the other hand, CHECKBOUNDS checks the index: it mustn't be
    // larger than the maximum given. So dynamic bounds must be checked
    // _after_ the multiplication; static bounds _before_ the multiplication.
    // For better error messages at runtime, don't do CHECKBOUNDS after the multiplication.
    if (!is_dynarray)
        WriteCmd(SCMD_CHECKBOUNDS, SREG_AX, dim);
    if (factor != 1)
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

// We're processing some struct component or global or local variable.
// If an array index follows, parse it and shorten symlist accordingly
void AGS::Parser::AccessData_ProcessArrayIndexIfThere(SrcList &expression, EvaluationResult &eres)
{
    if (kKW_OpenBracket != expression.PeekNext())
        return;
    expression.GetNext(); // Eat '['

    bool const is_dynarray = _sym.IsDynarrayVartype(eres.Vartype);
    bool const is_array = _sym.IsArrayVartype(eres.Vartype);
    if (!is_dynarray && !is_array)
        UserError("Array index is only legal after an array expression");

    Vartype const element_vartype = _sym[eres.Vartype].VartypeD->BaseVartype;
    size_t const element_size = _sym.GetSize(element_vartype);
    std::vector<size_t> dim_sizes;
    std::vector<size_t> dynarray_dims = { 0, };
    std::vector<size_t> &dims = is_dynarray ? dynarray_dims : _sym[eres.Vartype].VartypeD->Dims;
    eres.Vartype = element_vartype;

    if (is_dynarray)
        AccessData_Dereference(eres);

    // Number of dimensions and the the size of the dimension for each dimension
    size_t const num_of_dims = dims.size();
    dim_sizes.resize(num_of_dims);
    size_t factor = element_size;
    for (int dim_idx = num_of_dims - 1; dim_idx >= 0; dim_idx--) // yes, "int"
    {
        dim_sizes[dim_idx] = factor;
        factor *= dims[dim_idx];
    }

    for (size_t dim_idx = 0; dim_idx < num_of_dims; dim_idx++)
    {
        EvaluationResult eres_index;
        AccessData_ProcessCurrentArrayIndex(dim_idx, dims[dim_idx], dim_sizes[dim_idx], is_dynarray, expression, eres_index);
        if (eres_index.SideEffects)
            eres.SideEffects = true;
        Symbol divider = expression.PeekNext();
        Expect(SymbolList{ kKW_CloseBracket, kKW_Comma }, divider);
        
        if (kKW_CloseBracket == divider)
        {
            expression.GetNext(); // Eat ']'
            divider = expression.PeekNext();
        }
        if (kKW_Comma == divider || kKW_OpenBracket == divider)
        {
            if (num_of_dims == dim_idx + 1)
                UserError("Expected %d indexes, found more", num_of_dims);
            expression.GetNext(); // Eat ',' or '['
            continue;
        }
        if (num_of_dims != dim_idx + 1)
            UserError("Expected %d indexes, but only found %d", num_of_dims, dim_idx + 1);
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
    _reg_track.SetRegister(SREG_MAR);

    eres.Type = eres.kTY_RunTimeValue;
    eres.Location = eres.kLOC_MemoryAtMAR;
    eres.Symbol = kKW_NoSymbol;
    eres.Vartype = _sym.GetVartype(varname);
    eres.LocalNonParameter = (ScT::kLocal == scope_type && entry.Scope != _sym.kParameterScope);
    eres.Modifiable = !var_tqs[TQ::kReadonly];
    return AccessData_ProcessArrayIndexIfThere(expression, eres);

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

void AGS::Parser::AccessData_FirstClause(VariableAccess access_type, SrcList &expression, EvaluationResult &eres, bool &implied_this_dot)
{
    implied_this_dot = false;

    // Set defaults that are almost always correct
    eres.SideEffects = false;

    Symbol const first_sym = expression.PeekNext();

    if (kKW_Null == first_sym ||
        _sym.IsConstant(first_sym) ||
        _sym.IsLiteral(first_sym))
    {
        if (VAC::kReading != access_type)
            UserError("Cannot modify '%s'", _sym.GetName(first_sym).c_str());

        expression.GetNext();
        Symbol lit = first_sym;
        expression.GetNext(); // eat the literal
        while (_sym.IsConstant(lit))
            lit = _sym[lit].ConstantD->ValueSym;
        SetCompileTimeLiteral(lit, eres);
        return;
    }

    if (_sym.IsFunction(first_sym))
    {
        expression.GetNext(); // Eat function symbol
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
            AccessData_ProcessArrayIndexIfThere(expression, eres);
        return;
    }

    if (kKW_This == first_sym)
    {
        if (kKW_NoSymbol == _sym.GetVartype(kKW_This))
            UserError("'this' is only legal in a non-static struct function");

        expression.GetNext(); // Eat 'this'

        AccessData_This(eres);

        if (kKW_Dot == expression.PeekNext())
        {
            expression.GetNext(); // Eat '.'
            // Going forward, we must "imply" "this." since we've just gobbled it.
            implied_this_dot = true;
        }
        return;
    }

    if (_sym.IsVariable(first_sym))
    {
        AccessData_Variable(access_type, expression, eres);
        return;
    }

    if (_sym.IsStructVartype(first_sym))
    {
        expression.GetNext(); // Eat the struct vartype
        // Return the struct itself, static access
        eres.Type = eres.kTY_StructName;
        eres.Location = eres.kLOC_SymbolTable;
        eres.Symbol = first_sym;
        eres.Vartype = kKW_NoSymbol;
        _marMgr.Reset();
        return;
    }

    // Can this unknown symbol be interpreted as a component of 'this'?
    Vartype const this_vartype = _sym.GetVartype(kKW_This);
    if (_sym.IsVartype(this_vartype) && _sym[this_vartype].VartypeD->Components.count(first_sym))
    {
        // Fake a "this." here
        // Eat the component, pretend that it is 'this'. We need to do this in order 
        // to force the code that will be emitted to be connected to the proper place in the source.
        expression.GetNext(); 
        AccessData_This(eres);

        // Going forward, the code should imply "this."
        // with the '.' already read in.
        implied_this_dot = true;
        // Then back up so that the component will be read again as the next symbol.
        expression.BackUp();
        return;
    }

    if (kKW_OnePastLongMax == first_sym)
        UserError("Integer literal is out of bounds (maximum is %d)", LONG_MAX);

    UserError("Unexpected '%s' in expression", _sym.GetName(first_sym).c_str());
}

// We're processing a STRUCT.STRUCT. ... clause.
// We've already processed some structs, and the type of the last one is vartype.
// Now we process a component of vartype.
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
            ReferenceMsgSym("Expected a component of '%s', found '%s' instead", vartype).c_str(),
            _sym.GetName(vartype).c_str(),
            _sym.GetName(unqualified_component).c_str());

    if (_sym.IsAttribute(qualified_component))
    {
        // make MAR point to the struct of the attribute
        _marMgr.UpdateMAR(_src.GetLineno(), _scrip);
        _reg_track.SetRegister(SREG_MAR);

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
        expression.GetNext(); // Eat the constant symbol

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
            UserError("Must specify a specific object for non-static function %s", _sym.GetName(qualified_component).c_str());

        expression.GetNext(); // Eat function symbol
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

        expression.BackUp();
        SrcList start_of_funccall = SrcList(expression, expression.GetCursor(), expression.Length());
        AccessData_FunctionCall(qualified_component, start_of_funccall, eres);
        if (_sym.IsDynarrayVartype(vartype))
            return AccessData_ProcessArrayIndexIfThere(expression, eres);
        return;
    }

    if (_sym.IsVariable(qualified_component))
    {
        if (static_access && !_sym[qualified_component].VariableD->TypeQualifiers[TQ::kStatic])
            UserError(
                ReferenceMsgSym("Must specify a specific object for non-static component %s", qualified_component).c_str(),
                _sym.GetName(qualified_component).c_str());

        AccessData_StructMember(qualified_component, access_type, access_via_this, expression, eres);
        return AccessData_ProcessArrayIndexIfThere(expression, eres);
    }

    InternalError("Unknown kind of component of '%s'", _sym.GetName(vartype).c_str());
}

AGS::Symbol AGS::Parser::FindStructOfComponent(Vartype strct, Symbol unqualified_component)
{
    while (strct > 0 && _sym.IsVartype(strct))
    {
        auto const &components = _sym[strct].VartypeD->Components;
        if (0 < components.count(unqualified_component))
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
        if (0 < components.count(unqualified_component))
            return components.at(unqualified_component);
        strct = _sym[strct].VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

// We are in a STRUCT.STRUCT.STRUCT... cascade.
// Check whether we have passed the last dot
bool AGS::Parser::AccessData_IsClauseLast(SrcList &expression)
{
    size_t const cursor = expression.GetCursor();
    SkipTo(kKW_Dot,  expression);
    bool is_last = (kKW_Dot != expression.PeekNext());
    expression.SetCursor(cursor);
    return is_last;
}

// Access a variable, constant, literal, func call, struct.component.component cascade, etc.
// At end of function, symlist and symlist_len will point to the part of the symbol string
// that has not been processed yet
void AGS::Parser::AccessData(VariableAccess access_type, SrcList &expression, EvaluationResult &eres)
{
    expression.StartRead();
    if (0 == expression.Length())
        InternalError("Empty expression");
   
    bool implied_this_dot = false; // only true when "this." is implied
    bool static_access = false; // only true when a vartype has just been parsed

    // If we are reading, then all the accesses are for reading.
    // If we are writing, then all the accesses except for the last one
    // are for reading and the last one will be for writing.
    AccessData_FirstClause(AccessData_IsClauseLast(expression) ? access_type : VAC::kReading, expression, eres, implied_this_dot);
    
    Vartype outer_vartype = kKW_NoSymbol;

    // If the previous function has assumed a "this." that isn't there,
    // then a '.' won't be coming up but the while body must be executed anyway.
    while (kKW_Dot == expression.PeekNext() || implied_this_dot)
    {
        if (!implied_this_dot)
            expression.GetNext(); // Eat '.'
        // Note: do not reset "implied_this_dot" here, it's still needed.

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

            if (_sym.IsDynarrayVartype(eres.Vartype) && _sym.FindOrAdd("Length") == expression.PeekNext())
            {
                // Pseudo attribute 'Length' will get the length of the dynarray
                expression.GetNext(); // eat 'Length'

                AccessData_GenerateDynarrayLengthFuncCall(eres);
                implied_this_dot = false;
                continue;
            }

            if (_sym.IsArrayVartype(eres.Vartype) || _sym.IsDynarrayVartype(eres.Vartype))
                UserError("Expected a struct in front of '.' but found an array instead");
            else if (!_sym.IsStructVartype(eres.Vartype))
                UserError(
                    "Expected a struct in front of '.' but found an expression of type '%s' instead",
                    _sym.GetName(outer_vartype).c_str());
        }
        if (expression.ReachedEOF())
            UserError("Expected struct component after '.' but did not find it");

        // If we are reading, then all the accesses are for reading.
        // If we are writing, then all the accesses except for the last one
        // are for reading and the last one will be for writing.
        AccessData_SubsequentClause(AccessData_IsClauseLast(expression) ? access_type : VAC::kReading, implied_this_dot, expression, eres);
        
        // Next component access, if there is any, is dependent on the current access, no longer on "this".
        implied_this_dot = false;
    }
}

// Insert Bytecode for:
// Copy at most OLDSTRING_LENGTH - 1 bytes from m[MAR...] to m[AX...]
// Stop when encountering a 0
void AGS::Parser::AccessData_StrCpy()
{
    BackwardJumpDest loop_start(_scrip);
    ForwardJump out_of_loop(_scrip);

    WriteCmd(SCMD_REGTOREG, SREG_AX, SREG_CX); // CX = dest
    WriteCmd(SCMD_REGTOREG, SREG_MAR, SREG_BX); // BX = src
    WriteCmd(SCMD_LITTOREG, SREG_DX, STRINGBUFFER_LENGTH - 1); // DX = count
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

// We are typically in an assignment LHS = RHS; the RHS has already been
// evaluated, and the result of that evaluation is in AX.
// Store AX into the memory location that corresponds to LHS, or
// call the attribute function corresponding to LHS.
void AGS::Parser::AccessData_AssignTo(SrcList &expression, EvaluationResult eres)
{
    // We'll evaluate expression later on which moves the cursor,
    // so save it here and restore later on
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
            if (!expression.ReachedEOF()  && lhs_eres.kTY_AttributeName != lhs_eres.Type)
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
        AccessData_StrCpy();
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

    Symbol const vartype_of_this = _sym[kKW_This].VariableD->Vartype;

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

        if (kKW_Colon == peeksym)
        {
            // This is only allowed if it can be matched to an open tern
            if (--tern_depth < 0)
                break;

            src.GetNext(); // Eat ':'
            continue;
        }

        if (kKW_Dot == peeksym)
        {
            src.GetNext(); // Eat '.'
            src.GetNext(); // Eat following symbol
            continue;
        }

        if (kKW_New == peeksym)
        {   // Only allowed if a type follows   
            src.GetNext(); // Eat 'new'
            Symbol const sym_after_new = src.PeekNext();
            if (_sym.IsVartype(sym_after_new))
            {
                src.GetNext(); // Eat symbol after 'new'
                continue;
            }
            src.BackUp(); // spit out 'new'
            break;
        }

        if (kKW_Null == peeksym)
        {   // Allowed.
            src.GetNext(); // Eat 'null'
            continue;
        }

        if (kKW_Tern == peeksym)
        {
            tern_depth++;
            src.GetNext(); // Eat '?'
            continue;
        }

        if (_sym.IsVartype(peeksym))
        {   // Only allowed if a dot follows
            src.GetNext(); // Eat the vartype
            Symbol const nextsym = src.PeekNext();
            if (kKW_Dot == nextsym)
                continue; // Do not eat the dot.
            src.BackUp(); // spit out the vartype
            break;
        }

        // Let a symbol through if it can be considered a component of 'this'.
        if (kKW_NoSymbol != vartype_of_this &&
            0 < _sym[vartype_of_this].VartypeD->Components.count(peeksym))
        {
            src.GetNext(); // Eat the peeked symbol
            continue;
        }

        if (!_sym.CanBePartOfAnExpression(peeksym))
            break;
        src.GetNext(); // Eat the peeked symbol
    }

    if (nesting_depth > 0)
        InternalError("Nesting corrupted");
}

void AGS::Parser::ParseExpression(SrcList &src, EvaluationResult &eres)
{
    size_t const expr_start = src.GetCursor();
    SkipToEndOfExpression(src);
    SrcList expression = SrcList(src, expr_start, src.GetCursor() - expr_start);
    if (0 == expression.Length())
        UserError("Expected an expression, found '%s' instead", _sym.GetName(src.GetNext()).c_str());

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

// We are parsing the left hand side of a += or similar statement.
void AGS::Parser::ParseAssignment_ReadLHSForModification(SrcList &expression, EvaluationResult &eres)
{
    AccessData(VAC::kReadingForLaterWriting, expression, eres);
    ParseExpression_CheckUsedUp(expression);
    
    // Also put the value into AX so that it can be read/modified as well as written
    EvaluationResult eres_dummy = eres;
    EvaluationResultToAx(eres_dummy); // Don't clobber eres
}

// "var = expression"; lhs is the variable
void AGS::Parser::ParseAssignment_Assign(SrcList &lhs)
{
    _src.GetNext(); // Eat '='
    EvaluationResult eres;
    ParseExpression(_src, eres); // RHS of the assignment
        
    return AccessData_AssignTo(lhs, eres);
}

// We compile something like "var += expression"
void AGS::Parser::ParseAssignment_MAssign(Symbol const ass_symbol, SrcList &lhs)
{
    _src.GetNext(); // Eat assignment symbol

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
    ParseExpression(_src, eres);
    
    if (eres.kTY_Literal != eres.Type)
        UserError("Cannot evaluate this expression at compile time, it cannot be used as initializer");

    CodeCell const litval = _sym[eres.Symbol].LiteralD->Value;

    if ((kKW_Float == wanted_vartype) != (kKW_Float == eres.Vartype))
        UserError(
            "Expected a '%s' value after '=' but found a '%s' value instead",
            _sym.GetName(wanted_vartype).c_str(),
            _sym.GetName(eres.Vartype).c_str());
    
    size_t const wanted_size = _sym.GetSize(wanted_vartype);
    initial_val.resize(wanted_size);
    switch (wanted_size)
    {
    default:
        UserError("Cannot give an initial value to a variable of type '%s' here", _sym.GetName(wanted_vartype));
        return;
    case 1:
        initial_val[0] = litval;
        return;
    case 2:
        (reinterpret_cast<int16_t *> (&initial_val[0]))[0] = litval;
        return;
    case 4:
        (reinterpret_cast<int32_t *> (&initial_val[0]))[0] = litval;
        return;
    }
}

void AGS::Parser::ParseVardecl_InitialValAssignment_OldString(std::vector<char> &initial_val)
{
    Symbol string_lit = _src.GetNext();
    if (_sym.IsConstant(string_lit))
        string_lit = _sym[string_lit].ConstantD->ValueSym;
    
	if (!_sym.IsLiteral(string_lit) ||
        _sym.VartypeWith(VTT::kConst, kKW_String) != _sym[string_lit].LiteralD->Vartype)
        UserError("Expected a string literal after '=', found '%s' instead", _sym.GetName(_src.PeekNext()).c_str());

    // The scanner has put the constant string into the strings table. That's where we must find and get it.
    std::string const lit_value = &(_scrip.strings[_sym[string_lit].LiteralD->Value]);
    
    if (lit_value.length() >= STRINGBUFFER_LENGTH)
        UserError(
            "Initializer string is too long (max. chars allowed: %d)",
            STRINGBUFFER_LENGTH - 1);

    initial_val.assign(lit_value.begin(), lit_value.end());
    initial_val.push_back('\0');
}

void AGS::Parser::ParseVardecl_InitialValAssignment(Symbol varname, std::vector<char> &initial_val)
{
    _src.GetNext(); // Eat '='

    Vartype const vartype = _sym.GetVartype(varname);
    if (_sym.IsManagedVartype(vartype))
        return Expect(kKW_Null, _src.GetNext());

    if (_sym.IsStructVartype(vartype))
        UserError("'%s' is a struct and cannot be initialized here", _sym.GetName(varname).c_str());
    if (_sym.IsArrayVartype(vartype))
        UserError("'%s' is an array and cannot be initialized here", _sym.GetName(varname).c_str());

    if (kKW_String == vartype)
        return ParseVardecl_InitialValAssignment_OldString(initial_val);

    if (_sym.IsAnyIntegerVartype(vartype) || kKW_Float == vartype)
        return ParseVardecl_InitialValAssignment_IntOrFloatVartype(vartype, initial_val);

    UserError(
        "Variable '%s' has type '%s' and cannot be initialized here",
        _sym.GetName(varname).c_str(),
        _sym.GetName(vartype).c_str());
}

void AGS::Parser::ParseVardecl_Var2SymTable(Symbol const var_name, Vartype const vartype)
{
    SymbolTableEntry &var_entry = _sym[var_name];
    _sym.MakeEntryVariable(var_name);
    var_entry.VariableD->Vartype = vartype;
    var_entry.Scope = _nest.TopLevel();
    _sym.SetDeclared(var_name, _src.GetCursor());
}

void AGS::Parser::ParseStruct_ConstantDefn(Symbol const name_of_struct)
{
    if (PP::kMain != _pp)
    {
        SkipTo(kKW_Semicolon, _src);
        Expect(kKW_Semicolon, _src.GetNext());
        return;
    }

    Vartype const vartype = ParseVartype();
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
        if (!in_struct && _nest.TopLevel() > 1)
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
    if (vartype == kKW_String && FlagIsSet(_options, SCOPT_OLDSTRINGS) == 0)
        UserError("Variables of type 'string' aren't supported any longer (use the type 'String' instead)");
    if (vartype == kKW_String && ScT::kImport == scope_type)
        // cannot import because string is really char *, and the pointer won't resolve properly
        UserError("Cannot import a 'string' variable; use 'char[]' instead");
    if (vartype == kKW_Void)
        UserError("'void' is not a valid type in this context");
}

// there was a forward declaration -- check that the real declaration matches it
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
    std::vector<char> initial_val(vartype_size + 1, '\0');

    if (kKW_Assign == _src.PeekNext())
        ParseVardecl_InitialValAssignment(var_name, initial_val);
    
    SymbolTableEntry &entry = _sym[var_name];
    entry.VariableD->Vartype = vartype;
    int const global_offset = _scrip.AddGlobal(vartype_size, &initial_val[0]);
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
        if (4u == var_size && !is_dyn)
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
    _src.GetNext(); // Eat '='
    EvaluationResult rhs_eres;
    ParseExpression(_src, rhs_eres);
    EvaluationResultToAx(rhs_eres);
    
    // Vartypes must match. This is true even if the lhs is readonly.
    // As a special case, a string may be assigned a const string because the const string will be copied, not modified.
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
        AccessData_StrCpy();
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
		ParseArray(var_name, vartype);

    // Don't warn for builtins or imports, they might have been predefined
    if (!_sym.IsBuiltinVartype(vartype) && ScT::kImport != scope_type && 0 == _sym.GetSize(vartype))
        Warning(
            ReferenceMsgSym("Variable '%s' has zero size", vartype).c_str(),
            _sym.GetName(var_name).c_str());

    // Enter the variable into the symbol table
    ParseVardecl_Var2SymTable(var_name, vartype);
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
        return ParseVardecl_Local(var_name, vartype);
    }
}

void AGS::Parser::ParseVardecl_CheckAndStashOldDefn(Symbol var_name)
{
    do // exactly 1 times
    {
        if (_sym.IsFunction(var_name))
        {
            Warning(
                ReferenceMsgSym("This hides the function '%s()'", var_name).c_str(),
                _sym.GetName(var_name).c_str());
            break;
        }

        if (_sym.IsPredefined(var_name))
            UserError("Cannot redefine the predefined '%s'", _sym.GetName(var_name));

        if (_sym.IsVariable(var_name))
            break;

        // Local compile-time constants can be overridden, but global constants can't
        if (_sym.IsConstant(var_name) && _sym[var_name].Scope > 0u)
            break;

        if (_sym.IsVartype(var_name))
            UserError(
                ReferenceMsgSym("'%s' is in use as a type elsewhere", var_name).c_str(),
                _sym.GetName(var_name).c_str());

        if (!_sym.IsInUse(var_name))
            break;

        UserError(
            ReferenceMsgSym("'%s' is already in use elsewhere", var_name).c_str(),
            _sym.GetName(var_name).c_str());
    }
    while (false);

    if (_nest.TopLevel() == _sym[var_name].Scope)
        UserError(
            ReferenceMsgSym("'%s' has already been defined in this scope", var_name).c_str(),
            _sym.GetName(var_name).c_str());
    if (SymbolTable::kParameterScope == _sym[var_name].Scope && SymbolTable::kFunctionScope == _nest.TopLevel())
        UserError(
            ReferenceMsgSym("'%s' has already been defined as a parameter", var_name).c_str(),
            _sym.GetName(var_name).c_str());
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

void AGS::Parser::ParseFuncBodyStart(Symbol struct_of_func,Symbol name_of_func)
{
    _nest.Push(NSType::kFunction);

    // write base address of function for any relocation needed later
    WriteCmd(SCMD_THISBASE, _scrip.codesize);
    SymbolTableEntry &entry = _sym[name_of_func];
    if (entry.FunctionD->NoLoopCheck)
        WriteCmd(SCMD_LOOPCHECKOFF);

    // If there are dynpointer parameters, then the caller has simply "pushed" them onto the stack.
    // We catch up here by reading each dynpointer and writing it again using MEMINITPTR
    // to declare that the respective cells will from now on be used for dynpointers.
    size_t const num_params = _sym.NumOfFuncParams(name_of_func);
    for (size_t param_idx = 1; param_idx <= num_params; param_idx++) // skip return value param_idx == 0
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
    if (struct_of_func > 0 && !_sym[name_of_func].FunctionD->TypeQualifiers[TQ::kStatic])
    {
        // Declare "this" but do not allocate memory for it
        this_entry.Scope = 0u;
        this_entry.Accessed = true; 
        this_entry.VariableD->Vartype = struct_of_func; // Don't declare this as dynpointer
        this_entry.VariableD->TypeQualifiers = {};
        this_entry.VariableD->TypeQualifiers[TQ::kReadonly] = true;
        this_entry.VariableD->Offset = 0u;
    }
}

void AGS::Parser::HandleEndOfFuncBody(Symbol &struct_of_current_func, Symbol &name_of_current_func)
{
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

// We have accepted something like "struct foo" and are waiting for "extends"
void AGS::Parser::ParseStruct_ExtendsClause(Symbol stname)
{
    _src.GetNext(); // Eat "extends"
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

// Check whether the qualifiers that accumulated for this decl go together
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

void AGS::Parser::ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype)
{
    if (tqs[TQ::kWriteprotected])
        UserError("Cannot apply 'writeprotected' to this function declaration");

    _src.GetNext(); // Eat '('
    ParseFuncdecl(tqs, vartype, struct_of_func, name_of_func, false, false);

    // Can't code a body behind the function, so the next symbol must be ';'
    return Expect(kKW_Semicolon, _src.PeekNext());
}

void AGS::Parser::ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype)
{
    SymbolTableEntry &entry = _sym[name_of_func];
    size_t const num_parameters_wanted = (is_indexed ? 1 : 0) + (is_setter ? 1 : 0);
    if (num_parameters_wanted != _sym.NumOfFuncParams(name_of_func))
    {
        std::string const msg = ReferenceMsgSym(
            "The attribute function '%s' should have %d parameter(s) but is declared with %d parameter(s) instead",
            name_of_func);
        UserError(msg.c_str(), entry.Name.c_str(), num_parameters_wanted, _sym.NumOfFuncParams(name_of_func));
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

    size_t p_idx = 1;
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
    auto &parameters = _sym[name_of_func].FunctionD->Parameters;
    FuncParameterDesc fpd = {};
    if (is_indexed)
    {
        fpd.Vartype = kKW_Int;
        parameters.push_back(fpd);
    }
    if (is_setter)
    {
        fpd.Vartype = vartype;
        parameters.push_back(fpd);
    }
}

// We are processing an attribute.
// This corresponds to a getter func and a setter func, declare one of them
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
    Symbol const qualified_getter = MangleStructAndComponent(name_of_struct, unqualified_getter);
    _sym[qualified_attribute].AttributeD->Getter = qualified_getter;
    ParseStruct_Attribute_DeclareFunc(tqs, name_of_struct, qualified_getter, unqualified_getter, false, is_indexed, vartype);
    _sym.SetDeclared(qualified_getter, declaration_start);

    if (!is_readonly)
    {
        // Declare attribute setter, e.g. set_ATTRIB(value)
        Symbol const unqualified_setter =
            ConstructAttributeFuncName(qualified_attribute, true, is_indexed);
        Symbol const qualified_setter = MangleStructAndComponent(name_of_struct, unqualified_setter);
        _sym[qualified_attribute].AttributeD->Setter = qualified_setter;
        ParseStruct_Attribute_DeclareFunc(tqs, name_of_struct, qualified_setter, unqualified_setter, true, is_indexed, vartype);
        _sym.SetDeclared(qualified_setter, declaration_start);
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
        _src.GetNext(); // Eat 'const'

    Vartype vartype = _src.GetNext();

    if (!_sym.IsVartype(vartype))
        UserError("Expected a type, found '%s' instead", _sym.GetName(vartype).c_str());
    if (is_const_vartype && kKW_String != vartype)
        UserError("The only allowed type that starts with 'const' is 'const string' (did you mean 'readonly attribute'?)");

    SetDynpointerInManagedVartype(vartype);
    EatDynpointerSymbolIfPresent(vartype);

    while (true)
    {
        Symbol const attribute = ParseVarname();
        
        bool const is_indexed = (kKW_OpenBracket == _src.PeekNext());
        if (is_indexed)
        {
            _src.GetNext(); // Eat '['
            Expect(kKW_CloseBracket, _src.GetNext());
        }

        ParseStruct_Attribute2SymbolTable(tqs, vartype, name_of_struct, attribute, is_indexed);

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_Semicolon }, punctuation);
        if (kKW_Semicolon == punctuation)
            return;
    }
}

// We're parsing an array var.
void AGS::Parser::ParseArray(Symbol vname, Vartype &vartype)
{
    _src.GetNext(); // Eat '['

    if (PP::kPreAnalyze == _pp)
    {
        // Skip the sequence of [...]
        while (true)
        {
            SkipToClose(kKW_CloseBracket);
            if (kKW_OpenBracket != _src.PeekNext())
                return;
            _src.GetNext(); // Eat '['
        }
    }

    if (kKW_CloseBracket == _src.PeekNext())
    {
        // Dynamic array
        _src.GetNext(); // Eat ']'
        if (vartype == kKW_String)
            UserError("Dynamic arrays of old-style strings are not supported");
        if (!_sym.IsAnyIntegerVartype(vartype) && !_sym.IsManagedVartype(vartype) && kKW_Float != vartype)
            UserError(
				"Can only have dynamic arrays of integer types, 'float', or managed structs. '%s' isn't any of this.", 
				_sym.GetName(vartype).c_str());
        vartype = _sym.VartypeWith(VTT::kDynarray, vartype);
        return;
    }

    std::vector<size_t> dims;

    // Static array
    while (true)
    {
        std::string msg = "For dimension #<dim> of array '<arr>': ";
        msg.replace(msg.find("<dim>"), 5u, std::to_string(dims.size()));
        msg.replace(msg.find("<arr>"), 5u, _sym.GetName(vname).c_str());
        Symbol const first_sym = _src.PeekNext();

        EvaluationResult eres;
        int const cursor = _src.GetCursor();
        SkipTo(kKW_Comma, _src);
        SrcList expression = SrcList(_src, cursor, _src.GetCursor() - cursor);
        expression.StartRead();
        ParseIntegerExpression(expression, eres, msg);
        if (eres.kTY_Literal != eres.Type)
            UserError(
                (msg + "Cannot evaluate the expression starting with '%s' at compile time").c_str(),
                _sym.GetName(first_sym).c_str());
            
        CodeCell const dimension_as_int = _sym[eres.Symbol].LiteralD->Value;
        if (dimension_as_int < 1)
            UserError(
                "Array dimension #%u of array '%s' must be at least 1 but is %d instead",
                dims.size(),
                _sym.GetName(vname).c_str(),
                dimension_as_int);

        dims.push_back(dimension_as_int);

        Symbol const punctuation = _src.GetNext();
        Expect(SymbolList{ kKW_Comma, kKW_CloseBracket }, punctuation);
        if (kKW_Comma == punctuation)
            continue;
        if (kKW_OpenBracket != _src.PeekNext())
            break;
        _src.GetNext(); // Eat '['
    }
    vartype = _sym.VartypeWithArray(dims, vartype);
}

void AGS::Parser::ParseStruct_VariableDefn(TypeQualifierSet tqs, Vartype vartype, Symbol name_of_struct, Symbol vname)
{
    if (PP::kMain != _pp)
        return SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon }, _src);

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
        ParseArray(vname, vartype);
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
        ParseStruct_FuncDecl(name_of_struct, qualified_component, tqs, vartype);
    else 
        ParseStruct_VariableDefn(tqs, vartype, name_of_struct, qualified_component);

    _sym.SetDeclared(qualified_component, declaration_start);
 }

void AGS::Parser::EatDynpointerSymbolIfPresent(Vartype vartype)
{
    if (kKW_Dynpointer != _src.PeekNext())
        return;

    if (PP::kPreAnalyze == _pp || _sym.IsManagedVartype(vartype))
    {
        _src.GetNext(); // Eat '*'
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
    Vartype vartype = ParseVartype();

    if (PP::kMain == _pp)
        // Check for illegal struct member types
		ParseStruct_CheckComponentVartype(name_of_struct, vartype);

    // "int [] func(...)"
    ParseDynArrayMarkerIfPresent(vartype);
    
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
        _src.GetNext(); // Eat ';'
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
        if (0 != (struct_size % STRUCT_ALIGNTO))
            struct_size += STRUCT_ALIGNTO - (struct_size % STRUCT_ALIGNTO);
    }

    _src.GetNext(); // Eat '}'

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
        _src.GetNext(); // Eat ';'
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
    EatDynpointerSymbolIfPresent(vartype);
    ParseDynArrayMarkerIfPresent(vartype);
    ScopeType const scope_type = (vardecl_tqs[TQ::kImport]) ? ScT::kImport : ScT::kGlobal;

    ParseVartype_MemberList(vardecl_tqs, vartype, scope_type, false, struct_of_current_func, name_of_current_func);
}

// We've accepted something like "enum foo { bar"; '=' follows
void AGS::Parser::ParseEnum_AssignedValue(Symbol vname, CodeCell &value)
{
    _src.GetNext(); // eat "="

    std::string msg = "In the assignment to <name>: ";
    msg.replace(msg.find("<name>"), 6u, _sym.GetName(vname));
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

AGS::Symbol AGS::Parser::ParseVartype(bool const with_dynpointer_handling)
{
    bool const leading_const = (kKW_Const == _src.PeekNext());
    if (leading_const)
        _src.GetNext(); // Eat 'const'

    Vartype vartype = _src.GetNext();
    if (!_sym.IsVartype(vartype))
        UserError("Expected a type, found '%s' instead", _sym.GetName(vartype).c_str());
    if (_sym[vartype].VartypeD->Flags[VTF::kUndefined])
        _structRefs[vartype] = _src.GetCursor();

    if (leading_const)
    {
        if (kKW_String != vartype)
            UserError("The only allowed type beginning with 'const' is 'const string' (did you want to use 'readonly'?)");
        vartype = _sym.VartypeWith(VTT::kConst, vartype);
    }

    if (with_dynpointer_handling)
    {
        SetDynpointerInManagedVartype(vartype);
        EatDynpointerSymbolIfPresent(vartype);
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
        _src.GetNext(); // Eat ';'
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
    ParseDynArrayMarkerIfPresent(vartype);
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

    Vartype vartype = ParseVartype();

    while (true)
    {
        Symbol const attribute = ParseVarname();
        
        bool const is_indexed = (kKW_OpenBracket == _src.PeekNext());
        if (is_indexed)
        {
            _src.GetNext(); // Eat '['
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
                _src.GetNext(); // Eat optional '*'
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
        _sym.GetName(func).c_str(),
        _sym[func].FunctionD->Offset,
        _sym.NumOfFuncParams(func) + 100 * _sym[func].FunctionD->IsVariadic);
    if (retval < 0)
        InternalError("Could not export function");
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
        _sym.GetName(var).c_str(),
        _sym[var].VariableD->Offset);
    if (retval < 0)
        InternalError("Could not export variable");
}

void AGS::Parser::ParseExport()
{
    if (PP::kPreAnalyze == _pp)
    {
        SkipTo(kKW_Semicolon, _src);
        _src.GetNext(); // Eat ';'
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
    _src.GetNext(); // Eat '('

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
    ParseFuncdecl(tqs, vartype, struct_name, func_name, false, body_follows);
    _sym.SetDeclared(func_name, declaration_start);

    if (!body_follows)
        return;

    if (0 < name_of_current_func)
        UserError(
            ReferenceMsgSym("Function bodies cannot nest, but the body of function %s is still open. (Did you forget a '}'?)", func_name).c_str(),
            _sym.GetName(name_of_current_func).c_str());
        
    _sym[func_name].FunctionD->NoLoopCheck = no_loop_check;

    // We've started a function, remember what it is.
    name_of_current_func = func_name;
    struct_of_current_func = struct_name;
}

void AGS::Parser::ParseVartype_VarDecl_PreAnalyze(Symbol var_name, ScopeType scope_type)
{
    if (0 != _givm.count(var_name))
    {
        if (_givm[var_name])
            UserError("'%s' is already defined as a global non-import variable", _sym.GetName(var_name).c_str());
        else if (ScT::kGlobal == scope_type && FlagIsSet(_options, SCOPT_NOIMPORTOVERRIDE))
            UserError("'%s' is defined as an import variable; that cannot be overridden here", _sym.GetName(var_name).c_str());
    }
    _givm[var_name] = (ScT::kGlobal == scope_type);

    // Apart from this, we aren't interested in var defns at this stage, so skip this defn
    SkipTo(SymbolList{ kKW_Comma, kKW_Semicolon }, _src);
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
    Vartype vartype = ParseVartype(false);

    // Imply a pointer for managed vartypes. However, do NOT do this in 'import' statements.
    // (Reason: automatically generated code does things like "import Object oFoo;" and
    // then it really does mean "Object", not "Object *".
    // This can only happen in automatically generated code:
    // Users are never allowed to define unpointered managed entities.
    if (kKW_Dynpointer == _src.PeekNext() ||
        _sym.IsAutoptrVartype(vartype) ||
        (ScT::kImport != scope_type && _sym.IsManagedVartype(vartype)))
    {
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
    }
    EatDynpointerSymbolIfPresent(vartype);
    
    // Imply a pointer for managed vartypes. However, do NOT
    // do this in import statements. (Reason: automatically
    // generated code does things like "import Object oFoo;" and
    // then it really does mean "Object", not "Object *".
    // This can only happen in automatically generated code:
    // Users are never allowed to define unpointered managed entities.
    if (kKW_Dynpointer == _src.PeekNext() ||
        _sym.IsAutoptrVartype(vartype) ||
        (ScT::kImport != scope_type && _sym.IsManagedVartype(vartype)))
    {
        vartype = _sym.VartypeWith(VTT::kDynpointer, vartype);
    }

    EatDynpointerSymbolIfPresent(vartype);
    
    // "int [] func(...)"
    ParseDynArrayMarkerIfPresent(vartype);
    
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

// Evaluate the header of an "if" clause, e.g. "if (i < 0)".
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
    _src.GetNext(); // Eat "else"
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

// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
void AGS::Parser::ParseWhile()
{
    // point to the start of the code that evaluates the condition
    CodeLoc const condition_eval_loc = _scrip.codesize;

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

    if (expression.Length() == 0)
        UserError("Unexpected symbol '%s'", _sym.GetName(_src.GetNext()).c_str());

    Symbol const assignment_symbol = _src.PeekNext();
    switch (assignment_symbol)
    {
    default:
    {
        // No assignment symbol following: This is an isolated expression, e.g., a function call
        EvaluationResult eres;
        size_t const expr_end = _src.GetCursor();
        ParseExpression_Term(expression, eres, false);
        _src.SetCursor(expr_end);
        if (eres.kTY_FunctionName == eres.Type)
            Expect(kKW_OpenParenthesis, _src.GetNext());
        if (eres.kTY_StructName == eres.Type)
            Expect(kKW_Dot, _src.GetNext());
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
    case kKW_AssignMultiply:
    case kKW_AssignPlus:
    case kKW_AssignShiftLeft:
    case kKW_AssignShiftRight:
        return ParseAssignment_MAssign(assignment_symbol, expression);
    }
}

void AGS::Parser::ParseFor_InitClauseVardecl()
{
    Vartype vartype = _src.GetNext();
    SetDynpointerInManagedVartype(vartype);
    EatDynpointerSymbolIfPresent(vartype);
    
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
            _src.GetNext(); // Eat ','
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
        _src.GetNext(); // Eat '}'
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
    CodeLoc const start_of_code_loc = _scrip.codesize;
    size_t const start_of_fixups = _scrip.numfixups;
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
        _nest.SwitchDefaultIdx() = _nest.SwitchCaseStart().size() - 1;
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
    if (size_of_local_vars > 0)
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
    for (nesting_level = _nest.TopLevel(); nesting_level > 0; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kSwitch == ltype || NSType::kWhile == ltype)
            break;
    }

    if (0u == nesting_level)
        UserError("Can only use 'break' inside a loop or a 'switch' statement block");

    _nest.JumpOutLevel() = std::min(_nest.JumpOutLevel(), nesting_level);

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    FreeDynpointersOfLocals(nesting_level + 1);
    RemoveLocalsFromStack(nesting_level + 1);
    
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
    for (nesting_level = _nest.TopLevel(); nesting_level > 0; nesting_level--)
    {
        NSType const ltype = _nest.Type(nesting_level);
        if (NSType::kDo == ltype || NSType::kWhile == ltype)
            break;
    }

    if (nesting_level == 0)
        UserError("Can only use 'continue' inside a loop");

    _nest.JumpOutLevel() = std::min(_nest.JumpOutLevel(), nesting_level);

    size_t const save_offset = _scrip.OffsetToLocalVarBlock;
    FreeDynpointersOfLocals(nesting_level + 1);
    RemoveLocalsFromStack(nesting_level + 1);

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
            return SkipToClose(kKW_CloseBrace);
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
    size_t nesting_level = 0;

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
            UserError("'%s' is illegal outside a function", _sym.GetName(leading_sym).c_str());

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
    for (size_t entries_idx = 0; entries_idx < _sym.entries.size(); entries_idx++)
    {
        if (_sym[entries_idx].Accessed)
            continue;

        // Don't "compact" the entries in the _scrip.imports[] array. They are referenced by index, and if you
        // change the indexes of the entries then you get dangling "references". So the only thing allowed is
        // setting unused import entries to "".
        if (_sym.IsFunction(entries_idx))
        {
            if(_sym[entries_idx].FunctionD->TypeQualifiers[TQ::kImport])
                _scrip.imports[_sym[entries_idx].FunctionD->Offset][0] = '\0';
            continue;
        }
        if (_sym.IsVariable(entries_idx))
        {
            if (_sym[entries_idx].VariableD->TypeQualifiers[TQ::kImport])
                _scrip.imports[_sym[entries_idx].VariableD->Offset][0] = '\0';
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
    vsprintf(&message[0u], msg, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    Error(false, &message[0u]);
}

void AGS::Parser::InternalError(char const *descr, ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    // '+ 1' for the trailing '\0'
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(&message[0u], descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    Error(true, &message[0u]);
}

void AGS::Parser::Warning(char const *descr, ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    // '+ 1' for the trailing '\0'
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(&message[0u], descr, vlist2);
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
    for (size_t fixup_idx = 0; fixup_idx < static_cast<size_t>(_scrip.numfixups); fixup_idx++)
    {
        if (FIXUP_IMPORT != _scrip.fixuptypes[fixup_idx])
            continue;
        int const code_idx = _scrip.fixups[fixup_idx];
        if (code_idx < 0 || code_idx >= _scrip.codesize)
            InternalError(
                "!Fixup #%d references non-existent code offset #%d",
                fixup_idx,
                code_idx);
        int const cv = _scrip.code[code_idx];
        if (cv < 0 || cv >= _scrip.numimports || '\0' == _scrip.imports[cv][0])
            InternalError(
                "Fixup #%d references non-existent import #%d",
                fixup_idx,
                cv);
    }
}

void AGS::Parser::Parse_ExportAllFunctions()
{
    for (size_t func_num = 0; func_num < _scrip.Functions.size(); func_num++)
    {
        if (0 > _scrip.AddExport(
            _scrip.Functions[func_num].Name,
            _scrip.Functions[func_num].CodeOffs,
            _scrip.Functions[func_num].NumOfParams))
            InternalError("Function export failed. Out of memory?");
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
        msg.replace(msg.find("<line>"), 6u, std::to_string(currentline));
        msg.append(e.what());

        _msgHandler.AddMessage(
            MessageHandler::kSV_InternalError,
            _src.SectionId2Section(_src.GetSectionId()),
            _src.GetLineno(),
            msg);
    }
}

// Scan inpl into scan tokens, build a symbol table
int cc_scan(std::string const &inpl, AGS::SrcList &src, AGS::ccCompiledScript &scrip, AGS::SymbolTable &symt, AGS::MessageHandler &mh)
{
    AGS::Scanner scanner = { inpl, src, scrip, symt, mh };
    scanner.Scan();
    return -static_cast<int>(mh.HasError());
}

int cc_parse(AGS::SrcList &src, AGS::FlagSet options, AGS::ccCompiledScript &scrip, AGS::SymbolTable &symt, AGS::MessageHandler &mh)
{
    AGS::Parser parser = { src, options, scrip, symt, mh };
    parser.Parse();
    return -static_cast<int>(mh.HasError());
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

    int error_code = cc_scan(inpl, src, scrip, symt, mh);
    if (error_code >= 0)
        error_code = cc_parse(src, options, scrip, symt, mh);
    sections = lh.CreateSectionList();
    return error_code;
}
