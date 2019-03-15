#ifndef __CS_TOKENIZER_H
#define __CS_TOKENIZER_H

#include <vector>
#include <string>

#include "cc_compiledscript.h"
#include "cc_internallist.h"
#include "cc_symboltable.h"

#include "cs_scanner.h"


namespace AGS
{
// Scans symstrings as necessary (through the scanner), converts them to tokens and returns tokens one-by-one.
// Strings are passed to the string collector.
class Tokenizer
{
public:
    enum Modes
    {
        kMode_Standard = 0, // Tokenizer isn't in any special mode
        kMode_StructDecl,   // Tokenizer is in a struct declaration
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
    void SetTokenList(struct ::ccInternalList *token_list);
    void SetSymbolTable(struct ::SymbolTable *symbol_table);
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

    inline int ConvertSymstringToTokenIndex(std::string symstring) { return _symbolTable->find_or_add(symstring.c_str()); }

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
            std::string Opener;
            std::string Closer;
            int Lineno;  // of the Opener symbol
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

    Modes _currentMode;
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

} // namespace AGS

#endif
