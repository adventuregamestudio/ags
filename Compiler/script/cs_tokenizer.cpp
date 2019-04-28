#include <string>

#include "cc_internallist.h"    // ccInternalList
#include "cs_parser_common.h"
#include "cc_symboltable.h"

#include "cs_tokenizer.h"

AGS::Tokenizer::Tokenizer()
    :
    _scanner(0),
    _tokenList(0),
    _symbolTable(0),
    _stringCollector(0)
{
    Reset();
}

AGS::Tokenizer::Tokenizer(AGS::Scanner *scanner, ::ccInternalList *token_list, ::SymbolTable *symbol_table, ::ccCompiledScript *string_collector)
    : _scanner(scanner)
    , _tokenList(token_list)
    , _symbolTable(symbol_table)
    , _stringCollector(string_collector)
{
    Reset();
}

void AGS::Tokenizer::SetScanner(AGS::Scanner *scanner)
{
    _scanner = scanner;
}

void AGS::Tokenizer::SetTokenList(ccInternalList *token_list)
{
    _tokenList = token_list;
}

void AGS::Tokenizer::SetSymbolTable(::SymbolTable *symbol_table)
{
    _symbolTable = symbol_table;
}

void AGS::Tokenizer::SetStringCollector(ccCompiledScript *string_collector)
{
    _stringCollector = string_collector;
}

void AGS::Tokenizer::Reset()
{
    _currentMode = kMode_Standard;
    _braceNestingDepthInStructDecl = 0;
    _structBeingDeclared = -1; // no struct open
    _tokenBuffer.clear();
    _lastError = "";
}

void AGS::Tokenizer::ProcessScannerSymstring(
    Scanner::ScanType scan_type,
    std::string &symstring,
    Symbol last_token,
    Symbol &token,
    bool &eof_encountered,
    bool &error_encountered)
{
    // Note: Do NOT prepend a dot to symbols that appear to be struct components. That's the parser's job
    // Note: Do NOT try to guess what might be components of what structs,
    // do NOT concatenate them as STRUCTNAME::COMPONENTNAME. That's the parser's job
    // In both cases, the tokenizer is lacking critical information to do that job properly.

    token = ConvertSymstringToTokenIndex(symstring);
    if (token < 0)
    {
        error_encountered = true;
        _lastError = "Symbol table overflow - could not add new symbol";
        return;
    }

    if (scan_type == Scanner::kSct_StringLiteral)
    {
        TokenizeStringLiteral(token, symstring);
        return;
    }

    if (scan_type == Scanner::kSct_IntLiteral)
    {
        SetTokenType(token, kSYM_LiteralInt);
        return;
    }

    if (scan_type == Scanner::kSct_FloatLiteral)
    {
        SetTokenType(token, kSYM_LiteralFloat);
        return;
    }

    // Check the nesting of () [] {}
    CheckMatcherNesting(token, error_encountered);
}


void AGS::Tokenizer::TokenizeStringLiteral(AGS::Symbol token, std::string const &symstring)
{
    SetTokenType(token, kSYM_LiteralString);
    SetTokenVartype(token, _symbolTable->getOldStringSym());

    // Enter the string into the string collector
    std::string s_content = symstring.substr(1, symstring.size() - 2); // i.e., without the surrounding quotes
    int offset = _stringCollector->add_string(s_content.c_str()); // scaffolding
    SetTokenOffsetInStrings(token, offset);
}


void AGS::Tokenizer::GetNextToken(AGS::Symbol &token, bool &eof_encountered, bool &error_encountered)
{
    token = -1;

    // Feed the buffer if it has run dry
    if (_tokenBuffer.size() < 2)
    {
        // Get the next symbol from the scanner
        std::string symstring;
        Scanner::ScanType scan_type;
        _scanner->GetNextSymstring(symstring, scan_type, eof_encountered, error_encountered);
        if (error_encountered)
        {
            // copy the error from the scanner
            _lastError = _scanner->GetLastError();
        }
        if (eof_encountered || error_encountered)
            return;

        // Special magic (or kludge) to interoperate with the dialog script generator
        // Scripts are generated in a bunch; when a new script starts, the line number must be reset
        if (symstring.substr(0, 18) == NEW_SCRIPT_TOKEN_PREFIX)
            _scanner->SetLineno(0);

        if (_tokenBuffer.empty())
            _tokenBuffer.push_back(-1);
        AGS::Symbol last_token = _tokenBuffer.back();

        ProcessScannerSymstring(scan_type, symstring, last_token, token, eof_encountered, error_encountered);
        if (eof_encountered || error_encountered)
            return;

        _tokenBuffer.push_back(token);
    }

    // If the buffer is still dry, return EOF
    if (_tokenBuffer.size() < 2)
    {
        eof_encountered = true;
        return;
    }

    // The first token has been returned the previous time, so pop it off now.
    _tokenBuffer.pop_front();

    // Return the next first token.
    eof_encountered = false;
    token = _tokenBuffer.front();
}


// Check the nesting of () [] {}, error if mismatch
void AGS::Tokenizer::CheckMatcherNesting(Symbol token, bool &error_encountered)
{
    switch (TokenType(token))
    {
    default: return;
    case kSYM_OpenParenthesis:
        _ocMatcher.Push("(", ")", _scanner->GetLineno());
        return;

    case kSYM_OpenBracket:
        _ocMatcher.Push("[", "]", _scanner->GetLineno());
        return;

    case kSYM_OpenBrace:
        _ocMatcher.Push("{", "}", _scanner->GetLineno());
        return;

    case kSYM_CloseParenthesis:
        _ocMatcher.PopAndCheck(")", _scanner->GetLineno(), error_encountered);
        if (error_encountered)
            _lastError = _ocMatcher.GetLastError();
        return;

    case kSYM_CloseBracket:
        _ocMatcher.PopAndCheck("]", _scanner->GetLineno(), error_encountered);
        if (error_encountered)
            _lastError = _ocMatcher.GetLastError();
        return;

    case kSYM_CloseBrace:
        _ocMatcher.PopAndCheck("}", _scanner->GetLineno(), error_encountered);
        if (error_encountered)
            _lastError = _ocMatcher.GetLastError();
        return;
    }
}


void AGS::Tokenizer::OpenCloseMatcher::Reset()
{
    _lastError = "";
    _openInfoStack.resize(0);
}


std::string AGS::Tokenizer::OpenCloseMatcher::GetLastError()
{
    return _lastError;
}


AGS::Tokenizer::OpenCloseMatcher::OpenCloseMatcher()
{
    Reset();
}


void AGS::Tokenizer::OpenCloseMatcher::Push(std::string const &opener, std::string const &expected_closer, int lineno)
{
    struct OpenInfo oi;
    oi.Opener = opener;
    oi.Closer = expected_closer;
    oi.Lineno = lineno;

    _openInfoStack.push_back(oi);
}


void AGS::Tokenizer::OpenCloseMatcher::PopAndCheck(std::string const &closer, int lineno, bool &error_encountered)
{
    if (_openInfoStack.empty())
    {
        error_encountered = true;
        _lastError = "There isn't any opening symbol that matches this closing '&1'";
        _lastError.replace(_lastError.find("&1"), 2, closer);
        return;
    }

    struct OpenInfo oi(_openInfoStack.back());
    _openInfoStack.pop_back();
    if (oi.Closer != closer)
    {
        error_encountered = true;
        _lastError = "Found '&1', this does not match the '&2' on line &3";
        if (oi.Lineno == lineno)
            _lastError = "Found '&1', this does not match the '&2' on this line";
        _lastError.replace(_lastError.find("&1"), 2, closer);
        _lastError.replace(_lastError.find("&2"), 2, oi.Opener);
        if (oi.Lineno != lineno)
            _lastError.replace(_lastError.find("&3"), 2, std::to_string(oi.Lineno));
    }
}
