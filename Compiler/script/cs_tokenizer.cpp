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
    _parenthesisNestingDepth = 0;
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
    // Note: Do NOT prepend a dot to symbols that appear to be struct components. (Parser's job)
    
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
    if (error_encountered)
        return;

    // Count parenthesis nesting separately, is needed in a late rewriting of struct member names later on
    if (TokenType(token) == kSYM_OpenParenthesis) ++_parenthesisNestingDepth;
    if (TokenType(token) == kSYM_CloseParenthesis) --_parenthesisNestingDepth;


    // If we are in a parameter declaration where the parameter is declared with the type and not the name
    // and an initialization follows, then "*=" should be interpreted as the separate symbols "*" and "=".
    // e.g.: "import void someMethod(someClass *= 0);"
    // [fw] This won't work here because we don't know about the types in the tokenizer.
    //      If we want to do it, the "*=" needs to be broken up in the parser, which is a hassle
    //      but doable in principle.

    // We enter struct declaration mode after encountering the keyword "struct".
    if ((last_token >= 0) && (TokenType(last_token) == kSYM_Struct))
    {
        _currentMode = kMode_StructDecl;
        _inTypeSubmode = true;
        _braceNestingDepthInStructDecl = 0;
        _structBeingDeclared = token;
        return;
    }

    if (_currentMode == kMode_StructDecl)
    {
        // The mode ends as soon as the { ... } after the "struct X" ends.
        if (TokenType(token) == kSYM_OpenBrace) ++_braceNestingDepthInStructDecl;
        if (TokenType(token) == kSYM_CloseBrace)
        {
            if (--_braceNestingDepthInStructDecl == 0)
                _currentMode = kMode_Standard;
        }

        // The mode also ends if we never had any { ... } to begin with, i.e. a simple "struct X;".
        if ((TokenType(token) == kSYM_Semicolon) && (_braceNestingDepthInStructDecl == 0))
            _currentMode = kMode_Standard;
    }

    // If we're in the "root" of a struct declaration, 
    // then all var and func names must have the struct name prepended.
    if ((_currentMode != kMode_StructDecl) ||
        (_braceNestingDepthInStructDecl != 1) ||
        (_parenthesisNestingDepth > 0))
        return;

    if (!_inTypeSubmode && // see below, comment for "switch (TokenType(token))"
        ((TokenType(token) == 0) || (TokenType(token) == kSYM_GlobalVar)))
    {
        std::string full_name = FullNameFromStructAndMember(_structBeingDeclared, token);
        token = ConvertSymstringToTokenIndex(full_name);
        if (token < 0)
        {
            error_encountered = true;
            _lastError = "Symbol table overflow - could not add new symbol";
            return;
        }
    }

    // "Type" submode is defined in such a way that we are in it whenever we 
    // wait for the type of struct variables or functions. 
    // We must NOT prepend the struct name to the keywords or variables 
    // encountered when in this submode.
    // We enter it directly after passing "struct X {" and whenever we pass a ";" 
    // while we are within the outermost brace level of the struct.
    // As soon as we have passed the first unclassified symbol or type symbol, 
    // we assume it is a type name (or the name of an enum) and we leave Type submode
    // NOTE that this heuristic might fail when we intermingle tokenizing with parsing.
    // We don't try to reproduce parsing in the scanner, we only do it far enough for 
    // the parser to work with proper input and for the parser to give meaningful 
    // errors with improper input.
    switch (TokenType(token))
    {
    default: break;
    case 0:             _inTypeSubmode = false; break; // unclassified symbol
    case kSYM_Semicolon: _inTypeSubmode = true;  break;
    case kSYM_Vartype:   _inTypeSubmode = false; break; // e.g., "int", "float"
    }
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


std::string AGS::Tokenizer::FullNameFromStructAndMember(AGS::Symbol struct_name_token, AGS::Symbol member_name_token)
{
    std::string member_name = _symbolTable->get_name_string(member_name_token);
    if (member_name.at(0) == '.')
        member_name.erase(0, 1);

    std::string struct_name = _symbolTable->get_name_string(struct_name_token);
    struct_name.append("::").append(member_name);
    return struct_name;
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
