/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - INTERFACE AND HIGH-LEVEL STRUCTURE

The processing is done in the following layers:
* [Preprocessing - This has been done separately before the input arrives here.]
    Expand macros, delete comments

* Scanning
    Read the characters of the input and partition it in symbols (e.g., identifier, number literal).
    Enter all the symbols into a symbol table (thus recognizing keywords)
    Enter all literal strings into a strings table
These two steps are performed separately _before_ the Parsing (below) begins.
The result is:
    the symbol table SymbolTable.
    the sequence of tokens SrcList
    the collected string literals that go into a struct ccCompiledScript.

* Parsing
    All the high-level logic.
    The parser augments the symbol table _sym as it goes along.
    The result is in a parameter scrip that is a struct ccCompiledScript *
    and has the following key components (amongst many other components):
        functions - all the functions defined (i.e. they have a body) in the current inputstring
        imports - all the functions declared (i.e. without body) in the current inputstring
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
#include <cstdarg>

#include "cc_compiledscript.h"
#include "cc_internallist.h"
#include "cc_symboltable.h"
#include "script/script_common.h"

namespace AGS
{

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
    // When reading, we need the value itself.
    // - It can be in AX (kVL_ax_is_value)
    // - or in m(MAR) (kVL_mar_pointsto_value).
    // When writing, we need a pointer to the adress that has to be modified.
    // - This can be MAR, i.e., the value to modify is in m(MAR) (kVL_mar_pointsto_value).
    // - or AX, i.e., the value to modify is in m(AX) (kVL_ax_is_value)
    // - attributes must be modified by calling their setter function (kVL_attribute)
    enum ValueLocation
    {
        kVL_ax_is_value,         // The value is in register AX
        kVL_mar_pointsto_value,  // The value is in m(MAR)
        kVL_attribute            // The value must be modified by calling an attribute setter
    };

    // This ought to replace the #defines in script_common.h
    // but we can't touch them since the engine uses them, too
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
        kGl_Local = 0,
        kGl_GlobalNoImport = 1,
        kGl_GlobalImport = 2,
    };

    typedef std::map<AGS::Symbol, bool> TGIVM; // Global Import Variable Mgr

    // Needs to be public because the manager is initialized outside of Parser
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
        FuncCallpointMgr(::SymbolTable &symt, ::ccCompiledScript &scrip);
        void Reset();

        // Enter a code location where a function is called that hasn't been defined yet.
        ErrorType TrackForwardDeclFuncCall(Symbol func, CodeLoc idx);

        // When code is ripped out of the codebase: 
        // Update list of calls to forward declared functions 
        ErrorType UpdateCallListOnYanking(CodeLoc start, size_t len, int id);

        // When code is inserted into the codebase:
        // Update list of calls to forward declared functions
        ErrorType UpdateCallListOnWriting(CodeLoc start, int id);

        // Set the callpoint for a function. 
        // Patch all the function calls of the given function to point to dest
        ErrorType SetFuncCallpoint(Symbol func, CodeLoc dest);

        inline bool HasFuncCallpoint(Symbol func) { return (_funcCallpointMap[func].Callpoint >= 0); }

        inline bool IsForwardDecl(Symbol func) { return (0 == _funcCallpointMap.count(func)); }

        // Give an error message and return a value < 0 iff there are still callpoints
        // without a location
        ErrorType CheckForUnresolvedFuncs();
    };

    struct Warning
    {
        size_t Pos; // in scanned source
        std::string Message;
    };

private:

    // Remember a code generation point.
    // If at some later time, Restore() is called,
    // then all bytecode that has been generated in the meantime is discarded.
    // Currently only undoes the bytecode, not the fixups.
    class RestorePoint
    {
    private:
        ::ccCompiledScript &_scrip;
        CodeLoc _restoreLoc;
        size_t  _lastEmittedSrcLineno;
    public:
        RestorePoint(::ccCompiledScript &scrip);
        void Restore();
        inline bool IsEmpty() { return _scrip.codesize == _restoreLoc; }
    };

    // Remember a point of the bytecode that is going to be the destination
    // of a backward jump. When at some later time, WriteJump() is called,
    // then the appropriate instruction(s) for a backward jump are generated.
    // This may entail a SCMD_LINENUM command.
    // This may make the last emitted src line invalid.
    class BackwardJumpDest
    {
    private:
        ::ccCompiledScript &_scrip;
        CodeLoc _dest;
        size_t  _lastEmittedSrcLineno;
    public:
        BackwardJumpDest(::ccCompiledScript &scrip);
        // Set the destination to the location given; default to current location in code
        void Set(CodeLoc cl = -1);
        inline CodeLoc Get() const { return _dest; }
        // Write a jump to the code location that I represent
        void WriteJump(CodeCell jump_op, size_t cur_line);
    };

    // A storage for parameters of forward jumps. When at some later time,
    // Patch() is called, then all the jumps will be patched to point to the current
    // point in code; if appropriate, the last emitted src line will be invalidated.
    class ForwardJump
    {
    private:
        ::ccCompiledScript &_scrip;
        std::vector<CodeLoc> _jumpDestParamLocs;
        size_t  _lastEmittedSrcLineno;

    public:
        ForwardJump(::ccCompiledScript &scrip);
        // Add the parameter of a forward jump 
        void AddParam(int offset = -1);
        // Patch all the forward jump parameters
        void Patch(size_t cur_line);
    };

    // The stack of nesting compound statements 
    class NestingStack
    {
    private:
        static int _chunkIdCtr; // for assigning unique IDs to chunks

        // A section of compiled code that needs to be moved or copied to a new location
        struct Chunk
        {
            std::vector<CodeCell> Code;
            std::vector<CodeLoc> Fixups;
            std::vector<char> FixupTypes;
            int CodeOffset;
            int FixupOffset;
            size_t SrcLine;
            int Id;
        };

        // All data that is associated with a nesting level of compound statements
        struct NestingInfo
        {
            SymbolType Type; // Type of the compound statement of this level
            BackwardJumpDest Start;
            ForwardJump JumpOut; // First byte after the statement
            Vartype SwitchExprVartype; // when switch: the Vartype of the switch expression
            BackwardJumpDest SwitchDefault; // when switch: Default label
            ForwardJump SwitchJumptable; // when switch: Jumptable
            std::vector<Chunk> Chunks; // Bytecode chunks that must be moved (FOR loops and SWITCH)
        };

        std::vector<NestingInfo> _stack;
        ::ccCompiledScript &_scrip;

    public:
        NestingStack(::ccCompiledScript &scrip);

        // Depth of the nesting == index of the innermost nesting level
        inline size_t Depth() const { return _stack.size(); };

        // Type of the innermost nesting
        inline SymbolType Type() const { return _stack.back().Type; }
        inline void SetType(SymbolType nt) { _stack.back().Type = nt; }
        // Type of the nesting at the given nesting level
        inline SymbolType Type(size_t level) const { return _stack.at(level).Type; }

        // If the innermost nesting is a loop that has a jump back to the start,
        // then this gives the location to jump to
        inline BackwardJumpDest &Start() { return _stack.back().Start; }
        // If the nesting at the given level has a jump back to the start,
        // then this gives the location to jump to
        inline BackwardJumpDest &Start(size_t level) { return _stack.at(level).Start; }

        // Collects the jumps to the Bytecode byte after the construct
        inline ForwardJump &JumpOut() { return _stack.back().JumpOut; }
        // Collects the jumps to the Bytecode byte after the construct
        inline ForwardJump &JumpOut(size_t level) { return _stack.at(level).JumpOut; }

        // If the innermost nesting is a SWITCH, the type of the switch expression
        inline Vartype SwitchExprVartype() { return _stack.back().SwitchExprVartype; };
        inline void SetSwitchExprVartype(Vartype vt) { _stack.back().SwitchExprVartype = vt; }

        // If the innermost nesting is a SWITCH, the location of the "default:" code
        inline BackwardJumpDest &SwitchDefault() { return _stack.back().SwitchDefault; }
        // If the nesting at the given level is a SWITCH, the location of the "default:" code
        inline BackwardJumpDest &SwitchDefault(size_t level) { return _stack.at(level).SwitchDefault; }

        // If the innermost nesting is a SWITCH, the location of the jump table
        inline ForwardJump &SwitchJumptable() { return _stack.back().SwitchJumptable; }
        // If the nesting at the given level is a SWITCH, the location of the  jump table
        inline ForwardJump &SwitchJumptable(size_t level) { return _stack.at(level).SwitchJumptable; }

        // If the innermost nesting contains code chunks that must be moved around
        // (e.g., in FOR loops), then this is true, else false
        inline bool ChunksExist() { return !_stack.back().Chunks.empty(); }
        inline bool ChunksExist(size_t level) { return !_stack.at(level).Chunks.empty(); }

        // Code chunks that must be moved around (e.g., in FOR, DO loops)
        inline std::vector<Chunk> Chunks() { return _stack.back().Chunks; };
        inline std::vector<Chunk> Chunks(size_t level) { return _stack.at(level).Chunks; };

        // Push a new nesting level
        ErrorType Push(SymbolType type);

        // Pop a nesting level
        inline void Pop() { _stack.pop_back(); };

        // Rip a generated chunk of code out of the codebase and stash it away for later 
        // Returns the unique ID of this code in id
        void YankChunk(size_t src_line, CodeLoc codeoffset, CodeLoc fixupoffset, int &id);

        // Write chunk of code back into the codebase that has been stashed in level given, at index
        void WriteChunk(size_t level, size_t index, int &id);
        // Write chunk of code back into the codebase stashed in the innermost level, at index
        inline void WriteChunk(size_t index, int &id) { WriteChunk(Depth() - 1, index, id); };
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

        // Write out the Bytecode necessary to bring MAR up-to-date
        void MakeMARCurrent(size_t lineno, ccCompiledScript &scrip);

        inline bool NothingDoneYet() const { return _Type != kSYM_NoType; };

        inline void Reset() { SetStart(kSYM_NoType, 0); };
    };

    // Measurements show that the checks whether imports already exist take up
    // considerable time. The Import Manager speeds this up by caching the lookups.
    class ImportMgr
    {
    private:
        std::map<std::string, size_t> _importIdx;
        ccCompiledScript *_scrip;

    public:
        ImportMgr();

        void Init(::ccCompiledScript *scrip);

        // Whether s is in the import table already (doesn't add)
        bool IsDeclaredImport(std::string s);

        // Finds s in the import table; adds it if not found;
        // returns the index of s in the table.
        int FindOrAdd(std::string s);
    } _importMgr;

    // Manage a list of all global import variables and track whether they are
    // re-defined as non-import later on.
    // Symbol maps to TRUE if it is global import, to FALSE if it is global non-import.
    // Only a global import may have a repeated identical definition.
    // Only a global import may be re-defined as a global non-import
    //    (that must be identical except for the "import" declarator),
    //    and this may only happen if the options don't forbid this.
    TGIVM _givm; // Global Import Variable Manager

    // Track the phase the parser is in.
    ParsingPhase _pp;

    // Main symbol table
    SymbolTable &_sym;

    // List of symbols from the tokenizer
    SrcList &_src;

    // Receives the parsing results
    ::ccCompiledScript &_scrip;

    // Receives the warnings
    std::vector<Warning> _warnings;

    // Manage a map of all the functions that have bodies (in the current source).
    FuncCallpointMgr _fcm;

    // Manage a map of all imported functions where the import decl comes after the function
    // The "Callpoint" of such a function is the index in the import table.
    FuncCallpointMgr _fim; // i for import

    size_t _lastEmittedSectionId;
    size_t _lastEmittedLineno;

    // Map type qualifiers to strings so that they can be output
    static std::map<TypeQualifier, std::string> _tq2String;

    // Buffer for ccCurScriptName
    std::string _scriptNameBuffer;

    void DoNullCheckOnStringInAXIfNecessary(Vartype valTypeTo);

    // Augment the message with a "See ..." indication
    std::string ReferenceMsg(std::string const &msg, int section_id, int line);
    // Augment the message with a "See ..." indication pointing to the declaration of sym
    std::string ReferenceMsgSym(std::string const &msg, AGS::Symbol sym);

    // These two need to be non-static because they can yield errors,
    // and errors need the parser object's line number information.
    ErrorType String2Int(std::string const &str, int &val);
    ErrorType String2Float(std::string const &str, float &val);

    bool IsIdentifier(Symbol symb);

    inline static Symbol Vartype2Symbol(Vartype vartype) { return static_cast<Symbol>(vartype); };

    void SetDynpointerInManagedVartype(Vartype &vartype);

    // Combine the arguments to stname::component, get the symbol for that
    Symbol MangleStructAndComponent(Symbol stname, Symbol component);

    // Skim through source, ignoring delimited content completely. Stop in the following cases:
    // .  A symbol is encountered whose type is in stoplist[]
    // .  A closing symbol is encountered that hasn't been opened.
    // Don't consume the symbol that stops the scan.
    ErrorType SkipTo(SrcList &source, SymbolType const stoplist[], size_t stoplist_len);

    // Mark the symbol as "accessed" in the symbol table
    inline void MarkAcessed(Symbol symb) { SetFlag(_sym[symb].Flags, kSFLG_Accessed, true); }

    // Return number of bytes to remove from stack to unallocate local vars
    // of levels that are above from_level
    int StacksizeOfLocals(size_t from_level);

    // Does vartype v contain releasable pointers?
    // Also determines whether vartype contains standard (non-dynamic) arrays.
    bool ContainsReleasableDynpointers(Vartype v);

    // We're at the end of a block and releasing a standard array of pointers.
    // MAR points to the array start. Release each array element (pointer).
    ErrorType FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements, bool &clobbers_ax);

    // We're at the end of a block and releasing all the pointers in a struct.
    // MAR already points to the start of the struct.
    void FreeDynpointersOfStruct(Symbol struct_vtype, bool &clobbers_ax);

    // We're at the end of a block and we're releasing a standard array of struct.
    // MAR points to the start of the array. Release all the pointers in the array.
    void FreeDynpointersOfStdArrayOfStruct(Symbol struct_vtype, SymbolTableEntry &entry, bool &clobbers_ax);

    // We're at the end of a block and releasing a standard array. MAR points to the start.
    // Release the pointers that the array contains.
    void FreeDynpointersOfStdArray(Symbol the_array, bool &clobbers_ax);

    void FreeDynpointersOfLocals0(int from_level, bool &clobbers_ax, bool &clobbers_mar);

    // Free the pointers of any locals in level from_level or higher
    ErrorType FreeDynpointersOfLocals(int from_level, Symbol name_of_current_func = 0, bool ax_irrelevant = false);

    ErrorType RemoveLocalsFromSymtable(int from_level);

    ErrorType IntLiteralOrConst2Value(Symbol symb, bool is_negative, std::string const &errorMsg, int &the_value);
    ErrorType FloatLiteral2Value(Symbol symb, bool is_negative, std::string const &errorMsg, float &the_value);


    // We're parsing a parameter list and we have accepted something like "(...int i"
    // We accept a default value clause like "= 15" if it follows at this point.
    ErrorType ParseParamlist_Param_DefaultValue(Vartype param_type, SymbolTableEntry::ParamDefault &default_value);

    // process a dynamic array declaration, when present
    // We have accepted something like "int foo" and we might expect a trailing "[]" here
    ErrorType ParseDynArrayMarkerIfPresent(Vartype &vartype);

    // Copy so that the forward decl can be compared afterwards to the real one     
    ErrorType CopyKnownSymInfo(SymbolTableEntry &entry, SymbolTableEntry &known_info);

    // Extender function, eg. function GoAway(this Character *someone)
    // We've just accepted something like "int func("
    // We'll accept something like "this Character *" --OR-- "static Character"
    ErrorType ParseFuncdecl_ExtenderPreparations(bool is_static_extender, Symbol &struct_of_func, Symbol &name_of_func, TypeQualifierSet &tqs);

    // In a parameter list, process the vartype of a parameter declaration
    ErrorType ParseParamlist_ParamType(Vartype &vartype);

    // We're accepting a parameter list. We've accepted something like "int".
    // We accept a param name such as "i" if present
    ErrorType ParseParamlist_Param_Name(bool body_follows, Symbol &param_name);

    void ParseParamlist_Param_AsVar2Sym(Symbol param_name, Vartype param_type, bool param_is_const, int param_idx);

    void ParseParamlist_Param_Add2Func(Symbol name_of_func, int param_idx, Symbol param_type, bool param_is_const, SymbolTableEntry::ParamDefault const &param_default);

    // process a parameter decl in a function parameter list, something like int foo(INT BAR
    ErrorType ParseParamlist_Param(Symbol name_of_func, bool body_follows, Vartype vartype, bool param_is_const, int param_idx);

    ErrorType ParseFuncdecl_Paramlist(Symbol funcsym, bool body_follows);

    void ParseFuncdecl_SetFunctype(Symbol name_of_function, Vartype return_vartype, TypeQualifierSet tqs);

    ErrorType ParseFuncdecl_CheckThatFDM_CheckDefaults(SymbolTableEntry const &this_entry, bool body_follows, SymbolTableEntry const &known_info);

    // there was a forward declaration -- check that the real declaration matches it
    ErrorType ParseFuncdecl_CheckThatKnownInfoMatches(SymbolTableEntry &this_entry, bool body_follows, SymbolTableEntry const &known_info);

    // Enter the function in the imports[] or functions[] array; get its index   
    ErrorType ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, CodeLoc &function_soffs, int &function_idx);

    // We're at something like "int foo(", directly before the "("
    // Get the symbol after the corresponding ")"
    ErrorType ParseFuncdecl_GetSymbolAfterParmlist(Symbol &symbol);

    // We're in a func decl. Check whether it is valid here.
    ErrorType ParseFuncdecl_CheckValidHere(Symbol name_of_func, Vartype return_vartype, bool body_follows);

    ErrorType ParseFuncdecl_HandleFunctionOrImportIndex(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, bool body_follows);

    // We're at something like "int foo(", directly before the "("
    // This might or might not be within a struct defn
    ErrorType ParseFuncdecl(Symbol &name_of_func, Vartype return_vartype, TypeQualifierSet tqs, Symbol &struct_of_func, bool &body_follows);

    // Find the index of the operator in the list that binds the least
    // so that either side of it can be evaluated first. -1 if no operator was found
    ErrorType IndexOfLeastBondingOperator(SrcList &slist, int &idx);

    // Change the generic opcode to the one that is correct for the vartypes
    // Also check whether the operator can handle the types at all
    ErrorType GetOpcodeValidForVartype(Vartype type1, Vartype type2, CodeCell &opcode);

    // Check for a type mismatch in one direction only
    bool IsVartypeMismatch_Oneway(Vartype vartype_is, Vartype vartype_wants_to_be);

    // Check whether there is a type mismatch; if so, give an error
    ErrorType IsVartypeMismatch(Vartype vartype_is, Vartype vartype_wants_to_be, bool orderMatters);

    // Whether this operator's vartype is always bool
    static bool IsBooleanOpcode(CodeCell opcode);

    // If we need a StringStruct but AX contains a string, 
    // then convert AX into a String object and set its type accordingly
    void ConvertAXStringToStringObject(Vartype wanted_vartype);

    static int GetReadCommandForSize(int the_size);

    static int GetWriteCommandForSize(int the_size);

    // Handle the cases where a value is a whole array or dynarray or struct
    ErrorType HandleStructOrArrayResult(AGS::Vartype &vartype, AGS::Parser::ValueLocation &vloc);

    // If the result isn't in AX, move it there. Dereferences a pointer
    ErrorType ResultToAX(ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // We're in the parameter list of a function call, and we have less parameters than declared.
    // Provide defaults for the missing values
    ErrorType AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    ErrorType AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    // Count parameters, check that all the parameters are non-empty; find closing paren
    ErrorType AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters, Symbol funcSymbol, size_t &index_of_close_paren, size_t &num_supplied_args);

    // We are processing a function call. General the actual function call
    void AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import);

    // We are processing a function call.
    // Get the parameters of the call and push them onto the stack.
    // Return the number of the parameters pushed
    ErrorType AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args);

    // Process a function call. The parameter list begins with expression[1].
    ErrorType AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, MemoryLocation &mloc, Vartype &rettype);

    // Check the vartype following "new"
    ErrorType ParseExpression_CheckArgOfNew(Vartype new_vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is unary NEW
    ErrorType ParseExpression_New(SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is unary '-'
    ErrorType ParseExpression_UnaryMinus(SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is a boolean or bitwise negation
    ErrorType ParseExpression_Negate(SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is a unary operator
    ErrorType ParseExpression_Unary(SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. Expression is a ternary a ? b : c
    ErrorType ParseExpression_Ternary(size_t op_idx, SrcList &expression, ValueLocation &vloc, int &scope, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator a binary operator.
    ErrorType ParseExpression_Binary(size_t op_idx, SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is '?' or a binary operator.
    ErrorType ParseExpression_BinaryOrTernary(size_t op_idx, SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. Expression begins with '('
    // Leaves the cursor pointing after the last token that was processed
    ErrorType ParseExpression_InParens(SrcList &expression, ValueLocation &vloc, int &scope, AGS::Vartype &vartype);

    // Parse the term given in EXPRESSION. Expression does not contain operators
    // Leaves the cursor pointing after the last token that was processed
    ErrorType ParseExpression_NoOps(SrcList &expression, ValueLocation &vloc, int &scope, Vartype &vartype);

    // Parse the term given in EXPRESSION.
    // Leaves the cursor pointing after the last token that was processed
    ErrorType ParseExpression_Term(SrcList &expression, ValueLocation &vloc, int &scope, Vartype &vartype);

    // Parse expression in parentheses
    // leaves src pointing after last token in expression, so do getnext() to get the following ; or whatever
    ErrorType ParseParenthesizedExpression();

    // Evaluate the supplied expression, putting the result into AX
    // leaves src pointing to last token in expression, so do getnext() to get the following ; or whatever
    ErrorType ParseExpression();

    ErrorType AccessData_ReadBracketedIntExpression(SrcList &expression);

    ErrorType AccessData_ReadIntExpression(SrcList &expression);

    // We access a variable or a component of a struct in order to read or write it.
    // This is a simple member of the struct.
    ErrorType AccessData_StructMember(Symbol component, bool writing, bool access_via_this, SrcList &expression, MemoryLocation &mloc, Vartype &vartype);

    // Get the symbol for the get or set function corresponding to the attribute given.
    ErrorType ConstructAttributeFuncName(Symbol attribsym, bool writing, bool indexed, Symbol &func);

    // We call the getter or setter of an attribute
    // The next symbol read is the attribute (the part after the '.')
    ErrorType AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, Vartype &vartype);

    // Location contains a pointer to another address. Get that address.
    ErrorType AccessData_Dereference(ValueLocation &vloc, MemoryLocation &mloc);

    ErrorType AccessData_ProcessArrayIndexConstant(Symbol index_symbol, size_t num_array_elements, size_t element_size, MemoryLocation &mloc);

    // Process one index in a sequence of array indexes
    ErrorType AccessData_ProcessCurrentArrayIndex(size_t dim, size_t factor, bool is_dynarray, SrcList &expression, MemoryLocation &mloc);

    // We're processing some struct component or global or local variable.
    // If a sequence of array indexes follows, parse it and shorten symlist accordingly
    ErrorType AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, SrcList &expression, ValueLocation &vloc, MemoryLocation &mloc, Vartype &vartype);

    ErrorType AccessData_GlobalOrLocalVar(bool is_global, bool writing, SrcList &expression, MemoryLocation &mloc, Vartype &vartype);

    ErrorType AccessData_Static(SrcList &expression, MemoryLocation &mloc, Vartype &vartype);

    ErrorType AccessData_FloatLiteral(bool negate, SrcList &expression, Vartype &vartype);

    ErrorType AccessData_IntLiteralOrConst(bool negate, SrcList &expression, Vartype &vartype);

    ErrorType AccessData_Null(SrcList &expression, Vartype &vartype);

    ErrorType AccessData_StringLiteral(SrcList &expression, Vartype &vartype);

    // We're getting a variable, literal, constant, func call or the first element
    // of a STRUCT.STRUCT.STRUCT... cascade.
    // This moves the cursor in all cases except for the cascade to the end of what is parsed,
    // and in case of a cascade, to the end of the first element of the cascade, i.e.,
    // to the position of the '.'. 
    ErrorType AccessData_FirstClause(bool writing, SrcList &expression, ValueLocation &vloc, int &scope, MemoryLocation &mloc, Vartype &vartype, bool &access_via_this, bool &static_access);

    // We're processing a STRUCT.STRUCT. ... clause.
    // We've already processed some structs, and the type of the last one is vartype.
    // Now we process a component of vartype.
    ErrorType AccessData_SubsequentClause(bool writing, bool access_via_this, bool static_access, SrcList &expression, ValueLocation &vloc, int &scope, MemoryLocation &mloc, Vartype &vartype);

    // Find the component of a struct (in the struct or in the ancestors of the struct)
    // and return the struct that the component is defined in
    // Return 0 if such a struct doesn't exist 
    Symbol FindStructOfComponent(Vartype strct, Symbol component);

    // Find the component of a struct (in the struct or in the ancestors of the struct)
    // and return the "real" component name
    Symbol AccessData_FindComponent(Vartype strct, Symbol component);

    // We are in a STRUCT.STRUCT.STRUCT... cascade.
    // Check whether we have passed the last dot
    ErrorType AccessData_IsClauseLast(SrcList &expression, bool &is_last);

    // Access a variable, constant, literal, func call, struct.component.component cascade, etc.
    // Result is in AX or m[MAR], dependent on vloc. Variable type is in vartype.
    // At end of function, symlist and symlist_len will point to the part of the symbol string
    // that has not been processed yet
    // NOTE: If this selects an attribute for writing, then the corresponding function will
    // _not_ be called and symlist[0] will be the attribute.
    ErrorType AccessData(bool writing, SrcList &expression, ValueLocation &vloc, int &scope, Vartype &vartype);

    // In order to avoid push AX/pop AX, find out common cases that don't clobber AX
    bool AccessData_MayAccessClobberAX(SrcList &expression);

    // Insert Bytecode for:
    // Copy at most OLDSTRING_SIZE-1 bytes from m[MAR...] to m[AX...]
    // Stop when encountering a 0
    void AccessData_StrCpy();

    // A "*" is allowed here. If it is here, gobble it.
    ErrorType EatDynpointerSymbolIfPresent(Vartype vartype);

    // We are typically in an assignment LHS = RHS; the RHS has already been
    // evaluated, and the result of that evaluation is in AX.
    // Store AX into the memory location that corresponds to LHS, or
    // call the attribute function corresponding to LHS.
    ErrorType AccessData_AssignTo(SrcList &expression);

    ErrorType SkipToEndOfExpression();

    // We are parsing the left hand side of a += or similar statement.
    ErrorType ParseAssignment_ReadLHSForModification(SrcList &lhs, ValueLocation &vloc, Vartype &lhstype);

    // "var = expression"; lhs is the variable
    ErrorType ParseAssignment_Assign(SrcList &lhs);

    // We compile something like "var += expression"
    ErrorType ParseAssignment_MAssign(Symbol ass_symbol, SrcList &lhs);

    // "var++" or "var--"
    ErrorType ParseAssignment_SAssign(Symbol ass_symbol, SrcList &lhs);

    // We've read a variable or selector of a struct into symlist[], the last identifying component is in cursym.
    // An assignment symbol is following. Compile the assignment.
    ErrorType ParseAssignment(Symbol ass_symbol, SrcList &lhs);

    ErrorType ParseVardecl_InitialValAssignment_Float(bool is_neg, void *&initial_val_ptr);

    ErrorType ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr);

    ErrorType ParseVardecl_InitialValAssignment_Inttype(bool is_neg, void *&initial_val_ptr);

    // if initial_value is non-null, it returns malloc'd memory that must be free
    ErrorType ParseVardecl_InitialValAssignment(Symbol varname, void *&initial_val_ptr);

    // Move variable information into the symbol table
    void ParseVardecl_Var2SymTable(Symbol var_name, Vartype vartype, Globalness globalness);

    // we have accepted something like "int a" and we're expecting "["
    ErrorType ParseArray(Symbol var_name, Vartype &vartype);

    ErrorType ParseVardecl_CheckIllegalCombis(Vartype vartype, Globalness globalness);

    // there was a forward declaration -- check that the real declaration matches it
    ErrorType ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info);

    ErrorType ParseVardecl_GlobalImport(Symbol var_name, bool has_initial_assignment);

    ErrorType ParseVardecl_GlobalNoImport(Symbol var_name, Vartype vartype, bool has_initial_assignment, void *&initial_val_ptr);

    ErrorType ParseVardecl_Local(Symbol var_name, Vartype vartype, bool has_initial_assignment);

    ErrorType ParseVardecl0(Symbol var_name, Vartype vartype, Globalness globalness);

    ErrorType ParseVardecl(Symbol var_name, Vartype vartype, Globalness globalness);

    ErrorType ParseFuncBody(NestingStack *nesting_stack, Symbol struct_of_func, Symbol name_of_func);

    ErrorType HandleEndOfFuncBody(NestingStack *nesting_stack, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    void ParseStruct_SetTypeInSymboltable(Symbol stname, TypeQualifierSet tqs);

    // We have accepted something like "struct foo" and are waiting for "extends"
    ErrorType ParseStruct_ExtendsClause(Symbol stname, size_t &size_so_far);

    void ParseQualifiers(TypeQualifierSet &tqs);

    ErrorType ParseStruct_CheckComponentVartype(Symbol stname, Vartype vartype, bool member_is_import);

    // check that we haven't extended a struct that already contains a member with the same name
    ErrorType ParseStruct_CheckForCompoInAncester(Symbol orig, Symbol compo, Symbol act_struct);

    ErrorType ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype);

    ErrorType ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype);

    ErrorType ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype);

    // We are processing an attribute.
    // This corresponds to a getter func and a setter func, declare one of them
    ErrorType ParseStruct_Attribute_DeclareFunc(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, TypeQualifierSet tqs, Vartype vartype);

    // We're in a struct declaration. Parse an attribute declaration.
    ErrorType ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname, Symbol vname, Vartype vartype);

    // We're inside a struct decl, processing a member variable
    ErrorType ParseStruct_VariableOrAttributeDefn(TypeQualifierSet tqs, Vartype curtype, Symbol stname, Symbol vname, size_t &size_so_far);

    // We have accepted something like "struct foo extends bar { const int".
    // We're waiting for the name of the member.
    ErrorType ParseStruct_MemberDefn(Symbol stname, TypeQualifierSet tqs, Vartype vartype, size_t &size_so_far);

    // We've accepted, e.g., "struct foo {". Now we're parsing a variable declaration or a function declaration
    ErrorType ParseStruct_Vartype(Symbol stname, TypeQualifierSet tqs, Vartype vartype, size_t &size_so_far);

    // Handle a "struct" definition clause
    ErrorType ParseStruct(TypeQualifierSet tqs, NestingStack &nesting_stack, Symbol struct_of_current_func, Symbol name_of_current_func);

    // We've accepted something like "enum foo { bar"; '=' follows
    ErrorType ParseEnum_AssignedValue(int &currentValue);

    void ParseEnum_Item2Symtable(Symbol enum_name, Symbol item_name, int currentValue);

    ErrorType ParseEnum_Name2Symtable(Symbol enumName);

    // We parse enum EnumName { value1, value2 }
    ErrorType ParseEnum0();

    // We parse enum eEnumName { value1, value2 };
    ErrorType ParseEnum(Symbol name_of_current_function);

    ErrorType ParseExport();

    ErrorType ParseReturn(NestingStack *nesting_stack, Symbol name_of_current_func);

    ErrorType ParseVarname(bool accept_member_access, Symbol &structname, Symbol &varname);

    ErrorType ParseVartype_CheckForIllegalContext(NestingStack const &nesting_stack);

    ErrorType ParseVartype_CheckIllegalCombis(bool is_function, TypeQualifierSet tqs);

    ErrorType ParseVartype_FuncDecl(Symbol func_name, TypeQualifierSet tqs, Vartype vartype, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    ErrorType ParseVartype_VarDecl_PreAnalyze(AGS::Symbol var_name, Globalness globalness);

    ErrorType ParseVartype_VarDecl(Symbol var_name, Globalness globalness, int nested_level, TypeQualifierSet tqs, Vartype vartype);

    // We accepted a variable type such as "int", so what follows is a variable or function declaration
    ErrorType ParseVartype(Vartype vartype, NestingStack const &nesting_stack, TypeQualifierSet tqs, Symbol &name_of_current_func, Symbol &struct_of_current_func);

    // After a command statement. This command might be the end of sequences such as
    // "if (...) while (...) stmt;"
    //  Find out what surrounding compound statements have ended and handle these endings.
    ErrorType HandleEndOfCompoundStmts(NestingStack *nesting_stack);

    // Parse start of a brace command
    ErrorType ParseBraceCommandStart(NestingStack *nesting_stack, Symbol struct_of_current_func, Symbol name_of_current_func);

    // Handle the end of a {...} command
    ErrorType HandleEndOfBraceCommand(NestingStack *nesting_stack);

    // Parse the head of a "do ... while(...)" clause
    ErrorType ParseDo(NestingStack *nesting_stack);

    // Handle the end of a "do" body and the "while" clause following it
    ErrorType HandleEndOfDo(NestingStack *nesting_stack);

    // Handle the end of an "else" body
    ErrorType HandleEndOfElse(NestingStack *nesting_stack);

    // The first clause of "for (I; W; C);" when it is a declaration
    ErrorType ParseFor_InitClauseVardecl(size_t nested_level);

    // The first clause of "for (I; W; C);"
    ErrorType ParseFor_InitClause(Symbol peeksym, size_t nested_level);

    // The middle clause of "for (I; W; C);"
    ErrorType ParseFor_WhileClause();

    // The last clause of "for (I; W; C);"
    ErrorType ParseFor_IterateClause();

    // Handle the head of "for (I; W; C);"
    ErrorType ParseFor(NestingStack *nesting_stack);

    // Evaluate an "if" clause, e.g. "if (i < 0)".
    ErrorType ParseIf(NestingStack *nesting_stack);

    // Handle the end of an "if" body
    ErrorType HandleEndOfIf(NestingStack *nesting_stack, bool &else_follows);

    // Parse, e.g., "switch (bar)"
    ErrorType ParseSwitch(NestingStack *nesting_stack);

    // Parse "case foo:" or "default:"
    ErrorType ParseSwitchLabel(Symbol cursym, NestingStack *nesting_stack);

    // Handle the end of a "switch" body
    ErrorType HandleEndOfSwitch(NestingStack *nesting_stack);

    // Parse, e.g., "while (i < 0)"
    // Is NOT responsible for handling the "while" in a "do ... while" clause
    ErrorType ParseWhile(NestingStack *nesting_stack);

    // Handle the end of a "while" body
    // (Will also handle the outer "for" nesting)
    ErrorType HandleEndOfWhile(NestingStack *nesting_stack);

    // We're compiling function body code; the code does not start with a keyword or type.
    // Thus, we should be at the start of an assignment or a funccall. Compile it.
    ErrorType ParseAssignmentOrExpression(Symbol cursym);

    ErrorType ExitNesting(size_t loop_level);

    ErrorType ParseBreak(NestingStack *nesting_stack);

    ErrorType ParseContinue(NestingStack *nesting_stack);

    ErrorType ParseCloseBrace(AGS::Parser::NestingStack *nesting_stack);

    ErrorType ParseCommand(Symbol cursym, Symbol &struct_of_current_func, Symbol &name_of_current_func, NestingStack *nesting_stack);

    // If a new section has begun at cursor position pos, tell _scrip to deal with that.
    // Refresh ccCurScriptName
    void HandleSrcSectionChangeAt(size_t pos);


    inline void WriteCmd(CodeCell op)
        { _scrip.refresh_lineno(_src.GetLineno()); _scrip.write_cmd(op); }
    inline void WriteCmd(CodeCell op, CodeCell p1)
        { _scrip.refresh_lineno(_src.GetLineno()); _scrip.write_cmd(op, p1); }
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2)
        { _scrip.refresh_lineno(_src.GetLineno()); _scrip.write_cmd(op, p1, p2); }
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2, CodeCell p3)
        { _scrip.refresh_lineno(_src.GetLineno()); _scrip.write_cmd(op, p1, p2, p3); }

    // Check whether the qualifiers that accumulated for this decl go together
    ErrorType Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl, Symbol decl_type = kSYM_NoType);

    void Parse_SkipToEndingBrace();


    // Analyse the decls and collect info about locally defined functions
    // This is a pre phase that only does simplified analysis
    ErrorType Parse_PreAnalyzePhase();

    // Generate code
    ErrorType Parse_MainPhase();

    ErrorType ParseInput();

    // Check that all struct functions have a declaration in the struct
    ErrorType Parse_CheckStructFuncs();

    // Only certain info should be carried over from the pre phase into the main phase.
    // Discard all the rest so that the main phase can start with a clean slate.
    ErrorType Parse_ReinitSymTable(const ::SymbolTable &tokenize_res);

    // Blank out all imports that haven't been referenced
    ErrorType Parse_BlankOutUnusedImports();

    // Casts around cc_error()
    // This is a dying message. After the function has been called,
    // the compiler needs to exit immediately with a negative return value
    // Report the error for the section and lineno specified.
    void ErrorWithPosition(int section_id, int lineno, char const *descr, ...);
    // Report the error for the section and lineno that _src currently is at.
    void Error(char const *descr, ...);

    // Record a warning for the current source position
    void Warning(char const *descr, ...);

public:
    // interpret the float as if it were an int (without converting it really);
    // return that int
    // This should be moved somewhere. It isn't Parser functionality
    static int InterpretFloatAsInt(float floatval);

    Parser(::SymbolTable &symt, SrcList &src, ::ccCompiledScript &scrip);

    ErrorType Parse();

    inline std::vector<struct Warning> const &GetWarnings() const { return _warnings; }

}; // class Parser
} // namespace AGS

// Only use this function for googletests. Scan and tokenize the input.
extern int cc_scan(
    char const *inpl,           // preprocessed text to be tokenized
    AGS::SrcList *src,          // store for the tokenized text
    ccCompiledScript *scrip,    // repository for the strings in the text
    SymbolTable *symt);         // symbol table 

// Only use this function for googletests. Parse the input
extern int cc_parse(
    AGS::SrcList *src,          // tokenized text
    ccCompiledScript *scrip,    // result of the compilation
    SymbolTable *symt);         // symbol table

// Compile the input.
extern int cc_compile(
    char const *inpl,           // preprocessed text to be compiled
    ccCompiledScript *scrip);   // store for the compiled text

#endif // __CS_PARSER_H
