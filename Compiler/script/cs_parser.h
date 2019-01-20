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
    Recognize structs and prepend "." to struct component names
These two steps are piped. They are performed separately _before_ the Parsing (below) begins.
The result is:
    the symbol table, a global sym that is a struct symbolTable.
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
#include <deque>
#include <string>
#include <sstream>

#include "cc_compiledscript.h"
#include "cc_internallist.h"
#include "cc_symboltable.h"


namespace AGS
{
/// \brief Scans the input, returning the symstrings one by one.
/// In this context, a "symstring" is defined to mean the string. 
/// When the symstring (i.e., the string) is entered into the symbol database, it becomes a "token". 
/// A "token" is defined to mean the integer that represents the symstring.
class Scanner
{
public:
    /// These types represent the different kinds of symstring that can be scanned.
    /// Since the symstrings haven't become tokens yet, this is just rudimentary information.
    enum ScanType
    {
        kSct_Unspecified = 0,
        kSct_Identifier,      ///< Identifier or keyword --- [A-Za-z][A-Za-z_]*
        kSct_FloatLiteral,    ///< Numbers containing a "." --- [0-9]+[.][0-9]*
        kSct_IntLiteral,      ///< Numbers not containing a "." --- [0-9]+
        kSct_StringLiteral,   ///< Quoted strings --- ["]([\\].[^"]*)*["]
        kSct_NonChar          ///< i.e., +, ++, /=; this can be one character or two characters
    };

    // ctors
    Scanner();
    Scanner(std::string const & Input, std::size_t _lineno, struct ::ccInternalList * _tokenList);

    // setters and getters
    void SetInput(const std::string &input);
    void SetLineno(std::size_t lineno);
    std::size_t GetLineno();
    void SetTokenList(struct ::ccInternalList *tokenList);

    /// If the input couldn't be scanned, this will explain the problem
    const std::string GetLastError();

    /// Get the next symstring from the input.
    /// \param[out] symstring  The symstring from the input
    /// \param[out] eof_encountered  Is true if the input is exhausted
    /// \param[out] error_encountered Is true if the input couldn't be scanned; details in GetLastError() 
    void GetNextSymstring(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered);

protected:
    // Don't use std::isdigit et al. here because those are locale dependent and we don't want that.
    inline bool isdigit(int ch) { return (ch >= '0' && ch <= '9'); }
    inline bool isupper(int ch) { return (ch >= 'A' && ch <= 'Z'); }
    inline bool islower(int ch) { return (ch >= 'a' && ch <= 'z'); }
    inline bool isspace(int ch) { return (std::strchr(" \t\n\v\f\r", ch) != 0); }

    inline void WriteNewLinenoMeta(int ln) { const Symbol smeta_linenum = (Symbol)1; _tokenList->write_meta(smeta_linenum, ln); }

private:
    std::istringstream _inputStream;
    std::size_t _lineno;
    ccInternalList *_tokenList;
    std::string _lastError;

    /// Skip through the input, ignoring it, until a non-whitespace is found. Don't eat the non-whitespace.
    void SkipWhitespace(bool &eof_encountered, bool &error_encountered);

    ///  Read in either an int literal or a float literal
    void ReadInNumberLit(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered);

    /// Read in a character literal; converts it internally into an equivalent int literal
    void ReadInCharLit(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    /// Read in a string literal
    void ReadInStringLit(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    /// Read in an identifier or a keyword 
    void ReadInIdentifier(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    /// Read in a single-char symstring
    void ReadIn1Char(std::string & symstring);

    /// \brief Read in a single-char symstring (such as "*"), or a double-char one (such as "*=")
    /// A double-char symstring is detected if and only if the second char is in PossibleSecondChars.
    /// Otherwise, a one-char symstring is detected and the second char is left for the next call
    void ReadIn1or2Char(
        const std::string &possible_second_chars,
        std::string &symstring,
        bool &eof_encountered,
        bool &error_encountered);

    /// Read in a symstring that begins with ".". This might yield a one- or three-char symstring.
    void ReadInDotCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered);


    /// Read in a symstring that begins with "<". This might yield a one-, two- or three-char symstring.
    void ReadInLTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    /// Read in a symstring that begins with ">". This might yield a one-, two- or three-char symstring.
    void ReadInGTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered);
};


/// Scans symstrings as necessary (through the scanner), converts them to tokens and returns tokens one-by-one.
/// Strings are passed to the string collector.
class Tokenizer
{
public:
    enum Modes
    {
        kMode_Standard = 0, ///< Tokenizer isn't in any special mode
        kMode_StructDecl,   ///< Tokenizer is in a struct declaration
    };

    // ctors
    Tokenizer();

    // Note: The tokenizer does not "own" the objects below: 
    // When it destructs, it will make no attempt to destruct *scanner etc.
    Tokenizer(
        Scanner *scanner,
        struct ::ccInternalList *token_list,
        struct ::SymbolTable *symbol_table,
        struct ::ccCompiledScript *string_collector);

    // setters
    void SetScanner(Scanner *scanner);
    void SetTokenList(struct ::ccInternalList * token_list);
    void SetSymbolTable(struct ::SymbolTable * symbol_table);
    void SetStringCollector(struct ::ccCompiledScript * string_collector);

    /// Initialize internal variables
    void Reset();

    /// Get next token, reading symstrings as necessary
    void GetNextToken(
        int &token,                 // The token that is read
        bool &eof_encountered,      // Is true if symstring couldn't be tokenized; details in GetLastError()
        bool &error_encountered);   // Is true if the scanner should reset the line numbering

    /// If Tokenizer couldn't tokenize a symstring, explain why
    const std::string GetLastError();

    /// The tokenizer gives some symbols a temporary type; reset those types
    void ResetTemporaryTypesInSymbolTable();


protected:
    // Does the real work processing the scanner symstring
    void ProcessScannerSymstring(Scanner::ScanType symbol_type, std::string & symstring, int last_token, int &token, bool & eof_encountered, bool & error_encountered);

    void TokenizeStringLiteral(int token, std::string const &symstring);

    // Collect parentheses, brackets, braces as they come up; check whether they match
    void CheckMatcherNesting(int token, bool &error_encountered);

    // Given the token of a struct name and a member name, generate a string denoting the full name of the member
    std::string FullNameFromStructAndMember(int struct_name_token, int member_name_token);

    int ConvertSymstringToTokenIndex(std::string symstring);

private:
    // Collect a sequence of opening ("([{") and closing (")]}") symbols; check matching
    class OpenCloseMatcher
    {
    public:
        // c'tor
        OpenCloseMatcher();

        // Parameters: an opening thing, the thing that is expected to close it later, the current line number
        // Pushes that onto a stack
        void Push(std::string const &opener, std::string const &expected_closer, int lineno);

        // Input a closing thing; this is matched against the symbol of the "put" method earlier.
        // If they don't match, an error is generated.
        void PopAndCheck(std::string const &closer, int lineno, bool &error_encountered);

        // Get the last error encountered.
        std::string GetLastError();

        // Reset the matcher.
        void Reset();


    private:
        struct OpenInfo
        {
            std::string _opener;
            std::string _closer;
            int _lineno;  // of the Opener symbol
        };
        std::vector<OpenInfo> _openInfoStack;
        std::string _lastError;
    };

private:
    Scanner *_scanner;
    struct ::ccInternalList *_tokenList;
    struct ::SymbolTable *_symbolTable;
    struct ::ccCompiledScript *_stringCollector;

    std::string _lastError;
    OpenCloseMatcher _ocMatcher;

    enum Modes _currentMode;
    bool _inTypeSubmode;
    int _braceNestingDepthInStructDecl; // only used in StructDeclMode
    int _structBeingDeclared;
    int _parenthesisNestingDepth;
    std::deque<int> _tokenBuffer;

    inline Symbol TokenType(int token) { return _symbolTable->entries[token].stype; }
    inline std::string TokenName(int token) { return _symbolTable->entries[token].sname; }
    inline void SetTokenType(int token, Symbol value) { _symbolTable->entries[token].stype = value; }
    inline void SetTokenOffsetInStrings(int token, int value) { _symbolTable->entries[token].soffs = value; }
    inline void SetTokenVartype(int token, Symbol value) { _symbolTable->entries[token].vartype = value; }
};


// A section of compiled code that needs to be moved or copied to a new location
struct ccChunk
{
    std::vector<AGS::CodeCell> Code;
    std::vector<int32_t> Fixups;
    std::vector<char> FixupTypes;
    int CodeOffset;
    int FixupOffset;
};

// All data that is associated with a nesting level
struct NestingInfo
{
    int Type; // Type of the level, see AGS::NestingStack::NestingType
    std::int32_t StartLoc; // Index of the first byte generated for the level
    std::int32_t Info; // Various uses that differ by nesting type
    std::int32_t DefaultLabelLoc; // Location of default label
    std::vector<ccChunk> Chunks; // Bytecode chunks that must be moved (FOR loops and SWITCH)
};

// The stack of nesting statements 
class NestingStack
{
private:
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
    inline size_t Depth() { return _stack.size(); };

    // Type of the innermost nesting
    inline NestingType Type() { return static_cast<NestingType>(_stack.back().Type); };
    inline void SetType(NestingType nt) { _stack.back().Type = nt; };
    // Type of the nesting at the given nesting level
    inline NestingType Type(size_t level) { return static_cast<NestingType>(_stack.at(level).Type); };

    // If the innermost nesting is a loop that has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline std::int32_t StartLoc() { return _stack.back().StartLoc; };
    inline void SetStartLoc(std::int32_t start) { _stack.back().StartLoc = start; };
    // If the nesting at the given level has a jump back to the start,
    // then this gives the location to jump to; otherwise, it is 0
    inline std::int32_t StartLoc(size_t level) { return _stack.at(level).StartLoc; };

    // If the innermost nesting features a jump out instruction, then this is the location of it
    inline std::intptr_t JumpOutLoc() { return _stack.back().Info; };
    inline void SetJumpOutLoc(std::intptr_t loc) { _stack.back().Info = loc; };
    // If the nesting at the given level features a jump out, then this is the location of it
    inline std::intptr_t JumpOutLoc(size_t level) { return _stack.at(level).Info; };

    // If the innermost nesting is a SWITCH, the type of the switch expression
    int SwitchExprType() { return static_cast<int>(_stack.back().Info); };
    inline void SetSwitchExprType(int ty) { _stack.back().Info = ty; };

    // If the innermost nesting is a SWITCH, the location of the "default:" label
    inline std::int32_t DefaultLabelLoc() { return _stack.back().DefaultLabelLoc; };
    inline void SetDefaultLabelLoc(int32_t loc) { _stack.back().DefaultLabelLoc = loc; }

    // If the innermost nesting contains code chunks that must be moved around
    // (e.g., in FOR loops), then this is true, else false
    inline bool ChunksExist() { return !_stack.back().Chunks.empty(); }
    inline bool ChunksExist(size_t level) { return !_stack.at(level).Chunks.empty(); }

    // Code chunks that must be moved around (e.g., in FOR, DO loops)
    inline std::vector<ccChunk> Chunks() { return _stack.back().Chunks; };
    inline std::vector<ccChunk> Chunks(size_t level) { return _stack.at(level).Chunks; };

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
    void YankChunk(::ccCompiledScript *scrip, AGS::CodeLoc codeoffset, AGS::CodeLoc fixupoffset);

    // Write chunk of code back into the codebase that has been stashed in level given, at index
    void WriteChunk(::ccCompiledScript *scrip, size_t level, size_t index);
    // Write chunk of code back into the codebase stashed in the innermost level, at index
    inline void WriteChunk(::ccCompiledScript *scrip, size_t index) { WriteChunk(scrip, Depth() - 1, index); };


};


struct NestStack
{
    char Type;
    long Start;
    long Info;
    std::int32_t AssignAddress;
    std::vector<ccChunk> Chunk;
};

} // namespace AGS


extern int cc_tokenize(
    const char *inpl,         // preprocessed text to be tokenized
    ccInternalList * targ,     // store for the tokenized text
    ccCompiledScript * scrip); // store for the strings in the text

extern int cc_compile(
    const char *inpl,           // preprocessed text to be compiled
    ccCompiledScript * scrip);   // store for the compiled text

#endif // __CS_PARSER_H

int parse_subexpr_OpIsFirst(const AGS::SymbolScript &symlist, int oploc, const size_t &symlist_len, ccCompiledScript * scrip);

int parse_subexpr_OpIsSecondOrLater(const AGS::SymbolScript &symlist, int &oploc, ccCompiledScript * scrip, const size_t &symlist_len);

int parse_subexpr_NoOps(size_t &symlist_len, AGS::SymbolScript &symlist, ccCompiledScript * scrip, bool hasNegatedLiteral);

int parse_subexpr_NewIsFirst(const size_t & symlist_len, const AGS::SymbolScript & symlist, int oploc, ccCompiledScript * scrip);

int parse_subexpr_UnaryMinusIsFirst(const size_t & symlist_len, ccCompiledScript * scrip, const AGS::SymbolScript & symlist);

int parse_subexpr_NotIsFirst(const size_t & symlist_len, ccCompiledScript * scrip, const AGS::SymbolScript & symlist);
