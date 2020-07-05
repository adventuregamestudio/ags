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

    static std::string const new_section_lit_prefix;


private:
    // Collect a sequence of opening ("([{") and closing (")]}") symbols; check matching
    class OpenCloseMatcher
    {
    private:
        struct OpenInfo
        {
            std::string const Opener;
            std::string const Closer;
            int SectionId; // of the Opener symbol
            int Lineno;  // of the Opener symbol
        };
        std::vector<OpenInfo> _openInfoStack;
        std::string _lastError;
        SrcList &_sectionIdConverter;

    public:
        OpenCloseMatcher(SrcList &sectionIdConverter);

        // We've encountered an opening symbol; push it and the expected closer onto a stack
        void Push(std::string const &opener, std::string const &expected_closer, int section_id, int lineno);

        // We've encountered a closing symbol; check whether this matches the corresponding opening symbol
        // If they don't match, generate error. Otherwise, pop from stack
        void PopAndCheck(std::string const &closer, int section_id, int lineno, bool &error_encountered);

        std::string GetLastError() const { return _lastError; };

        // Reset the matcher.
        void Reset();
    } _ocMatcher;

    std::istringstream _inputStream;
    std::size_t _lineno;
    std::string _section;
    SrcList &_tokenList;
    std::string _lastError;
    struct ::SymbolTable &_sym;
    struct ::ccCompiledScript &_stringCollector;

    // Skip through the input, ignoring it, until a non-whitespace is found. Don't eat the non-whitespace.
    void SkipWhitespace(bool &eof_encountered, bool &error_encountered);

    // We encountered a new line; process it
    void NewLine(size_t lineno);

    // We encountered a section start; process it
    void NewSection(std::string const section);

    //  Read in either an int literal or a float literal
    void ReadInNumberLit(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered);

    // Translate a '\\' combination into a character, backslash is already read in
    int EscapedChar2Char(int first_char_after_backslash, bool &error_encountered);

    std::string MakeStringPrintable(std::string const &inp);

    // Read oct combination \777; backslash is already read in
    int OctChar2Char(int first_digit_char);

    // Read hex combination \x77; backslash is already read in
    int HexChar2Char();

    // Read in a character literal; converts it internally into an equivalent int literal
    void ReadInCharLit(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    // Read in a string literal
    void ReadInStringLit(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    // Read in an identifier or a keyword 
    void ReadInIdentifier(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    // Read in a single-char symstring
    void ReadIn1Char(std::string & symstring);

    // Read in a single-char symstring (such as "*"), or a double-char one (such as "*=")
    // A double-char symstring is detected if and only if the second char is in PossibleSecondChars.
    // Otherwise, a one-char symstring is detected and the second char is left for the next call
    void ReadIn1or2Char(
        const std::string &possible_second_chars,
        std::string &symstring,
        bool &eof_encountered,
        bool &error_encountered);

    // Read in a symstring that begins with ".". This might yield a one- or three-char symstring.
    void ReadInDotCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    // Read in a symstring that begins with "<". This might yield a one-, two- or three-char symstring.
    void ReadInLTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    // Read in a symstring that begins with ">". This might yield a one-, two- or three-char symstring.
    void ReadInGTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered);

    void SymstringToSym(std::string const &symstring, ScanType scan_type, Symbol &symb, bool eof_encountered, bool error_encountered);

    void CheckMatcherNesting(Symbol token, bool &error_encountered);

protected:
    // Don't use std::isdigit et al. here because those are locale dependent and we don't want that.
    inline static bool IsDigit(int ch) { return (ch >= '0' && ch <= '9'); }
    inline static bool IsUpper(int ch) { return (ch >= 'A' && ch <= 'Z'); }
    inline static bool IsLower(int ch) { return (ch >= 'a' && ch <= 'z'); }
    inline static bool IsSpace(int ch) { return (std::strchr(" \t\n\v\f\r", ch) != 0); }

public:
    Scanner(std::string const &input, SrcList &token_list, struct ::ccCompiledScript &string_collector, SymbolTable &symt);

    // Scan the input into token_list; symbols into symt; strings into _string_collector
    void Scan(bool &error_encountered);

    inline size_t GetLineno() const { return _lineno; }

    inline std::string const GetSection() const{ return _section; }

    inline std::string const GetLastError() const { return _lastError; }

    // Get the next symstring from the input. Only exposed for googletests
    void GetNextSymstring(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered);

    // Get next token from the input. Only exposed for googletests
    Symbol GetNextSymbol(bool &eof_encountered, bool &error_encountered);
};

} // namespace AGS

#endif
