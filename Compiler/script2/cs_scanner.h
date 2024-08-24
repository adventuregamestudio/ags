//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __CS_SCANNER_H
#define __CS_SCANNER_H

#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <map>

#include "cc_internallist.h"
#include "cc_symboltable.h"
#include "cc_compiledscript.h"

namespace AGS
{

// Scans the input, returns list of symbols
class Scanner
{
public:
    // These types represent the different kinds of symstring that can be scanned.
    // Since the symstrings haven't become tokens yet, this is just rudimentary information.
    // Only exposed for googletests
    enum ScanType
    {
        kSct_Unspecified = 0,
        kSct_Identifier,      // Identifier or keyword --- [A-Za-z][A-Za-z_]*
        kSct_FloatLiteral,    // Float literal in C++ syntax
        kSct_IntLiteral,      // Integer literal in C++ syntax
        kSct_NonAlphanum,     // i.e., +, ++, /=; this can be one character or two characters
        kSct_OnePastLongMax,  // 1 plus largest signed integer
        kSct_SectionChange,   // String literal beginning with magic string
        kSct_StringLiteral,   // Quoted strings --- ["]([\\].[^"]*)*["]
    };

    static std::string const kNewSectionLitPrefix;
    static size_t const kNewSectionLitPrefixSize;

private:
    // Thrown whenever a scanning run is aborted
    // Don't throw directly, call 'UserError()' or 'InternalError()' instead
    class ScanningError : public std::exception
    {
        std::string _msg;
        const char *what(void) const noexcept { return _msg.c_str(); }

    public:
        ScanningError()
            : _msg("Compiling error")
        {}
        ScanningError(std::string const &msg)
            : _msg(msg)
        {}
    };

    // Collect a sequence of opening ("([{") and closing (")]}") symbols; check matching
    class OpenCloseMatcher
    {
    private:
        struct OpenInfo
        {
            Symbol Opener;
            size_t Pos;  // of the Opener symbol in the token list
        };
        std::vector<OpenInfo> _openInfoStack;
        Scanner &_scanner;

    public:
        OpenCloseMatcher(Scanner &scanner);

        // We've encountered an opening symbol; push it and the expected closer onto a stack
        void Push(Symbol opener, size_t opener_pos);

        // We've encountered a closing symbol; check whether this matches the corresponding opening symbol
        // If they don't match, generate error. Otherwise, pop from stack
        void PopAndCheck(Symbol closer, size_t closer_pos);

        // At end of input, check whether any unclosed openers remain.
        void EndOfInputCheck();
    } _ocMatcher;
    friend OpenCloseMatcher;

    std::istringstream _inputStream;
    // eof() won't fire if we read beyond the end-of-stream repeatedly.
    // We need that, so we collect in _eofReached the fact that end-of-stream has ever been reached.
    bool _eofReached = false;
    bool _failed = false;
    std::size_t _lineno;
    std::string _section;
    SrcList &_tokenList;
    MessageHandler &_msgHandler;
    SymbolTable &_sym;
    ccCompiledScript &_stringCollector;

    // Get the next char from the input stream
    inline int Get() { int ret = _inputStream.get(); _eofReached |= _inputStream.eof(); _failed |= _inputStream.fail(); return ret; }
    // Undoes the last character read. But won't reset _eofReached: Once the end-of-stream is reached, it stays reached.
    void  UnGet() { _inputStream.unget(); }
    // Look ahead at the next char to be read without reading it. If there aren't any chars to be read, _eofReached is set.
    inline int Peek() { int ret = _inputStream.peek(); _eofReached |= _inputStream.eof(); _failed |= _inputStream.fail(); return ret; }

    // Skip through the input, ignoring it, until a non-whitespace is found. Don't eat the non-whitespace.
    void SkipWhitespace();

    // We encountered a new line; process it
    void NewLine(size_t lineno);

    // We encountered a section start; process it
    void NewSection(std::string const &section);

    // Convert 'valstring' to a long long (!)
    int64_t StringToLongLong(std::string const &valstring, bool &conversion_successful) const;

    // Read in either an int literal or a float literal
    // Note: appends to symstring, doesn't clear it first.
    void ReadInNumberLit(std::string &symstring, ScanType &scan_type, CodeCell &value);

    // Translate a '\\' combination into a character, backslash is already read in
    void EscapedChar2Char(int first_char_after_backslash, std::string &symstring, int &converted);

    // Read oct combination \777; backslash is already read in
    int OctDigits2Char(int first_digit_char, std::string &symstring);

    // Read hex combination \x77; backslash is already read in
    int HexDigits2Char(std::string &symstring);

    // Read in a character literal
    void ReadInCharLit(std::string &symstring, CodeCell &value);
    
    // Read in a string literal. valstring is the interpreted literal (no quotes, '\\' combinations resolved)
    void ReadInStringLit(std::string &symstring, std::string &valstring);

    // Read in an identifier or a keyword 
    void ReadInIdentifier(std::string &symstring);

    // Read in a single-char symstring
    void ReadIn1Char(std::string & symstring);

    // Read in a single-char symstring (such as "*"), or a double-char one (such as "*=")
    // A double-char symstring is detected if and only if the second char is in PossibleSecondChars.
    // Otherwise, a one-char symstring is detected and the second char is left for the next call
    void ReadIn1or2Char(std::string const &possible_second_chars, std::string &symstring);

    // Read in a symstring that begins with ".". This might yield a one- or three-char symstring.
    void ReadInDotCombi(std::string &symstring, ScanType &scan_type);

    // Read in a symstring that begins with "<". This might yield a one-, two- or three-char symstring.
    void ReadInLTCombi(std::string &symstring);

    // Read in a symstring that begins with ">". This might yield a one-, two- or three-char symstring.
    void ReadInGTCombi(std::string &symstring);

    // Get the next symstring from the input.
    void GetNextSymstring(std::string &symstring, ScanType &scan_type, CodeCell &value);

    void SymstringToSym(std::string const &symstring, ScanType scan_type, CodeCell value, Symbol &symb);

    void CheckMatcherNesting(Symbol token);

    // Record error message, throw exception
    void Error(bool is_internal, std::string const &message);

    // Abort scanning with an error caused by wrong user input
    void UserError(char const *desc ...);

    // Abort scanning with an internal error
    void InternalError(char const *desc  ...);

    // Warn, but don't abort.
    void Warning(char const *desc ...);

protected:
    inline static bool IsDigit(int ch) { return (ch >= '0' && ch <= '9'); }
    inline static bool IsHexDigit(int ch) { return IsDigit(ch) || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }
    inline static bool IsAlpha(int ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }
    inline static bool IsSpace(int ch) { return (std::strchr(" \t\n\v\f\r", ch) != 0); }

    // Change 'where': replace the first occurrence of 'token' in 'where' by 'replacement'.
    inline static void ReplaceToken(std::string &where, std::string const &token, std::string const &replacement) { where.replace(where.find(token), token.length(), replacement); }

public:
    Scanner(std::string const &input, SrcList &token_list, ccCompiledScript &string_collector, SymbolTable &symt, MessageHandler &messageHandler);

    // Scan the input into token_list; symbols into symt; strings into _string_collector
    void Scan();

    // Returns whether we've encountered EOF.
    inline bool EOFReached() const { return _eofReached; };
    // Returns whether we've encountered a read failure
    inline bool Failed() const { return _failed; };

    inline size_t GetLineno() const { return _lineno; }

    inline std::string const GetSection() const { return _section; }

    // Get the next symstring from the input. Only use for googletests
    int GetNextSymstringT(std::string &symstring, ScanType &scan_type, CodeCell &value);

    // Get next token from the input. Only exposed for googletests
    void GetNextSymbol(Symbol &symbol);
};

} // namespace AGS

#endif
