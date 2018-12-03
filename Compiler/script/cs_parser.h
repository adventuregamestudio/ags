/*
'C'-style script compiler development file. (c) 2000,2001 Chris Jones

SCOM is a script compiler for the 'C' language. The current version
implements:
* #define macros, definition of and use of
* "//" and "/*---* /" comments
* global and local variables; calling functions; assignments
* most of the standard 'C' operators
* structures and arrays
* import and export of variables and functions from parent program
* strings get allocated 200 bytes of storage automatically

It currently does NOT do:
* #define with parenthesis, eg. #define func(a) bar(a+3)
* typedefs
* optimize code generated - it could check if MAR already contains location
to read, for example

Some interesting points about how this works:
* while loops are stored internally as "else"-blocks, but with an extra
bit of data storing the start of the while test condition to go back to
* array index accesses are generated as code to allow anything inside
the brackets, whereas structure member accesses are hardcoded into the
offset in the code since the member has a fixed offset from the structure
start

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


namespace ags
{

/// Collect a sequence of opening ("([{") and closing (")]}") symbols; check matching
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
        std::string Opener;
        std::string Closer;
        int Lineno;  // of the Opener symbol
    };
    std::vector<struct OpenInfo> OpenInfoStack;
    std::string LastError;
};


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
        SctUnspecified = 0,
        SctIdentifier,      ///< Identifier or keyword --- [A-Za-z][A-Za-z_]*
        SctFloatLiteral,    ///< Numbers containing a "." --- [0-9]+[.][0-9]*
        SctIntLiteral,      ///< Numbers not containing a "." --- [0-9]+
        SctStringLiteral,   ///< Quoted strings --- ["]([\\].[^"]*)*["]
        SctNonChar          ///< i.e., +, ++, /=; this can be one character or two characters
    };

    // ctors
    Scanner();
    Scanner(std::string const & Input, std::size_t Lineno, struct ::ccInternalList * TokenList);

    // setters and getters
    void SetInput(const std::string & Input);
    void SetLineno(std::size_t Lineno);
    std::size_t GetLineno();
    void SetTokenList(struct ::ccInternalList *TokenList);

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

    inline void WriteNewLinenoMeta(int ln) { const Symbol smeta_linenum = (Symbol)1; TokenList->write_meta(smeta_linenum, ln); }

private:
    std::istringstream InputStream;
    std::string Error;
    std::size_t Lineno;
    ccInternalList *TokenList;
    std::string LastError;

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
        ModeStandard = 0, ///< Tokenizer isn't in any special mode
        ModeStructDecl,   ///< Tokenizer is in a struct declaration
    };

    // ctors
    Tokenizer();

    // Note: The tokenizer does not "own" the objects below: 
    // When it destructs, it will make no attempt to destruct *scanner etc.
    Tokenizer(
        Scanner *scanner,
        struct ::ccInternalList *token_list,
        struct ::symbolTable *symbol_table,
        struct ::ccCompiledScript *string_collector);

    // setters
    void SetScanner(Scanner *scanner);
    void SetTokenList(struct ::ccInternalList * token_list);
    void SetSymbolTable(struct ::symbolTable * symbol_table);
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
    Scanner *Scanner;
    struct ::ccInternalList *TokenList;
    struct ::symbolTable *SymbolTable;
    struct ::ccCompiledScript *StringCollector;

    std::string LastError;
    OpenCloseMatcher OCMatcher;

    enum Modes CurrentMode;
    bool InTypeSubmode;
    int BraceNestingDepthInStructDecl; // only used in StructDeclMode
    int StructBeingDeclared;
    int ParenthesisNestingDepth;
    std::deque<int> TokenBuffer;

    inline Symbol TokenType(int token) { return SymbolTable->entries[token].stype; }
    inline std::string TokenName(int token) { return SymbolTable->entries[token].sname; }
    inline void SetTokenType(int token, Symbol value) { SymbolTable->entries[token].stype = value; }
    inline void SetTokenOffsetInStrings(int token, int value) { SymbolTable->entries[token].soffs = value; }
    inline void SetTokenVartype(int token, Symbol value) { SymbolTable->entries[token].vartype = value; }
};

} // namespace ags



#define NEST_FUNCTION   1  // it's a function
#define NEST_NOTHING    2  // no reason - they just put { } in the code
#define NEST_IF         3  // it's an IF statement
#define NEST_IFSINGLE   4  // single IF statment (ie. no braces)
#define NEST_ELSE       5
#define NEST_ELSESINGLE 6
#define NEST_STRUCT     7
#define NEST_DO         8 // Do statement (to be followed by a while)
#define NEST_DOSINGLE   9 // Single Do statement
#define NEST_FOR        10 // For statement
#define NEST_SWITCH     11 // Case block for a switch statement




extern int cc_tokenize(
    const char * inpl,         // preprocessed text to be tokenized
    ccInternalList * targ,     // store for the tokenized text
    ccCompiledScript * scrip); // store for the strings in the text

extern int cc_compile(
    const char * inpl,           // preprocessed text to be compiled
    ccCompiledScript * scrip);   // store for the compiled text

int cc_compile_HandleLinesAndMeta(const ags::Symbol &cursym, int &currentlinewas, ccCompiledScript * scrip, ccInternalList & targ, bool &retflag);



// A section of compiled code that needs to be moved or copied to a new location
struct ccChunk {
    std::vector<intptr_t> code;
    std::vector<int32_t> fixups;
    std::vector<char> fixuptypes;
    int codeoffset;
    int fixupoffset;
};


struct NestStack
{
    char Type;
    long Start;
    long Info;
    std::int32_t AssignAddress;
    std::vector<ccChunk> Chunk;
};

typedef std::vector<NestStack> NestStack_t;




#endif // __CS_PARSER_H