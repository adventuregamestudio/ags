/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones
SCOM is a script compiler for the 'C' language.

BIRD'S EYE OVERVIEW - INTERFACE AND HIGH-LEVEL STRUCTURE

The processing is done in the following layers:
* [Preprocessing - This has been done separately before the input arrives here.]
    Expand macros, delete comments

* Scanning
    Read the characters of the input and partition it in symbols (e.g., identifier, number literal).
    Enter all the symbols into a symbol table (thus recognizing keywords and literals)
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
    // Needs to be public because the manager is initialized outside of Parser
    class FuncCallpointMgr
    {
    private:
        int const kCodeBaseId = 0;  // Magic number, means: This is in codebase, not in a yanked piece of code
        int const kPatchedId = -1;  // Magic number, means: This is in codebase and has already been patched in

        Parser &_parser;

        struct PatchInfo
        {
            int ChunkId;
            CodeLoc Offset;
            size_t InSource;    // where the reference happens
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
        FuncCallpointMgr(Parser &parser);
        void Reset();

        // Enter a code location where a function is called that hasn't been defined yet.
        ErrorType TrackForwardDeclFuncCall(Symbol func, CodeLoc idx, size_t in_source);

        // When code is ripped out of the codebase: 
        // Update list of calls to forward declared functions 
        ErrorType UpdateCallListOnYanking(CodeLoc start, size_t len, int id);

        // When code is inserted into the codebase:
        // Update list of calls to forward declared functions
        ErrorType UpdateCallListOnWriting(CodeLoc start, int id);

        // Set the address that must be called to call the function.
        // Patch all the function calls of the given function to point to dest
        // These codecells contained a dummy value before, now they get the proper address
        ErrorType SetFuncCallpoint(Symbol func, CodeLoc dest);

        inline bool HasFuncCallpoint(Symbol func) { return (_funcCallpointMap[func].Callpoint >= 0); }

        inline bool IsForwardDecl(Symbol func) { return (0 == _funcCallpointMap.count(func)); }

        // Give an error message and return a value < 0 iff there are still callpoints
        // without a location
        ErrorType CheckForUnresolvedFuncs();
    };
    friend FuncCallpointMgr;

private:
    enum FunctionType
    {
        kFT_PureForward = 0,
        kFT_Import = 1,
        kFT_LocalBody = 2,
    };

    // This indicates where a value is delivered.
    // When reading, we need the value itself.
    // - It can be in AX (kVL_AX_is_value)
    // - or in m(MAR) (kVL_MAR_pointsto_value)
    // - or a constant float or int value (kVL_compile_time_literal)
    //      In this case the symbol that points to the value is in symbol
    // When writing, we need a pointer to the adress that has to be modified.
    // - This can be MAR, i.e., the value to modify is in m(MAR) (kVL_MAR_pointsto_value).
    // - or AX, i.e., the value to modify is in m(AX) (kVL_AX_is_value)
    // - attributes must be modified by calling their setter function (kVL_Attribute)

    struct ValueLocation
    {
        enum
        {
            kAX_is_value,            // The value is in register AX
            kMAR_pointsto_value,     // The value is in m(MAR)
            kAttribute,              // The value must be modified by calling an attribute setter
            kCompile_time_literal,   // The value is in 'symbol'
        } location;
        Symbol symbol; // only meaningful für kCompile_time_literal
    };

    // This ought to replace the #defines in script_common.h
    // but we can't touch them since the engine uses them, too
    enum FxFixupType : FixupType // see script_common.h
    {
        kFx_NoFixup = 0,
        kFx_DataData = FIXUP_DATADATA,     // globaldata[fixup] += &globaldata[0]
        kFx_Code = FIXUP_FUNCTION,         // code[fixup] += &code[0]
        kFx_GlobalData = FIXUP_GLOBALDATA, // code[fixup] += &globaldata[0]
        kFx_Import = FIXUP_IMPORT,         // code[fixup] = &imported_thing[code[fixup]]
        kFx_Stack = FIXUP_STACK,           // code[fixup] += &stack[0]
        kFx_String = FIXUP_STRING,         // code[fixup] += &strings[0]
    };

    // The stack of nesting compound statements 
    class NestingStack
    {
    public:
        enum class NSType
        {
            kNone,
            kBraces, // { } without any preceding if, while etc.
            kDo,
            kElse,
            kFor,
            kFunction,
            kIf,
            kParameters, // Parameters of a function
            kSwitch,
            kWhile,
        };

    private:
        static int _chunkIdCtr; // for assigning unique IDs to chunks

        // A section of compiled code that needs to be moved or copied to a new location
        struct Chunk
        {
            std::vector<CodeCell> Code;
            std::vector<CodeLoc> Fixups;
            std::vector<char> FixupTypes;
            size_t SrcLine;
            int Id;
        };

        // All data that is associated with a level of the nested compound statements
        struct NestingInfo
        {
            NSType Type; // Shows what kind of statement this level pertains to (e.g., while or for)
            BackwardJumpDest Start; // of the construct
            ForwardJump JumpOut; // First byte after the construct
            Vartype SwitchExprVartype; // when switch: the vartype of the switch expression
            BackwardJumpDest SwitchDefault; // when switch: code location of the default: label
            std::vector<BackwardJumpDest> SwitchCases; // when switch: code locations of the case labels
            ForwardJump SwitchJumptable; // when switch: code location of the "jumptable"
            std::vector<Chunk> Chunks; // Bytecode chunks that must be moved (FOR loops and SWITCH)
            // Symbols defined on the current level, if applicable together with the respective old definition they hide
            std::map<Symbol, SymbolTableEntry> OldDefinitions;

            NestingInfo(NSType stype, ccCompiledScript &scrip);
        };

        std::vector<NestingInfo> _stack;
        ccCompiledScript &_scrip;

    public:
        NestingStack(ccCompiledScript &scrip);

        // Index of the innermost nesting level
        inline size_t TopLevel() const { return _stack.size() - 1; };

        // Type of the innermost nesting
        inline NSType Type() const { return _stack.back().Type; }
        inline NSType Type(size_t level) const { return _stack.at(level).Type; }
        inline void SetType(NSType nt) { _stack.back().Type = nt; }
        
        inline std::map<Symbol, SymbolTableEntry> const &GetOldDefinitions(size_t level) const { return _stack.at(level).OldDefinitions; }
        // Add an old symbol table entry to the innermost nesting; return true if there is one already.
        bool AddOldDefinition(Symbol s, SymbolTableEntry const &entry);
        
        // If the innermost nesting is a loop that has a jump back to the start,
        // then this gives the location to jump to
        inline BackwardJumpDest &Start() { return _stack.back().Start; }
        inline BackwardJumpDest &Start(size_t level) { return _stack.at(level).Start; }

        // Collects the jumps to the Bytecode byte after the construct
        inline ForwardJump &JumpOut() { return _stack.back().JumpOut; }
        inline ForwardJump &JumpOut(size_t level) { return _stack.at(level).JumpOut; }

        // If the innermost nesting is a SWITCH, the type of the switch expression
        inline Vartype SwitchExprVartype() const { return _stack.back().SwitchExprVartype; };
        inline void SetSwitchExprVartype(Vartype vt) { _stack.back().SwitchExprVartype = vt; }

        // If the innermost nesting is a SWITCH, the location of the "default:" code
        inline BackwardJumpDest &SwitchDefault() { return _stack.back().SwitchDefault; }
        
        // If the innermost nesting is a SWITCH, the location of code of the cases
        inline std::vector<BackwardJumpDest> &SwitchCases() { return _stack.back().SwitchCases; }

        // If the innermost nesting is a SWITCH, the location of the jump table
        inline ForwardJump &SwitchJumptable() { return _stack.back().SwitchJumptable; }
        // If the nesting at the given level is a SWITCH, the location of the  jump table
        inline ForwardJump &SwitchJumptable(size_t level) { return _stack.at(level).SwitchJumptable; }

        // Whether the innermost nesting contains code chunks that must be moved around (e.g., in FOR loops)
        inline bool ChunksExist() const { return !_stack.back().Chunks.empty(); }
        inline bool ChunksExist(size_t level) const { return !_stack.at(level).Chunks.empty(); }

        // Code chunks that must be moved around (e.g., in FOR, DO loops)
        inline std::vector<Chunk> Chunks() { return _stack.back().Chunks; };
        inline std::vector<Chunk> Chunks(size_t level) { return _stack.at(level).Chunks; };

        inline void Push(NSType type) { _stack.emplace_back(type, _scrip); }
        inline void Pop() { _stack.pop_back(); };

        // Rip a generated chunk of code out of the codebase and stash it away for later 
        // At end of function call, id will contain the unique ID of this code
        void YankChunk(size_t src_line, CodeLoc code_start, size_t fixups_start, int &id);

        // Write chunk of code back into the codebase that has been stashed in level given, at index
        void WriteChunk(size_t level, size_t chunk_idx, int &id);
        // Write chunk of code back into the codebase stashed in the innermost level, at index
        inline void WriteChunk(size_t chunk_idx, int &id) { WriteChunk(TopLevel(), chunk_idx, id); };
    } _nest;
    typedef NestingStack::NSType NSType;

    // Track the phase the parser is in.
    enum class PP
    {
        kPreAnalyze = 0, // A pre-phase that finds out, amongst others, what functions have (local) bodies
        kMain,           // The main phase that generates the bytecode.
    } _pp;
    typedef PP ParsingPhase;

    enum class VariableAccess
    {
        kReading,
        kWriting,
        kReadingForLaterWriting,
    };
    typedef VariableAccess VAC;

    // We set the MAR register lazily to save on runtime computation. This object
    // encapsulates the stashed operations that haven't been done on MAR yet.
    class MemoryLocation
    {
    private:
        Parser &_parser;
        ScopeType _ScType;
        size_t _startOffs;
        size_t _componentOffs;

    public:
        MemoryLocation(Parser &parser);

        // Set the type and the start offset of the MAR register
        ErrorType SetStart(ScopeType type, size_t offset);

        inline void AddComponentOffset(size_t offset) { _componentOffs += offset; };

        // Write out the Bytecode necessary to bring MAR up-to-date; reset the object
        ErrorType MakeMARCurrent(size_t lineno, ccCompiledScript &scrip);

        inline bool OpsPending() const { return ScT::kNone != _ScType || 0u < _startOffs || 0u < _componentOffs; };

        void Reset();
    };

    // Manage a list of all global import variables and track whether they are
    // re-defined as non-import later on.
    // Symbol maps to TRUE if it is global import, to FALSE if it is global non-import.
    // Only a global import may have a repeated identical definition.
    // Only a global import may be re-defined as a global non-import
    //    (that must be identical except for the "import" declarator),
    //    and this may only happen if the options don't forbid this.
    typedef std::map<Symbol, bool> TGIVM; // Global Import Variable Mgr
    TGIVM _givm; // Global Import Variable Manager

    // Main symbol table
    SymbolTable &_sym;

    // List of symbols from the tokenizer
    SrcList &_src;

    // Compile flags, as specified in cc_options
    FlagSet _options;

    // Receives the parsing results
    ccCompiledScript &_scrip;

    // Receives the errors and warnings
    MessageHandler &_msg_handler;

    // Manage a map of all the functions that have bodies (in the current source).
    FuncCallpointMgr _fcm;

    // Manage a map of all imported functions where the import decl comes after the function
    // The "Callpoint" of such a function is the index in the import table.
    FuncCallpointMgr _fim; // i for import

    // Track a forward-declared struct and one of its references to it.
    std::map<Symbol, size_t> _structRefs;

    size_t _lastEmittedSectionId;
    size_t _lastEmittedLineno;

    // Buffer for ccCurScriptName
    std::string _scriptNameBuffer;

    // Augment the message with a "See ..." indication
    // declared is the point in _src where the thing is declared
    std::string const ReferenceMsgLoc(std::string const &msg, size_t declared);
    // Augment the message with a "See ..." indication pointing to the declaration of sym
    std::string const ReferenceMsgSym(std::string const &msg, Symbol symb);

    // Adds the symbol to the list if it isn't there yet.
    void AddToSymbolList(Symbol symb, SymbolList &list);

    std::string const TypeQualifierSet2String(TypeQualifierSet tqs) const;

    void SetDynpointerInManagedVartype(Vartype &vartype);

    // Combine the arguments to stname::component, get the symbol for that
    Symbol MangleStructAndComponent(Symbol stname, Symbol component);

    // Skim through source, ignoring delimited content completely. Stop in the following cases:
    // .  A symbol is encountered whose type is in stoplist[]
    // .  A closing symbol is encountered that hasn't been opened.
    // Don't consume the symbol that stops the scan.
    ErrorType SkipTo(SymbolList const &stoplist, SrcList &source);

    // Skim through source, ignoring delimited content completely.
    // Stop when a closing symbol is encountered that hasn't been opened.
    // Eat that symbol and if it isn't closer, report an _internal_ error.
    ErrorType SkipToClose(Predefined closer);

    // If the actual symbol isn't equal to the expected symbol, give an error.
    // custom_msg, if given, replaces the "Expected " part of the message
    ErrorType Expect(Symbol expected, Symbol actual, std::string const &custom_msg = "");

    // If the actual symbol isn't in the list of expected symbols, give an error.
    ErrorType Expect(std::vector<Symbol> const &expected, Symbol actual);

    // Mark the symbol as "accessed" in the symbol table
    inline void MarkAcessed(Symbol symb) { _sym[symb].Accessed = true; }

    // The size of all local variables that have been allocated at a level higher than from_level
    size_t StacksizeOfLocals(size_t from_level);

    // Does vartype v contain releasable pointers?
    // Also determines whether vartype contains standard (non-dynamic) arrays.
    bool ContainsReleasableDynpointers(Vartype vartype);

    // We're at the end of a block and releasing a standard array of dynpointers.
    // MAR points to the array start. Release each array element (dynpointer).
    ErrorType FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements, bool &clobbers_ax);

    // We're at the end of a block and releasing all the dynpointers in a struct.
    // MAR already points to the start of the struct.
    void FreeDynpointersOfStruct(Vartype struct_vtype, bool &clobbers_ax);

    // We're at the end of a block and we're releasing a standard array of struct.
    // MAR points to the start of the array. Release all the pointers in the array.
    void FreeDynpointersOfStdArrayOfStruct(Vartype element_vtype, size_t num_of_elements, bool &clobbers_ax);

    // We're at the end of a block and releasing a standard array. MAR points to the start.
    // Release the pointers that the array contains.
    void FreeDynpointersOfStdArray(Symbol the_array, bool &clobbers_ax);

    ErrorType FreeDynpointersOfLocals0(size_t from_level, bool &clobbers_ax, bool &clobbers_mar);

    // Free the pointers of any locals in level from_level or higher
    ErrorType FreeDynpointersOfLocals(size_t from_level);

    // Free the pointers of all locals at the end of a function. This function
    // returns a Dynarray or Dynpointer, so the result must be protected from
    // dropping its last reference, which could mean the pointer is released prematurely.
    ErrorType FreeDynpointersOfAllLocals_DynResult(void);

    // Free the pointers of all locals at the end of a function. Do not clobber AX.
    ErrorType FreeDynpointersOfAllLocals_KeepAX(void);

    // Restore those definitions that have a nesting level of 'from_level' or higher
    // to what they were on the level 'from_level - 1'.
    ErrorType RestoreLocalsFromSymtable(size_t from_level);

    // Remove at nesting_level or higher.
    ErrorType RemoveLocalsFromStack(size_t nesting_level);

    // Read a symbol that must be a literal or a const. If it is a const, dereference it until it becomes a literal.
    ErrorType ReadLiteralOrConst(SrcList &src, Symbol &lit);
    inline ErrorType ReadLiteralOrConst(Symbol &lit) { return ReadLiteralOrConst(_src, lit); }
    ErrorType ReadIntLiteralOrConst(Symbol &lit, std::string const &msg);

    // When 'symb' corresponds to 'value', set it to the symbol that corresponds to -'value'.
    ErrorType NegateLiteral(Symbol &symb);

    // Record the literal as a compile time literal
    ErrorType SetCompileTimeLiteral(Symbol lit, ValueLocation &vloc, Vartype &vartype);

    // Find or create a symbol that is a literal for the value 'value'.
    ErrorType FindOrAddIntLiteral(CodeCell value, Symbol &symb);


    // We're parsing a parameter list and we have accepted something like "(...int i"
    // We accept a default value clause like "= 15" if it follows at this point.
    // If there isn't any default, kKW_NoSymbol is returned.
    // Otherwise, a symbol is returned that is a literal.
    ErrorType ParseParamlist_Param_DefaultValue(Vartype param_vartype, Symbol &default_value);

    // process a dynamic array declaration, when present
    // We have accepted something like "int foo" and we might expect a trailing "[]" here
    ErrorType ParseDynArrayMarkerIfPresent(Vartype &vartype);

    // Extender function, eg. function GoAway(this Character *someone)
    // We've just accepted something like "int func("
    // We'll accept something like "this Character *" --OR-- "static Character"
    ErrorType ParseFuncdecl_ExtenderPreparations(bool is_static_extender, Symbol &struct_of_func, Symbol &name_of_func, TypeQualifierSet &tqs);

    // In a parameter list, process the vartype of a parameter declaration
    ErrorType ParseParamlist_ParamType(Vartype &vartype);

    // We're accepting a parameter list. We've accepted something like "int".
    // We accept a param name such as "i" if present
    ErrorType ParseParamlist_Param_Name(bool body_follows, Symbol &param_name);

    // Additional handling to ParseVardecl_Var2SymTable() that is special for parameters
    ErrorType ParseParamlist_Param_AsVar2Sym(Symbol param_name, TypeQualifierSet tqs, Vartype param_vartype, int param_idx);

    // process a parameter decl in a function parameter list
    ErrorType ParseParamlist_Param(Symbol name_of_func, bool body_follows, TypeQualifierSet tqs, Vartype param_vartype, size_t param_idx);

    ErrorType ParseFuncdecl_Paramlist(Symbol funcsym, bool body_follows);

    void ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_function, Symbol name_of_function, bool body_follows);

    // there was a forward declaration -- check that the real declaration matches it
    ErrorType ParseFuncdecl_CheckThatKnownInfoMatches(std::string const &func_name, SymbolTableEntry::FunctionDesc const *this_entry, SymbolTableEntry::FunctionDesc const *known_info, size_t declared, bool body_follows);

    // Enter the function in the imports[] or functions[] array; get its index   
    ErrorType ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, size_t num_of_parameters, CodeLoc &function_soffs);

    // We're at something like "int foo(", directly before the "("
    // Find out whether the symbol that follows the corresponding ")" is "{"
    ErrorType ParseFuncdecl_DoesBodyFollow(bool &body_follows);

    // We're in a func decl. Check whether the declaration is valid.
    ErrorType ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check);

    ErrorType ParseFuncdecl_HandleFunctionOrImportIndex(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool body_follows);

    // We're at something like "int foo(", directly before the "("
    // If this is an extender function, we've already resolved that.
    // Parse the function declaration
    // This might or might not be within a struct defn
    ErrorType ParseFuncdecl(size_t declaration_start, TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_func, Symbol name_of_func, bool no_loop_check, bool &body_follows);

    // Find the index of the operator in the list that binds the least
    // so that either side of it can be evaluated first. -1 if no operator was found
    ErrorType IndexOfLeastBondingOperator(SrcList &expression, int &idx);

    // Also check whether the operator can handle the types at all
    ErrorType GetOpcode(Symbol op_sym, Vartype vartype1, Vartype vartype2, CodeCell &opcode);

    // Check for a type mismatch in one direction only
    bool IsVartypeMismatch_Oneway(Vartype vartype_is, Vartype vartype_wants_to_be) const;

    // Check whether there is a type mismatch; if so, give an error
    ErrorType IsVartypeMismatch(Vartype vartype_is, Vartype vartype_wants_to_be, bool orderMatters);

    // Whether this operator's vartype is always bool
    static bool IsBooleanOpcode(CodeCell opcode);

    // 'current_vartype' must be the vartype of AX. If it is 'string' and
    // wanted_vartype is 'String', then AX will be converted to 'String'.
    // then convert AX into a String object and set its type accordingly
    void ConvertAXStringToStringObject(Vartype current_vartype, Vartype &wanted_vartype);

    static int GetReadCommandForSize(int the_size);

    static int GetWriteCommandForSize(int the_size);

    // Handle the cases where a value is a whole array or dynarray or struct
    ErrorType HandleStructOrArrayResult(Vartype &vartype, Parser::ValueLocation &vloc);

    // If the result isn't in AX, move it there. Dereferences a pointer
    void ResultToAX(Vartype vartype, ValueLocation &vloc);

    // We're in the parameter list of a function call, and we have less parameters than declared.
    // Provide defaults for the missing values
    ErrorType AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    ErrorType AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    // Count parameters, check that all the parameters are non-empty; find closing paren
    ErrorType AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters, Symbol name_of_func, size_t &index_of_close_paren, size_t &num_supplied_args);

    // We are processing a function call. General the actual function call
    void AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import);

    // Generate the function call for the function that returns the number of elements
    // of a dynarray.
    void AccessData_GenerateDynarrayLengthFuncCall(MemoryLocation &mloc, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // We are processing a function call.
    // Get the parameters of the call and push them onto the stack.
    // Return the number of the parameters pushed
    ErrorType AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args);

    // Process a function call. The parameter list begins with expression[1].
    ErrorType AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, MemoryLocation &mloc, Vartype &rettype);

    // Check the vartype following "new"
    ErrorType ParseExpression_CheckArgOfNew(Vartype new_vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is unary NEW
    ErrorType ParseExpression_New(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is unary '-'
    ErrorType ParseExpression_UnaryMinus(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is unary '+'
    ErrorType ParseExpression_UnaryPlus(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is a boolean or bitwise negation
    ErrorType ParseExpression_Negate(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is a unary operator
    ErrorType ParseExpression_Unary(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. Expression is a ternary a ? b : c
    ErrorType ParseExpression_Ternary(size_t tern_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator a binary operator.
    ErrorType ParseExpression_Binary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. The lowest-binding operator is '?' or a binary operator.
    ErrorType ParseExpression_BinaryOrTernary(size_t op_idx, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. Expression begins with '('
    // Leaves the cursor pointing after the last token that was processed
    ErrorType ParseExpression_InParens(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION. Expression does not contain operators
    // Leaves the cursor pointing after the last token that was processed
    ErrorType ParseExpression_NoOps(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse the term given in EXPRESSION.
    // Leaves the cursor pointing after the last token that was processed
    ErrorType ParseExpression_Term(SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

    // Parse expression in parentheses
    // leaves src pointing after last token in expression, so do getnext() to get the following ; or whatever
    ErrorType ParseParenthesizedExpression();

    // Evaluate the supplied expression, putting the result into AX
    // leaves src pointing to last token in expression, so do getnext() to get the following ; or whatever
    ErrorType ParseExpression(ScopeType &scope_type, Vartype &vartype);
    ErrorType ParseExpression();

    ErrorType AccessData_ReadBracketedIntExpression(SrcList &expression);

    ErrorType AccessData_ReadIntExpression(SrcList &expression);

    // We access a variable or a component of a struct in order to read or write it.
    // This is a simple member of the struct.
    ErrorType AccessData_StructMember(Symbol component, VariableAccess access_type, bool access_via_this, SrcList &expression, MemoryLocation &mloc, Vartype &vartype);

    // Get the symbol for the get or set function corresponding to the attribute given.
    ErrorType ConstructAttributeFuncName(Symbol attribsym, bool is_setter, bool is_indexed, Symbol &func);

    // We call the getter or setter of an attribute
    // The next symbol read is the attribute (the part after the '.')
    ErrorType AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, Vartype &vartype);

    // Memory location contains a pointer to another address. Get that address.
    ErrorType AccessData_Dereference(ValueLocation &vloc, MemoryLocation &mloc);

    ErrorType AccessData_ProcessArrayIndexConstant(size_t idx, Symbol index_symbol, bool negate, size_t num_array_elements, size_t element_size, MemoryLocation &mloc);

    // Process one index in a sequence of array indexes
    ErrorType AccessData_ProcessCurrentArrayIndex(size_t idx, size_t dim, size_t factor, bool is_dynarray, SrcList &expression, MemoryLocation &mloc);

    // We're processing some struct component or global or local variable.
    // If a sequence of array indexes follows, parse it and shorten symlist accordingly
    ErrorType AccessData_ProcessAnyArrayIndex(ValueLocation vloc_of_array, SrcList &expression, ValueLocation &vloc, MemoryLocation &mloc, Vartype &vartype);

    ErrorType AccessData_Variable(ScopeType scope_type, VariableAccess access_type, SrcList &expression, MemoryLocation &mloc, Vartype &vartype);

    // We're getting a variable, literal, constant, func call or the first element
    // of a STRUCT.STRUCT.STRUCT... cascade.
    // This moves the cursor in all cases except for the cascade to the end of what is parsed,
    // and in case of a cascade, to the end of the first element of the cascade, i.e.,
    // to the position of the '.'.
    // The "return_scope_type" is used for deciding what values can be returned from a function.
    // implied_this_dot is set if subsequent processing should imply that
    // the expression starts with "this.", with the '.' already read in
    ErrorType AccessData_FirstClause(VariableAccess access_type, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type, MemoryLocation &mloc, Vartype &vartype, bool &implied_this_dot, bool &static_access);

    // We're processing a STRUCT.STRUCT. ... clause.
    // We've already processed some structs, and the type of the last one is vartype.
    // Now we process a component of vartype.
    ErrorType AccessData_SubsequentClause(VariableAccess access_type, bool access_via_this, bool static_access, SrcList &expression, ValueLocation &vloc, ScopeType &return_scope_type, MemoryLocation &mloc, Vartype &vartype);

    // Find the component of a struct in the struct or in the ancestors of the struct
    // and return the name of the struct (!) that the component is defined in
    // Return kKW_NoSymbol if such a struct doesn't exist 
    Symbol FindStructOfComponent(Vartype strct, Symbol unqualified_component);

    // Find the component of a struct in the struct or in the ancestors of the struct
    // and return the name of the component(!), qualified by the struct that the component was defined in
    // Return kKW_NoSymbol if such a struct doesn't exist 
    Symbol FindComponentInStruct(Vartype strct, Symbol unqualified_component);

    // We are in a STRUCT.STRUCT.STRUCT... cascade.
    // Check whether we have passed the last dot
    ErrorType AccessData_IsClauseLast(SrcList &expression, bool &is_last);

    // Access a variable, constant, literal, func call, struct.component.component cascade, etc.
    // Result is in AX or m[MAR], dependent on vloc. Variable type is in vartype.
    // At end of function, symlist and symlist_len will point to the part of the symbol string
    // that has not been processed yet
    // NOTE: If this selects an attribute for writing, then the corresponding function will
    // _not_ be called and symlist[0] will be the attribute.
    ErrorType AccessData(VariableAccess access_type, SrcList &expression, ValueLocation &vloc, ScopeType &scope_type, Vartype &vartype);

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
    ErrorType AccessData_AssignTo(ScopeType sct, Vartype vartype, SrcList &expression);

    ErrorType SkipToEndOfExpression();

    // We are parsing the left hand side of a += or similar statement.
    ErrorType ParseAssignment_ReadLHSForModification(SrcList &lhs, ScopeType &scope_type, ValueLocation &vloc, Vartype &lhstype);

    // "var = expression"; lhs is the variable
    ErrorType ParseAssignment_Assign(SrcList &lhs);

    // We compile something like "var += expression"
    ErrorType ParseAssignment_MAssign(Symbol ass_symbol, SrcList &lhs);

    // "var++" or "var--"
    ErrorType ParseAssignment_SAssign(Symbol ass_symbol, SrcList &lhs);

    ErrorType ParseVardecl_InitialValAssignment_IntVartypeOrFloat(Vartype var, void *&initial_val_ptr);

    ErrorType ParseVardecl_InitialValAssignment_OldString(void *&initial_val_ptr);

    // if initial_value is non-null, return with 'malloc'ed memory that must be 'free'd
    ErrorType ParseVardecl_InitialValAssignment(Symbol varname, void *&initial_val_ptr);

    // Move variable information into the symbol table
    ErrorType ParseVardecl_Var2SymTable(Symbol var_name, Vartype vartype, ScopeType scope_type);

    // we have accepted something like "int a" and we're expecting "["
    ErrorType ParseArray(Symbol vname, Vartype &vartype);

    ErrorType ParseVardecl_CheckIllegalCombis(Vartype vartype, ScopeType scope_type);

    // there was a forward declaration -- check that the real declaration matches it
    ErrorType ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info, bool body_follows);

    ErrorType ParseVardecl_Global(Symbol var_name, Vartype vartype, void *&initial_val_ptr);

    ErrorType ParseVardecl_Import(Symbol var_name);

    ErrorType ParseVardecl_Local(Symbol var_name, Vartype vartype);

    ErrorType ParseVardecl0(Symbol var_name, Vartype vartype, ScopeType scope_type, TypeQualifierSet tqs);

    // Checks whether an old definition exists that may be stashed; stashes it if possible
    ErrorType ParseVardecl_CheckAndStashOldDefn(Symbol var_name);

    ErrorType ParseVardecl(Symbol var_name, Vartype vartype, ScopeType scope_type, TypeQualifierSet tqs);

    ErrorType ParseFuncBodyStart(Symbol struct_of_func, Symbol name_of_func);

    ErrorType HandleEndOfFuncBody(Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // Helper for ParseStruct_CheckForwardDecls()
    ErrorType ParseStruct_GenerateForwardDeclError(Symbol stname, TypeQualifierSet tqs, TypeQualifier tq, VartypeFlag vtf);

    // If there are forward declarations, check that their type qualifiers match 
    ErrorType ParseStruct_CheckForwardDecls(Symbol stname, TypeQualifierSet tqs);

    void ParseStruct_SetTypeInSymboltable(Symbol stname, TypeQualifierSet tqs);

    // We have accepted something like "struct foo" and are waiting for "extends"
    ErrorType ParseStruct_ExtendsClause(Symbol stname);

    ErrorType ParseQualifiers(TypeQualifierSet &tqs);

    ErrorType ParseStruct_CheckComponentVartype(Symbol stname, Vartype vartype);

    ErrorType ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype);

    ErrorType ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype);

    ErrorType ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype);

    // We are processing an attribute.
    // This corresponds to a getter func and a setter func, declare one of them
    ErrorType ParseStruct_Attribute_DeclareFunc(TypeQualifierSet tqs, Symbol strct, Symbol qualified_name, Symbol unqualified_name, bool is_setter, bool is_indexed, Vartype vartype);

    // We're in a struct declaration. Parse an attribute declaration.
    ErrorType ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname, Symbol vname, Vartype vartype);

    // We're inside a struct decl, processing a member variable
    ErrorType ParseStruct_VariableOrAttributeDefn(TypeQualifierSet tqs, Vartype curtype, Symbol stname, Symbol vname);

    // We have accepted something like "struct foo extends bar { const int".
    // We're waiting for the name of the member.
    ErrorType ParseStruct_MemberDefn(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype);

    // We've accepted, e.g., "struct foo {". Now we're parsing a variable declaration or a function declaration
    ErrorType ParseStruct_Vartype(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype);

    // Handle a "struct" definition clause
    ErrorType ParseStruct(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // We've accepted something like "enum foo { bar"; '=' follows
    ErrorType ParseEnum_AssignedValue(Symbol vname, CodeCell &value);

    // Define the item 'item_name' to have the value 'value'. 
    ErrorType ParseEnum_Item2Symtable(Symbol enum_name, Symbol item_name, int value);

    ErrorType ParseEnum_Name2Symtable(Symbol enum_name);

    // We parse enum eEnumName { value1, value2 };
    ErrorType ParseEnum(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_function);

    ErrorType ParseExport_Function(Symbol func);
    ErrorType ParseExport_Variable(Symbol var);
    ErrorType ParseExport();

    ErrorType ParseReturn(Symbol name_of_current_func);

    // Helper function for parsing a varname
    ErrorType ParseVarname0(bool accept_member_access, Symbol &structname, Symbol &varname);

    // Parse a variable name; may contain '::'
    // If it does contain '::' then varname will contain the qualified name (a::b) and structname the vartype (a)
    inline ErrorType ParseVarname(Symbol &structname, Symbol &varname) { return ParseVarname0(true, structname, varname); }
    // Parse a variable name; may not contain '::'
    inline ErrorType ParseVarname(Symbol &varname) { Symbol dummy; return ParseVarname0(false, dummy, varname); }

    ErrorType ParseVartype_CheckForIllegalContext();

    ErrorType ParseVartype_CheckIllegalCombis(bool is_function, TypeQualifierSet tqs);

    ErrorType ParseVartype_FuncDecl(TypeQualifierSet tqs, Vartype vartype, Symbol struct_name, Symbol func_name, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func, bool &body_follows);

    ErrorType ParseVartype_VarDecl_PreAnalyze(Symbol var_name, ScopeType scope_type);

    ErrorType ParseVartype_VarDecl(Symbol var_name, ScopeType scope_type, TypeQualifierSet tqs, Vartype vartype);

    // We accepted a variable type such as "int", so what follows is a variable or function declaration
    ErrorType ParseVartype(Vartype vartype, TypeQualifierSet tqs, Symbol &name_of_current_func, Symbol &struct_of_current_func);

    // After a command statement. This command might be the end of sequences such as
    // "if (...) while (...) stmt;"
    //  Find out what surrounding compound statements have ended and handle these endings.
    ErrorType HandleEndOfCompoundStmts();

    // Handle the end of a {...} command
    ErrorType HandleEndOfBraceCommand();

    // Parse the head of a "do ... while(...)" clause
    ErrorType ParseDo();

    // Handle the end of a "do" body and the "while" clause following it
    ErrorType HandleEndOfDo();

    // Handle the end of an "else" body
    ErrorType HandleEndOfElse();

    // The first clause of "for (I; W; C);" when it is a declaration
    ErrorType ParseFor_InitClauseVardecl();

    // The first clause of "for (I; W; C);"
    ErrorType ParseFor_InitClause(Symbol peeksym);

    // The middle clause of "for (I; W; C);"
    ErrorType ParseFor_WhileClause();

    // The last clause of "for (I; W; C);"
    ErrorType ParseFor_IterateClause();

    // Handle the head of "for (I; W; C);"
    ErrorType ParseFor();

    // Evaluate an "if" clause, e.g. "if (i < 0)".
    ErrorType ParseIf();

    // Handle the end of an "if" body
    ErrorType HandleEndOfIf(bool &else_follows);

    // Parse, e.g., "switch (bar)"
    ErrorType ParseSwitch();

    // Parse "case foo:" or "default:"
    ErrorType ParseSwitchLabel(Symbol case_or_default);

    // Handle the end of a "switch" body
    ErrorType HandleEndOfSwitch();

    // Parse, e.g., "while (i < 0)"
    // Is NOT responsible for handling the "while" in a "do ... while" clause
    ErrorType ParseWhile();

    // Handle the end of a "while" body
    // (Will also handle the outer "for" nesting)
    ErrorType HandleEndOfWhile();

    // We're compiling function body code; the code does not start with a keyword or type.
    // Thus, we should be at the start of an assignment or a funccall. Compile it.
    ErrorType ParseAssignmentOrExpression(Symbol cursym);

    ErrorType ParseBreak();

    ErrorType ParseContinue();

    ErrorType ParseCloseBrace();

    // Parse a command. The leading symbol has already been eaten
    ErrorType ParseCommand(Symbol leading_sym, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // If a new section has begun at cursor position pos, tell _scrip to deal with that.
    // Refresh ccCurScriptName
    ErrorType HandleSrcSectionChangeAt(size_t pos);

    inline void WriteCmd(CodeCell op)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op); }
    inline void WriteCmd(CodeCell op, CodeCell p1)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op, p1); }
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op, p1, p2); }
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2, CodeCell p3)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op, p1, p2, p3); }
    inline void PushReg(CodeCell reg)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.PushReg(reg); }
    inline void PopReg(CodeCell reg)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.PopReg(reg); }

    // Check whether the qualifiers that accumulated for this decl go together
    ErrorType Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl);
    ErrorType Parse_CheckTQSIsEmpty(TypeQualifierSet tqs);

    // Analyse the decls and collect info about locally defined functions
    // This is a pre phase that only does simplified analysis
    ErrorType Parse_PreAnalyzePhase();

    // Generate code
    ErrorType Parse_MainPhase();

    ErrorType ParseInput();

    // Only certain info should be carried over from the pre phase into the main phase.
    // Discard all the rest so that the main phase can start with a clean slate.
    ErrorType Parse_ReinitSymTable(size_t size_after_scanning);

    // Check whether a forward-declared struct has actually been referenced and never defined
    ErrorType Parse_CheckForUnresolvedStructForwardDecls();

    // Sanity check for the fixups
    ErrorType Parse_CheckFixupSanity();

    ErrorType Parse_ExportAllFunctions();

    // Blank out all imports that haven't been referenced
    ErrorType Parse_BlankOutUnusedImports();

    // Report a message for the section and lineno specified.
    void MessageWithPosition(MessageHandler::Severity sev, int section_id, size_t lineno, char const *descr, ...);

    // Report an error for the section and lineno that _src currently is at.
    void Error(char const *descr, ...);

    // Record a warning for the current source position
    void Warning(char const *descr, ...);

public:
    Parser(SrcList &src, FlagSet options, ccCompiledScript &scrip, SymbolTable &symt, MessageHandler &mh);

    ErrorType Parse();

}; // class Parser
} // namespace AGS

// Compile the input, return any messages in mh, cc_error() does not get called
extern int cc_compile(
    std::string const &source,  // preprocessed text to be compiled
    AGS::FlagSet options,            // as defined in cc_options 
    AGS::ccCompiledScript &scrip,    // store for the compiled text
    AGS::MessageHandler &mh);        // warnings and the error   

#endif // __CS_PARSER_H
