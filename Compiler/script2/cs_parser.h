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
#include "script/cc_internal.h"

namespace AGS
{

class Parser
{
public:
    // Needs to be public because the manager is initialized outside of Parser
    class FuncLabelMgr
    {
    private:
        ccCompiledScript &_scrip;
        size_t _kind;

    public:
        const size_t _size = 10;

        // 'kind' differentiates between kinds of labels and must be an non-negative integer lower than '_size'.
        // Different function managers must be initialized with different kinds.
        FuncLabelMgr(ccCompiledScript &scrip, int kind);

        // Get the label of a function, in order to insert it into the code,
        // this label will be replaced by its value later on
        CodeCell Function2Label(Symbol func) { return func * _size + _kind; }

        // Keep track of the location of a label that needs to be replaced later on
        void TrackLabelLoc(Symbol func, CodeLoc loc);

        // Give the label that corresponds to 'func' the value 'val'
        void SetLabelValue(Symbol func, CodeCell val);
    };

private:
    // Thrown whenever a compiling run is aborted
    // Don't throw directly, call 'UserError()' or 'InternalError()' instead
    class CompilingError : public std::exception
    {
        std::string _msg;
        const char *what(void) const noexcept { return _msg.c_str(); }

    public:
        CompilingError()
            : _msg("Compiling error")
        {}
        CompilingError(std::string const &msg)
            : _msg(msg)
        {}
    };

    enum FunctionType
    {
        kFT_PureForward = 0,
        kFT_Import = 1,
        kFT_LocalBody = 2,
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
        static int const kNoDefault = INT_MAX;
        static int const kNoJumpOut = INT_MAX;

    private:
        static int _chunkIdCtr; // for assigning unique IDs to chunks

        // All data that is associated with a level of the nested compound statements
        struct NestingInfo
        {
            NSType Type; // Shows what kind of statement this level pertains to (e.g., while or for)
            BackwardJumpDest Start; // of the construct
            ForwardJump JumpOut; // First byte after the construct
            // 'break', 'continue', and 'return' potentially jump out of several nestings.
            // In these cases, 'JumpOutLevel' is set to the level they jump out to.
            // If 'JumpOutLevel' < 'TopLevel' then we have passed a dead end and
            // code execution can't reach the current commands.
            size_t JumpOutLevel; 
            bool DeadEndWarned; // Whether a warning about the dead end has already been issued
            // 'if' and 'switch' have more than one branch. This is set to the _highest_ 
            // jumpout level of the 'returns' etc. encountered in the branches.
            // Only if 'JumpOutLevel' < 'TopLevel' for _all_ the branches, then we can
            // guarantee that execution can't pass the end of this compound statement.
            size_t BranchJumpOutLevel; 
            Vartype SwitchExprVartype; // when switch: the vartype of the switch expression
            std::vector<BackwardJumpDest> SwitchCaseStart; // when switch: code locations of the case labels
            size_t SwitchDefaultIdx; // when switch: code location of the default: label
            ForwardJump SwitchJumptable; // when switch: code location of the "jumptable"
            std::vector<Snippet> Snippets; // Bytecode snippets that must be moved (FOR loops and SWITCH)
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
        inline Vartype SwitchExprVartype() const { return _stack.back().SwitchExprVartype; }
        inline void SetSwitchExprVartype(Vartype vt) { _stack.back().SwitchExprVartype = vt; }

        // If the innermost nesting is a SWITCH, the location of the "default:" code
        inline size_t &SwitchDefaultIdx() { return _stack.back().SwitchDefaultIdx; }

        inline size_t &JumpOutLevel() { return _stack.back().JumpOutLevel; }
        inline bool &DeadEndWarned() { return _stack.back().DeadEndWarned; }
        inline size_t &BranchJumpOutLevel() { return _stack.back().BranchJumpOutLevel; }
        
        // If the innermost nesting is a SWITCH, the location of the code of the switch cases
        inline std::vector<BackwardJumpDest> &SwitchCaseStart() { return _stack.back().SwitchCaseStart; }

        // If the innermost nesting is a SWITCH, the location of the jump table
        inline ForwardJump &SwitchJumptable() { return _stack.back().SwitchJumptable; }
        // If the nesting at the given level is a SWITCH, the location of the  jump table
        inline ForwardJump &SwitchJumptable(size_t level) { return _stack.at(level).SwitchJumptable; }

        // Whether the innermost nesting contains code snippets that must be moved around
        inline bool SnippetsExist() const { return !_stack.back().Snippets.empty(); }
        inline bool SnippetsExist(size_t level) const { return !_stack.at(level).Snippets.empty(); }

        // Code snippets that must be moved around (e.g., in 'do' loops)
        inline std::vector<Snippet> &Snippets() { return _stack.back().Snippets; };
        inline std::vector<Snippet> &Snippets(size_t level) { return _stack.at(level).Snippets; };

        inline void Push(NSType type) { _stack.emplace_back(type, _scrip); }
        inline void Pop() { _stack.pop_back(); };

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

    struct EvaluationResult
    {
        enum Type
        {
            kTY_None = 0,
            kTY_AttributeName,
            kTY_Literal,
            kTY_FunctionName,
            kTY_RunTimeValue,
            kTY_StructName,
        } Type = kTY_None;

        enum Location
        {
            kLOC_None = 0,
            kLOC_MemoryAtMAR, // memory[MAR] after MAR has been updated
            kLOC_AX,
            kLOC_SymbolTable, // in the entry _sym[this->Symbol]
        } Location = kLOC_None;

        Symbol Symbol = kKW_NoSymbol; 
        Vartype Vartype = kKW_NoSymbol;
        bool LocalNonParameter = true;
        bool SideEffects = false;
        bool Modifiable = false;

        EvaluationResult() = default;
    };

    // Track when register values are clobbered
    class RegisterTracking
    {
    public:
        typedef unsigned long TickT;

    private:
        ccCompiledScript &_scrip;
        TickT _register[CC_NUM_REGISTERS];
        std::vector<size_t>_register_list;
        TickT _tick;

    public:
        RegisterTracking(ccCompiledScript &scrip);

        // Track that the previous content of register 'reg' is invalid (has been clobbered)
        // Only consider SREG_AX .. SREG_DX and SREG_MAR
        inline void SetRegister(size_t reg) { _register[reg] = ++_tick; }
        inline void SetRegister(size_t reg, TickT tick) { _register[reg] = tick; }

        // Track that the previous content of all registers is invalid (e.g., after a call)
        // Only consider SREG_AX .. SREG_DX and SREG_MAR
        void SetAllRegisters(void);

        inline TickT GetRegister(size_t reg) const { return _register[reg]; }

        inline TickT GetTick() const { return _tick; }

        // true when the value of register 'reg' that was set at 'tick' is still valid
        // Only consider SREG_AX .. SREG_DX and SREG_MAR
        bool IsValid(size_t reg, TickT tick) const { return _register[reg] <= tick; }

        // Find the general purpose register that was set the longest time ago
        // Only return one of SREG_AX, SREG_BX, SREG_CX, SREG_DX
        size_t GetGeneralPurposeRegister() const;
    } _reg_track;

    // We set the MAR register lazily to save on runtime computation. This object
    // encapsulates the stashed operations that haven't been done on MAR yet.
    class MarMgr
    {
    private:
        Parser &_parser;
        ScopeType _scType;
        size_t _startOffs;
        size_t _componentOffs;

    public:
        MarMgr(Parser &parser);
        MarMgr& MarMgr::operator=(const MarMgr &other);

        // Set the type and the start offset of the MAR register
        void SetStart(ScopeType type, size_t offset);

        inline void AddComponentOffset(size_t offset) { _componentOffs += offset; };

        // Write out the Bytecode necessary to bring MAR up-to-date; reset the object
        void UpdateMAR(size_t lineno, ccCompiledScript &scrip);

        inline bool AreAllOpsPending() const { return ScT::kNone != _scType; };

        void Reset();
    } _marMgr;

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
    MessageHandler &_msgHandler;

    // For functions that have local bodies, manage their start locations
    FuncLabelMgr _callpointLabels = FuncLabelMgr{ _scrip, 0 };

    // For imported functions, manage their import numbers 
    FuncLabelMgr _importLabels = FuncLabelMgr{ _scrip, 1 };

    // Track a forward-declared struct and one of its references to it.
    std::map<Symbol, size_t> _structRefs;

    size_t _lastEmittedSectionId;
    size_t _lastEmittedLineno;

    // Buffer for ccCurScriptName
    std::string _scriptNameBuffer;

    // Augment the message with a "See ..." indication
    // 'declared' is the point in _src where the thing is declared
    std::string const ReferenceMsgLoc(std::string const &msg, size_t declared);
    // Augment the message with a "See ..." indication pointing to the declaration of 'symb'
    std::string const ReferenceMsgSym(std::string const &msg, Symbol symb);

    std::string const TypeQualifierSet2String(TypeQualifierSet tqs) const;

    void SetDynpointerInManagedVartype(Vartype &vartype);

    // Combine the arguments to stname::component, get the symbol for that
    Symbol MangleStructAndComponent(Symbol stname, Symbol component);

    // Skim through 'source', ignoring delimited content completely. Stop in the following cases:
    // .  A symbol is encountered whose type is in 'stoplist[]'
    // .  A closing symbol is encountered that hasn't been opened.
    // Don't consume the symbol that stops the scan.
    void SkipTo(SymbolList const &stoplist, SrcList &source);
    // Skim through 'source'. Stop when 'stopsym' is encountered or a closing symbol
    // that hasn't been opened. Don't consume the symbol that stops the scan.
    inline void SkipTo(Symbol stopsym, SrcList &source) { SkipTo(SymbolList{ stopsym }, source); }

    // Skim through source, ignoring delimited content completely.
    // Stop when a closing symbol is encountered that hasn't been opened.
    // Eat that symbol and if it isn't 'closer', report an _internal_ error.
    void SkipToClose(Predefined closer);

    // If the symbol 'actual' isn't in the list 'expected', give an error.
    // 'custom_msg', if given, replaces the "Expected ..." part of the message
    void Expect(SymbolList const &expected, Symbol actual, std::string const &custom_msg = "");
    // If the symbol 'actual' isn't equal to the symbol 'expected', give an error.
    // 'custom_msg', if given, replaces the "Expected " part of the message
    inline void Expect(Symbol expected, Symbol actual, std::string const &custom_msg = "")
        { Expect(SymbolList{ expected }, actual, custom_msg); }

    // Mark the symbol as "accessed" in the symbol table
    inline void MarkAcessed(Symbol symb) { _sym[symb].Accessed = true; }

    // The size of all local variables that have been allocated at a level higher than from_level
    size_t StacksizeOfLocals(size_t from_level);

    // Does vartype v contain releasable pointers?
    // Also determines whether vartype contains standard (non-dynamic) arrays.
    bool ContainsReleasableDynpointers(Vartype vartype);

    // We're at the end of a block and releasing a standard array of dynpointers.
    // MAR points to the array start. Release each array element (dynpointer).
    void FreeDynpointersOfStdArrayOfDynpointer(size_t num_of_elements);

    // We're at the end of a block and releasing all the dynpointers in a struct.
    // MAR already points to the start of the struct.
    void FreeDynpointersOfStruct(Vartype struct_vtype);

    // We're at the end of a block and we're releasing a standard array of struct.
    // MAR points to the start of the array. Release all the pointers in the array.
    void FreeDynpointersOfStdArrayOfStruct(Vartype element_vtype, size_t num_of_elements);

    // We're at the end of a block and releasing a standard array. MAR points to the start.
    // Release the pointers that the array contains.
    void FreeDynpointersOfStdArray(Symbol the_array);

    // Free the pointers of any locals in level from_level or higher
    void FreeDynpointersOfLocals(size_t from_level);

    // Free the pointers of all locals at the end of a function. This function
    // returns a Dynarray or Dynpointer, so the result must be protected from
    // dropping its last reference, which could mean the pointer is released prematurely.
    void FreeDynpointersOfAllLocals_DynResult(void);

    // Free the pointers of all locals at the end of a function. Do not clobber AX.
    void FreeDynpointersOfAllLocals_KeepAX(void);

    // Restore those definitions that have a nesting level of 'from_level' or higher
    // to what they were on the level 'from_level - 1'.
    void RestoreLocalsFromSymtable(size_t from_level);

    // Remove at nesting_level or higher.
    void RemoveLocalsFromStack(size_t nesting_level);

    // Record the literal as a compile time literal 
    void SetCompileTimeLiteral(Symbol lit, EvaluationResult &eres);

    // Find or create a symbol that is a literal for the value 'value'.
    void FindOrAddIntLiteral(CodeCell value, Symbol &symb);

    // We're parsing a parameter list and we have accepted something like "(...int i"
    // We accept a default value clause like "= 15" if it follows at this point.
    // If there isn't any default, kKW_NoSymbol is returned.
    // Otherwise, a symbol is returned that is a literal.
    Symbol ParseParamlist_Param_DefaultValue(size_t idx, Vartype param_vartype);

    // We have accepted something like 'int foo' and a trailing '[]' might follow.
    // If it does, convert 'vartype' to a dynarray.
    void ParseDynArrayMarkerIfPresent(Vartype &vartype);

    // Extender function, eg. 'function GoAway(this Character *someone)'
    // We've just accepted something like 'int func('
    // We'll accept something like 'this Character *' --OR-- 'static Character'
    void ParseFuncdecl_ExtenderPreparations(bool is_static_extender, Symbol &struct_of_func, Symbol &name_of_func, TypeQualifierSet &tqs);

    // We're accepting a parameter list. We've accepted something like 'int'.
    // We accept a param name such as 'i' if present
    Symbol ParseParamlist_Param_Name(bool body_follows);

    // Additional handling to ParseVardecl_Var2SymTable() that is special for parameters
    void ParseParamlist_Param_AsVar2Sym(Symbol param_name, TypeQualifierSet tqs, Vartype param_vartype, int param_idx);

    // Process a parameter decl in a function parameter list
    void ParseParamlist_Param(Symbol name_of_func, bool body_follows, TypeQualifierSet tqs, Vartype param_vartype, size_t param_idx);

    // Process a function parameter list
    void ParseFuncdecl_Paramlist(Symbol funcsym, bool body_follows);

    void ParseFuncdecl_MasterData2Sym(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_function, Symbol name_of_function, bool body_follows);

    // There was a forward declaration -- check that the real declaration matches it
    void ParseFuncdecl_CheckThatKnownInfoMatches(std::string const &func_name, SymbolTableEntry::FunctionDesc const *this_entry, SymbolTableEntry::FunctionDesc const *known_info, size_t declared, bool body_follows);

    // Enter the function in the 'imports[]' or 'functions[]' array of '_script'; get its index   
    void ParseFuncdecl_EnterAsImportOrFunc(Symbol name_of_func, bool body_follows, bool func_is_import, size_t num_of_parameters, CodeLoc &function_soffs);

    // We're at something like 'int foo(', directly before the '('
    // Return in 'body_follows' whether the symbol that follows the corresponding ')' is '{'
    bool ParseFuncdecl_DoesBodyFollow();

    // We're in a func decl. Check whether the declaration is valid.
    void ParseFuncdecl_Checks(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, Vartype return_vartype, bool body_follows, bool no_loop_check);

    void ParseFuncdecl_HandleFunctionOrImportIndex(TypeQualifierSet tqs, Symbol struct_of_func, Symbol name_of_func, bool body_follows);

    // Parse a function declaration.
    // We're behind the opening '(', and any first extender parameter has already be resolved.
    void ParseFuncdecl(TypeQualifierSet tqs, Vartype return_vartype, Symbol struct_of_func, Symbol name_of_func, bool no_loop_check, bool body_follows);

    // Return in 'idx' the index of the operator in the list that binds the least
    // so that either side of it can be evaluated first. '-1' if no operator was found
    int IndexOfLeastBondingOperator(SrcList &expression);

    // Return in 'opcode' the opcode that corresponds to the operator 'op_sym'
    // when its parameters have the vartypes 'vartype1' and 'vartype2', respectively.
    // Check whether the operator 'op_sym' can handle the types at all
    CodeCell GetOpcode(Symbol op_sym, Vartype vartype1, Vartype vartype2);

    // Check for a type mismatch in one direction only (does not yield a UserError if there is)
    bool IsVartypeMismatch_Oneway(Vartype vartype_is, Vartype vartype_wants_to_be) const;

    // Check whether there is a type mismatch; if so, give an error. 'msg' for specializing the error message
    void CheckVartypeMismatch(Vartype vartype_is, Vartype vartype_wants_to_be, bool orderMatters, std::string const &msg = "");

    // 'current_vartype' must be the vartype of AX. If it is 'string' and
    // wanted_vartype is 'String', then AX will be converted to 'String'.
    // then convert AX into a String object and set its type accordingly
    void ConvertAXStringToStringObject(Vartype wanted_vartype, Vartype &current_vartype);

    static int GetReadCommandForSize(int the_size);

    static int GetWriteCommandForSize(int the_size);

    // Handle the cases where a value is a whole array or dynarray or struct
    void HandleStructOrArrayResult(EvaluationResult &eres);

    // If the result isn't in AX, move it there. Dereferences a pointer
    void EvaluationResultToAx(EvaluationResult &eres);

    // We're in the parameter list of a function call, and we have less parameters than declared.
    // Provide defaults for the missing values
    void AccessData_FunctionCall_ProvideDefaults(int num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    void AccessData_FunctionCall_PushParams(SrcList &parameters, size_t closed_paren_idx, size_t num_func_args, size_t num_supplied_args, Symbol funcSymbol, bool func_is_import);

    // Count parameters, check that all the parameters are non-empty; find closing paren
    void AccessData_FunctionCall_CountAndCheckParm(SrcList &parameters, Symbol name_of_func, size_t &index_of_close_paren, size_t &num_supplied_args);

    // We are processing a function call. General the actual function call
    void AccessData_GenerateFunctionCall(Symbol name_of_func, size_t num_args, bool func_is_import);

    // Generate the function call for the function that returns the number of elements
    // of a dynarray.
    void AccessData_GenerateDynarrayLengthFuncCall(EvaluationResult &eres);

    // We are processing a function call.
    // Get the parameters of the call and push them onto the stack.
    // Return the number of the parameters pushed
    void AccessData_PushFunctionCallParams(Symbol name_of_func, bool func_is_import, SrcList &parameters, size_t &actual_num_args);

    // Process a function call. The parameter list begins with 'expression[1u]' (!)
    void AccessData_FunctionCall(Symbol name_of_func, SrcList &expression, EvaluationResult &eres);

    // Evaluate 'vloc_lhs op_sym vloc_rhs' at compile time, return the result in 'vloc'.
    // Return whether this is possible.
    bool ParseExpression_CompileTime(Symbol op_sym, EvaluationResult const &eres_lhs, EvaluationResult const &eres_rhs, EvaluationResult &eres);

    // Check the vartype following 'new'
    void ParseExpression_CheckArgOfNew(Vartype new_vartype);

    // Parse the term given in 'expression'. The lowest-binding operator is unary 'new'
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_New(SrcList &expression, EvaluationResult &eres);

    // Parse the term given in 'expression'. The lowest-binding operator is unary '-'
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_PrefixMinus(SrcList &expression, EvaluationResult &eres);

    // Parse the term given in 'expression'. The lowest-binding operator is unary '+'
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_PrefixPlus(SrcList &expression, EvaluationResult &eres);

    // Parse the term given in 'expression'. The lowest-binding operator is a boolean or bitwise negation
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_PrefixNegate(Symbol operation, SrcList &expression, EvaluationResult &eres);

    // Parse the term given in 'expression'. The lowest-binding operator is '++' or '--'.
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_PrefixCrement(Symbol op_sym, SrcList &expression, EvaluationResult &eres);
    // Parse the term given in 'expression'. The lowest-binding operator is '++' or '--'.
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_PostfixCrement(Symbol op_sym, SrcList &expression, EvaluationResult &eres);

    // Parse literal int with the lowest possible value
    // This will arrive at the parser as '-' '2147483648'
    void ParseExpression_LongMin(EvaluationResult &eres);

    // If consecutive parentheses surround the expression, strip them.
    void StripOutermostParens(SrcList &expression);

    // Parse the term given in 'expression'. The lowest-binding operator is a unary operator
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_Prefix(SrcList &expression, EvaluationResult &eres);

    // Parse the term given in 'expression'. The lowest-binding operator is a unary operator
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    // If result_used == false then the calling function doesn't use the term result for calculating
    // This happens when a term is called for side effect only, e.g. in the statement 'foo++;'
    void ParseExpression_Postfix(SrcList &expression, EvaluationResult &eres, bool result_used);

    void ParseExpression_Ternary_Term2(EvaluationResult &eres_term1, bool term1_has_been_ripped_out,
        SrcList &term2, EvaluationResult &eres_term2, bool result_used);

    // Parse the term given in 'expression'. Expression is a ternary 'a ? b : c'
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    // If result_used == false then the calling function doesn't use the term result for calculating
    // This happens when a term is called for side effect only, e.g. in the statement 'i ? --foo : ++foo;'
    void ParseExpression_Ternary(size_t tern_idx, SrcList &expression, EvaluationResult &eres, bool result_used);

    // Parse the term given in 'expression'. The lowest-binding operator a binary operator.
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    void ParseExpression_Binary(size_t op_idx, SrcList &expression, EvaluationResult &eres);

    // Parse the term given in 'expression'. Expression begins with '('
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    // If result_used == false then the calling function doesn't use the term result for calculating
    // This happens when a term is called for side effect only, e.g. in the statement '(--foo);'
    void ParseExpression_InParens(SrcList &expression, EvaluationResult &eres, bool result_used);

    // Parse the term given in 'expression'. Expression does not contain operators
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    // If result_used == false then the calling function doesn't use the term result for calculating
    // This happens when a term is called for side effect only, e.g. in the statement '--foo;'
    void ParseExpression_NoOps(SrcList &expression, EvaluationResult &eres, bool result_used);

    // Check whether spurious symbols exist after a subterm is processed
    void ParseExpression_CheckUsedUp(AGS::SrcList &expression);

    // Parse the term given in 'expression'.
    // 'expression' is parsed from the beginning. The term must use up 'expression' completely.
    // If result_used == false then the calling function doesn't use the term result for calculating
    // This happens when a term is called for side effect only, e.g. in the statement '--foo;'
    void ParseExpression_Term(SrcList &expression, EvaluationResult &eres, bool result_used = true);

    // Parse an expression that must evaluate to a constant at compile time.
    // Return the symbol that signifies the constant.
    // 'src' may be longer than the expression. In this case, leave src pointing to last token in expression.
    // If 'msg' is specified, it is used for targeted error messages.
    // 'src' is parsed from the point where the cursor is.
    Symbol ParseConstantExpression(SrcList &src, std::string const &msg = "");

    // Parse an expression that must convert to an int.
    // 'src' may be longer than the expression. In this case, leave src pointing to last token in expression.
    // 'src'  is parsed from the point where the cursor is.
    void ParseIntegerExpression(SrcList &src, EvaluationResult &eres, std::string const &msg = "");
    
    // Parse expression in delimiters, e.g., parentheses
    // 'src' may be longer than the expression. In this case, leave src pointing to last token in expression.
    // 'src'  is parsed from the point where the cursor is.
    void ParseDelimitedExpression(SrcList &src, Symbol opener, EvaluationResult &eres);

    // Parse and evaluate an expression
    // 'src' may be longer than the expression. In this case, leave src pointing to last token in expression.
    // 'src'  is parsed from the point where the cursor is.

    void ParseExpression(SrcList &src, EvaluationResult &eres);

    // We access a variable or a component of a struct in order to read or write it.
    // This is a simple member of the struct.
    void AccessData_StructMember(Symbol component, VariableAccess access_type, bool access_via_this, SrcList &expression, EvaluationResult &eres);

    // Return the symbol for the get or set function corresponding to the attribute given.
    Symbol ConstructAttributeFuncName(Symbol attribsym, bool is_setter, bool is_indexed);

    // We call the getter or setter of an attribute
    void AccessData_CallAttributeFunc(bool is_setter, SrcList &expression, Vartype vartype);

    // Memory location contains a pointer to another address. Get that address.
    void AccessData_Dereference(EvaluationResult &eres);

    // Process one index in a sequence of array indexes
    void AccessData_ProcessCurrentArrayIndex(size_t idx, size_t dim, size_t factor, bool is_dynarray, SrcList &expression, EvaluationResult &eres);

    // We're processing some struct component or global or local variable.
    // If a sequence of array indexes follows, parse it and shorten symlist accordingly
    void AccessData_ProcessArrayIndexIfThere(SrcList &expression, EvaluationResult &eres);

    void AccessData_Variable(VariableAccess access_type, SrcList &expression, EvaluationResult &eres);

    void AGS::Parser::AccessData_This(EvaluationResult &eres);

    // We're getting a variable, literal, constant, func call or the first element
    // of a STRUCT.STRUCT.STRUCT... cascade.
    // This moves the cursor in all cases except for the cascade to the end of what is parsed,
    // and in case of a cascade, to the end of the first element of the cascade, i.e.,
    // to the position of the '.'.
    // The "return_scope_type" is used for deciding what values can be returned from a function.
    // implied_this_dot is set if subsequent processing should imply that
    // the expression starts with "this.", with the '.' already read in
    void AccessData_FirstClause(VariableAccess access_type, SrcList &expression, EvaluationResult &eres, bool &implied_this_dot);

    // We're processing a STRUCT.STRUCT. ... clause.
    // We've already processed some structs, and the type of the last one is vartype.
    // Now we process a component of vartype.
    void AccessData_SubsequentClause(VariableAccess access_type, bool access_via_this, SrcList &expression, EvaluationResult &eres);

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
    bool AccessData_IsClauseLast(SrcList &expression);

    // Access a variable, constant, literal, func call, struct.component.component cascade, etc.
    // Result is in AX or m[MAR], dependent on vloc. Variable type is in vartype.
    // At end of function, symlist and symlist_len will point to the part of the symbol string
    // that has not been processed yet
    void AccessData(VariableAccess access_type, SrcList &expression, EvaluationResult &eres);

    // Insert Bytecode for:
    // Copy at most OLDSTRING_SIZE-1 bytes from m[MAR...] to m[AX...]
    // Stop when encountering a 0
    void AccessData_StrCpy();

    // An optional '*' can follow at this point. If it is here, eat it.
    void EatDynpointerSymbolIfPresent(Vartype vartype);

    // We are typically in an assignment LHS = RHS; the RHS has already been
    // evaluated, and the result of that evaluation is in AX.
    // Store AX into the memory location that corresponds to LHS, or
    // call the attribute function corresponding to LHS.
    void AccessData_AssignTo(SrcList &expression, EvaluationResult eres);

    void SkipToEndOfExpression(SrcList &src);

    // We are parsing the left hand side of a '+=' or similar statement.
    void ParseAssignment_ReadLHSForModification(SrcList &lhs, EvaluationResult &eres);

    // "var = expression"; 'lhs' is the variable
    void ParseAssignment_Assign(SrcList &lhs);

    // We compile something like 'var += expression'
    void ParseAssignment_MAssign(Symbol ass_symbol, SrcList &lhs);

    // 'const int foo = 77;'
    void ParseConstantDefn();

    void ParseVardecl_InitialValAssignment_IntOrFloatVartype(Vartype var, std::vector<char> &initial_val);

    void ParseVardecl_InitialValAssignment_OldString(std::vector<char> &initial_val);

    // Parse the assignment of an initial value to a variable. 
    void ParseVardecl_InitialValAssignment(Symbol varname, std::vector<char> &initial_val);

    // Move variable information into the symbol table
    void ParseVardecl_Var2SymTable(Symbol var_name, Vartype vartype);

    // we have accepted something like "int a" and we're expecting "["
    void ParseArray(Symbol vname, Vartype &vartype);

    void ParseVardecl_CheckIllegalCombis(Vartype vartype, ScopeType scope_type);

    // there was a forward declaration -- check that the real declaration matches it
    void ParseVardecl_CheckThatKnownInfoMatches(SymbolTableEntry *this_entry, SymbolTableEntry *known_info, bool body_follows);

    void ParseVardecl_Global(Symbol var_name, Vartype vartype);

    void ParseVardecl_Import(Symbol var_name);

    void ParseVardecl_Local(Symbol var_name, Vartype vartype);

    void ParseVardecl0(Symbol var_name, Vartype vartype, ScopeType scope_type, TypeQualifierSet tqs);

    // Checks whether an old definition exists that may be stashed; stashes it if possible
    void ParseVardecl_CheckAndStashOldDefn(Symbol var_name);

    void ParseVardecl(TypeQualifierSet tqs, Vartype vartype, Symbol var_name, ScopeType scope_type);

    void ParseFuncBodyStart(Symbol struct_of_func, Symbol name_of_func);

    void HandleEndOfFuncBody(Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // Helper for ParseStruct_CheckForwardDecls()
    void ParseStruct_GenerateForwardDeclError(Symbol stname, TypeQualifierSet tqs, TypeQualifier tq, VartypeFlag vtf);

    // If there are forward declarations, check that their type qualifiers match 
    void ParseStruct_CheckForwardDecls(Symbol stname, TypeQualifierSet tqs);

    void ParseStruct_SetTypeInSymboltable(Symbol stname, TypeQualifierSet tqs);

    // We have accepted something like "struct foo" and are waiting for "extends"
    void ParseStruct_ExtendsClause(Symbol stname);

    void ParseQualifiers(TypeQualifierSet &tqs);

    void ParseStruct_FuncDecl(Symbol struct_of_func, Symbol name_of_func, TypeQualifierSet tqs, Vartype vartype);

    void ParseStruct_Attribute_ParamList(Symbol struct_of_func, Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype);

    void ParseStruct_Attribute_CheckFunc(Symbol name_of_func, bool is_setter, bool is_indexed, Vartype vartype);

    // We are processing an attribute.
    // This corresponds to a getter func and a setter func, declare one of them
    void ParseStruct_Attribute_DeclareFunc(TypeQualifierSet tqs, Symbol strct, Symbol qualified_name, Symbol unqualified_name, bool is_setter, bool is_indexed, Vartype vartype);

    void ParseStruct_Attribute2SymbolTable(TypeQualifierSet tqs, Vartype const vartype, Symbol const name_of_struct, Symbol const unqualified_attribute, bool const is_indexed);

    // We're in a struct declaration. Parse an attribute declaration.
    void ParseStruct_Attribute(TypeQualifierSet tqs, Symbol stname);

    // We're inside a struct decl, processing a member variable
    void ParseStruct_VariableDefn(TypeQualifierSet tqs, Vartype curtype, Symbol stname, Symbol vname);
    
    // We're inside a struct decl, processing a compile-time constant
    void ParseStruct_ConstantDefn(Symbol name_of_struct);

    // We have accepted something like 'struct foo extends bar { const int'.
    // We're waiting for the name of the member.
    void ParseStruct_VariableOrFunctionDefn(Symbol name_of_struct, TypeQualifierSet tqs, Vartype vartype);

    void ParseStruct_CheckComponentVartype(Symbol stname, Vartype vartype);

    // We've accepted, e.g., 'struct foo {'. Now we're parsing a variable declaration or a function declaration
    void ParseStruct_Vartype(Symbol name_of_struct, TypeQualifierSet tqs);

    // Within a struct, we've accepted a type. Accept a list of function or variable members
    void ParseStruct_Vartype_MemberList(TypeQualifierSet tqs, const AGS::Symbol containing_struct, Vartype vartype);

    // Handle a 'struct' definition clause
    void ParseStruct(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // We've accepted something like 'enum foo { bar'; '=' follows
    void ParseEnum_AssignedValue(Symbol vname, CodeCell &value);

    // Define the item 'item_name' to have the value 'value'. 
    void ParseEnum_Item2Symtable(Symbol enum_name, Symbol item_name, int value);

    void ParseEnum_Name2Symtable(Symbol enum_name);

    // Parse an enum declaration, possibly followed by vars of this new enum
    void ParseEnum(TypeQualifierSet tqs, Symbol &struct_of_current_func, Symbol &name_of_current_function);

    // Read a vartype (that must already be defined at this point)
    // This is either a vartype name or 'const string'
    Symbol ParseVartype(bool with_dynpointer_handling = true);

    void ParseExport_Function(Symbol func);
    void ParseExport_Variable(Symbol var);
    void ParseExport();

    void ParseReturn(Symbol name_of_current_func);

    // Helper function for parsing a varname
    void ParseVarname0(bool accept_member_access, Symbol &structname, Symbol &varname);

    // Parse a variable name; may contain '::'
    // If it does contain '::' then varname will contain the qualified name (a::b) and structname the vartype (a)
    inline void ParseVarname(Symbol &structname, Symbol &varname) { ParseVarname0(true, structname, varname); }
    // Parse a variable name; may not contain '::'
    inline Symbol ParseVarname() { Symbol dummy, varname; ParseVarname0(false, dummy, varname); return varname; }

    void ParseVartype_CheckForIllegalContext();

    void ParseVartype_CheckIllegalCombis(bool is_function, TypeQualifierSet tqs);

    void ParseVartype_FuncDecl(TypeQualifierSet tqs, Vartype vartype, Symbol struct_name, Symbol func_name, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func, bool &body_follows);

    void ParseVartype_VarDecl_PreAnalyze(Symbol var_name, ScopeType scope_type);

    void ParseAttribute(TypeQualifierSet tqs, Symbol name_of_current_func);

    void ParseVartype_VariableDefn(TypeQualifierSet tqs, Vartype vartype, Symbol var_name, ScopeType scope_type);

    void ParseVartype_MemberList(TypeQualifierSet tqs, Vartype vartype, ScopeType scope_type, bool no_loop_check, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // We accepted a variable type such as 'int', so what follows is a variable, compile-time constant, or function declaration
    void ParseVartypeClause(TypeQualifierSet tqs, Symbol &name_of_current_func, Symbol &struct_of_current_func);

    // After a command that might be the end of sequences such as 'if (...) while (...) command;'
    //  Find out what surrounding compound statements have ended and handle these endings.
    void HandleEndOfCompoundStmts();

    // Handle the end of a statement sequence in braces
    void HandleEndOfBraceCommand();

    // Parse the head of a 'do ... while(...)' clause
    void ParseDo();

    // Handle the end of a 'do' clause and the 'while' clause following it
    void HandleEndOfDo();

    // Handle the end of an 'else' clause
    void HandleEndOfElse();

    // The first clause of 'for (I; W; C);' when it is a declaration
    void ParseFor_InitClauseVardecl();

    // The first clause of 'for (I; W; C);'
    void ParseFor_InitClause(Symbol peeksym);

    // The middle clause of 'for (I; W; C);'
    void ParseFor_WhileClause();

    // The last clause of 'for (I; W; C);'
    void ParseFor_IterateClause();

    // Handle the head of 'for (I; W; C);'
    void ParseFor();

    // Evaluate an 'if' clause, e.g. 'if (i < 0)'
    void ParseIf();

    // Handle the end of an 'if' body
    void HandleEndOfIf(bool &else_follows);

    // Parse, e.g., "switch (bar)"
    void ParseSwitch();

    // We're in a 'switch' body. Parse 'fallthrough;'
    void ParseSwitchFallThrough();

    // We're in a 'switch' body. Parse 'case foo:' or 'default:'
    void ParseSwitchLabel(Symbol case_or_default);

    // Handle the end of a 'switch' body
    void HandleEndOfSwitch();

    // Parse the head of a 'while' statement, e.g., 'while (i < 0)'
    // NOTE: This function NOT responsible for handling the 'while'
    // clause of  a 'do ... while' statement.
    void ParseWhile();

    // Handle the end of a 'while' body
    // Also handle the outer 'for' nesting.
    void HandleEndOfWhile();

    // We're compiling function body code; the code does not start with a keyword or type.
    // Thus, we should be at the start of an assignment or a funccall. Compile it.
    // No symbols of the statement have been consumed yet.
    void ParseAssignmentOrExpression();

    // Parse a 'break;' statement
    void ParseBreak();

    // Parse a 'continue;' statement
    void ParseContinue();

    void ParseOpenBrace(Symbol struct_of_current_func, Symbol name_of_current_func);

    // Parse a command. The leading symbol has already been eaten
    void ParseCommand(Symbol leading_sym, Symbol &struct_of_current_func, Symbol &name_of_current_func);

    // Execute 'block' that will presumably emit Bytecode.
    // If that Bytecode clobbers any register in 'guarded_registers',
    // emit an enclosing 'PushReg(register)' / 'PopReg(register)' around that Bytecode
    void RegisterGuard(RegisterList const &guarded_registers, std::function<void(void)> block);
    // Execute 'block' that will presumably emit Bytecode.
    // If that Bytecode clobbers the register 'guarded_register',
    // emit an enclosing 'PushReg(register)' / 'PopReg(register)' around that Bytecode
    void RegisterGuard(size_t guarded_register, std::function<void(void)> block)
        { return RegisterGuard(RegisterList{ guarded_register }, block); }

    // If a new section has begun at cursor position pos, tell _scrip to deal with that.
    // Refresh ccCurScriptName
    void HandleSrcSectionChangeAt(size_t pos);

    // Emit an opcode without parameters
    inline void WriteCmd(CodeCell op)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op); }
    // Emit an opcode and one parameter
    inline void WriteCmd(CodeCell op, CodeCell p1)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op, p1); }
    // Emit an opcode and 2 parameters
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op, p1, p2); }
    // Emit an opcode and 3 parameters
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2, CodeCell p3)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.WriteCmd(op, p1, p2, p3); }

    // Emit a PUSH; keep track of the size of the block on the stack that contains pushed values
    inline void PushReg(CodeCell reg)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.PushReg(reg); }
    // Emit a POP; keep track of the size of the block on the stack that contains pushed values
    inline void PopReg(CodeCell reg)
        { _scrip.RefreshLineno(_src.GetLineno()); _scrip.PopReg(reg); }

    // Make sure that the qualifiers that accumulated for this decl go together
    void Parse_CheckTQ(TypeQualifierSet tqs, bool in_func_body, bool in_struct_decl);
    // Make sure that no qualifiers have accumulated for this decl
    void Parse_CheckTQSIsEmpty(TypeQualifierSet tqs);

    // Analyse the decls and collect info about locally defined functions
    // This is a pre phase that only does simplified analysis
    void Parse_PreAnalyzePhase();

    // Generate code
    void Parse_MainPhase();

    void ParseInput();

    // Only certain info should be carried over from the pre phase into the main phase.
    // Discard all the rest so that the main phase can start with a clean slate.
    void Parse_ReinitSymTable(size_t size_after_scanning);

    // Check whether a forward-declared struct has actually been referenced and never defined
    void Parse_CheckForUnresolvedStructForwardDecls();

    // Sanity check for the fixups
    void Parse_CheckFixupSanity();

    void Parse_ExportAllFunctions();

    // Blank out all imports that haven't been referenced
    void Parse_BlankOutUnusedImports();

    // Enter msg into message handler, throw exception
    void Error(bool is_internal, std::string const &message);

    // Report an user error for the section and lineno that _src currently is at; will throw exception
    void UserError(char const *descr, ...);

    // Report an internal error for the section and lineno that _src currently is at; will throw exception
    void InternalError(char const *descr, ...);

    // Record a warning for the current source position; will NOT throw exception
    void Warning(char const *descr, ...);

public:
    Parser(SrcList &src, FlagSet options, ccCompiledScript &scrip, SymbolTable &symt, MessageHandler &mh);

    void Parse();

}; // class Parser
} // namespace AGS

// Compile the input, return any messages in mh, cc_error() does not get called
extern int cc_compile(
    std::string const &source,  // preprocessed text to be compiled
    AGS::FlagSet options,            // as defined in cc_options 
    AGS::ccCompiledScript &scrip,    // store for the compiled text
    AGS::MessageHandler &mh);        // warnings and the error   

#endif // __CS_PARSER_H
