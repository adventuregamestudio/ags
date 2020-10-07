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
        kSct_FloatLiteral,    // Numbers containing a "." --- [0-9]+[.][0-9]*
        kSct_IntLiteral,      // Numbers not containing a "." --- [0-9]+
        kSct_SectionChange,   // String literal beginning with magic string
        kSct_StringLiteral,   // Quoted strings --- ["]([\\].[^"]*)*["]
        kSct_NonChar          // i.e., +, ++, /=; this can be one character or two characters
    };

    static std::string const kNewSectionLitPrefix;

private:
    // Collect a sequence of opening ("([{") and closing (")]}") symbols; check matching
    class OpenCloseMatcher
    {
    private:
        struct OpenInfo
        {
            std::string const Opener;
            std::string const Closer;
            size_t Pos;  // of the Opener symbol in the token list

            OpenInfo(std::string const &opener, std::string const &closer, size_t pos);
        };
        std::vector<OpenInfo> _openInfoStack;
        Scanner &_scanner;

    public:
        OpenCloseMatcher(Scanner &scanner);

        // We've encountered an opening symbol; push it and the expected closer onto a stack
        void Push(std::string const &opener, std::string const &expected_closer, size_t opener_pos);

        // We've encountered a closing symbol; check whether this matches the corresponding opening symbol
        // If they don't match, generate error. Otherwise, pop from stack
        ErrorType PopAndCheck(std::string const &closer, size_t closer_pos);

        // At end of input, check whether any unclosed openers remain.
        ErrorType EndOfInputCheck();
    } _ocMatcher;
    friend OpenCloseMatcher;

    std::istringstream _inputStream;
    std::size_t _lineno;
    std::string _section;
    SrcList &_tokenList;
    MessageHandler &_messageHandler;
    struct ::SymbolTable &_sym;
    struct ::ccCompiledScript &_stringCollector;

    // Skip through the input, ignoring it, until a non-whitespace is found. Don't eat the non-whitespace.
    ErrorType SkipWhitespace(bool &eof_encountered);

    // We encountered a new line; process it
    void NewLine(size_t lineno);

    // We encountered a section start; process it
    void NewSection(std::string const &section);

    //  Read in either an int literal or a float literal
    // Note: appends to symstring, doesn't clear it first.
    ErrorType ReadInNumberLit(std::string &symstring, ScanType &scan_type, bool &eof_encountered);

    // Translate a '\\' combination into a character, backslash is already read in
    ErrorType EscapedChar2Char(int first_char_after_backslash, int &converted);

    static std::string MakeStringPrintable(std::string const &inp);

    // Read oct combination \777; backslash is already read in
    int OctDigits2Char(int first_digit_char);

    // Read hex combination \x77; backslash is already read in
    int HexDigits2Char(void);

    // Read in a character literal; converts it internally into an equivalent int literal
    ErrorType ReadInCharLit(std::string &symstring, bool &eof_encountered);

    // Read in a string literal
    ErrorType ReadInStringLit(std::string &symstring, bool &eof_encountered);

    // Read in an identifier or a keyword 
    ErrorType ReadInIdentifier(std::string &symstring, bool &eof_encountered);

    // Read in a single-char symstring
    ErrorType ReadIn1Char(std::string & symstring);

    // Read in a single-char symstring (such as "*"), or a double-char one (such as "*=")
    // A double-char symstring is detected if and only if the second char is in PossibleSecondChars.
    // Otherwise, a one-char symstring is detected and the second char is left for the next call
    ErrorType ReadIn1or2Char(std::string const &possible_second_chars, std::string &symstring, bool &eof_encountered);

    // Read in a symstring that begins with ".". This might yield a one- or three-char symstring.
    ErrorType ReadInDotCombi(std::string &symstring, ScanType &scan_type, bool &eof_encountered);

    // Read in a symstring that begins with "<". This might yield a one-, two- or three-char symstring.
    ErrorType ReadInLTCombi(std::string &symstring, bool &eof_encountered);

    // Read in a symstring that begins with ">". This might yield a one-, two- or three-char symstring.
    ErrorType ReadInGTCombi(std::string &symstring, bool &eof_encountered);

    ErrorType SymstringToSym(std::string const &symstring, ScanType scan_type, Symbol &symb, bool eof_encountered);

    ErrorType CheckMatcherNesting(Symbol token);

    void Error(char const *msg, ...);

protected:
    // Don't use std::isdigit et al. here because those are locale dependent and we don't want that.
    inline static bool IsDigit(int ch) { return (ch >= '0' && ch <= '9'); }
    inline static bool IsUpper(int ch) { return (ch >= 'A' && ch <= 'Z'); }
    inline static bool IsLower(int ch) { return (ch >= 'a' && ch <= 'z'); }
    inline static bool IsSpace(int ch) { return (std::strchr(" \t\n\v\f\r", ch) != 0); }

    // Change where: replace the first occurrence of token in where by replacement.
    inline static void ReplaceToken(std::string &where, std::string const &token, std::string const &replacement) { where.replace(where.find(token), token.length(), replacement); }

public:
    Scanner(std::string const &input, SrcList &token_list, ::ccCompiledScript &string_collector, SymbolTable &symt, MessageHandler &messageHandler);

    // Scan the input into token_list; symbols into symt; strings into _string_collector
    ErrorType Scan();

    inline size_t GetLineno() const { return _lineno; }

    inline std::string const GetSection() const { return _section; }

    // Get the next symstring from the input. Only exposed for googletests
    ErrorType GetNextSymstring(std::string &symstring, ScanType &scan_type, bool &eof_encountered);

    // Get next token from the input. Only exposed for googletests
    ErrorType GetNextSymbol(Symbol &symbol, bool &eof_encountered);
};

} // namespace AGS

#endif
