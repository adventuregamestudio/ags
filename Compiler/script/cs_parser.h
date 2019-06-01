/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - INTERFACE AND HIGH-LEVEL STRUCTURE

The processing is done in the following layers:
* [Preprocessing - This has been done separately before the input arrives here.]
    Expand macros, delete comments

* Scanning
    Read the characters of the input and partition it in symbols (e.g., identifier, number literal).
* Tokenizing
    Enter all the symbols into a symbol table (thus recognizing keywords)
    Enter all literal strings into a strings table
These two steps are piped. They are performed separately _before_ the Parsing (below) begins.
The result is:
    the symbol table, a parameter sym that is a struct SymbolTable.
    the sequence of tokens, a parameter targ that is a struct ccInternalList *.
    the collected string literals that go into a struct ccCompiledScript.

* Parsing
    All the high-level logic.
The parsing functions get the input in a parameter targ that is a ccInternalList *.
The parser augments the symbol table sym as it goes along.
The result is in a parameter scrip that is a struct ccCompiledScript *
and has the following key components (amongst many other components):
    functions - all the functions defined (i.e. they have a body) in the current inputstring
    imports - all the functions declared (i.e. without body) in the current inputstring
        NOTE: This includes "forward" declarations where a func definition will arrive later after all.
    exports - all the functions that are made available to other entities
    code, fixups - the Bytecode that is generated.

(For an overview of the implementation details, see cs_parser.cpp)
*/

//-----------------------------------------------------------------------------
//  Should be used only internally by cs_compiler.cpp
//-----------------------------------------------------------------------------

#ifndef __CS_PARSER_H
#define __CS_PARSER_H

#include <vector>
#include <string>
#include <map>

#include "cc_compiledscript.h"
#include "cc_internallist.h"
#include "cc_symboltable.h"
#include "script/script_common.h"

namespace AGS
{

// The stack of nesting statements 
class NestingStack
{
private:
    static int _chunkIdCtr; // for assigning unique IDs to chunks

    // A section of compiled code that needs to be moved or copied to a new location
    struct Chunk
    {
        std::vector<AGS::CodeCell> Code;
        std::vector<AGS::CodeLoc> Fixups;
        std::vector<char> FixupTypes;
        int CodeOffset;
        int FixupOffset;
        int Id;
    };

    // All data that is associated with a nesting level
    struct NestingInfo
    {
        int Type; // Type of the level, see AGS::NestingStack::NestingType
        AGS::CodeLoc StartLoc; // Index of the first byte generated for the level
        AGS::CodeLoc Info; // Various uses that differ by nesting type
        AGS::CodeLoc DefaultLabelLoc; // Location of default label
        std::vector<Chunk> Chunks; // Bytecode chunks that must be moved (FOR loops and SWITCH)
    };

    std::vector<NestingInfo> _stack;

public:
    enum NestingType
    {
        kNT_Nothing = 0,  // {...} in the code without a particular purpose
        kNT_Function,     // A function
        kNT_BracedThen,   // THEN clause with braces
        kNT_UnbracedThen, // THEN clause without braces (i.e, it's a single simple statement)
        kNT_BracedElse,   // ELSE/inner FOR/WHILE clause with braces
        kNT_UnbracedElse, // ELSE/inner FOR/WHILE clause without braces
        kNT_BracedDo,     // DO clause with braces
        kNT_UnbracedDo,   // DO clause without braces 
        kNT_For,          // Outer FOR clause
        kNT_Switch,       // SWITCH clause
        kNT_Struct,       // Struct defn
    };

    NestingStack();

    // Depth of the nesting == index of the innermost nesting level
    inline size_t Depth() const { return _stack.size(); };

    // Type of the innermost nesting
    inline NestingType Type() { return static_cast<NestingType>(_stack.back().Type); };
    inline void SetType(NestingType nt) { _stack.back().Type = nt; };
    // Type of the nesting at the given nesting level
    inline NestingType Type(size_t level) { return static_cast<NestingType>(_stack.at(level).Type); };

    // If the innermost nesting is a loop that has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline AGS::CodeLoc StartLoc() { return _stack.back().StartLoc; };
    inline void SetStartLoc(AGS::CodeLoc start) { _stack.back().StartLoc = start; };
    // If the nesting at the given level has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline AGS::CodeLoc StartLoc(size_t level) { return _stack.at(level).StartLoc; };

    // If the innermost nesting features a jump out instruction, 
    // then this is the location of the bytecode symbol that says where to jump
    inline std::intptr_t JumpOutLoc() { return _stack.back().Info; };
    inline void SetJumpOutLoc(std::intptr_t loc) { _stack.back().Info = loc; };
    // If the nesting at the given level features a jump out, then this is the location of it
    inline AGS::CodeLoc JumpOutLoc(size_t level) { return _stack.at(level).Info; };

    // If the innermost nesting is a SWITCH, the type of the switch expression
    int SwitchExprType() { return static_cast<int>(_stack.back().Info); };
    inline void SetSwitchExprType(int ty) { _stack.back().Info = ty; };

    // If the innermost nesting is a SWITCH, the location of the "default:" label
    inline AGS::CodeLoc DefaultLabelLoc() { return _stack.back().DefaultLabelLoc; };
    inline void SetDefaultLabelLoc(AGS::CodeLoc loc) { _stack.back().DefaultLabelLoc = loc; }

    // If the innermost nesting contains code chunks that must be moved around
    // (e.g., in FOR loops), then this is true, else false
    inline bool ChunksExist() { return !_stack.back().Chunks.empty(); }
    inline bool ChunksExist(size_t level) { return !_stack.at(level).Chunks.empty(); }

    // Code chunks that must be moved around (e.g., in FOR, DO loops)
    inline std::vector<Chunk> Chunks() { return _stack.back().Chunks; };
    inline std::vector<Chunk> Chunks(size_t level) { return _stack.at(level).Chunks; };

    // True iff the innermost nesting is unbraced
    inline bool IsUnbraced()
    {
        NestingType nt = Type();
        return (nt == kNT_UnbracedThen) || (nt == kNT_UnbracedElse) || (nt == kNT_UnbracedDo);
    }

    // Push a new nesting level (returns a  value < 0 on error)
    int Push(NestingType type, AGS::CodeLoc start, AGS::CodeLoc info);
    inline int Push(NestingType type) { return Push(type, 0, 0); };

    // Pop a nesting level
    inline void Pop() { _stack.pop_back(); };

    // Rip a generated chunk of code out of the codebase and stash it away for later 
    // Returns the unique ID of this code in id
    void YankChunk(::ccCompiledScript &scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset, int &id);


    // Write chunk of code back into the codebase that has been stashed in level given, at index
    void WriteChunk(::ccCompiledScript &scrip, size_t level, size_t index, int &id);
    // Write chunk of code back into the codebase stashed in the innermost level, at index
    inline void WriteChunk(::ccCompiledScript &scrip, size_t index, int &id) { WriteChunk(scrip, Depth() - 1, index, id); };
};

class FuncCallpointMgr
{
private:
    int const CodeBaseId = 0;  // Magic number, means: This is in codebase, not in a yanked piece of code
    int const PatchedId = -1;  // Magic number, means: This is in codebase and has already been patched in

    SymbolTable &_sym;
    ::ccCompiledScript &_scrip;

    struct PatchInfo
    {
        int ChunkId;
        AGS::CodeLoc Offset;
    };
    typedef std::vector<PatchInfo> PatchList;

    struct CallpointInfo
    {
        CodeLoc Callpoint;
        PatchList List;
        CallpointInfo();
    };

    typedef std::map<Symbol, CallpointInfo> CallMap;
    CallMap _funcCallpointMap;

public:
    FuncCallpointMgr(::ccCompiledScript &scrip);
    void Reset();

    // Enter a code location where a function is called that hasn't been defined yet.
    int TrackForwardDeclFuncCall(Symbol func, CodeLoc idx);

    // Enter a code location to jump to in order to end a function
    int TrackExitJumppoint(Symbol func, CodeLoc idx);

    // When code is ripped out of the codebase: 
    // Update list of calls to forward declared functions 
    int UpdateCallListOnYanking(CodeLoc start, size_t len, int id);

    // When code is inserted into the codebase:
    // Update list of calls to forward declared functions
    int UpdateCallListOnWriting(CodeLoc start, int id);

    // Set the callpoint for a function. 
    // Patch all the function calls of the given function to point to dest
    int SetFuncCallpoint(Symbol func, AGS::CodeLoc dest);

    // Set the exit point of a function.
    // Patch this address into all the jumps to exit points.
    int SetFuncExitJumppoint(Symbol func, AGS::CodeLoc dest);

    inline int HasFuncCallpoint(Symbol func) { return (_funcCallpointMap[func].Callpoint >= 0); }

    inline bool IsForwardDecl(Symbol func) { return (0 == _funcCallpointMap.count(func)); }

    // Gives an error message and returns a value < 0 iff there are still callpoints
    // without a location
    int CheckForUnresolvedFuncs();
};

typedef long TypeQualifierSet;

class ImportMgr
{
private:
    std::map<std::string, size_t> _importIdx;
    ccCompiledScript *_scrip;

public:
    ImportMgr();

    void Init(::ccCompiledScript *scrip);

    int FindOrAdd(std::string s);
};

// We set the MAR register lazily to save on runtime computation. This object
// encapsulates the stashed operations that haven't been done on MAR yet.
class MemoryLocation
{
private:
    SymbolType _Type; // kSYM_GlobalVar, kSYM_Import, kSYM_LocalVar, kSYM_NoType (determines what fixup to use)
    size_t _StartOffs;
    size_t _ComponentOffs;

public:
    MemoryLocation()
        : _Type(kSYM_NoType)
        , _StartOffs(0)
        , _ComponentOffs(0) {};

    // Set the type and the offset of the MAR register
    void SetStart(SymbolType type, size_t offset);

    // Add an offset
    inline void AddComponentOffset(size_t offset) { _ComponentOffs += offset; };

    // Write out the opcodes necessary to bring MAR up-to-date
    void MakeMARCurrent(ccCompiledScript &scrip);

    inline bool NothingDoneYet() const { return _Type != kSYM_NoType; };

    inline void Reset() { SetStart(kSYM_NoType, 0); };
};

class Parser
{
public:
    enum ParsingPhase
    {
        kPP_PreAnalyze = 0, // A pre-phase that finds out, amongst others, what functions have (local) bodies
        kPP_Main,           // The main phase that generates the bytecode.
    };

    enum FunctionType
    {
        kFT_PureForward = 0,
        kFT_Import = 1,
        kFT_LocalBody = 2,
    };

    // This indicates where a value is stored.
    // When reading, we need the value itself (but see below for arrays and structs)
    // - It can be in AX (kVL_ax_is_value)
    // - or in m(MAR) (kVL_mar_pointsto_value).
    // When writing, we can't use a value itself. Instead, we need a pointer
    // to the adress that has to be modified.
    // - This can be MAR, i.e., the value to modify is in m(MAR) (kVL_mar_pointsto_value).
    // - or AX, i.e., the value to modify is in m(AX) (kVL_ax_is_value)
    // - attributes must be modified by calling their setter function (kVL_attribute)
    enum ValueLocation
    {
        kVL_ax_is_value,         // The value is in register AX
        kVL_mar_pointsto_value,  // The value is in m(MAR)
        kVL_attribute            // The value must be modified by calling an attribute setter
    };

    // [fw] This ought to replace the #defines in script_common.h
    //      but we can't touch them since the engine uses them, too
    enum FxFixupType : AGS::FixupType // see script_common.h
    {
        kFx_NoFixup = 0,
        kFx_DataData = FIXUP_DATADATA,     // globaldata[fixup] += &globaldata[0]
        kFx_Code = FIXUP_FUNCTION,         // code[fixup] += &code[0]
        kFx_GlobalData = FIXUP_GLOBALDATA, // code[fixup] += &globaldata[0]
        kFx_Import = FIXUP_IMPORT,         // code[fixup] = &imported_thing[code[fixup]]
        kFx_Stack = FIXUP_STACK,           // code[fixup] += &stack[0]
        kFx_String = FIXUP_STRING,         // code[fixup] += &strings[0]

    };

    enum Globalness
    {
        kGl_Local = 0,            // Local
        kGl_GlobalNoImport = 1,   // Global, not imported
        kGl_GlobalImport = 2,     // Global, imported
    };

    enum Importness
    {
        kIm_NoImport = 0,
        kIm_ImportType1 = 1,
        kIm_ImportType2 = 2,
    };

    enum TypeQualifier
    {
        kTQ_Attribute = 1 << 0,
        kTQ_Autoptr = 1 << 1,
        kTQ_Builtin = 1 << 2,
        kTQ_Const = 1 << 3,
        kTQ_ImportStd = 1 << 4,
        kTQ_ImportTry = 1 << 5,
        kTQ_Noloopcheck = 1 << 6,
        kTQ_Managed = 1 << 7,
        kTQ_Protected = 1 << 8,
        kTQ_Readonly = 1 << 9,
        kTQ_Static = 1 << 10,
        kTQ_Stringstruct = 1 << 11,
        kTQ_Writeprotected = 1 << 12,
        kTQ_Import = kTQ_ImportStd | kTQ_ImportTry,
    };

    typedef std::map<AGS::Symbol, bool> TGIVM; // Global Import Variable Mgr

    inline static bool FlagIsSet(AGS::Flags fl_set, long flag) { return 0 != (fl_set & flag); }
    inline static void SetFlag(AGS::Flags &fl_set, long flag, bool val) { if (val) fl_set |= flag; else fl_set &= ~flag; }

private:
    // Measurements show that the checks whether imports already exist take up
    // considerable time. The Import Manager speeds this up by caching the lookups.
    ImportMgr _importMgr;

    // Manage a list of all global import variables and track whether they are
    // re-defined as non-import later on.
    // Symbol maps to TRUE if it is global import, to FALSE if it is global non-import.
    // Only a global import may have a repeated identical definition.
    // Only a global import may be re-defined as a global non-import (identical except for the "import" declarator),
    //    and this may only happen if the options don't forbid this.
    TGIVM _givm; // Global Import Variable Manager

    // Track the phase the parser is in.
    ParsingPhase _pp;

    // Main symbol table
    SymbolTable &_sym;

    // Auxiliary symbol table that is used in the first phase.
    typedef std::map<AGS::Symbol, SymbolTableEntry> TSym1Table;
    TSym1Table _sym1;

    // List of symbols from the tokenizer
    ::ccInternalList &_targ;

    // Receives the parsing results
    ::ccCompiledScript &_scrip;

    // Manage a map of all the functions that have bodies (in the current source).
    FuncCallpointMgr _fcm;

private:
    void DoNullCheckOnStringInAXIfNecessary(Vartype valTypeTo);

    static int String2Int(std::string str, int &val, bool send_error);

    // The higher the MATHEMATICAL priority of an operator, the MORE binding it is.
    // For example, "*" has a higher mathematical priority than "-".
    // In contrast to this, "size" gives the priority in the INVERSE way: 
    // The higher _sym.entries.at(op).ssize is, the LESS binding is the operator op.
    // To convert, we must subtract this value from some suitable value 
    // (any will do that doesn't cause underflow of the subtraction).
    inline int MathPrio(Symbol op) const { return 100 - _sym.entries.at(op).ssize; };

    // If the vartype implies that it must be a dynpointer, then set it to be dynpointer
    Vartype DeduceDynPointerness(Vartype vty);

    bool IsIdentifier(Symbol symb);

    bool IsPrimitiveVartype(Symbol symbl);

    bool Parser::IsAnyTypeOfString(Vartype symtype);

    bool Parser::IsDynpointerVartype(Vartype vartype);

    // Combine the arguments to stname::component, get the symbol for that
    Symbol MangleStructAndComponent(Symbol stname, Symbol component);

    int Parser::SkipTo(const Symbol stoplist[], size_t stoplist_len);

    int SkipToScript0(Symbol *end_sym_ptr, const Symbol stoplist[], size_t stoplist_len, Symbol *&act_sym_ptr);

    // Like SkipTo, but for symbol scripts
    int SkipToScript(const Symbol stoplist[], size_t stoplist_len, SymbolScript &symlist, size_t &symlist_len);

    // Reference to the symbol table that works irrespective of the phase we are in
    inline SymbolTableEntry &GetSymbolTableEntryAnyPhase(Symbol symb) { return (kPP_Main == _pp) ? sym.entries.at(symb) : _sym1[symb]; }

    // Get the type of symb; this will work irrespective of the phase we are in
    inline SymbolType GetSymbolTypeAnyPhase(Symbol symb) { return (symb < 0) ? kSYM_NoType : GetSymbolTableEntryAnyPhase(symb & kVTY_FlagMask).stype; };

    // Return number of bytes to remove from stack to unallocate local vars
    // of level from_level or higher
    int StacksizeOfLocals(size_t from_level);

    // Does vartype v contain releasable pointers?
    // Also determines whether vartype contains standard (non-dynamic) arrays.
    bool ContainsReleasablePointers(Vartype v);

    // We're at the end of a block and releasing a standard array of pointers.
    // MAR points to the array start. Release each array element (pointer).
    int FreePointersOfStdArrayOfPointer(size_t arrsize, bool &clobbers_ax);

    // We're at the end of a block and releasing all the pointers in a struct.
    // MAR already points to the start of the struct.
    void FreePointersOfStruct(Symbol struct_vtype, bool &clobbers_ax);

    // We're at the end of a block and we're releasing a standard array of struct.
    // MAR points to the start of the array. Release all the pointers in the array.
    void FreePointersOfStdArrayOfStruct(Symbol struct_vtype, SymbolTableEntry &entry, bool &clobbers_ax);

    // We're at the end of a block and releasing a standard array. MAR points to the start.
    // Release the pointers that the array contains.
    void FreePointersOfStdArray(SymbolTableEntry &entry, bool &clobbers_ax);

    void FreePointersOfLocals0(int from_level, bool &clobbers_ax, bool &clobbers_mar);

    // Free the pointers of any locals in level from_level or higher
    int FreePointersOfLocals(int from_level, Symbol name_of_current_func = 0, bool ax_irrelevant = false);

    int RemoveLocalsFromSymtable(int from_level);

    int DealWithEndOfElse(NestingStack *nesting_stack, bool &else_after_then);

    int DealWithEndOfDo(NestingStack *nesting_stack);

    int DealWithEndOfSwitch(NestingStack *nesting_stack);

    int ParseLiteralOrConstvalue(Symbol fromSym, int &theValue, bool isNegative, std::string errorMsg);

    // We're parsing a parameter list and we have accepted something like "(...int i"
    // We accept a default value clause like "= 15" if it follows at this point.
    int ParseParamlist_Param_DefaultValue(bool &has_default_int, int &default_int_value);

    // process a dynamic array declaration, when present
    // We have accepted something like "int foo" and we might expect a trailing "[]" here
    // Return values:  0 -- not an array, 1 -- an array, -1 -- error occurred
    int ParseParamlist_Param_DynArrayMarker(Symbol typeSym, bool isPointer);

    // Copy so that the forward decl can be compared afterwards to the real one     
    int CopyKnownSymInfo(SymbolTableEntry &entry, SymbolTableEntry &known_info);

    // Extender function, eg. function GoAway(this Character *someone)
    // We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
    // We'll accept something like "this Character *"
    int ParseFuncdecl_ExtenderPreparations(bool is_static_extender, Symbol &name_of_func, Symbol &struct_of_func);

    int ParseParamlist_ParamType(Symbol param_vartype, bool &param_is_ptr);

    // We're accepting a parameter list. We've accepted something like "int".
    // We accept a param name such as "i" if present
    int ParseParamlist_Param_Name(bool body_follows, Symbol &param_name);

    void ParseParamlist_Param_AsVar2Sym(Symbol param_name, Vartype param_type, bool param_is_ptr, bool param_is_const, bool param_is_dynarray, int param_idx);

    void ParseParamlist_Param_Add2Func(Symbol name_of_func, int param_idx, Symbol param_type, bool param_is_ptr, bool param_is_const, bool param_is_dynarray, bool param_has_int_default, int param_int_default);

    // process a parameter decl in a function parameter list, something like int foo(INT BAR
    int ParseParamlist_Param(Symbol name_of_func, bool body_follows, Symbol param_type, bool param_is_const, int param_idx);

    int ParseFuncdecl_Paramlist(Symbol funcsym, bool body_follows, int &numparams);

    void ParseFuncdecl_SetFunctype(SymbolTableEntry &entry, int return_type, bool func_returns_dynpointer, bool func_returns_dynarray, bool func_is_static, bool func_is_protected, int numparams);

    int ParseFuncdecl_CheckThatFDM_CheckDefaults(SymbolTableEntry *this_entry, bool body_follows, SymbolTableEntry *known_info);

    // there was a forward declaration -- check that the real declaration matches it
    int ParseFuncdecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, bool body_follows, SymbolTableEntry *known_info);

    // Enter the function in the imports[] or functions[] array; get its index   
    int ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, CodeLoc &function_soffs, int &function_idx);

    // We're at something like "int foo(", directly before the "("
    // Get the symbol after the corresponding ")"
    int ParseFuncdecl_GetSymbolAfterParmlist(Symbol &symbol);

    // We're at something like "int foo(", directly before the "("
    // This might or might not be within a struct defn
    int ParseFuncdecl(Symbol &name_of_func, int return_type, bool func_returns_dynpointer, bool func_returns_dynarray, TypeQualifierSet tqs, Symbol &struct_of_func, bool &body_follows);

    // return the index of the lowest MATHEMATICAL priority operator in the list,
    // so that either side of it can be evaluated first.
    // returns -1 if no operator was found
    int IndexOfLowestBondingOperator(SymbolScript slist, size_t slist_len);

    bool IsOldstring(Vartype vartype);

    // Change the generic operator vcpuOp to the one that is correct for the vartypes
    // Also check whether the operator can handle the types at all
    int GetOperatorValidForVartype(Vartype type1, Vartype type2, CodeCell &vcpuOp);

    // Check for a type mismatch in one direction only
    bool IsVartypeMismatch_Oneway(Vartype vartype_is, Vartype vartype_wants_to_be);

    // Check whether there is a type mismatch; if so, give an error
    int IsVartypeMismatch(Vartype vartype_is, Vartype vartype_wants_to_be, bool orderMatters);

    // returns whether this operator's val type is always bool
    static bool IsBooleanVCPUOperator(int scmdtype);

    // If we need a StringStruct but AX contains a string, 
    // then convert AX into a String object and set its type accordingly
    void ConvertAXStringToStringObject(Vartype vartype_wanted);

    int GetReadCommandForSize(int the_size);

    int GetWriteCommandForSize(int the_size);

    int ParseExpression_NewIsFirst(const SymbolScript &symlist, size_t symlist_len);

    // We're parsing an expression that starts with '-' (unary minus)
    int ParseExpression_UnaryMinusIsFirst(const SymbolScript &symlist, size_t symlist_len);

    // We're parsing an expression that starts with '!' (boolean NOT)
    int ParseExpression_NotIsFirst(const SymbolScript & symlist, size_t symlist_len);

    // The lowest-binding operator is the first thing in the expression
    // This means that the op must be an unary op.
    int ParseExpression_OpIsFirst(const SymbolScript &symlist, size_t symlist_len);

    // The lowest-binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
    int ParseExpression_OpIsSecondOrLater(size_t op_idx, const SymbolScript &symlist, size_t symlist_len);

    int ParseExpression_OpenParenthesis(SymbolScript & symlist, size_t symlist_len);

    // We're in the parameter list of a function call, and we have less parameters than declared.
    // Provide defaults for the missing values
    int AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    int AccessData_FunctionCall_PushParams(const SymbolScript &paramList, size_t closedParenIdx, size_t num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    // Count parameters, check that all the parameters are non-empty; find closing paren
    int AccessData_FunctionCall_CountAndCheckParm(const SymbolScript &paramList, size_t paramListLen, Symbol funcSymbol, size_t &indexOfCloseParen, size_t &num_supplied_args);

    // We are processing a function call. General the actual function call
    void AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import);

    // We are processing a function call.
    // Get the parameters of the call and push them onto the stack.
    // Return the number of the parameters pushed
    // NOTE: If keep_mar, we must be careful not to clobber the MAR register
    int AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SymbolScript &paramList, size_t paramListLen, bool keep_mar, size_t &actual_num_args);

    int AccessData_FunctionCall(Symbol name_of_func, SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, Vartype &rettype);

    int ParseExpression_NoOps(SymbolScript symlist, size_t symlist_len);

    int ParseExpression_Subexpr(SymbolScript symlist, size_t symlist_len);

    // symlist starts a bracketed expression; parse it
    int AccessData_ArrayIndexIntoAX(SymbolScript symlist, size_t symlist_len);

    // We access a variable or a component of a struct in order to read or write it.
    // This is a simple member of the struct.
    int AccessData_StructMember(Symbol component, bool writing, bool access_via_this, SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, Vartype &vartype);

    // Get the symbol for the get or set function corresponding to the attribute given.
    int ConstructAttributeFuncName(Symbol attribsym, bool writing, bool indexed, Symbol &func);

    // We call the getter or setter of an attribute
    int AccessData_Attribute(bool is_attribute_set_func, SymbolScript &symlist, size_t &symlist_len, Vartype &vartype);

    // Location contains a pointer to another address. Get that address.
    int AccessData_Dereference(ValueLocation &vloc, MemoryLocation &mloc);

    int AccessData_ProcessArrayIndexConstant(Symbol index_symbol, int array_size, size_t element_size, MemoryLocation &mloc);

    // We're processing some struct component or global or local variable.
    // If an array index follows, parse it and shorten symlist accordingly
    int AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, int array_size, SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, MemoryLocation &mloc, Vartype &vartype);

    int AccessData_GlobalOrLocalVar(bool is_global, bool writing, SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, Vartype &vartype);

    int AccessData_Static(SymbolScript &symlist, size_t &symlist_len, MemoryLocation &mloc, Vartype &vartype);

    int AccessData_LitFloat(bool negate, SymbolScript &symlist, size_t &symlist_len, Vartype &vartype);

    int AccessData_LitOrConst(bool negateLiteral, SymbolScript &symlist, size_t &symlist_len, Vartype &vartype);

    int AccessData_Null(bool negate, SymbolScript &symlist, size_t &symlist_len, Vartype &vartype);

    int AccessData_String(bool negate, SymbolScript &symlist, size_t &symlist_len, Vartype &vartype);

    // Negates the value; this clobbers AX and BX
    void AccessData_Negate(ValueLocation vloc);

    // We're getting a variable, literal, constant, func call or the first element
    // of a STRUCT.STRUCT.STRUCT... cascade.
    // This moves symlist in all cases except for the cascade to the end of what is parsed,
    // and in case of a cascade, to the end of the first element of the cascade, i.e.,
    // to the position of the '.'. 
    int AccessData_FirstClause(bool writing, SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, MemoryLocation &mloc, Vartype &vartype, bool &access_via_this, bool &static_access, bool &need_to_negate);

    // We're processing a STRUCT.STRUCT. ... clause.
    // We've already processed some structs, and the type of the last one is vartype.
    // Now we process a component of vartype.
    int AccessData_SubsequentClause(bool writing, bool access_via_this, bool static_access, SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, MemoryLocation &mloc, Vartype &vartype);

    // Find the component of a struct, in the struct or in one of its ancestors
    Symbol AccessData_FindComponent(Vartype strct, Symbol component);

    // We are in a STRUCT.STRUCT.STRUCT... cascade.
    // Check whether we have passed the last dot
    int AccessData_IsClauseLast(SymbolScript symlist, size_t symlist_len, bool &is_last);

    // Access a variable, constant, literal, func call, struct.component.component cascade, etc.
    // Result is in AX or m[MAR], dependent on vloc. Type is in vartype.
    // At end of function, symlist and symlist_len will point to the part of the symbol string
    // that has not been processed yet
    // NOTE: If this selects an attribute for writing, then the corresponding function will
    // _not_ be called and symlist[0] will be the attribute.
    int AccessData(bool writing, bool need_to_negate, SymbolScript &symlist, size_t &symlist_len, ValueLocation &vloc, int &scope, Vartype &vartype);

    // In order to avoid push AX/pop AX, find out common cases that don't clobber AX
    bool AccessData_MayAccessClobberAX(SymbolScript symlist, size_t symlist_len);

    // Insert Bytecode for:
    // Copy at most OLDSTRING_SIZE-1 bytes from m[MAR...] to m[AX...]
    // Stop when encountering a 0
    void AccessData_StrCpy();

    // We are typically in an assignment LHS = RHS; the RHS has already been
    // evaluated, and the result of that evaluation is in AX.
    // Store AX into the memory location that corresponds to LHS, or
    // call the attribute function corresponding to LHS.
    int AccessData_Assign(SymbolScript symlist, size_t symlist_len);

    int ReadDataIntoAX(SymbolScript symlist, size_t symlist_len, bool negate = false);

    // Read the symbols of an expression and buffer them into expr_script
    // At end of routine, the cursor will be positioned in such a way
    // that targ->getnext() will get the symbol after the expression
    int BufferExpression(ccInternalList &expr_script);

    // evaluate the supplied expression, putting the result into AX
    // returns 0 on success or -1 if compile error
    // leaves targ pointing to last token in expression, so do getnext() to get the following ; or whatever
    int ParseExpression();

    // We are parsing the left hand side of a += or similar statement.
    int ParseAssignment_ReadLHSForModification(ccInternalList const *lhs, ValueLocation &vloc, Vartype &lhstype);

    // "var = expression"; lhs is the variable
    int ParseAssignment_Assign(ccInternalList const *lhs);

    // We compile something like "var += expression"
    int ParseAssignment_MAssign(Symbol ass_symbol, ccInternalList const *lhs);

    // "var++" or "var--"
    int ParseAssignment_SAssign(Symbol ass_symbol, ccInternalList const *lhs);

    // We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
    // An assignment symbol is following. Compile the assignment.
    int ParseAssignment(Symbol ass_symbol, ccInternalList const *lhs);

    int ParseVardecl_InitialValAssignment_Float(bool is_neg, void *& initial_val_ptr);

    int ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr);

    int ParseVardecl_InitialValAssignment_Inttype(bool is_neg, void *&initial_val_ptr);

    // if initial_value is non-null, it returns malloc'd memory that must be free
    int ParseVardecl_InitialValAssignment(Symbol varname, void *&initial_val_ptr);

    // Move variable information into the symbol table
    void ParseVardecl_Var2SymTable(int var_name, Globalness is_global, bool is_dynpointer, int size_of_defn, Vartype vartype);

    // we have accepted something like "int a" and we're expecting "["
    int ParseVardecl_Array(Symbol var_name, Vartype vartype, size_t &size_of_defn);

    int ParseVardecl_CheckIllegalCombis(Vartype vartype, bool is_dynpointer, Globalness is_global);

    // there was a forward declaration -- check that the real declaration matches it
    int ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info);

    int ParseVardecl_GlobalImport(Symbol var_name, bool has_initial_assignment);

    int ParseVardecl_GlobalNoImport(Symbol var_name, const Vartype vartype, size_t size_of_defn, bool has_initial_assignment, void *&initial_val_ptr);

    int ParseVardecl_Local(Symbol var_name, size_t size_of_defn, bool has_initial_assignment);

    int ParseVardecl0(Symbol var_name, Vartype vartype, SymbolType next_type, Globalness globalness, bool is_dynpointer, bool &another_var_follows);

    int ParseVardecl(Symbol var_name, Vartype vartype, SymbolType next_type, Globalness is_global, bool is_dynpointer, bool &another_var_follows);

    void ParseOpenbrace_FuncBody(Symbol name_of_func, int struct_of_func, bool is_noloopcheck, NestingStack *nesting_stack);

    int ParseOpenbrace(NestingStack *nesting_stack, Symbol name_of_current_func, Symbol struct_of_current_func, bool is_noloopcheck);

    int ParseClosebrace(NestingStack *nesting_stack, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    void ParseStruct_SetTypeInSymboltable(Symbol stname, TypeQualifierSet tqs);

    // We're processing the extends clause of a struct. Copy over all the parent elements
    // except for functions and attributes into the current struct.
    int ParseStruct_Extends_CopyParentComponents(Symbol parent, Symbol stname);

    // We have accepted something like "struct foo" and are waiting for "extends"
    int ParseStruct_ExtendsClause(Symbol stname, Symbol &parent, size_t &size_so_far);

    void ParseStruct_MemberQualifiers(TypeQualifierSet &tqs);

    int ParseStruct_CheckComponentVartype(int stname, Symbol vartype, bool member_is_import);

    // check that we haven't extended a struct that already contains a member with the same name
    int ParseStruct_CheckForCompoInAncester(Symbol orig, Symbol compo, Symbol act_struct);

    int ParseStruct_Function(TypeQualifierSet tqs, Vartype curtype, Symbol stname, Symbol vname, Symbol name_of_current_func, bool type_is_dynpointer, bool isDynamicArray);

    int ParseStruct_CheckAttributeFunc(SymbolTableEntry &entry, bool is_setter, bool is_indexed, Vartype vartype);

    int ParseStruct_EnterAttributeFunc(SymbolTableEntry &entry, bool is_setter, bool is_indexed, Vartype vartype);

    // We are processing an attribute.
    // This corresponds to a getter func and a setter func, declare one of them
    int ParseStruct_DeclareAttributeFunc(Symbol func, bool is_setter, bool is_indexed, Vartype vartype);

    // We're in a struct declaration, parsing a struct attribute
    int ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname, Symbol vname);

    // We're inside a struct decl, parsing an array var.
    int ParseStruct_Array(Symbol stname, Symbol vname, size_t &size_so_far);

    // We're inside a struct decl, processing a member variable
    int ParseStruct_VariableOrAttribute(TypeQualifierSet tqs, Vartype curtype, bool type_is_dynpointer, Symbol stname, Symbol vname, size_t &size_so_far);

    // We have accepted something like "struct foo extends bar { const int".
    // We're waiting for the name of the member.
    int ParseStruct_MemberDefnVarOrFuncOrArray(Symbol parent, Symbol stname, Symbol current_func, TypeQualifierSet tqs, Vartype curtype, bool type_is_dynpointer, size_t &size_so_far);

    int ParseStruct_MemberStmt(Symbol stname, Symbol name_of_current_func, Symbol parent, size_t &size_so_far);

    // Handle a "struct" definition clause
    int ParseStruct(TypeQualifierSet tqs, NestingStack &nesting_stack, Symbol name_of_current_func, Symbol struct_of_current_func);

    // We've accepted something like "enum foo { bar"; '=' follows
    int ParseEnum_AssignedValue(int &currentValue);

    void ParseEnum_Item2Symtable(Symbol enum_name, Symbol item_name, int currentValue);

    int ParseEnum_Name2Symtable(Symbol enumName);

    // enum EnumName { value1, value2 }
    int ParseEnum0();

    // enum eEnumName { value1, value2 };
    int ParseEnum(Symbol name_of_current_function);

    int ParseExport();

    int ParseVartype_GetVarName(Symbol &varname, Symbol &struct_of_member_fct);

    int ParseVartype_CheckForIllegalContext(NestingStack *nesting_stack);

    int ParseVartype_CheckIllegalCombis(bool is_function, bool is_member_definition, TypeQualifierSet tqs);

    int ParseVartype_FuncDef(Symbol &func_name, Symbol vartype, bool isPointer, bool isDynamicArray, TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    int ParseVartype_VarDecl(Symbol &var_name, Globalness is_global, int nested_level, bool is_readonly, Symbol vartype, SymbolType next_type, bool isPointer, bool &another_var_follows);

    // We accepted a variable type such as "int", so what follows is a function or variable definition
    int ParseVartype0(Symbol vartype, NestingStack *nesting_stack, TypeQualifierSet tqs, Symbol &name_of_current_func, Symbol &struct_of_current_func, bool &noloopcheck_is_set);

    int ParseCommand_EndOfDoIfElse(NestingStack *nesting_stack);

    int ParseReturn(Symbol name_of_current_func);

    // Evaluate the head of an "if" clause, e.g. "if (i < 0)".
    int ParseIf(Symbol cursym, NestingStack *nesting_stack);

    // Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
    int ParseWhile(Symbol cursym, NestingStack *nesting_stack);

    int ParseDo(NestingStack *nesting_stack);

    // We're compiling function body code; the code does not start with a keyword or type.
    // Thus, we should be at the start of an assignment or a funccall. Compile it.
    int ParseAssignmentOrFunccall(Symbol cursym);

    int ParseFor_InitClauseVardecl(size_t nested_level);

    // The first clause of a for header
    int ParseFor_InitClause(Symbol peeksym, size_t nested_level);

    int ParseFor_WhileClause();

    int ParseFor_IterateClause();

    int ParseFor(Symbol &cursym, NestingStack *nesting_stack);

    int ParseSwitch(NestingStack *nesting_stack);

    int ParseSwitchLabel(Symbol cursym, NestingStack *nesting_stack);

    int ParseBreak(NestingStack *nesting_stack);

    int ParseContinue(NestingStack *nesting_stack);

    int ParseCommand(Symbol cursym, Symbol &name_of_current_func, Symbol &struct_of_current_func, NestingStack *nesting_stack, bool next_is_noloopcheck);

    int Parse_HandleLines(int &currentlinewas);

    int Parse_TQCombiError(TypeQualifierSet tqs);

    // Check whether the qualifiers that accumulated for this decl go together
    int Parse_CheckTQ(TypeQualifierSet tqs, Symbol decl_type);

    int ParseVartype(Symbol cursym, TypeQualifierSet tqs, NestingStack &nesting_stack, Symbol &name_of_current_func, Symbol &struct_of_current_func, bool &set_nlc_flag);

    void Parse_SkipToEndingBrace();

    void Parse_StartNewSection(Symbol mangled_section_name);

    int ParseInput();

    // Copy all the func headers from the PreAnalyse phase into the "real" symbol table
    int Parse_FuncHeaders2Sym();


public:
    // interpret the float as if it were an int (without converting it really);
    // return that int
    // [fw] This should be moved somewhere. It isn't Parser functionality
    static int InterpretFloatAsInt(float floatval);

    Parser(::SymbolTable &sym_t, ::ccInternalList &targ, ::ccCompiledScript &scrip);

    int Parse();

}; // class Parser
} // namespace AGS


extern int cc_tokenize(
    const char *inpl,           // preprocessed text to be tokenized
    ccInternalList *targ,      // store for the tokenized text
    ccCompiledScript *scrip,    // repository for the strings in the text
    SymbolTable &sym_t);        // symbol table 

extern int cc_compile(
    const char *inpl,           // preprocessed text to be compiled
    ccCompiledScript *scrip);   // store for the compiled text

#endif // __CS_PARSER_H
