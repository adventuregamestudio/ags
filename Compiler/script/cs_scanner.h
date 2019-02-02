#ifndef __CS_SCANNER_H
#define __CS_SCANNER_H

#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <map>
#include "cc_internallist.h"

namespace AGS
{

/// Scans the input, returning the symstrings one by one.
/// In this context, a "symstring" is defined to mean the string. 
/// When the symstring (i.e., the string) is entered into the symbol database, it becomes a "symbol". 
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

} // namespace AGS

#endif
