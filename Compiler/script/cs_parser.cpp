
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <sstream>
#include <cctype>
#include <limits>
#include <algorithm>
#include "cs_parser.h"
#include "cc_internallist.h"    // ccInternalList
#include "cs_parser_common.h"
#include "cc_symboltable.h"
#include "script/cc_options.h"
#include "script/script_common.h"
#include "script/cc_error.h"
#include "cc_variablesymlist.h"

#include "fmem.h"

// Declared in Common/script/script_common.h 
// Defined in Common/script/script_common.cpp
extern int currentline;

char ccCopyright[] = "ScriptCompiler32 v" SCOM_VERSIONSTR " (c) 2000-2007 Chris Jones and 2011-2014 others";
static char scriptNameBuffer[256];

int  evaluate_expression(ccInternalList *targ, ccCompiledScript *script, bool consider_paren_nesting);

int read_variable_into_ax(ccCompiledScript*scrip, ags::SymbolScript_t syml, int syml_len, bool negateLiteral);
int read_variable_into_ax(ccCompiledScript*scrip, ags::SymbolScript_t syml, int syml_len, bool negateLiteral, bool mustBeWritable, bool &write_same_as_read_access);

int do_variable_memory_access(
    ccCompiledScript *scrip, ags::Symbol_t variableSym,
    int variableSymType, bool isProperty,
    bool writing, bool mustBeWritable,
    bool addressof, bool extraoffset,
    int soffset, bool pointerIsOnStack,
    bool wholePointerAccess,
    ags::Symbol_t mainVariableSym, int mainVariableType,
    bool isDynamicArray, bool negateLiteral);
int parse_subexpr(ccCompiledScript *scrip, ags::SymbolScript_t symlist, size_t symlist_len);


enum FxFixupType
{
    FxNoFixup = 0,
    FxFixupDataData = 1, // needed e.g. when a global string is declared
    FxFixupType2 = 2  // needed e.g. when a local var defn has an initializer expression
};

enum Globalness
{
    GlLocal = 0,            // Local
    GlGlobalNoImport = 1,   // Global, not imported
    GlGlobalImport = 2      // Global, imported
};

enum Importness
{
    ImNoImport = 0,
    ImImportType1 = 1,  
    ImImportType2 = 2   
};

ags::Scanner::Scanner()
    :
    Lineno(1),
    TokenList(0),
    LastError("")
{
}

ags::Scanner::Scanner(std::string const &input, std::size_t lineno, ::ccInternalList *token_list)
    :
    Lineno(lineno),
    TokenList(token_list),
    LastError("")
{
    SetInput(input);
}

void ags::Scanner::SetInput(const std::string &input)
{
    InputStream.str(input);
}

void ags::Scanner::SetLineno(std::size_t lineno)
{
    Lineno = lineno;
}

std::size_t ags::Scanner::GetLineno()
{
    return Lineno;
}

void ags::Scanner::SetTokenList(ccInternalList *token_list)
{
    TokenList = token_list;
}

void ags::Scanner::GetNextSymstring(std::string & symstring, ScanType & scan_type, bool & eof_encountered, bool & error_encountered)
{
    eof_encountered = false;
    error_encountered = false;

    SkipWhitespace(eof_encountered, error_encountered);
    if (eof_encountered || error_encountered) return;

    int next_char = InputStream.peek();
    eof_encountered = InputStream.eof();
    if (eof_encountered) return;
    error_encountered = InputStream.fail();
    if (error_encountered) return;

    // Integer or float literal
    if (isdigit(next_char))
    {
        ReadInNumberLit(symstring, scan_type, eof_encountered, error_encountered);
        return;
    }

    // Character literal
    if (next_char == '\'')
    {
        // Note that this converts the literal to an equivalent integer string "'A'" >>-> "65"
        ReadInCharLit(symstring, eof_encountered, error_encountered);
        scan_type = SctIntLiteral;
        return;
    }

    // Identifier or keyword
    if (isupper(next_char) || islower(next_char) || (next_char == '_'))
    {
        ReadInIdentifier(symstring, eof_encountered, error_encountered);
        scan_type = SctIdentifier;
        return;
    }

    // String literal
    if (next_char == '"')
    {
        ReadInStringLit(symstring, eof_encountered, error_encountered);
        scan_type = SctStringLiteral;
        return;
    }

    // Non-char symstrings, such as "*="
    scan_type = SctNonChar;
    switch (next_char)
    {   
    default:  break;
    case '!': ReadIn1or2Char("=", symstring, eof_encountered, error_encountered); return;
    case '%': ReadIn1or2Char("=", symstring, eof_encountered, error_encountered); return;
    case '&': ReadIn1or2Char("&=", symstring, eof_encountered, error_encountered); return;
    case '(': ReadIn1Char(symstring); return;
    case ')': ReadIn1Char(symstring); return;
    case '*': ReadIn1or2Char("=", symstring, eof_encountered, error_encountered); return;
    case '+': ReadIn1or2Char("+=", symstring, eof_encountered, error_encountered); return;
    case ',': ReadIn1Char(symstring); return;
    case '-': ReadIn1or2Char("-=>", symstring, eof_encountered, error_encountered); return;
    case '.': ReadInDotCombi(symstring, eof_encountered, error_encountered); return;
        // Note that the input is pre-processed,
        // so it cannot contain comments of the form //...EOL or /*...*/
    case '/': ReadIn1or2Char("=", symstring, eof_encountered, error_encountered); return;
    case ':': ReadIn1or2Char(":", symstring, eof_encountered, error_encountered); return;
    case ';': ReadIn1Char(symstring); return;
    case '<': ReadInLTCombi(symstring, eof_encountered, error_encountered); return;
    case '=': ReadIn1or2Char("=", symstring, eof_encountered, error_encountered); return;
    case '>': ReadInGTCombi(symstring, eof_encountered, error_encountered); return;
    case '?': ReadIn1Char(symstring); return;
    case '[': ReadIn1Char(symstring); return;
    case ']': ReadIn1Char(symstring); return;
    case '^': ReadIn1or2Char("=", symstring, eof_encountered, error_encountered); return;
    case '{': ReadIn1Char(symstring); return;
    case '|': ReadIn1or2Char("=|", symstring, eof_encountered, error_encountered); return;
    case '}': ReadIn1Char(symstring); return;
    case '~': ReadIn1Char(symstring); return;
    }

    // Here when we don't know how to process the next char to be read
    error_encountered = true;
    LastError = "The character '&1' is not legal in this context";
    std::string chstring(1, next_char);
    LastError.replace(LastError.find("&1"), 2, chstring);
    return;
}

const std::string ags::Scanner::GetLastError()
{
    return this->LastError;
}


void ags::Scanner::SkipWhitespace(bool & eof_encountered, bool & error_encountered)
{
    while (true)
    {
        int ch = InputStream.get();
        eof_encountered = InputStream.eof();
        if (eof_encountered) return;
        error_encountered = InputStream.fail();
        if (error_encountered)
        {
            LastError = "Error whilst skipping whitespace (file corrupt?)";
            return;
        }

        if (!isspace(ch))
        {
            InputStream.putback(ch);
            return;
        }

        // Gobble the CR of a CRLF combination
        if ((ch == '\r') && (InputStream.peek() == '\n')) continue;

        if (ch == '\n')
        {
            // Write pseudocode for increased line number
            WriteNewLinenoMeta(++Lineno);
        }
    }
}

void ags::Scanner::ReadInNumberLit(std::string & symstring, ScanType &scan_type, bool & eof_encountered, bool & error_encountered)
{
    bool decimal_point_encountered = false;

    scan_type = SctIntLiteral;
    symstring.assign(1, InputStream.get());

    while (true)
    {
        int ch = InputStream.get();
        eof_encountered = InputStream.eof();
        if (eof_encountered) return;
        error_encountered = InputStream.fail();
        if (error_encountered)
        {
            LastError = "Error encountered while scanning a number literal (file corrupt?)";
            return;
        }
        if (eof_encountered || error_encountered) return;

        if (isdigit(ch))
        {
            symstring.push_back(ch);
            continue;
        }
        if (ch == '.')
        {
            if (!decimal_point_encountered)
            {
                decimal_point_encountered = true;
                scan_type = SctFloatLiteral;
                symstring.push_back(ch);
                continue;
            }
        }
        InputStream.putback(ch); // no longer part of the number literal, so put it back
        break;
    }
}

void ags::Scanner::ReadInCharLit(std::string & symstring, bool & eof_encountered, bool & error_encountered)
{
    symstring = "";
    int lit_char;

    do // exactly 1 time
    {
        // Opening '\''
        InputStream.get();

        // The character inside
        lit_char = InputStream.get();
        eof_encountered = InputStream.eof();
        if (eof_encountered)
        {
            error_encountered = true;
            LastError = "Expected a character and an apostrophe, but input ended instead";
            return;
        }
        error_encountered = InputStream.fail();
        if (error_encountered) break; // to error processing

        if (lit_char == '\'')
        {
            // The next char is escaped, whatever it may be. 
            // Note that AGS doesn't follow C syntax here:
            // In C, '\n' is a newline; in AGS, it is the letter 'n'.
            lit_char = InputStream.get();
            eof_encountered = InputStream.eof();  // This is an error
            if (eof_encountered)
            {
                error_encountered = true;
                LastError = "Expected a character and an apostrophe, but input ended instead";
                return;
            }
            error_encountered = InputStream.fail();
            if (error_encountered) break; // to error processing
        }

        // Closing '\''
        int ch = InputStream.get();
        eof_encountered = InputStream.eof();
        if (eof_encountered)
        {
            error_encountered = true;
            LastError = "Expected an apostrophe, but input ended instead";
            return;
        }
        error_encountered = InputStream.fail();
        if (error_encountered) break; // to error processing
        if (ch != '\'')
        {
            error_encountered = true;
            std::string wrong_letter_as_string(1, ch);
            LastError = "Expected apostrophe, but found '&1' instead";
            LastError.replace(LastError.find("&1"), 2, wrong_letter_as_string);
            return;
        }
        // Convert the char literal to an int literal
        // Note: We do NOT return the char literal, 
        // we return an equivalent integer literal, instead.
        symstring = std::to_string(lit_char);
        return;
    } while (false);

    // Here when we got a read error
    error_encountered = true;
    LastError = "Could not read the input (corrupt file?)";
    return;
}

void ags::Scanner::ReadInStringLit(std::string & symstring, bool & eof_encountered, bool & error_encountered)
{
    symstring = "\"";
    InputStream.get(); // We know that this is a '"'
    while (true)
    {
        int ch = InputStream.get();
        eof_encountered = InputStream.eof(); // This is an error, too
        error_encountered = InputStream.fail();
        if (eof_encountered || error_encountered || (strchr("\r\n", ch) != 0))
        {
            break; // to error msg
        }

        symstring.push_back(ch);

        // End of string
        if (ch == '"') return;

        if (ch == '\\')
        {
            // Now some character MUST follow; any one is allowed, but no line changes 
            // Note that AGS doesn't follow C syntax here.
            // In C, "\n" is a newline, but in AGS, "\n" is "n".
            int ch = InputStream.get();
            eof_encountered = InputStream.eof(); // This is an error, too
            error_encountered = InputStream.fail();
            if (eof_encountered || error_encountered || (strchr("\r\n", ch) != 0))
            {
                break; // to error msg
            }
            symstring.push_back(ch);
        }
    }
    // Here whenever an error or EOF or LF came before the string was terminated
    error_encountered = true;
    LastError = "Incorrectly terminated string literal (Use '[' for line feed)";
    return;
}


void ags::Scanner::ReadInIdentifier(std::string & symstring, bool & eof_encountered, bool & error_encountered)
{
    symstring.assign(1, InputStream.get());

    while (true)
    {
        int ch = InputStream.get();
        eof_encountered = InputStream.eof();
        if (eof_encountered) return;
        error_encountered = InputStream.fail();
        if (error_encountered)
        {
            LastError = "Error encountered while scanning an identifier (file corrupt?)";
            return;
        }
        if (eof_encountered || error_encountered) return;

        if (isupper(ch) || islower(ch) || isdigit(ch) || (ch == '_'))
        {
            symstring.push_back(ch);
            continue;
        }
        // That last char doesn't belong to the literal, so put it back.
        InputStream.putback(ch);
        return;
    }
}



void ags::Scanner::ReadIn1or2Char(const std::string &possible_second_chars, std::string & symstring, bool & eof_encountered, bool & error_encountered)
{
    symstring.assign(1, InputStream.get());

    int second_char = InputStream.peek();
    if (possible_second_chars.find(second_char) != std::string::npos)
    {
        InputStream.get(); // Gobble the character that was peek()ed
        symstring.push_back(second_char);
    }
}

void ags::Scanner::ReadIn1Char(std::string & symstring)
{
    symstring.assign(1, InputStream.get());
}

void ags::Scanner::ReadInDotCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    ReadIn1or2Char(".", symstring, eof_encountered, error_encountered);
    if (eof_encountered || error_encountered) return;

    if (symstring == ".") return;

    if (InputStream.peek() == '.')
    {
        symstring.push_back(InputStream.get());
        return;
    }

    LastError = "Must either use '.' or '...'";
    error_encountered = true;    
}


void ags::Scanner::ReadInLTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    ReadIn1or2Char("<=", symstring, eof_encountered, error_encountered);
    if (eof_encountered || error_encountered) return;

    if ((symstring == "<<") && (InputStream.peek() == '='))
    {
        symstring.push_back(InputStream.get());
    }
}

void ags::Scanner::ReadInGTCombi(std::string & symstring, bool & eof_encountered, bool & error_encountered)
{
    ReadIn1or2Char(">=", symstring, eof_encountered, error_encountered);
    if (eof_encountered || error_encountered) return;

    if ((symstring == ">>") && (InputStream.peek() == '='))
    {
        symstring.push_back(InputStream.get());
    }
}


ags::Tokenizer::Tokenizer()
    :
    Scanner(0),
    TokenList(0),
    SymbolTable(0),
    StringCollector(0)
{
    Reset();
}

ags::Tokenizer::Tokenizer(ags::Scanner * scanner, ::ccInternalList * token_list, ::symbolTable * symbol_table, ::ccCompiledScript * string_collector)
    :
    Scanner(scanner),
    TokenList(token_list),
    SymbolTable(symbol_table),
    StringCollector(string_collector)
{
    Reset();
}

void ags::Tokenizer::SetScanner(ags::Scanner * scanner)
{
    Scanner = scanner;
}

void ags::Tokenizer::SetTokenList(ccInternalList * token_list)
{
    TokenList = token_list;
}

void ags::Tokenizer::SetSymbolTable(symbolTable * symbol_table)
{
    SymbolTable = symbol_table;
}

void ags::Tokenizer::SetStringCollector(ccCompiledScript * string_collector)
{
    StringCollector = string_collector;
}

void ags::Tokenizer::Reset()
{
    CurrentMode = ModeStandard;
    BraceNestingDepthInStructDecl = 0;
    StructBeingDeclared = -1; // no struct open
    TokenBuffer.clear();
    ParenthesisNestingDepth = 0;
    LastError = "";
}


const std::string ags::Tokenizer::GetLastError()
{
    return LastError;
}

void ags::Tokenizer::ResetTemporaryTypesInSymbolTable()
{
    // clear any temporary types set
    for (size_t tok = 0; tok < SymbolTable->entries.size(); tok++)
    {
        if (TokenType(tok) == SYM_TEMPORARYTYPE) SetTokenType(tok, static_cast<ags::Symbol_t>(0));
    }
}

void ags::Tokenizer::ProcessScannerSymstring(
    Scanner::ScanType symbol_type,
    std::string & symstring,
    int last_token,
    int &token,
    bool & eof_encountered,
    bool & error_encountered)
{
    if ((symbol_type == Scanner::SctIdentifier) && (last_token >= 0) && (TokenType(last_token) == SYM_DOT))
    {
        // Prepend a "." so that identifiers inside structs have a different name space from those outside of structs.
        symstring.insert(0, ".");
    }

    token = ConvertSymstringToTokenIndex(symstring);
    if (token < 0)
    {
        error_encountered = true;
        LastError = "Symbol table overflow - could not add new symbol";
        return;
    }

    if (symbol_type == Scanner::SctStringLiteral)
    {
        TokenizeStringLiteral(token, symstring);
        return;
    }

    if (symbol_type == Scanner::SctIntLiteral)
    {
        SetTokenType(token, SYM_LITERALVALUE);
        return;
    }

    if (symbol_type == Scanner::SctFloatLiteral)
    {
        SetTokenType(token, SYM_LITERALFLOAT);
        return;
    }

    // Check the nesting of () [] {}
    CheckMatcherNesting(token, error_encountered);
    if (error_encountered) return;

    // Count parenthesis nesting separately, is needed in a late rewriting of struct member names later on
    if (TokenType(token) == SYM_OPENPARENTHESIS) ++ParenthesisNestingDepth;
    if (TokenType(token) == SYM_CLOSEPARENTHESIS) --ParenthesisNestingDepth;


    // If we are in a parameter declaration where the parameter is declared with the type and not the name
    // and an initialization follows, then "*=" should be interpreted as the separate symbols "*" and "=".
    // e.g.: "import void someMethod(someClass *= 0);"
    // [fw] This won't work here because we don't know about the types in the tokenizer.
    //      If we want to do it, the "*=" needs to be broken up in the parser, which is a hassle
    //      but doable in principle.

    // We enter struct declaration mode after encountering the keyword "struct".
    if ((last_token >= 0) && (TokenType(last_token) == SYM_STRUCT))
    {
        CurrentMode = ModeStructDecl;
        InTypeSubmode = true;
        BraceNestingDepthInStructDecl = 0;
        StructBeingDeclared = token;
        return;
    }

    if (CurrentMode == ModeStructDecl)
    {
        // The mode ends as soon as the { ... } after the "struct X" ends.
        if (TokenType(token) == SYM_OPENBRACE) ++BraceNestingDepthInStructDecl;
        if (TokenType(token) == SYM_CLOSEBRACE)
        {
            if (--BraceNestingDepthInStructDecl == 0) CurrentMode = ModeStandard;
        }

        // The mode also ends if we never had any { ... } to begin with, i.e. a simple "struct X;".
        if ((TokenType(token) == SYM_SEMICOLON) && (BraceNestingDepthInStructDecl == 0))
        {
            CurrentMode = ModeStandard;
        }
    }

    // If we're in the "root" of a struct declaration, 
    // then all var and func names must have the struct name prepended.
    if ((CurrentMode != ModeStructDecl) ||
        (BraceNestingDepthInStructDecl > 1) ||
        (ParenthesisNestingDepth > 0)) return;

    if (!InTypeSubmode && // see below, comment for "switch (TokenType(token))"
        ((TokenType(token) == 0) || (TokenType(token) == SYM_GLOBALVAR)))
    {
        std::string full_name = FullNameFromStructAndMember(StructBeingDeclared, token);
        token = ConvertSymstringToTokenIndex(full_name);
        if (token < 0)
        {
            error_encountered = true;
            LastError = "Symbol table overflow - could not add new symbol";
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
    case 0:             InTypeSubmode = false; break; // unclassified symbol
    case SYM_LOCALVAR:  InTypeSubmode = false; break;
    case SYM_FUNCTION:  InTypeSubmode = false; break; // keyword "function"
    case SYM_GLOBALVAR: InTypeSubmode = false; break;
    case SYM_SEMICOLON: InTypeSubmode = true; break;
    case SYM_VARTYPE:   InTypeSubmode = false; break; // e.g., "int", "float"
    }
}


void ags::Tokenizer::TokenizeStringLiteral(int token, std::string const & symstring)
{
    SetTokenType(token, SYM_STRING);
    SetTokenVartype(token, SymbolTable->normalStringSym);

    // Enter the string into the string collector
    std::string s_content = symstring.substr(1, symstring.size() - 2); // i.e., without the surrounding quotes
    int offset = StringCollector->add_string(s_content.c_str()); // scaffolding
    SetTokenOffsetInStrings(token, offset);
}


void ags::Tokenizer::GetNextToken(
    int &token,
    bool & eof_encountered, bool & error_encountered)
{
    token = -1;

    // Feed the buffer if it has run dry
    if (TokenBuffer.size() < 2)
    {
        // Get the next symbol from the scanner
        std::string symstring;
        Scanner::ScanType symbol_type;
        Scanner->GetNextSymstring(symstring, symbol_type, eof_encountered, error_encountered);
        if (error_encountered)
        {
            // copy the error from the scanner
            LastError = Scanner->GetLastError();
        }
        if (eof_encountered || error_encountered) return;

        // Special magic (or kludge) to interoperate with the dialog script generator
        // Scripts are generated in a bunch; when a new script starts, the line number must be reset
        if (symstring.substr(0, 18) == NEW_SCRIPT_TOKEN_PREFIX)
        {
            Scanner->SetLineno(0);
        }

        if (TokenBuffer.empty()) TokenBuffer.push_back(-1);
        int last_token = TokenBuffer.back();

        ProcessScannerSymstring(symbol_type, symstring, last_token, token, eof_encountered, error_encountered);
        if (eof_encountered || error_encountered) return;

        TokenBuffer.push_back(token);
    }

    // If the buffer is still dry, return EOF
    if (TokenBuffer.size() < 2)
    {
        eof_encountered = true;
        return;
    }

    // The first token has been returned the previous time, so pop it off now.
    TokenBuffer.pop_front();

    // Return the next first token.
    eof_encountered = false;
    token = TokenBuffer.front();
}


// Check the nesting of () [] {}, error if mismatch
void ags::Tokenizer::CheckMatcherNesting(int token, bool & error_encountered)
{
    switch (TokenType(token))
    {
    default: return;
    case SYM_OPENPARENTHESIS:
        OCMatcher.Push("(", ")", Scanner->GetLineno());
        return;

    case SYM_OPENBRACKET:
        OCMatcher.Push("[", "]", Scanner->GetLineno());
        return;

    case SYM_OPENBRACE:
        OCMatcher.Push("{", "}", Scanner->GetLineno());
        return;

    case SYM_CLOSEPARENTHESIS:
        OCMatcher.PopAndCheck(")", Scanner->GetLineno(), error_encountered);
        if (error_encountered) LastError = OCMatcher.GetLastError();
        return;

    case SYM_CLOSEBRACKET:
        OCMatcher.PopAndCheck("]", Scanner->GetLineno(), error_encountered);
        if (error_encountered) LastError = OCMatcher.GetLastError();
        return;

    case SYM_CLOSEBRACE:
        OCMatcher.PopAndCheck("}", Scanner->GetLineno(), error_encountered);
        if (error_encountered) LastError = OCMatcher.GetLastError();
        return;
    }
}


std::string ags::Tokenizer::FullNameFromStructAndMember(int struct_name_token, int member_name_token)
{
    std::string member_name = SymbolTable->get_name(member_name_token);
    if (member_name.at(0) == '.') member_name.erase(0, 1);

    std::string struct_name = SymbolTable->get_name(struct_name_token);
    struct_name.append("::").append(member_name);
    return struct_name;
}


int ags::Tokenizer::ConvertSymstringToTokenIndex(std::string symstring)
{
    int token = SymbolTable->find(symstring.c_str());
    if (token < 0)
    {
        token = SymbolTable->add(symstring.c_str());
    }
    return token;
}


void ags::OpenCloseMatcher::Reset()
{
    LastError = "";
    OpenInfoStack.resize(0);
}


std::string ags::OpenCloseMatcher::GetLastError()
{
    return LastError;
}


ags::OpenCloseMatcher::OpenCloseMatcher()
{
    Reset();
}


void ags::OpenCloseMatcher::Push(std::string const & opener, std::string const & expected_closer, int lineno)
{
    struct OpenInfo oi;
    oi.Opener = opener;
    oi.Closer = expected_closer;
    oi.Lineno = lineno;

    OpenInfoStack.push_back(oi);
}


void ags::OpenCloseMatcher::PopAndCheck(std::string const & closer, int lineno, bool & error_encountered)
{
    if (OpenInfoStack.empty())
    {
        error_encountered = true;
        LastError = "There isn't any opening symbol that matches this closing '&1'";
        LastError.replace(LastError.find("&1"), 2, closer);
        return;
    }

    struct OpenInfo oi(OpenInfoStack.back());
    OpenInfoStack.pop_back();
    if (oi.Closer != closer)
    {
        error_encountered = true;
        LastError = "Found '&1', this does not match the '&2' on line &3";
        if (oi.Lineno == lineno) LastError = "Found '&1', this does not match the '&2' on this line";
        LastError.replace(LastError.find("&1"), 2, closer);
        LastError.replace(LastError.find("&2"), 2, oi.Opener);
        if (oi.Lineno != lineno) LastError.replace(LastError.find("&3"), 2, std::to_string(oi.Lineno));
    }
}

int ags::NestingStack::Push(NestingType type, ags::CodeLoc_t start, ags::CodeLoc_t info)
{
    std::vector<ags::ccChunk> dummy_chunk;
    struct ags::NestingInfo ni = { type, start, info, 0, dummy_chunk };
    try
    {
        Stack.push_back(ni);
    }
    catch (...)
    {
        cc_error("Out of memory");
        return -1;
    }
    return 0;
}

ags::NestingStack::NestingStack()
{
    // Push first record on stack so that it isn't empty
    Push(ags::NestingStack::NTNothing);
}


// Rip the code that has already been generated, starting from codeoffset, out of scrip
// and move it into the vector at list, instead.
void ags::NestingStack::YankChunk(ccCompiledScript *scrip, ags::CodeLoc_t codeoffset, ags::CodeLoc_t fixupoffset)
{
    ags::ccChunk item;

    for (size_t code_idx = codeoffset; code_idx < scrip->codesize; code_idx++)
    {
        item.Code.push_back(scrip->code[code_idx]);
    }

    for (size_t fixups_idx = fixupoffset; fixups_idx < scrip->numfixups; fixups_idx++)
    {
        item.Fixups.push_back(scrip->fixups[fixups_idx]);
        item.FixupTypes.push_back(scrip->fixuptypes[fixups_idx]);
    }
    item.CodeOffset = codeoffset;
    item.FixupOffset = fixupoffset;

    Stack.back().Chunks.push_back(item);

    // Cut out the code that has been pushed
    scrip->codesize = codeoffset;
    scrip->numfixups = fixupoffset;
}


// Copy the code in ccChunk to the end of the bytecode vector 
void ags::NestingStack::WriteChunk(ccCompiledScript *scrip, size_t level, size_t index)
{
    const ags::ccChunk item = Chunks(level).at(index);
    scrip->flush_line_numbers();
    ags::CodeLoc_t adjust = scrip->codesize - item.CodeOffset;

    size_t limit = item.Code.size();
    for (size_t index = 0; index < limit; index++)
    {
        scrip->write_code(item.Code[index]);
    }

    limit = item.Fixups.size();
    for (size_t index = 0; index < limit; index++)
    {
        scrip->add_fixup(item.Fixups[index] + adjust, item.FixupTypes[index]);
    }
}


std::string ConstructedMemberName; // size limitation removed

const char *get_member_full_name(ags::Symbol_t structSym, ags::Symbol_t memberSym)
{

    // Get C-string of member, de-mangle it if appropriate
    const char* memberNameStr = sym.get_name(memberSym);
    if (memberNameStr[0] == '.') memberNameStr = &memberNameStr[1];

    ConstructedMemberName = sym.get_name(structSym);
    ConstructedMemberName += "::";
    ConstructedMemberName += memberNameStr;

    return ConstructedMemberName.c_str();
}


int sym_find_or_add(symbolTable &sym, const char *sname)
{
    int sym_index = sym.find(sname);
    if (sym_index < 0) sym_index = sym.add(sname);
    return sym_index;
}


// Scan inpl into scan tokens, write line number opcodes, build a symbol table, mangle complex symbols
int cc_tokenize(const char * inpl, ccInternalList * targ, ccCompiledScript * scrip)
{
    ags::Scanner scanner(inpl, 1, targ);
    ags::Tokenizer tokenizer(&scanner, targ, &sym, scrip);

    // Write pseudo opcode for "Starting line 1"
    targ->write_meta(SMETA_LINENUM, 1);

    bool eof_encountered = false;
    bool error_encountered = false;
    while (true)
    {
        int token;
        tokenizer.GetNextToken(token, eof_encountered, error_encountered);
        if (error_encountered)
        {
            cc_error(tokenizer.GetLastError().c_str());
            return -1;
        }
        if (eof_encountered || error_encountered) break;
        targ->write(static_cast<::ags::Symbol_t>(token));
    }

    // Write pseudo opcode for "This ends this tokenization"
    targ->write_meta(SMETA_END, 0);

    tokenizer.ResetTemporaryTypesInSymbolTable();

    return 0;
}


void free_pointer(ccCompiledScript *scrip, int spOffset, int zeroCmd, ags::Symbol_t arraySym) 
{
    scrip->write_cmd1(SCMD_LOADSPOFFS, spOffset);
    scrip->write_cmd(zeroCmd);

    if ((sym.entries[arraySym].flags & (SFLG_ARRAY | SFLG_DYNAMICARRAY)) == SFLG_ARRAY)
    {
        // array of pointers -- release each one
        for (int ee = 1; ee < sym.entries[arraySym].arrsize; ee++)
        {
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, 4);
            scrip->write_cmd(zeroCmd);
        }
    }
}


void free_pointers_from_struct(ccCompiledScript *scrip, ags::Symbol_t structVarSym) 
{
    size_t structType = sym.entries[structVarSym].vartype;

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].stype != SYM_STRUCTMEMBER) continue;
        if (sym.entries[entries_idx].extends != structType) continue;
        if ((sym.entries[entries_idx].flags & SFLG_IMPORTED) != 0) continue;
        if ((sym.entries[entries_idx].flags & SFLG_PROPERTY) != 0) continue;
        
        if ((sym.entries[entries_idx].flags & SFLG_POINTER) == 0)
        {
            // original comment: "if non-pointer struct, need to process its members
            // **** TODO"
            // [fw] depends on how that struct of struct is in the symbol table;
            //      STRUCTSYM.SUBSTRUCTSYM perhaps?
            //      ags:Symbol substruct = find(....);
            //      if (substruct is not in the symtable) continue;

            //      free_pointers_from_struct(scrip, substruct);
            
            continue;
        }
        
        // Locate where the pointer is on the stack
        int spOffs = (scrip->cur_sp - sym.entries[structVarSym].soffs) - sym.entries[entries_idx].soffs;

        free_pointer(scrip, spOffs, SCMD_MEMZEROPTR, static_cast<ags::Symbol_t>(entries_idx));

        if (sym.entries[structVarSym].flags & SFLG_ARRAY)
        {
            // an array of structs, free any pointers in them
            for (int ii = 1; ii < sym.entries[structVarSym].arrsize; ii++)
            {
                spOffs -= sym.entries[structType].ssize;
                free_pointer(scrip, spOffs, SCMD_MEMZEROPTR, static_cast<ags::Symbol_t>(entries_idx));
            }
        }
    }
}


inline bool is_any_type_of_string(ags::Symbol_t symtype)
{
    symtype &= ~(STYPE_CONST | STYPE_POINTER);
    if ((symtype == sym.normalStringSym) || (symtype == sym.stringStructSym))
        return true;
    return false;
}


// Return number of bytes to remove from stack to unallocate local vars
// of level from_level or higher
int stacksize_of_locals(int from_level)
{
    int totalsub = 0;
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope <= from_level) continue;
        if (sym.entries[entries_idx].stype != SYM_LOCALVAR) continue;

        // caller will sort out stack, so ignore parameters
        if ((sym.entries[entries_idx].flags & SFLG_PARAMETER) != 0) continue;
       
        if ((sym.entries[entries_idx].flags & SFLG_DYNAMICARRAY) != 0)
        {    
            totalsub += 4; // size of an implicit pointer
            continue;
        }

        // Calculate the size of one var of the given type
        size_t ssize = sym.entries[entries_idx].ssize;
        if (sym.entries[entries_idx].flags & SFLG_STRBUFFER) ssize += STRING_LENGTH;
        
        // Calculate the number of vars
        size_t number = 1;
        if (sym.entries[entries_idx].flags & SFLG_ARRAY) number = sym.entries[entries_idx].arrsize;
        
        totalsub += ssize * number;
    }
    return totalsub;
}


// Free the pointers of any locals in level from_level or higher
int free_pointers_of_locals(ccCompiledScript *scrip, int from_level)
{
    int totalsub = 0;
    int zeroPtrCmd = SCMD_MEMZEROPTR;
    if (from_level == 0) zeroPtrCmd = SCMD_MEMZEROPTRND;

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope <= from_level) continue;
        if (sym.entries[entries_idx].stype != SYM_LOCALVAR) continue;
        
        // don't touch the this pointer
        if (sym.entries[entries_idx].flags & SFLG_THISPTR) continue;

        if (((sym.entries[entries_idx].flags & SFLG_POINTER) != 0) ||
            ((sym.entries[entries_idx].flags & SFLG_DYNAMICARRAY) != 0))
        {
            free_pointer(scrip, scrip->cur_sp - sym.entries[entries_idx].soffs, zeroPtrCmd, static_cast<ags::Symbol_t>(entries_idx));
            continue;
        }
 
        if (sym.entries[sym.entries[entries_idx].vartype].flags & SFLG_STRUCTTYPE)
        {
            // free any pointers that this struct contains
            free_pointers_from_struct(scrip, static_cast<ags::Symbol_t>(entries_idx));
            continue;
        }
    }
    return 0;
}

// Remove defns from the sym table of vars defined on from_level or higher
int remove_locals_from_symtable(int from_level)
{
    
    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        if (sym.entries[entries_idx].sscope < from_level) continue;
        if (sym.entries[entries_idx].stype != SYM_LOCALVAR) continue;

        sym.entries[entries_idx].stype = 0;
        sym.entries[entries_idx].sscope = 0;
        sym.entries[entries_idx].flags = 0;
    }
    return 0;
}


// The type NT(Un)bracedElse isn't only used for ELSE branches, but also for the
// body of WHILE and other statements. 
// If the symbol "else" follows a THEN clause of an IF, this is handled most easily
// by adding an unconditional jump out and then changing the THEN clause into an ELSE clause. 
int deal_with_end_of_ifelse(ccInternalList *targ, ccCompiledScript*scrip, ags::NestingStack *nesting_stack, bool &else_after_then)
{
    // Check whether the symbol "else" follows a then branch
    else_after_then = false;
    if (nesting_stack->Type() == ags::NestingStack::NTUnbracedElse);
    else if (nesting_stack->Type()== ags::NestingStack::NTBracedElse);
    else if (sym.get_type(targ->peeknext()) == SYM_ELSE)
    {
        targ->getnext();  // eat "else"
        scrip->write_cmd1(SCMD_JMP, 0); // jump out, to be patched later
        else_after_then = true;
    }

    if (nesting_stack->StartLoc()) // a loop that features a jump back to the start
    {
        scrip->flush_line_numbers();
        // if it's a for loop, drop the yanked chunk (loop increment) back in
        if (nesting_stack->ChunksExist())
        {
            nesting_stack->WriteChunk(scrip, 0);
            nesting_stack->Chunks().clear();
        }

        // jump back to the start location
        scrip->write_cmd1(SCMD_JMP, -((scrip->codesize + 2) - nesting_stack->StartLoc()));
    }

    // Patch the jump out of the construct to point to here
    scrip->code[nesting_stack->JumpOutLoc()] =
        (scrip->codesize - nesting_stack->JumpOutLoc()) - 1;

    if (else_after_then)
    {
        // convert the THEN branch into an ELSE, i.e., stay on the same Depth()
        nesting_stack->SetType(ags::NestingStack::NTUnbracedElse);
        if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
        {
            nesting_stack->SetType(ags::NestingStack::NTBracedElse);
            targ->getnext();
        }
        
        nesting_stack->SetJumpOutLoc(scrip->codesize - 1);

        // We're continuing the current nesting until the "else" branch has ended.
        // So we haven't ended the current nesting yet. So we leave at this point.
        return 0;
    }
    
    // Clause ends, so pop the level off the stack
    nesting_stack->Pop();

    if (nesting_stack->Type() == ags::NestingStack::NTFor)
    {
        // A FOR is represented by two nestings, so we need to pop another level
        nesting_stack->Pop();
        
        // The outer nesting of the FOR can contain defns, e.g., "for (int i = 0;...)"
        // defines i. Free these definitions
        free_pointers_of_locals(scrip, nesting_stack->Depth() - 1);
        int totalsub = stacksize_of_locals(nesting_stack->Depth() - 1);
        if (totalsub > 0)
        {
            scrip->cur_sp -= totalsub;
            scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
        }
        remove_locals_from_symtable(nesting_stack->Depth());
    }
    
    return 0;
}


int deal_with_end_of_do(ccInternalList *targ, ccCompiledScript *scrip, ags::NestingStack *nesting_stack) 
{
    scrip->flush_line_numbers();

    ags::Symbol_t cursym = targ->getnext();
    if (sym.get_type(cursym) != SYM_WHILE)
    {
        cc_error("Do without while");
        return -1;
    }
    if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    scrip->flush_line_numbers();

    int retval = evaluate_expression(targ, scrip, true);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // Jump back to the start of the loop while the condition is true
    scrip->write_cmd1(SCMD_JNZ, -((scrip->codesize + 2) - nesting_stack->StartLoc()));
    // Patch the jump out of the loop; it should point to here
    scrip->code[nesting_stack->JumpOutLoc()] = (scrip->codesize - nesting_stack->JumpOutLoc()) - 1;
    
    // The clause has ended, so pop the level off the stack
    nesting_stack->Pop();

    return 0;
}


int deal_with_end_of_switch(ccInternalList *targ, ccCompiledScript *scrip, ags::NestingStack *nesting_stack) 
{
    const size_t ns_level = nesting_stack->Depth() - 1;

    // If there was no terminating break, write a jump at the end of the last case
    if (scrip->code[scrip->codesize - 2] != SCMD_JMP || 
        scrip->code[scrip->codesize - 1] != nesting_stack->StartLoc() - scrip->codesize + 2)
    {
        scrip->write_cmd1(SCMD_JMP, 0);
    }

    // We begin the jump table; remember this address
    ags::CodeLoc_t jumptable_loc = scrip->codesize;
    
    // Patch the instruction "Jump to the jump table" at the start of the switch
    // so that it points to the correct address, i.e., here
    scrip->code[nesting_stack->StartLoc() + 1] = (jumptable_loc - nesting_stack->StartLoc()) - 2;

    // Get correct comparison operation: Don't compare strings as pointers but as strings
    int noteq_op = is_any_type_of_string(nesting_stack->SwitchExprType()) ? SCMD_STRINGSNOTEQ : SCMD_NOTEQUAL;
    
    const size_t size_of_chunks = nesting_stack->Chunks().size();
    for (size_t index = 0; index < size_of_chunks; index++)
    {
        // Put the result of the expression into AX
        nesting_stack->WriteChunk(scrip, index);
        // Do the comparison
        scrip->write_cmd2(noteq_op, SREG_AX, SREG_BX);
        scrip->write_cmd1(SCMD_JZ, nesting_stack->Chunks().at(index).CodeOffset - scrip->codesize - 2);
    }
    
    // Write the default jump if necessary
    if (nesting_stack->DefaultLabelLoc() != -1)
        scrip->write_cmd1(SCMD_JMP, nesting_stack->DefaultLabelLoc() - scrip->codesize - 2);
    
    // Patch th jump to the end of the switch block (for break statements)
    scrip->code[nesting_stack->StartLoc() + 3] = scrip->codesize - nesting_stack->StartLoc() - 4;
    
    // Patch the jump for the end of the switch block
    scrip->code[jumptable_loc - 1] = scrip->codesize - jumptable_loc;

    nesting_stack->Chunks().clear(); 
    nesting_stack->Pop();

    return 0;
}

// Given a struct STRUCT and a member MEMBER, 
// finds the symbol of the fully qualified name STRUCT::MEMBER and returns it in memSym.
// Gives error if the fully qualified symbol is protected.
int find_member_sym(ags::Symbol_t structSym, ags::Symbol_t &memSym, bool allowProtected) {

    // Construct a string out of struct and member, look it up in the symbol table
    const char *name_as_cstring = get_member_full_name(structSym, memSym);
    ags::Symbol_t full_name = sym.find(name_as_cstring);

    if (full_name < 0)
    {
        if (sym.entries[structSym].extends > 0)
        {
            // look for the member in the ancesters recursively
            if (find_member_sym(sym.entries[structSym].extends, memSym, allowProtected) == 0) return 0;
            // the inherited member was not found, so fall through to the error message
        }
        cc_error("'%s' is not a public member of '%s'. Are you sure you spelt it correctly (remember, capital letters are important)?",
            sym.get_friendly_name(memSym).c_str(),
            sym.get_friendly_name(structSym).c_str());
        return -1;
    }

    if ((!allowProtected) && (sym.entries[full_name].flags & SFLG_PROTECTED))
    {
        cc_error("Cannot access protected member '%s'", sym.get_friendly_name(full_name).c_str());
        return -1;
    }
    memSym = full_name;
    return 0;
}


std::string friendly_int_symbol(int symidx, bool isNegative)
{
    return (isNegative? "-" : "") + sym.get_friendly_name(symidx);
}


int accept_literal_or_constant_value(ags::Symbol_t fromSym, int &theValue, bool isNegative, const char *errorMsg) {
    if (sym.get_type(fromSym) == SYM_CONSTANT)
    {
        theValue = sym.entries[fromSym].soffs;
        if (isNegative) theValue = -theValue;
        return 0;
    }

    if (sym.get_type(fromSym) == SYM_LITERALVALUE)
    {
        std::string literalStrValue = std::string(sym.get_name(fromSym));
        if (isNegative) literalStrValue = '-' + literalStrValue;

        // convert to LONG, but reject the result if out ouf INT range.
        errno = 0;
        char *endptr = 0;
        const long longValue = strtol(literalStrValue.c_str(), &endptr, 10);
        if ((longValue == LONG_MIN && errno == ERANGE) ||
            (isNegative && (endptr[0] != '\0')) ||
            (longValue < INT_MIN))
        {
            cc_error("Literal value '%s' is too low (min. is '%d')", literalStrValue.c_str(), INT_MIN);
            return -1;
        }

        if ((longValue == LONG_MAX && errno == ERANGE) ||
            ((!isNegative) && (endptr[0] != '\0')) ||
            (longValue > INT_MAX))
        {
            cc_error("Literal value %s is too high (max. is %d)", literalStrValue.c_str(), INT_MAX);
            return -1;
        }

        theValue = static_cast<int>(longValue);
        return 0;
    }

    cc_error((char*)errorMsg);
    return -1;
}


bool ReachedEOF(ccInternalList *targ)
{
    if (targ->peeknext() != SCODE_INVALID) return false;

    // We are past the last symbol in the file
    targ->getnext();
    currentline = targ->lineAtEnd;
    return true;
}


// We're parsing a parameter list and we have accepted something like "(...int i"
// We accept a default value clause like "= 15" if it follows at this point.
int param_list_param_DefaultValue(
    ccInternalList *targ,
    bool &has_default_int,
    int &default_int_value)
{

    if (sym.get_type(targ->peeknext()) != SYM_ASSIGN)
    {
        has_default_int = false;
        return 0;
    }

    has_default_int = true;

    // parameter has default value
    targ->getnext();   // eat "="

    bool default_is_negative = false;
    ags::Symbol_t default_value_symbol = targ->getnext(); // may be '-', too
    if (default_value_symbol == sym.find("-"))
    {
        default_is_negative = true;
        default_value_symbol = targ->getnext();
    }

    // extract the default value
    int retval = accept_literal_or_constant_value(default_value_symbol, default_int_value, default_is_negative, "Parameter default value must be literal");
    if (retval < 0) return -1;

    return 0;
}


// process a dynamic array declaration, when present
// We have accepted something like "int foo" and we might expect a trailing "[]" here
// Return values:  0 -- not an array, 1 -- an array, -1 -- error occurred
int param_list_param_DynArrayMarker(
    ccInternalList *targ,
    ags::Symbol_t typeSym,
    bool isPointer)
{
    if (sym.get_type(targ->peeknext()) != SYM_OPENBRACKET) return 0;

    // Gobble the '[', expect and gobble ']'
    targ->getnext();
    if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
    {
        cc_error("Fixed array size cannot be used here (use '[]' instead)");
        return -1;
    }

    if (sym.entries[typeSym].flags & SFLG_STRUCTTYPE)
    {
        if (!(sym.entries[typeSym].flags & SFLG_MANAGED))
        {
            cc_error("Cannot pass non-managed struct array");
            return -1;
        }
        if (!isPointer)
        {
            cc_error("Cannot pass non-pointer struct array");
            return -1;
        }
    }
    return 1;
}


// extender function, eg. function GoAway(this Character *someone)
// We've just accepted something like "int func(", we expect "this" --OR-- "static" (!)
// We'll accept something like "this Character *"
int process_function_decl_ExtenderPreparations(
    ccInternalList * targ,
    ccCompiledScript * scrip,
    bool func_is_static_extender,
    Importness func_is_import,
    std::string & functionName,
    ags::Symbol_t &funcsym,
    ags::Symbol_t &struct_extended_by_the_func,
    SymbolTableEntry * oldDefinition)
{
    targ->getnext(); // accept "this" or "static"
    if (sym.get_type(targ->peeknext()) != SYM_VARTYPE)
    {
        cc_error("'%s' must be followed by a struct name",
            (func_is_static_extender ? "static" : "this"));
        return -1;
    }

    if ((sym.entries[targ->peeknext()].flags & SFLG_STRUCTTYPE) == 0)
    {
        cc_error("'%s' cannot be used with primitive types",
            (func_is_static_extender ? "static" : "this"));
        return -1;
    }

    if (functionName.find_first_of(':', 0) != std::string::npos)
    {
        cc_error("Extender functions cannot be part of a struct");
        return -1;
    }

    struct_extended_by_the_func = targ->peeknext();
    functionName = sym.get_name(struct_extended_by_the_func);
    functionName += "::";
    functionName += sym.get_name(funcsym);

    funcsym = sym.find(functionName.c_str());
    if (funcsym < 0) funcsym = sym.add(functionName.c_str());

    if (func_is_import == ImNoImport && (sym.entries[funcsym].flags & SFLG_IMPORTED) != 0)
    {
        // Copy so that the forward definition can be compared afterwards to the real one 
        int retval = scrip->copy_import_symbol_table_entry(funcsym, oldDefinition);
        if (retval < 0) return retval;
        oldDefinition->flags &= ~SFLG_IMPORTED; // Strip import flag, since the real defn won't be exported

        // Check whether the import has been referenced or whether imports may not be overridden;
        // if so, complain; remove the import flags
        retval = scrip->just_remove_any_import(funcsym);
        if (retval < 0) return retval;

    }

    if (sym.entries[funcsym].stype != 0)
    {
        cc_error("Function '%s' is already defined", functionName);
        return -1;
    }

    sym.entries[funcsym].flags = SFLG_STRUCTMEMBER;
    if (func_is_static_extender)
    {
        sym.entries[funcsym].flags |= SFLG_STATIC;
    }

    targ->getnext();
    if (!func_is_static_extender && targ->getnext() != sym.find("*"))
    {
        cc_error("Instance extender function must be pointer");
        return -1;
    }

    if ((sym.get_type(targ->peeknext()) != SYM_COMMA) &&
        (sym.get_type(targ->peeknext()) != SYM_CLOSEPARENTHESIS))
    {
        if (strcmp(sym.get_name(targ->getnext()), "*") == 0)
            cc_error("Static extender function cannot be pointer");
        else
            cc_error("Parameter name cannot be defined for extender type");
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == SYM_COMMA)
    {
        targ->getnext();
    }

    return 0;
}


int process_function_decl_NonExtenderPreparations(
    int struct_containing_the_func,
    std::string & functionName,
    ags::Symbol_t & funcsym)
{
    if (sym.get_type(funcsym) != SYM_VARTYPE) return 0; // nothing to do

    if (struct_containing_the_func != 0)
    {
        // Construct STRUCT::FOO, enter it into the symbol table if necessary
        functionName = sym.get_name(struct_containing_the_func);
        functionName += "::";
        functionName += sym.get_name(funcsym);
        funcsym = sym.find(functionName.c_str());
        if (funcsym < 0) funcsym = sym.add(functionName.c_str());
    }
    if (sym.entries[funcsym].stype != 0)
    {
        cc_error("'%s' is already defined", functionName);
        return -1;
    }

    return 0;
}


int process_func_paramlist_ParamType(ccInternalList * targ, int param_type, int param_idx, bool &param_is_ptr)
{
    // Determine whether the type is a pointer
    if (strcmp(sym.get_name(targ->peeknext()), "*") == 0)
    {
        param_is_ptr = true;
        targ->getnext(); // gobble the '*'
    }
    if (sym.entries[param_type].flags & SFLG_AUTOPTR)
    {
        param_is_ptr = true;
    }

    // Safety checks on the parameter type
    if (param_type == sym.normalVoidSym)
    {
        cc_error("A function parameter must not have the type 'void'");
        return -1;
    }
    if (param_is_ptr)
    {
        if ((sym.entries[param_type].flags & SFLG_MANAGED) == 0)
        {
            // can only point to managed structs
            cc_error("Cannot declare pointer to non-managed type");
            return -1;
        }
        if (sym.entries[param_type].flags & SFLG_AUTOPTR)
        {
            cc_error("Invalid use of pointer");
            return -1;
        }
    }
    if ((sym.entries[param_type].flags & SFLG_STRUCTTYPE) && (!param_is_ptr))
    {
        cc_error("A struct cannot be passed as parameter");
        return -1;
    }
    return 0;
}


enum param_defaults
{
    pd_no_default = 0,
    pd_int_default,
    pd_string_default         // not implemented yet
};

// We're accepting a parameter list. We've accepted something like "int".
// We accept a param name such as "i" if present and a default clause such as "= 5" if present.
int process_func_paramlist_param_NameAndDefault(
    ccInternalList * targ,
    bool param_list_is_declaration,
    ags::Symbol_t & param_name,
    bool & param_has_int_default,
    int & param_int_default)
{
    param_name = -1;
    param_int_default = 0;

    if (param_list_is_declaration)
    {
        // Ignore the parameter name when present, it won't be used later on
        param_name = -1;
        int next_type = sym.get_type(targ->peeknext());
        if ((next_type == 0) || next_type == SYM_GLOBALVAR)
        {
            targ->getnext();
        }

        // but Get the value of a default assignment, must be returned out of the function
        int retval = param_list_param_DefaultValue(targ, param_has_int_default, param_int_default);
        if (retval < 0) return -1;

        return 0;
    }

    // parameter list is a definition
    if (sym.get_type(targ->peeknext()) == SYM_GLOBALVAR)
    {
        // This is a definition -- so the parameter name must not be a global variable
        cc_error("The name '%s' is already used for a global variable", sym.get_name(targ->peeknext()));
        return -1;
    }

    if (sym.get_type(targ->peeknext()) != 0)
    {
        // We need to have a real parameter name here
        cc_error("Expected a parameter name here, found '%s' instead", sym.get_name(targ->peeknext()));
    }

    param_name = targ->getnext(); // get and gobble the parameter name

    return 0;
}


// process a parameter decl in a function parameter list, something like int foo(INT BAR
int process_func_paramlist_Param(
    ccInternalList * targ,
    ccCompiledScript * scrip,
    ags::Symbol_t funcsym,
    Importness func_is_import,
    ags::Symbol_t cursym,
    int curtype,
    bool param_is_const,
    int param_idx)
{
    // Determine the parameter type and gobble it completely
    // (Note: Later on, the type might turn out to be dynamic array)
    int param_type = cursym;
    bool param_is_ptr = false;

    int retval = process_func_paramlist_ParamType(targ, param_type, param_idx, param_is_ptr);
    if (retval < 0) return retval;

    if (ReachedEOF(targ))
    {
        cc_error("Reached end of input within an open parameter list");
        return -1;
    }

    // Currently, a parameter list is seen as a declaration (instead of a definition) when
    // the function statement has the "import" keyword. 
    // A declaration doesn't have a function body following. 
    // We create local variables iff we are within a definition
    bool param_list_is_declaration = (func_is_import != ImNoImport);
    bool createdLocalVar = !param_list_is_declaration;

    // Process the parameter name (when present and meaningful) and the default (when present)
    ags::Symbol_t param_name;
    bool param_has_int_default;
    int param_int_default;
    retval = process_func_paramlist_param_NameAndDefault(
        targ, param_list_is_declaration, param_name, param_has_int_default, param_int_default);
    if (retval < 0) return retval;

    // Process a dynamic array signifier (when present)
    retval = param_list_param_DynArrayMarker(targ, cursym, param_is_ptr);
    if (retval < 0) return retval;
    bool param_is_dynarray = (retval == 1);

    // If a local variable has been created, enter this into the symbol table
    if (createdLocalVar)
    {
        sym.entries[param_name].stype = SYM_LOCALVAR;
        sym.entries[param_name].extends = false;
        sym.entries[param_name].arrsize = 1;
        sym.entries[param_name].vartype = param_type;
        sym.entries[param_name].ssize = 4;
        sym.entries[param_name].sscope = 1;
        sym.entries[param_name].flags |= SFLG_PARAMETER;
        if (param_is_ptr)      sym.entries[param_name].flags |= SFLG_POINTER;
        if (param_is_const)    sym.entries[param_name].flags |= SFLG_CONST | SFLG_READONLY;
        if (param_is_dynarray) sym.entries[param_name].flags |= SFLG_DYNAMICARRAY | SFLG_ARRAY;
        // the parameters are pushed backwards, so the top of the
        // stack has the first parameter. The +1 is because the
        // call will push the return address onto the stack as well
        sym.entries[param_name].soffs = scrip->cur_sp - (param_idx + 1) * 4;
    }

    // Augment the function type in the symbol table  
    sym.entries[funcsym].funcparamtypes[param_idx] = param_type;
    sym.entries[funcsym].funcParamHasDefaultValues[param_idx] = param_has_int_default;
    sym.entries[funcsym].funcParamDefaultValues[param_idx] = param_int_default;

    if (param_is_ptr)      sym.entries[funcsym].funcparamtypes[param_idx] |= STYPE_POINTER;
    if (param_is_const)    sym.entries[funcsym].funcparamtypes[param_idx] |= STYPE_CONST;
    if (param_is_dynarray) sym.entries[funcsym].funcparamtypes[param_idx] |= STYPE_DYNARRAY;

    return 0;
}


int process_function_decl_paramlist(
    ccInternalList * targ,
    ccCompiledScript * scrip,
    ags::Symbol_t funcsym,
    Importness func_is_import,
    int &numparams)
{
    bool param_is_const = false;
    while (true)
    {
        ags::Symbol_t cursym = targ->getnext();
        int curtype = sym.get_type(cursym);

        if (curtype == SYM_CLOSEPARENTHESIS) break;

        if (curtype == SYM_VARARGS)
        {
            // variable number of arguments
            numparams += 100;
            cursym = targ->getnext();
            if (sym.get_type(cursym) != SYM_CLOSEPARENTHESIS)
            {
                cc_error("Expected ')' after variable-args");
                return -1;
            }
            break;
        }

        if (curtype == SYM_CONST)
        {
            // get next token, this must be a variable type
            ags::Symbol_t cursym = targ->getnext();
            int curtype = sym.get_type(cursym);
            if (curtype != SYM_VARTYPE)
            {
                cc_error("Expected a type after 'const'");
                return -1;
            }
            param_is_const = true;
        }

        if (curtype == SYM_VARTYPE)
        {
            if ((numparams % 100) >= MAX_FUNCTION_PARAMETERS)
            {
                cc_error("Too many parameters defined for function");
                return -1;
            }

            int retval = process_func_paramlist_Param(targ, scrip, funcsym, func_is_import, cursym, curtype, param_is_const, numparams);
            if (retval < 0) return retval;
            numparams++;
            param_is_const = false;
            int nexttype = sym.get_type(targ->peeknext());
            if (nexttype == SYM_COMMA) targ->getnext();
            continue;
        }

        // something odd was inside the parentheses
        cc_error("Unexpected '%s' in function parameter list", sym.get_friendly_name(cursym).c_str());
        return -1;
    }
    return 0;
}


int process_function_decl_CheckForIllegalCombis(bool func_is_readonly, int & in_func, size_t stack_depth)
{
    if (func_is_readonly)
    {
        cc_error("Readonly cannot be applied to a function");
        return -1;
    }

    if ((in_func >= 0) || (stack_depth > 1))
    {
        cc_error("Nested functions not supported (you may have forgotten a closing brace)");
        return -1;
    }
    return 0;
}


int process_function_decl_SetFunctype(
    ags::Symbol_t funcsym,
    int ret_parameter_size,
    int return_type,
    bool func_returns_ptr,
    bool func_returns_dynarray,
    bool func_is_static)
{
    sym.entries[funcsym].stype = SYM_FUNCTION;
    sym.entries[funcsym].ssize = ret_parameter_size;
    sym.entries[funcsym].funcparamtypes[0] = return_type;

    if (func_returns_ptr)
    {
        sym.entries[funcsym].funcparamtypes[0] |= STYPE_POINTER;
    }
    if (func_returns_dynarray)
    {
        sym.entries[funcsym].funcparamtypes[0] |= STYPE_DYNARRAY;
    }

    if ((!func_returns_ptr) && (!func_returns_dynarray) &&
        ((sym.entries[return_type].flags & SFLG_STRUCTTYPE) != 0))
    {
        cc_error("Cannot return entire struct from function");
        return -1;
    }
    if (func_is_static)
    {
        sym.entries[funcsym].flags |= SFLG_STATIC;
    }

    return 0;
}


// We're at something like "int foo(", directly before the "("
// This might or might not be within a struct defn
int process_function_declaration(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    ags::Symbol_t &funcsym,
    int return_type, // from "vtwas" in the caller - is the return type in here
    bool func_returns_ptr,
    bool func_returns_dynarray,
    bool func_is_static,
    Importness func_is_import,  // NOT a bool: it can contain 0 .. 2
    int struct_containing_the_func,
    int &idx_of_func, // Index of the function functions[] array; = -1 for imports
    ags::Symbol_t &struct_extended_by_the_func, // the BAR in "int FOO(this BAR *," 
    SymbolTableEntry *oldDefinition)
{
    int numparams = 1; // Counts the number of parameters including the ret parameter, so start at 1
    int ret_parameter_size = sym.entries[return_type].ssize;

    // Internal name of the function being defined, may be re-written (mangled) later on
    std::string functionName = sym.get_name(funcsym);

    // skip the opening "("
    targ->getnext();
    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    bool func_is_static_extender = (sym.get_type(targ->peeknext()) == SYM_STATIC);
    bool func_is_extender = (func_is_static_extender) || (targ->peeknext() == sym.find("this"));

    // Set up the function
    if (func_is_extender)
    {
        int retval = process_function_decl_ExtenderPreparations(
            targ, scrip, func_is_static_extender,
            func_is_import, functionName, funcsym, struct_extended_by_the_func,
            oldDefinition);
        if (retval < 0) return retval;
    }
    else // !func_is_extender
    {
        int retval = process_function_decl_NonExtenderPreparations(struct_containing_the_func, functionName, funcsym);
        if (retval < 0) return retval;
    }

    // Type the function in the symbol table
    int retval = process_function_decl_SetFunctype(
        funcsym, ret_parameter_size, return_type, func_returns_ptr,
        func_returns_dynarray, func_is_static);
    if (retval < 0) return retval;

    // Get function number and function index
    int funcNum = -1;

    // Enter the function in the imports[] or functions[] array; get its index
    if (func_is_import != ImNoImport)
    {
        // in_func = Index of the function in the ccScript::imports[] array
        // Note: currently, add_new_import() cannot return values < 0, so idx_of_func must be >= 0 here
        idx_of_func = scrip->add_new_import(functionName.c_str());
    }
    else
    {
        // in_func = Index of the function in the ccCompiledScript::functions[] array
        idx_of_func = scrip->add_new_function(functionName.c_str(), &funcNum);
        if (idx_of_func < 0)
        {
            cc_error("Max. number of functions exceeded");
            return -1;
        }
    }

    sym.entries[funcsym].soffs = idx_of_func;  // save index of the function
    scrip->cur_sp += 4;  // the return address will be pushed

    // process parameter list, get number of parameters
    retval = process_function_decl_paramlist(targ, scrip, funcsym, func_is_import, numparams);
    if (retval < 0) return retval;

    // save the number of parameters (not counting the ret parameter)
    sym.entries[funcsym].sscope = (numparams - 1);
    if (funcNum >= 0) scrip->funcnumparams[funcNum] = sym.entries[funcsym].sscope;

    // Non-imported functions must be followed by a body
    if (func_is_import == ImNoImport)
    {
        if (sym.get_type(targ->peeknext()) != SYM_OPENBRACE)
        {
            cc_error("Expected '{'");
            return -1;
        }
        return 0;
    }


    // import functions
    sym.entries[funcsym].flags |= SFLG_IMPORTED;

    if (struct_containing_the_func > 0)
    {
        // Append the number of parameters to the name of the import
        char appendage[10];
        sprintf(appendage, "^%d", sym.entries[funcsym].sscope);
        strcat(scrip->imports[idx_of_func], appendage);
    }

    // member function expects the ';' to still be there whereas normal function does not
    ags::Symbol_t nextvar = targ->peeknext();
    if (struct_containing_the_func == 0) nextvar = targ->getnext();

    if (sym.get_type(nextvar) != SYM_SEMICOLON)
    {
        cc_error("';' expected (cannot define body of imported function)");
        return -1;
    }

    // This is to keep the caller from overwriting the function symbol.
    idx_of_func = -1;
    return 0;
}


// interpret the float as if it were an int (without converting it really);
// return that int
inline int interpret_float_as_int(float floatval)
{
    float *floatptr = &floatval; // Get pointer to the float
    int *intptr = reinterpret_cast<int *>(floatptr); // pretend that it points to an int
    return *intptr; // return the int that the pointer points to
}


// Whether the current symbol can still be part of the expression
bool canBePartOfExpression(ccInternalList *targ, size_t script_idx)
{
    // NOTEXPRESSION is so defined that all lower symbols can be part of an expression
    if (sym.get_type(targ->script[script_idx]) < NOTEXPRESSION)  return true;

    // "new TYPE" is legal, too (reserving dynamic space)
    if (sym.get_type(targ->script[script_idx]) == SYM_NEW)
    {
        return true;
    }
    if ((sym.get_type(targ->script[script_idx]) == SYM_VARTYPE) &&
        (script_idx > 0) &&
        (sym.get_type(targ->script[script_idx - 1]) == SYM_NEW))
    {
        return true;
    }

    // "TYPE ." is legal, too (accessing a component of a static class)
    if (((int)script_idx < targ->length - 1) &&
        (sym.get_type(targ->script[script_idx + 1]) == SYM_DOT) &&
        (sym.get_type(targ->script[script_idx]) == SYM_VARTYPE))
    {
        return true;
    }

    return false;
}


// The higher the MATHEMATICAL priority of an operator, the MORE binding it is.
// For example, "*" has a higher mathematical priority than "-".
// In contrast to this, "size" gives the priority in the INVERSE way: 
// The higher sym.entries[op].ssize is, the LESS binding is the operator op.
// To convert, we must subtract this value from some suitable value 
// (any will do that doesn't cause underflow of the subtraction).
inline int math_prio(ags::Symbol_t op)
{
    return 100 - sym.entries[op].ssize;
}


// return the index of the lowest MATHEMATICAL priority operator in the list,
// so that either side of it can be evaluated first.
// returns -1 if no operator was found
int index_of_lowest_bonding_operator(ags::SymbolScript_t slist, size_t slist_len)
{
    size_t bracket_nesting_depth = 0;
    size_t paren_nesting_depth = 0;

    int lowest_math_prio_found = std::numeric_limits<int>::max(); // c++ STL lingo for MAXINT
    int index_of_lowest_math_prio = -1;

    for (size_t slist_idx = 0; slist_idx < slist_len; slist_idx++)
    {
        int thisType = sym.get_type(slist[slist_idx]);
        switch (thisType)
        {
        default:
            break;
        case SYM_OPENBRACKET:
            bracket_nesting_depth++;
            continue;
        case SYM_CLOSEBRACKET:
            if (bracket_nesting_depth > 0) bracket_nesting_depth--;
            continue;
        case SYM_OPENPARENTHESIS:
            paren_nesting_depth++;
            continue;
        case SYM_CLOSEPARENTHESIS:
            if (paren_nesting_depth > 0) paren_nesting_depth--;
            continue;
        }

        // Continue if we aren't at zero nesting depth, since ()[] take priority
        if (paren_nesting_depth > 0 || bracket_nesting_depth > 0) continue;

        if (thisType != SYM_OPERATOR && thisType != SYM_NEW) continue;

        int this_math_prio = math_prio(slist[slist_idx]);
        if (this_math_prio > lowest_math_prio_found) continue; // can't be lowest priority

        // remember this and keep looking
        lowest_math_prio_found = this_math_prio;
        index_of_lowest_math_prio = slist_idx;
    }
    return index_of_lowest_math_prio;
}


inline bool is_string(int valtype)
{
    if (valtype == 0) return false;
    const char *type_string = sym.get_name(valtype);
    if (!type_string) return false;

    if (strcmp(type_string, "const string") == 0) return true;
    if (strcmp(type_string, "string") == 0) return true;
    if (strcmp(type_string, "char*") == 0) return true;

    return false;
}


// Change the generic operator vcpuOp to the one that is correct for the types
int get_operator_valid_for_type(int type1, int type2, int &vcpuOp)
{
    if ((type1 == sym.normalFloatSym) || (type2 == sym.normalFloatSym))
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to float values");
            return -1;
        case SCMD_ADD:      vcpuOp = SCMD_FADD; break;
        case SCMD_ADDREG:   vcpuOp = SCMD_FADDREG; break;
        case SCMD_DIVREG:   vcpuOp = SCMD_FDIVREG; break;
        case SCMD_GREATER:  vcpuOp = SCMD_FGREATER; break;
        case SCMD_GTE:      vcpuOp = SCMD_FGTE; break;
        case SCMD_ISEQUAL:  break;
        case SCMD_LESSTHAN: vcpuOp = SCMD_FLESSTHAN; break;
        case SCMD_LTE:      vcpuOp = SCMD_FLTE; break;
        case SCMD_MULREG:   vcpuOp = SCMD_FMULREG; break;
        case SCMD_NOTEQUAL: break;
        case SCMD_SUB:      vcpuOp = SCMD_FSUB; break;
        case SCMD_SUBREG:   vcpuOp = SCMD_FSUBREG; break;
        }
    }

    if (is_any_type_of_string(type1) && is_any_type_of_string(type2))
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to string values");
            return -1;
        case SCMD_ISEQUAL:  vcpuOp = SCMD_STRINGSEQUAL; return 0;
        case SCMD_NOTEQUAL: vcpuOp = SCMD_STRINGSNOTEQ; return 0;
        }
    }

    if ((type1 & STYPE_POINTER) != 0 && (type2 & STYPE_POINTER) != 0)
    {
        switch (vcpuOp)
        {
        default:
            cc_error("The operator cannot be applied to pointers");
            return -1;
        case SCMD_ISEQUAL:  return 0;
        case SCMD_NOTEQUAL: return 0;
        }
    }

    // Other combinations of pointers and/or strings won't mingle
    if (is_string(type1) ||
        is_string(type2) ||
        (type1 & STYPE_POINTER) != 0 ||
        (type2 & STYPE_POINTER) != 0)
    {
        cc_error("The operator cannot be applied to values of these types");
        return -1;
    }

    return 0;
}


// Check for a type mismatch in one direction only
bool is_type_mismatch_oneway(int typeIs, int typeWantsToBe)
{
    // cannot convert 'void' to anything
    if (typeIs == sym.normalVoidSym) return true;

    // Don't convert if no conversion is called for
    if (typeIs == typeWantsToBe) return false;

    // cannot convert const to non-const
    if (((typeIs & STYPE_CONST) != 0) && ((typeWantsToBe & STYPE_CONST) == 0)) return true;

    // can convert String* to const string
    if ((typeIs == (STYPE_POINTER | sym.stringStructSym)) &&
        (typeWantsToBe == (STYPE_CONST | sym.normalStringSym)))
    {
        return false;
    }
    if (is_string(typeIs) != is_string(typeWantsToBe)) return true;
    if (is_string(typeIs)) return false;

    // Can convert from NULL to pointer
    if ((typeIs == (STYPE_POINTER | sym.nullSym)) && ((typeWantsToBe & STYPE_DYNARRAY) != 0))  return false;

    // Cannot convert non-dynarray to dynarray or vice versa
    if ((typeIs & STYPE_DYNARRAY) != (typeWantsToBe & STYPE_DYNARRAY)) return true;

    // From here on, don't mind constness or dynarray-ness
    typeIs &= ~(STYPE_CONST | STYPE_DYNARRAY);
    typeWantsToBe &= ~(STYPE_CONST | STYPE_DYNARRAY);

    // floats cannot mingle with other types
    if ((typeIs == sym.normalFloatSym) != (typeWantsToBe == sym.normalFloatSym)) return true;

    // Checks to do if at least one is a pointer
    if ((typeIs & STYPE_POINTER) || (typeWantsToBe & STYPE_POINTER))
    {
        // null can be cast to any pointer type
        if (typeIs == (STYPE_POINTER | sym.nullSym))
        {
            if (typeWantsToBe & STYPE_POINTER) return false;
        }

        // BOTH sides must be pointers
        if ((typeIs & STYPE_POINTER) != (typeWantsToBe & STYPE_POINTER)) return true;

        // Types need not be identical here, but check against inherited classes
        int isClass = typeIs & ~STYPE_POINTER;
        while (sym.entries[isClass].extends > 0)
        {
            isClass = sym.entries[isClass].extends;
            if ((isClass | STYPE_POINTER) == typeWantsToBe) return false;
        }
        return true;
    }

    // Checks to do if at least one is a struct
    bool typeIsIsStruct = (0 != (sym.entries[typeIs].flags & SFLG_STRUCTTYPE));
    bool typeWantsToBeIsStruct = (0 != (sym.entries[typeWantsToBe].flags & SFLG_STRUCTTYPE));
    if (typeIsIsStruct || typeWantsToBeIsStruct)
    {
        // The types must match exactly
        if (typeIs != typeWantsToBe) return true;

        return false;
    }

    return false;
}

// Check whether there is a type mismatch; if so, give an error
int check_type_mismatch(int typeIs, int typeWantsToBe, bool orderMatters)
{
    if (!is_type_mismatch_oneway(typeIs, typeWantsToBe)) return 0;
    if (!orderMatters && !is_type_mismatch_oneway(typeWantsToBe, typeIs)) return 0;

    cc_error(
        "Type mismatch: cannot convert '%s' to '%s'",
        sym.get_friendly_name(typeIs).c_str(),
        sym.get_friendly_name(typeWantsToBe).c_str());
    return -1;
}


inline bool isVCPUOperatorBoolean(int scmdtype) 
{
    // returns whether this operator's val type is always bool
    if ((scmdtype >= SCMD_ISEQUAL) &&
        (scmdtype <= SCMD_OR))
    {
        return true;
    }

    if ((scmdtype >= SCMD_FGREATER) &&
        (scmdtype <= SCMD_FLTE))
    {
        return true;
    }

    if ((scmdtype == SCMD_STRINGSNOTEQ) ||
        (scmdtype == SCMD_STRINGSEQUAL))
    {
        return true;
    }
       
    return false;
}


int read_var_or_funccall_DotItem_HandleFuncCall(ccInternalList *targ, int & funcAtOffs, const ags::SymbolScript_t &slist, size_t & slist_len)
{
    int paren_expr_startline = currentline;
    funcAtOffs = slist_len - 1;
    slist[slist_len++] = targ->getnext();

    if (sym.get_type(slist[slist_len - 1]) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }

    // include the member function params in the returned value
    size_t paren_nesting_depth = 1; // No. of '(' that wait for their matching ')'
    while (paren_nesting_depth > 0)
    {
        if (ReachedEOF(targ))
        {
            currentline = paren_expr_startline;
            cc_error("The '(' on line %d does not have a matching ')'", paren_expr_startline);
            return -1;
        }

        if (slist_len >= TEMP_SYMLIST_LENGTH - 1)
        {
            cc_error("Buffer exceeded: The '(' on line %d probably does not have a matching ')'", paren_expr_startline);
            return -1;
        }

        slist[slist_len++] = targ->getnext();
        if (sym.get_type(slist[slist_len - 1]) == SYM_CLOSEPARENTHESIS) paren_nesting_depth--;
        if (sym.get_type(slist[slist_len - 1]) == SYM_OPENPARENTHESIS)  paren_nesting_depth++;
    }

    return 0;
}

// We have accepted something like "m.a."
int read_var_or_funccall_GotDot(ccInternalList *targ, bool justHadBrackets, ags::SymbolScript_t slist, size_t &slist_len, ags::Symbol_t &current_member, int &funcAtOffs)
{
    bool mustBeStaticMember = false;

    // Get the type of the component before the dot.
    ags::Symbol_t current_member_type = 0;
    if (sym.get_type(current_member) == SYM_VARTYPE)
    {
        // static member access, eg. "Math.Func()"
        mustBeStaticMember = true;
        current_member_type = current_member;
    }
    else
    {
        current_member_type = sym.entries[current_member].vartype;
        if (current_member_type < 1)
        {
            cc_error("structure required on left side of '.'");
            return -1;
        }
    }

    if (((sym.entries[current_member].flags & SFLG_ARRAY) != 0) && (!justHadBrackets))
    {
        cc_error("'[' expected");
        return -1;
    }

    // allow protected member access with the "this" ptr only
    bool allowProtectedMembers = false;
    if (sym.entries[current_member].flags & SFLG_THISPTR) allowProtectedMembers = true;

    // Get the symbol to the right of the dot
    slist[slist_len++] = targ->getnext();

    // convert the member's sym to the structmember version
    int retval = find_member_sym(current_member_type, slist[slist_len - 1], allowProtectedMembers);
    if (retval < 0) return retval;

    if ((sym.entries[slist[slist_len-1]].flags & SFLG_STRUCTMEMBER) == 0)
    {
        cc_error("structure member required after '.'");
        return -1;
    }

    if ((mustBeStaticMember) && (0 == (sym.entries[slist[slist_len-1]].flags & SFLG_STATIC)))
    {
        cc_error("Must have an instance of the struct to access a non-static member");
        return -1;
    }

    current_member = slist[slist_len - 1];

    if (sym.get_type(current_member) == SYM_FUNCTION)
    {
        // The symbol after the dot was a function name. This function is called.
        // We encountered something like s.m; we're waiting for '('
        int retval = read_var_or_funccall_DotItem_HandleFuncCall(targ, funcAtOffs, slist, slist_len);
        if (retval < 0) return retval;
    }

    return 0;
}


// We've read something like "a.b.c[", we've already stored the '['
int read_var_or_funccall_GotOpenBracket(ccInternalList *targ, ags::SymbolScript_t slist, size_t &slist_len)
{

    if ((sym.entries[slist[slist_len - 2]].flags & SFLG_ARRAY) == 0)
    {
        cc_error("%s is not an array", sym.get_friendly_name(slist[slist_len - 2]).c_str());
        return -1;
    }

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
        cc_error("Array index not specified");
        return -1;
    }

    size_t bracket_nesting_depth = 1;
    int bracket_expr_startline = currentline;

    // accept and buffer the contents of the brackets and the closing bracket
    // comma is allowed because you can have e.g. array[func(a,b)]
    // vartype is allowed to permit access to static members, e.g. array[Game.GetColorFromRGB(0, 0, 0)]
    while (bracket_nesting_depth > 0)
    {
        ags::Symbol_t next_symbol = targ->getnext();
        if (next_symbol == SCODE_INVALID)
        {
            currentline = bracket_expr_startline;
            cc_error("The '[' on line %d does not have a matching ']'", bracket_expr_startline);
            return -1;
        }

        if ((sym.get_type(next_symbol) >= NOTEXPRESSION) &&
            (sym.get_type(next_symbol) != SYM_COMMA) &&
            !((sym.get_type(next_symbol) == SYM_VARTYPE) && (sym.entries[slist[next_symbol]].flags & SFLG_STRUCTTYPE)))
        {
            cc_error("Unexpected symbol '%s'", sym.get_friendly_name(next_symbol).c_str());
            return -1;
        }

        slist[slist_len++] = next_symbol;
        if (sym.get_type(next_symbol) == SYM_CLOSEBRACKET) bracket_nesting_depth--;
        if (sym.get_type(next_symbol) == SYM_OPENBRACKET)  bracket_nesting_depth++;
        if (slist_len >= TEMP_SYMLIST_LENGTH - 1)
        {
            cc_error("Buffer exceeded, the '[' on line %d probably does not have a matching ']'", bracket_expr_startline);
            return -1;
        }
    }

    return 0;
}


// Copies the parts of a  variable name or array expression or function call into slist[]
// If there isn't any of this here, this will return without error
int read_var_or_funccall(ccInternalList *targ, ags::Symbol_t fsym, ags::SymbolScript_t slist, size_t &slist_len, int &funcAtOffs)
{
    funcAtOffs = -1;

    bool mustBeStaticMember = false;

    // We read and buffer one of the following cases:
    // (1) A loadable var
    // (2) A struct, then '.', then a struct member
    // (3) A type, then '.', then a static member
    // If that member is a function call, we read and buffer that function call
    // If that member is an array and we have [, we read the array expression

    if (!sym.entries[fsym].is_loadable_variable())
    {
        if (sym.get_type(fsym) != SYM_VARTYPE) return 0; // Must be a type
    }

    slist_len = 0;
    slist[slist_len++] = fsym; 

    bool justHadBrackets = false;
    
    for (int nexttype = sym.get_type(targ->peeknext()); 
        nexttype == SYM_DOT || nexttype == SYM_OPENBRACKET;
        nexttype = sym.get_type(targ->peeknext()))
    {
        if (ReachedEOF(targ))
        {
            cc_error("Dot operator must be followed by member function or property");
            return -1;
        }

        if (slist_len >= TEMP_SYMLIST_LENGTH - 5)
        {
            cc_error("Name expression too long: Probably a ']' was missing above.");
            return -1;
        }

        // store the '.' or '['
        slist[slist_len++] = targ->getnext();

        switch (nexttype)
        {
        default: // This can't happen
            cc_error("Internal error: '.' or '[' expected");
            return -99;

        case SYM_DOT:
        {
            int retval = read_var_or_funccall_GotDot(targ, justHadBrackets, slist, slist_len, fsym, funcAtOffs);
            if (retval < 0) return retval;
            justHadBrackets = false;
            break;
        }

        case SYM_OPENBRACKET:
        {
            int retval = read_var_or_funccall_GotOpenBracket(targ, slist, slist_len);
            if (retval < 0) return retval;
            justHadBrackets = true;
            break;
        }
        } // switch (nexttype)

    }
    return 0;
}


void DoNullCheckOnStringInAXIfNecessary(ccCompiledScript *scrip, int valTypeTo) {

    if (((scrip->ax_val_type & (~STYPE_POINTER)) == sym.stringStructSym) &&
        ((valTypeTo & (~STYPE_CONST)) == sym.normalStringSym))
    {
        scrip->write_cmd1(SCMD_CHECKNULLREG, SREG_AX);
    }

}


// If we need a String but AX contains a normal literal string, 
// then convert AX into a String object and set its type accordingly
void ConvertAXIntoStringObject(ccCompiledScript *scrip, int valTypeTo)
{
    if (((scrip->ax_val_type & (~STYPE_CONST)) == sym.normalStringSym) &&
        ((valTypeTo & (~STYPE_POINTER)) == sym.stringStructSym))
    {
        scrip->write_cmd1(SCMD_CREATESTRING, SREG_AX); // convert AX
        scrip->ax_val_type = STYPE_POINTER | sym.stringStructSym; // set type of AX
    }
}


void set_ax_scope(ccCompiledScript *scrip, int scrip_idx)
{
    scrip->ax_val_scope = SYM_GLOBALVAR;

    // "null" is considered to be a global var
    if (sym.get_type(scrip_idx) == SYM_NULL) return;
    // if it's a parameter, pretend it's a global var
    // this allows it to be returned back from the function
    if (sym.entries[scrip_idx].flags & SFLG_PARAMETER) return;

    scrip->ax_val_scope = sym.entries[scrip_idx].stype;
}


int findClosingBracketOffs(size_t openBracketOffs, ags::SymbolScript_t symlist, size_t symlist_len, size_t &brac_idx)
{
    int nesting_depth = 0;
    for (brac_idx = openBracketOffs + 1; brac_idx < symlist_len; brac_idx++)
    {
        int symtype = sym.get_type(symlist[brac_idx]);
        if ((symtype == SYM_OPENBRACKET) || (symtype == SYM_OPENPARENTHESIS))
            nesting_depth++;
        if ((symtype == SYM_CLOSEBRACKET) || (symtype == SYM_CLOSEPARENTHESIS))
        {
            nesting_depth--;
            if (nesting_depth < 0) return 0;
        }
    }
    // Did not find it
    return -1;
}


int findOpeningBracketOffs(size_t closeBracketOffs, ags::SymbolScript_t symlist, size_t &brac_idx)
{
    int nesting_depth = 0;

    // don't convert to for loop, "for(..., brac_idx >= 0,...)" will NOT work
    brac_idx = closeBracketOffs;
    do
    {
        brac_idx--;
        int symtype = sym.get_type(symlist[brac_idx]);
        if ((symtype == SYM_OPENBRACKET) || (symtype == SYM_OPENPARENTHESIS))
        {
            nesting_depth--;
            if (nesting_depth < 0) return 0;
        }
        if ((symtype == SYM_CLOSEBRACKET) || (symtype == SYM_CLOSEPARENTHESIS))
            nesting_depth++;
    } while (brac_idx > 0);
    // Didn't find it
    return -1;
}


int extractPathIntoParts(VariableSymlist *variablePath, ags::SymbolScript_t syml, size_t syml_len, size_t &variablePathSize)
{
    variablePathSize = 0;
    int lastOffs = 0;
    size_t syml_idx;

    // Separate out syml into a VariablePath for the clause
    // between each dot. If it's just a simple variable access,
    // we will only create one.
    for (syml_idx = 0; syml_idx < syml_len; syml_idx++)
    {
        if ((sym.get_type(syml[syml_idx]) == SYM_OPENBRACKET) ||
            (sym.get_type(syml[syml_idx]) == SYM_OPENPARENTHESIS))
        {
            // an array index, skip it
            findClosingBracketOffs(syml_idx, syml, syml_len, syml_idx);
        }

        bool createPath = false;

        if (sym.get_type(syml[syml_idx]) == SYM_DOT)
        {
            createPath = true;
        }
        else if (syml_idx >= syml_len - 1)
        {
            // end of data stream, store the last bit
            syml_idx++;
            createPath = true;
        }

        if (!createPath) continue;

        if (variablePathSize >= MAX_VARIABLE_PATH)
        {
            cc_error("Variable path too long");
            return -1;
        }

        VariableSymlist *vpp = &variablePath[variablePathSize];
        vpp->init(syml_idx - lastOffs);
        for (int vsyml_idx = 0; vsyml_idx < vpp->len; vsyml_idx++)
        {
            vpp->syml[vsyml_idx] = syml[lastOffs + vsyml_idx];
        }
        lastOffs = syml_idx + 1;
        variablePathSize++;
    }

    return 0;
}


inline int get_read_command_for_size(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMREAD;
    case 1:  return SCMD_MEMREADB;
    case 2:  return SCMD_MEMREADW;
    }
}


inline int get_write_command_for_size(int the_size)
{
    switch (the_size)
    {
    default: return SCMD_MEMWRITE;
    case 1:  return SCMD_MEMWRITEB;
    case 2:  return SCMD_MEMWRITEW;
    }
}


// [fw] Passing info around through a global variable: That is a HUGE code smell.
int readcmd_lastcalledwith = 0;


// Get the bytecode for reading or writing memory of size the_size
inline int get_readwrite_cmd_for_size(int the_size, bool write_operation)
{
    // [fw] Passing info around through a global variable: That is a HUGE code smell.
    if (the_size != 0) readcmd_lastcalledwith = the_size;

    return (write_operation) ? get_write_command_for_size(the_size) : get_read_command_for_size(the_size);
}


int parse_subexpr_NewIsFirst(ccCompiledScript * scrip, const ags::SymbolScript_t & symlist, const size_t & symlist_len)
{
    if (symlist_len < 2 || sym.get_type(symlist[1]) != SYM_VARTYPE)
    {
        cc_error("Expected a type after 'new'");
        return -1;
    }

    // "new TYPE", nothing following
    if (symlist_len <= 3)
    {
        if (sym.entries[symlist[1]].flags & SFLG_BUILTIN)
        {
            cc_error("Built-in type '%s' cannot be instantiated directly", sym.get_name(symlist[1]));
            return -1;
        }
        const size_t size = sym.entries[symlist[1]].ssize;
        scrip->write_cmd2(SCMD_NEWUSEROBJECT, SREG_AX, size);
        scrip->ax_val_type = symlist[1] | STYPE_POINTER;
        return 0;
    }

    // "new TYPE[EXPR]", nothing following
    if (sym.get_type(symlist[2]) == SYM_OPENBRACKET && sym.get_type(symlist[symlist_len - 1]) == SYM_CLOSEBRACKET)
    {
        ags::Symbol_t arrayType = symlist[1];

        // Expression for length of array begins after "[", ends before "]"
        // So expression_length = whole_length - 3 - 1
        int retval = parse_subexpr(scrip, &symlist[3], symlist_len - 4);
        if (retval < 0) return retval;

        if (scrip->ax_val_type != sym.normalIntSym)
        {
            cc_error("Array size must be an int");
            return -1;
        }

        bool isManagedType = false;
        int size = sym.entries[arrayType].ssize;
        if (sym.entries[arrayType].flags & SFLG_MANAGED)
        {
            isManagedType = true;
            size = 4;
        }
        else if (sym.entries[arrayType].flags & SFLG_STRUCTTYPE)
        {
            cc_error("Cannot create a dynamic array of an unmanaged struct");
            return -1;
        }

        scrip->write_cmd3(SCMD_NEWARRAY, SREG_AX, size, isManagedType);
        scrip->ax_val_type = arrayType | STYPE_DYNARRAY;

        if (isManagedType) scrip->ax_val_type |= STYPE_POINTER;

        return 0;
    }

    cc_error("Unexpected characters following 'new %s'", sym.get_name(symlist[1]));
    return -1;
}


// We're parsing an expression that starts with '-' (unary minus)
int parse_subexpr_UnaryMinusIsFirst(ccCompiledScript * scrip, const ags::SymbolScript_t & symlist, const size_t & symlist_len)
{
    if (symlist_len < 2)
    {
        cc_error("Parse error at '-'");
        return -1;
    }
    // parse the rest of the expression into AX
    int retval = parse_subexpr(scrip, &symlist[1], symlist_len - 1);
    if (retval < 0) return retval;

    // now, subtract the result from 0 (which negates it)
    int cpuOp = SCMD_SUBREG; // get correct bytecode for the subtraction
    retval = get_operator_valid_for_type(scrip->ax_val_type, 0, cpuOp);
    if (retval < 0) return retval;

    scrip->write_cmd2(SCMD_LITTOREG, SREG_BX, 0);
    scrip->write_cmd2(cpuOp, SREG_BX, SREG_AX);
    scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
    return 0;
}


// We're parsing an expression that starts with '!' (boolean NOT)
int parse_subexpr_NotIsFirst(ccCompiledScript * scrip, const ags::SymbolScript_t & symlist, const size_t & symlist_len)
{

    if (symlist_len < 2)
    {
        cc_error("Parse error at '!'");
        return -1;
    }

    // parse the rest of the expression into AX
    int retval = parse_subexpr(scrip, &symlist[1], symlist_len - 1);
    if (retval < 0) return retval;

    // negate the result
    // First determine the correct bytecode for the negation
    int cpuOp = SCMD_NOTREG;
    retval = get_operator_valid_for_type(scrip->ax_val_type, 0, cpuOp);
    if (retval < 0) return retval;

    // now, NOT the result
    scrip->write_cmd1(SCMD_NOTREG, SREG_AX);
    return 0;
}


// The lowest-binding operator is the first thing in the expression
// This means that the op must be an unary op.
int parse_subexpr_OpIsFirst(ccCompiledScript * scrip, const ags::SymbolScript_t &symlist, const size_t &symlist_len)
{
    if (sym.get_type(symlist[0]) == SYM_NEW)
    {
        // we're parsing something like "new foo"
        return parse_subexpr_NewIsFirst(scrip, symlist, symlist_len);
    }

    if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG)
    {
        // we're parsing something like "- foo"
        return parse_subexpr_UnaryMinusIsFirst(scrip, symlist, symlist_len);
    }

    if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_NOTREG)
    {
        // we're parsing something like "! foo"
        return parse_subexpr_NotIsFirst(scrip, symlist, symlist_len);
    }

    // All the other operators need a non-empty left hand side
    cc_error("Unexpected operator '%s' without a preceding expression", sym.get_friendly_name(symlist[0]).c_str());
    return -1;
}


// The lowest-binding operator has a left-hand and a right-hand side, e.g. "foo + bar"
int parse_subexpr_OpIsSecondOrLater(ccCompiledScript * scrip, size_t op_idx, const ags::SymbolScript_t &symlist, const size_t &symlist_len)
{

    int vcpuOperator = sym.entries[symlist[op_idx]].operatorToVCPUCmd();

    if (vcpuOperator == SCMD_NOTREG)
    {
        // you can't do   a = b ! c;
        cc_error("Invalid use of operator '!'");
        return -1;
    }

    if ((vcpuOperator == SCMD_SUBREG) &&
        (op_idx > 1) &&
        (sym.get_type(symlist[op_idx - 1]) == SYM_OPERATOR))
    {
        // We aren't looking at a subtraction; instead, the '-' is the unary minus of a negative value
        // Thus, the "real" operator must be further to the right, find it.
        op_idx = index_of_lowest_bonding_operator(symlist, op_idx);
        vcpuOperator = sym.entries[symlist[op_idx]].operatorToVCPUCmd();
    }

    // process the left hand side and save result onto stack
    // This will be in vain if we find out later on that there isn't any right hand side,
    // but doing the left hand side first means that any errors will be generated from left to right
    int retval = parse_subexpr(scrip, &symlist[0], op_idx);
    if (retval < 0) return retval;

    if (op_idx + 1 >= symlist_len)
    {
        // there is no right hand side for the expression
        cc_error("Parse error: invalid use of operator '%s'", sym.get_friendly_name(symlist[op_idx]).c_str());
        return -1;
    }

    ags::CodeLoc_t jump_dest_loc_to_patch = -1;
    if (vcpuOperator == SCMD_AND)
    {
        // "&&" operator lazy evaluation ... 
        // if AX is 0 then the AND has failed, 
        // so just jump directly past the AND instruction;
        // AX will still be 0 so that will do as the result of the calculation
        scrip->write_cmd1(SCMD_JZ, 0);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        jump_dest_loc_to_patch = scrip->codesize - 1;
    }
    else if (vcpuOperator == SCMD_OR)
    {
        // "||" operator lazy evaluation ... 
        // if AX is non-zero then the OR has succeeded, 
        // so just jump directly past the OR instruction; 
        // AX will still be non-zero so that will do as the result of the calculation
        scrip->write_cmd1(SCMD_JNZ, 0);
        // We don't know the end of the instruction yet, so remember the location we need to patch
        jump_dest_loc_to_patch = scrip->codesize - 1;
    }

    int valtype_leftsize = scrip->ax_val_type;

    scrip->push_reg(SREG_AX);
    retval = parse_subexpr(scrip, &symlist[op_idx + 1], symlist_len - (op_idx + 1));
    if (retval < 0) return retval;
    scrip->pop_reg(SREG_BX); // <-- note, we pop to BX although we have pushed AX
    // now the result of the left side is in BX, of the right side is in AX

    // Check whether the left side type and right side type match either way
    retval = check_type_mismatch(scrip->ax_val_type, valtype_leftsize, false);
    if (retval < 0) return retval;

    retval = get_operator_valid_for_type(scrip->ax_val_type, valtype_leftsize, vcpuOperator);
    if (retval < 0) return retval;

    scrip->write_cmd2(vcpuOperator, SREG_BX, SREG_AX);
    scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);

    if (jump_dest_loc_to_patch >= 0)
    {
        ags::CodeLoc_t jump_offset = scrip->codesize - jump_dest_loc_to_patch - 1;
        scrip->code[jump_dest_loc_to_patch] = jump_offset;
    }

    // Operators like == return a bool (in our case, that's an int);
    // other operators like + return the type that they're operating on
    if (isVCPUOperatorBoolean(vcpuOperator)) scrip->ax_val_type = sym.normalIntSym;

    return 0;
}


int parse_subexpr_OpenParenthesis(ccCompiledScript * scrip, ags::SymbolScript_t & symlist, size_t symlist_len)
{
    int matching_paren_idx = -1;
    size_t paren_nesting_depth = 1; // we've already read a '('
    // find the corresponding closing parenthesis
    for (size_t idx = 1; idx < symlist_len; idx++)
    {
        switch (sym.get_type(symlist[idx]))
        {
        default:
            continue;

        case SYM_OPENPARENTHESIS:
            paren_nesting_depth++;
            continue;

        case SYM_CLOSEPARENTHESIS:
            if (--paren_nesting_depth > 0) continue;

            matching_paren_idx = idx; // found the index of the matching ')'
            break;
        }

        break;
    }

    if (matching_paren_idx < 0)
    {
        cc_error("Open parenthesis does not have a matching close parenthesis");
        return -1;
    }

    if (matching_paren_idx <= 1)
    {
        cc_error("Unexpected \"()\"");
        return -1;
    }

    // Recursively compile the subexpression
    int retval = parse_subexpr(scrip, &symlist[1], matching_paren_idx - 1) < 0;
    if (retval < 0) return -1;

    symlist += matching_paren_idx + 1;
    symlist_len -= matching_paren_idx + 1;
    if (symlist_len > 0)
    {
        // there is some code after the ')'
        // this should not be possible unless the user does
        // something like "if ((x) 1234)", i.e. with an operator missing
        cc_error("Parse error: operator expected");
        return -1;
    }
    return 0;
}


// We're in the parameter list of a function call, and we have less parameters than declared.
// Provide defaults for the missing values
int parse_subexpr_FunctionCall_ProvideDefaults(ccCompiledScript * scrip, int num_func_args, size_t num_supplied_args, ags::Symbol_t funcSymbol)
{
    for (size_t arg_idx = num_func_args; arg_idx > num_supplied_args; arg_idx--)
    {
        if (!sym.entries[funcSymbol].funcParamHasDefaultValues[arg_idx])
        {
            cc_error("Function call parameter # %d isn't provided and does not have a default value", arg_idx);
            return -1;
        }

        // push the default value onto the stack
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[funcSymbol].funcParamDefaultValues[arg_idx]);

        if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        else
            scrip->push_reg(SREG_AX);
    }
    return 0;
}


int parse_subexpr_FunctionCall_PushParams(ccCompiledScript * scrip, const ags::SymbolScript_t &paramList, size_t closedParenIdx, size_t num_func_args, size_t num_supplied_args, ags::Symbol_t funcSymbol)
{
    size_t param_num = num_supplied_args;
    size_t start_of_this_param = 0;
    int end_of_this_param = closedParenIdx;  // can become < 0
    // Go backwards through the parameters, since they must be pushed that way
    do
    {
        // Find the start of the next parameter
        param_num--;
        int paren_nesting_depth = 0;
        for (size_t paramListIdx = end_of_this_param - 1; true; paramListIdx--)
        {
            // going backwards so ')' increases the depth level
            if (sym.get_type(paramList[paramListIdx]) == SYM_CLOSEPARENTHESIS) paren_nesting_depth++;
            if (sym.get_type(paramList[paramListIdx]) == SYM_OPENPARENTHESIS) paren_nesting_depth--;
            if ((sym.get_type(paramList[paramListIdx]) == SYM_COMMA) && (paren_nesting_depth == 0))
            {
                start_of_this_param = paramListIdx + 1;
                break;
            }
            if (paramListIdx == 0) break; // don't put into for(), it won't work
        }
        // Compile the parameter
        if (parse_subexpr(scrip, &paramList[start_of_this_param], end_of_this_param - start_of_this_param)) return -1;

        if (param_num <= num_func_args) // we know what type to expect
        {
            // If we need a string object ptr but AX contains a normal string, convert AX
            int parameterType = sym.entries[funcSymbol].funcparamtypes[param_num];
            ConvertAXIntoStringObject(scrip, parameterType);

            if (check_type_mismatch(scrip->ax_val_type, parameterType, true)) return -1;

            // If we need a normal string but AX contains a string object ptr, 
            // check that this ptr isn't null
            DoNullCheckOnStringInAXIfNecessary(scrip, parameterType);
        }


        if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
        {
            scrip->write_cmd1(SCMD_PUSHREAL, SREG_AX);
        }
        else
        {
            scrip->push_reg(SREG_AX);
        }

        end_of_this_param = start_of_this_param - 1;

    } while (end_of_this_param >= 0);

    return 0;
}


int parse_subexpr_FunctionCall_CountAndCheckParm(const ags::SymbolScript_t &paramList, size_t paramListLen, ags::Symbol_t funcSymbol, size_t &indexOfClosedParen, size_t &num_supplied_args)
{
    int paren_nesting_depth = 0;
    num_supplied_args = 1;
    int numParamSymbols = 0;
    size_t paramListIdx;

    // Count parameters, check that all the parameters are non-empty; find closing paren
    for (paramListIdx = 0; paramListIdx < paramListLen; paramListIdx++)
    {
        if (sym.get_type(paramList[paramListIdx]) == SYM_OPENPARENTHESIS) paren_nesting_depth++;
        if (sym.get_type(paramList[paramListIdx]) == SYM_CLOSEPARENTHESIS)
        {
            paren_nesting_depth--;
            if (paren_nesting_depth < 0) break;
        }

        if ((sym.get_type(paramList[paramListIdx]) == SYM_COMMA) && (paren_nesting_depth == 0))
        {
            num_supplied_args++;
            if (numParamSymbols < 1)
            {
                cc_error("Missing argument in function call");
                return -1;
            }
            numParamSymbols = 0;
        }
        else
            numParamSymbols++;
    }

    indexOfClosedParen = paramListIdx;
    if (indexOfClosedParen >= paramListLen)
    {
        cc_error("Missing ')' at the end of the parameter list");
        return -1;
    }

    if (sym.get_type(paramList[indexOfClosedParen]) != SYM_CLOSEPARENTHESIS)
    {
        // [fw] This can't happen, but the original code checks this so keep it here
        cc_error("Expected ')'");
        return -1;
    }

    if (indexOfClosedParen < paramListLen - 1 &&
        sym.get_type(paramList[indexOfClosedParen + 1]) != SYM_SEMICOLON)
    {
        cc_error("Internal error: Unexpected symbols trailing the parameter list");
        return -1;
    }

    if (paren_nesting_depth >= 0)
    {
        cc_error("Internal error: Parser confused near '%s'", sym.get_friendly_name(funcSymbol).c_str());
        return -1;
    }

    // function call with no arguments -- set num_supplied back to 0
    if ((numParamSymbols == 0) && (num_supplied_args == 1)) num_supplied_args = 0;

    return 0;
}


int parse_subexpr_FunctionCall(ccCompiledScript * scrip, int funcSymbolIdx, ags::SymbolScript_t vnlist, ags::SymbolScript_t & symlist, size_t & symlist_len)
{
    ags::SymbolScript_t workList;
    int workListLen;

    // workList is the function call beginning at the func symbol proper
    workList = symlist;
    workListLen = symlist_len;
    if (funcSymbolIdx > 0)
    {
        workList = &vnlist[funcSymbolIdx];
        workListLen = symlist_len - funcSymbolIdx;
    }


    ags::Symbol_t funcSymbol = workList[0];

    // Make sure that a '(' follows the funcname of the function call 
    if (sym.get_type(workList[1]) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }

    // paramList begins at the parameters, after the leading '('
    ags::SymbolScript_t paramList = workList + 2;
    size_t paramListLen = workListLen - 2;


    // Generate code so that the runtime stack contains, bottom-to-top:
    //      a pointer to "this" if applicable
    //      the parameters in reverse sequence, so that the first parameter will pop off first 


    // Find out whether we use "this"; in this case, generate a push to the stack
    // This is supposed to push a pointer to "this" onto the stack as hidden first argument
    bool using_op = false;
    if (funcSymbolIdx > 0)
    {
        // functions in struct usually use "this" (method calls)
        using_op = true;
        // but static functions don't have an object instance, so no "this"
        if (sym.entries[funcSymbol].flags & SFLG_STATIC) using_op = 0;
    }

    // push a pointer to the current object onto the stack before the parameters if applicable
    if (using_op) scrip->push_reg(SREG_OP);

    // Expected number of arguments, or expected minimal number of arguments
    size_t num_func_args = sym.entries[funcSymbol].sscope;
    bool func_is_varargs = false;
    if (num_func_args >= 100)
    {
        num_func_args -= 100;
        func_is_varargs = true;
    }

    // Count the parameters and check them
    size_t indexOfClosedParen;
    size_t num_supplied_args;
    int retval = parse_subexpr_FunctionCall_CountAndCheckParm(paramList, paramListLen, funcSymbol, indexOfClosedParen, num_supplied_args);
    if (retval < 0) return retval;

    // Push default parameters onto the stack when applicable
    // This will give an error if there aren't enough default parameters
    if (num_supplied_args < num_func_args)
    {
        int retval = parse_subexpr_FunctionCall_ProvideDefaults(scrip, num_func_args, num_supplied_args, funcSymbol);
        if (retval < 0) return retval;
    }
    if (num_supplied_args > num_func_args && !func_is_varargs)
    {
        cc_error("Expected just %d parameters but found %d", num_func_args, num_supplied_args);
        return -1;
    }
    // ASSERT at this point, the number of parameters is okay

    // Push the explicit arguments of the function
    if (num_supplied_args > 0)
    {
        int retval = parse_subexpr_FunctionCall_PushParams(scrip, paramList, indexOfClosedParen, num_func_args, num_supplied_args, funcSymbol);
        if (retval < 0) return retval;
    }

    if (using_op)
    {
        // write the address of the function's object to the OP reg
        read_variable_into_ax(scrip, vnlist, funcSymbolIdx, false);
        scrip->write_cmd1(SCMD_CALLOBJ, SREG_AX);
    }

    size_t actual_num_args = std::max(num_supplied_args, num_func_args);
    if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
    {
        // tell it how many args for this call (nested imported functions
        // causes stack problems otherwise)
        scrip->write_cmd1(SCMD_NUMFUNCARGS, actual_num_args);
    }

    // call it
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, sym.entries[funcSymbol].soffs);
    if (sym.entries[funcSymbol].flags & SFLG_IMPORTED)
    {
        scrip->fixup_previous(FIXUP_IMPORT);
        // do the call
        scrip->write_cmd1(SCMD_CALLEXT, SREG_AX);
        if (actual_num_args > 0) scrip->write_cmd1(SCMD_SUBREALSTACK, actual_num_args);
    }
    else
    {
        scrip->fixup_previous(FIXUP_FUNCTION);
        scrip->write_cmd1(SCMD_CALL, SREG_AX);

        // We will arrive here when the function call has returned
        // restore the stack
        if (actual_num_args > 0)
        {
            scrip->cur_sp -= actual_num_args * 4;
            scrip->write_cmd2(SCMD_SUB, SREG_SP, actual_num_args * 4);
        }
    }


    // function return type
    // This is an alias for "type of the current expression". 
    scrip->ax_val_type = sym.entries[funcSymbol].funcparamtypes[0];
    scrip->ax_val_scope = SYM_LOCALVAR;

    if (using_op) scrip->pop_reg(SREG_OP);

    // Note that this function has been accessed at least once
    sym.entries[funcSymbol].flags |= SFLG_ACCESSED;
    return 0;
}


int parse_subexpr_NoOps(ccCompiledScript * scrip, ags::SymbolScript_t symlist, size_t symlist_len)
{

    // Can't check whether type is 0, because e.g. "this" doesn't have a type

    if (sym.get_type(symlist[0]) == SYM_OPENPARENTHESIS)
    {
        return parse_subexpr_OpenParenthesis(scrip, symlist, symlist_len);
    }

    if (sym.get_type(symlist[0]) == SYM_OPERATOR)
    {
        // check for unary minus
        if (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG)
        {
            if (symlist_len == 2) // negative literal
            {
                int retval = read_variable_into_ax(scrip, &symlist[1], 1, true);
                if (retval < 0) return retval;
                return 0;
            }

            // If there are bogus tokens after a term that begins with unary minus, 
            // then the problem is the bogus tokens, beginning at index 2. 
            // Otherwise, the problem is the unary minus itself, at index 0. 
            cc_error(
                "Parse error: unexpected '%s'",
                sym.get_friendly_name(symlist[(symlist_len > 2) ? 2 : 0]).c_str());
            return -1;
        }

        // We don't know this unary operator. "new", perhaps?
        cc_error("Parse error: Unexpected '%s'", sym.get_friendly_name(symlist[0]).c_str());
        return -1;
    }

    // Find out whether this is a variable or function call; if so, copy it to vnlist
    // We need to copy the bytes because a lot of code depends on the fact that there
    // are no METAs interspersed between the symbols. 
    ags::Symbol_t vnlist[TEMP_SYMLIST_LENGTH];
    size_t vnlist_len;
    int funcAtOffs = 0;

    // Cast an internal list around symlist
    ccInternalList tlist;
    tlist.pos = 0;
    tlist.script = symlist;
    tlist.length = symlist_len;
    tlist.cancelCurrentLine = 0;

    int retval = read_var_or_funccall(&tlist, tlist.getnext(), vnlist, vnlist_len, funcAtOffs);
    // stop tlist trying to free the memory
    tlist.script = NULL;
    tlist.length = 0;
    if (retval < 0) return retval;

    if ((sym.get_type(symlist[0]) == SYM_FUNCTION) || (funcAtOffs > 0))
    {
        return parse_subexpr_FunctionCall(scrip, funcAtOffs, vnlist, symlist, symlist_len);
    }

    if (symlist_len == 1)
    {
        // Must be a variable or literal, otherwise it's invalid
        int retval = read_variable_into_ax(scrip, symlist, symlist_len, false);
        if (retval < 0) return retval;
        return 0;
    }

    if (symlist_len == vnlist_len)
    {
        int retval = read_variable_into_ax(scrip, vnlist, vnlist_len, false);
        if (retval < 0) return retval;
        return 0;
    }

    cc_error("Parse error in expr near '%s'", sym.get_friendly_name(symlist[0]).c_str());
    return -1;
}

int parse_subexpr(ccCompiledScript *scrip, ags::SymbolScript_t symlist, size_t symlist_len)
{
    if (symlist_len == 0)
    {
        cc_error("Internal error: Cannot parse empty subexpression");
        return -1;
    }
    if (sym.get_type(symlist[0]) == SYM_CLOSEBRACKET)
    {
        cc_error("Unexpected ')' at start of expression");
        return -1;
    }

    int lowest_op_idx = index_of_lowest_bonding_operator(symlist, symlist_len);  // can be < 0

    // If the lowest bonding operator is right in front and an integer follows,
    // then it has been misinterpreted so far: 
    // it's really a unary minus. So let's try that.
    // [fw] Why don't we treat literal floats in the same way?
    if ((lowest_op_idx == 0) &&
        (symlist_len > 1) &&
        (sym.get_type(symlist[1]) == SYM_LITERALVALUE) &&
        (sym.entries[symlist[0]].operatorToVCPUCmd() == SCMD_SUBREG))
    {
        lowest_op_idx = index_of_lowest_bonding_operator(&symlist[1], symlist_len - 1);
        if (lowest_op_idx >= 0) lowest_op_idx++;
    }

    if (lowest_op_idx == 0)
    {
        return parse_subexpr_OpIsFirst(scrip, symlist, symlist_len);
    }

    if (lowest_op_idx > 0)
    {
        return parse_subexpr_OpIsSecondOrLater(scrip, static_cast<size_t>(lowest_op_idx), symlist, symlist_len);
    }

    // There is no operator in the expression -- therefore, there will
    // just be a variable name or function call or a parenthesized expression

    return parse_subexpr_NoOps(scrip, symlist, symlist_len);
}


int get_array_index_into_ax(ccCompiledScript *scrip, ags::SymbolScript_t symlist, int openBracketOffs, int closeBracketOffs, bool checkBounds, bool multiplySize) {

    // "push" the ax val type (because this is just an array index,
    // we're actually interested in the type of the variable being read)
    int axValTypeWas = scrip->ax_val_type;

    // save the size of the array element, so it doesn't get
    // overwritten by the size of the array index variable
    // [fw] Passing info around through a global variable: That is a HUGE code smell.
    int saveOldReadcmd = readcmd_lastcalledwith;
    // parse expression inside brackets to return the array index in AX
    int retval = parse_subexpr(scrip, &symlist[openBracketOffs + 1], closeBracketOffs - (openBracketOffs + 1));
    if (retval < 0) return retval;
    readcmd_lastcalledwith = saveOldReadcmd;

    // array index must be an int
    retval = check_type_mismatch(scrip->ax_val_type, sym.normalIntSym, true);
    if (retval < 0) return retval;

    // "pop" the ax val type
    scrip->ax_val_type = axValTypeWas;

    ags::Symbol_t arrSym = symlist[openBracketOffs - 1];

    if ((sym.entries[arrSym].flags & SFLG_ARRAY) == 0)
    {
        cc_error("Internal error: '%s' is not an array", sym.get_friendly_name(arrSym).c_str());
        return -1;
    }

    if (checkBounds)
    {
        // check the array bounds that have been calculated in AX,
        // before they are added to the overall offset
        if ((sym.entries[arrSym].flags & SFLG_DYNAMICARRAY) == 0)
        {
            scrip->write_cmd2(SCMD_CHECKBOUNDS, SREG_AX, sym.entries[arrSym].arrsize);
        }
    }

    if (multiplySize)
    {
        // multiply up array index (in AX) by size of array element
        // to get memory offset
        scrip->write_cmd2(SCMD_MUL, SREG_AX, sym.entries[arrSym].ssize);
    }

    return 0;
}


// parse array brackets
int parseArrayIndexOffsetsIfPresent(ccCompiledScript *scrip, VariableSymlist *thisClause, bool writingOperation, bool &isArrayOffset) 
{

    if ((thisClause->len <= 1) || (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET))
    {
        // No '[', so no array index clause. Return without error.
        return 0;
    }

    // find where the brackets end
    size_t arrIndexEnd;
    findClosingBracketOffs(1, thisClause->syml, thisClause->len, arrIndexEnd);
    if (arrIndexEnd != thisClause->len - 1)
    {
        cc_error("Unexpected token after array index");
        return -1;
    }

    bool propertyIndexer = false;
    bool checkBounds = true, multiplySize = true;

    if ((sym.entries[thisClause->syml[0]].flags & SFLG_PROPERTY) ||
        (sym.entries[thisClause->syml[0]].flags & SFLG_POINTER))
    {
        // an array property, or array of pointers; in this case,
        // don't touch CX but just calculate the index value into DX
        propertyIndexer = true;
        multiplySize = false;
        // don't check bounds, the property getter will do that
        if (sym.entries[thisClause->syml[0]].flags & SFLG_PROPERTY)
            checkBounds = false;
    }

    // the value to write is in AX; preserve it
    if (writingOperation) scrip->push_reg(SREG_AX);

    // save the current offset in CX if there is one,
    // because parse_subexpr might destroy it
    if (isArrayOffset) scrip->push_reg(SREG_CX);

    // get the byte offset of the array index into AX
    int retval = get_array_index_into_ax(scrip, thisClause->syml, 1, arrIndexEnd, checkBounds, multiplySize);
    if (retval < 0) return retval;

    // if there is a current offset saved in CX, restore it
    // then add the result to CX (which is counting the overall offset)
    if (isArrayOffset)
    {
        scrip->pop_reg(SREG_CX);
        scrip->write_cmd2(
            (propertyIndexer ? SCMD_REGTOREG : SCMD_ADDREG),
            SREG_AX,
            SREG_DX);
    }
    else
    {
        scrip->write_cmd2(SCMD_REGTOREG,
            SREG_AX,
            (propertyIndexer ? SREG_DX : SREG_CX));
    }

    if (!propertyIndexer)
        isArrayOffset = true;

    if (writingOperation)
        scrip->pop_reg(SREG_AX);

    // the array offset has now been added to CX (normal array)
    // or put into DX (property)

    return 0;
}


// We access a variable or a component of a struct in order to read or write it.
// This is a simple member of the struct.
inline void do_variable_ax_PrepareComponentAccess_Elementary(ags::Symbol_t variableSym, int & currentComponentOffset)
{

    // since the member has a fixed offset into the structure, don't
    // write out any code to calculate the offset - instead, modify
    // the hard offset value which will be written to MAR
    currentComponentOffset += sym.entries[variableSym].soffs;
}


// We access a component of a struct in order to read or write it. 
// This is a function that is a member of a struct.
inline int do_variable_ax_PrepareComponentAccess_MemberFunction(bool isLastClause, bool & getJustTheAddressIntoAX, bool & doMemoryAccessNow)
{
    // This is only possible if it is last in the clause
    if (!isLastClause)
    {
        cc_error("Function().Member not supported");
        return -1;
    }
    // A function isn't _really_ part of a struct. 
    // In reality, it's just a "normal" function that takes the struct as an (implicit) first parameter. 
    // So what we need is the address of the struct itself to be able to process the function call
    getJustTheAddressIntoAX = true;
    doMemoryAccessNow = true;
    return 0;
}


// We access a component of a struct in order to read or write it. 
// This is a property.
int do_variable_ax_PrepareComponentAccess_Property(ccCompiledScript * scrip, ags::Symbol_t variableSym, VariableSymlist * thisClause, bool writing, bool writingThisTime, bool mustBeWritable, bool & getJustTheAddressIntoAX, bool &doMemoryAccessNow, bool &isArrayOffset)
{
    // since a property is effectively a function call, load the address of the object
    getJustTheAddressIntoAX = true;
    doMemoryAccessNow = true;

    int retval = parseArrayIndexOffsetsIfPresent(scrip, thisClause, writing != 0, isArrayOffset);
    if (retval < 0) return retval;

    if (writing)
    {
        if ((writingThisTime) && (sym.entries[variableSym].flags & SFLG_READONLY))
        {
            cc_error("Property '%s' is read-only", sym.get_friendly_name(variableSym).c_str());
            return -1;
        }

        // Property Set -- move the new value into BX, so
        // that the object address can be retrieved into AX
        scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
    }

    return 0;
}


// We access a variable or a component of a struct in order to read or write it.
// This is a pointer
int do_variable_ax_PrepareComponentAccess_Pointer(ccCompiledScript * scrip, ags::Symbol_t variableSym, VariableSymlist * thisClause, int currentByteOffset, bool & isDynamicArray, bool writing, ags::Symbol_t firstVariableType, ags::Symbol_t firstVariableSym, bool isLastClause, bool pointerIsOnStack, bool & isArrayOffset, bool & getJustTheAddressIntoAX, int & currentComponentOffset, bool & accessActualPointer, bool & doMemoryAccessNow)
{
    bool isArrayOfPointers = false;

    if (sym.entries[variableSym].flags & SFLG_ARRAY)
    {
        // array of pointers

        if ((thisClause->len <= 1) ||
            (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET))
        {
            // normally, the whole array can be used as a pointer.
            // this is not the case with an pointer array, so catch
            // it here and give an error
            if ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0)
            {
                cc_error("Expected array index after '%s'", sym.get_friendly_name(variableSym).c_str());
                return -1;
            }
            isDynamicArray = true;
        }
        else
        {
            // put array index into DX
            int retval = parseArrayIndexOffsetsIfPresent(scrip, thisClause, writing, isArrayOffset);
            if (retval < 0) return retval;

            isArrayOfPointers = true;
        }
    }

    // if they are just saying "ptr" (or doing a "ptr.Func" call)
    // then move the address being pointed to into AX
    // (member function call passes in "ptr.")
    if (isLastClause) getJustTheAddressIntoAX = true;

    // Push the pointer address onto the stack, where it can be
    // retrieved by do_variable_memory_access later on
    if (sym.entries[variableSym].flags & SFLG_THISPTR)
    {
        if (pointerIsOnStack)
        {
            // already a pointer on the stack
            cc_error("Internal error: Found nested this pointers");
            return -1;
        }

        // for the "this" pointer, just use the Object Pointer
        scrip->push_reg(SREG_OP);
        currentComponentOffset = 0;
        return 0;
    }
    

    if (pointerIsOnStack) 
    {
        // already a pointer on the stack
        scrip->pop_reg(SREG_MAR);
        scrip->write_cmd(SCMD_CHECKNULL);
        if (currentComponentOffset > 0)
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, currentComponentOffset);
    }
    else if (firstVariableType == SYM_LOCALVAR)
    {
        scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - currentComponentOffset);
    }
    else if (firstVariableType == SYM_GLOBALVAR)
    {
        if (sym.entries[firstVariableSym].flags & SFLG_IMPORTED)
        {
            scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, sym.entries[firstVariableSym].soffs);
            scrip->fixup_previous(FIXUP_IMPORT);
            if (currentComponentOffset != 0)
                scrip->write_cmd2(SCMD_ADD, SREG_MAR, currentByteOffset);
        }
        else
        {
            scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, currentByteOffset);
            scrip->fixup_previous(FIXUP_GLOBALDATA);
        }
    }
    else
    {
        cc_error("Invalid type for pointer");
        return -1;
    }

    // if an array, the array indexer was put into DX
    if (isArrayOfPointers)
    {
        scrip->write_cmd2(SCMD_MUL, SREG_DX, 4);

        if (sym.entries[variableSym].flags & SFLG_DYNAMICARRAY)
        {
            // pointer to an array -- dereference the pointer
            scrip->write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
            scrip->write_cmd(SCMD_CHECKNULL);
            scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_DX);
        }

        scrip->write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_DX);
    }

    // push the pointer's address
    scrip->push_reg(SREG_MAR);
    getJustTheAddressIntoAX = true;
    accessActualPointer = true;
    doMemoryAccessNow = true;
    
    currentComponentOffset = 0;
    return 0;
}


int do_variable_ax_PrepareComponentAccess_JustTheAddressCases(ags::Symbol_t variableSym, VariableSymlist * thisClause, bool isLastClause, bool & getJustTheAddressIntoAX, bool & cannotAssign)
{
    // array without index specified
    if ((sym.entries[variableSym].flags & SFLG_ARRAY) &&
        ((thisClause->len == 1) || (sym.get_type(thisClause->syml[1]) != SYM_OPENBRACKET)) &&
        ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0))
    {
        if (sym.entries[variableSym].flags & SFLG_PROPERTY)
        {
            // Returning an array property as a whole is not supported
            cc_error("Expected array index after '%s'", sym.get_friendly_name(variableSym).c_str());
            return -1;
        }
        getJustTheAddressIntoAX = true;
        cannotAssign = true;
        return 0;
    }

    // struct variable without member access
    if (((sym.entries[variableSym].flags & SFLG_POINTER) == 0) &&
        ((sym.entries[sym.entries[variableSym].vartype].flags & SFLG_STRUCTTYPE) != 0) &&
        ((sym.entries[variableSym].flags & SFLG_DYNAMICARRAY) == 0) &&
        isLastClause)
    {
        getJustTheAddressIntoAX = true;
        cannotAssign = true;
    }
    return 0;
}


// We access the a variable or a component of a struct in order to read or write it. 
int do_variable_ax_PrepareComponentAccess(ccCompiledScript * scrip, ags::Symbol_t variableSym, int variableSymType, bool isLastClause, VariableSymlist * thisClause, bool writing, bool mustBeWritable, bool writingThisTime, ags::Symbol_t firstVariableType, ags::Symbol_t firstVariableSym, int &currentComponentOffset, bool &getJustTheAddressIntoAX, bool &doMemoryAccessNow, bool &isProperty, bool &isArrayOffset, bool &write_same_as_read_access, bool &isDynamicArray, bool &pointerIsOnStack, bool &accessActualPointer, bool &cannotAssign)
{
    isArrayOffset = false;
    write_same_as_read_access = true;
    getJustTheAddressIntoAX = false;
    doMemoryAccessNow = false;
    accessActualPointer = false;
    cannotAssign = false;

    isProperty = (0 != (sym.entries[variableSym].flags & SFLG_PROPERTY));
    isDynamicArray = (0 != (sym.entries[variableSym].flags & SFLG_DYNAMICARRAY));
    bool isPointer = (0 != (sym.entries[variableSym].flags & (SFLG_POINTER | SFLG_AUTOPTR)));
    bool isImported = (0 != (sym.entries[variableSym].flags & SFLG_IMPORTED));

    // Simple component access - increment the offset from the start of the structure,
    // which is known at compile time
    if (((variableSymType == SYM_GLOBALVAR) ||
        (variableSymType == SYM_LOCALVAR) ||
        (variableSymType == SYM_STRUCTMEMBER) ||
        (variableSymType == SYM_STRING)) &&
        (!isProperty) &&
        (!isImported))
    {
        do_variable_ax_PrepareComponentAccess_Elementary(variableSym, currentComponentOffset);
    }

    if (variableSymType == SYM_FUNCTION)
    {
        int retval = do_variable_ax_PrepareComponentAccess_MemberFunction(isLastClause, getJustTheAddressIntoAX, doMemoryAccessNow);
        if (retval < 0) return retval;
    }
    else if (isProperty)
    {
        // Writing a property calls a function, reading it calls another function.
        // Avert the caller so that it doesn't try, e.g., to do "++" in-memory.
        // When setting the property, they must always go the long way and first
        // evaluate the new value in AX, then set the property explicitly.
        write_same_as_read_access = false;

        int retval = do_variable_ax_PrepareComponentAccess_Property(scrip, variableSym, thisClause, writing, writingThisTime, mustBeWritable, getJustTheAddressIntoAX, doMemoryAccessNow, isArrayOffset);
        if (retval < 0) return retval;
    }
    else if (isPointer)
    {
        // currentComponentOffset has been set at the start of this loop so it is safe to use
        int retval = do_variable_ax_PrepareComponentAccess_Pointer(scrip, variableSym, thisClause, currentComponentOffset, isDynamicArray, writing, firstVariableType, firstVariableSym, isLastClause, pointerIsOnStack, isArrayOffset, getJustTheAddressIntoAX, currentComponentOffset, accessActualPointer, doMemoryAccessNow);
        if (retval < 0) return retval;

        pointerIsOnStack = true;
    }
    else
    {
        int retval = parseArrayIndexOffsetsIfPresent(scrip, thisClause, writing, isArrayOffset);
        if (retval < 0) return retval;   
    }

    int retval = do_variable_ax_PrepareComponentAccess_JustTheAddressCases(variableSym, thisClause, isLastClause, getJustTheAddressIntoAX, cannotAssign);
    if (retval < 0) return retval;
    return 0;
}


int do_variable_ax_ActualMemoryAccess(ccCompiledScript * scrip, ags::Symbol_t variableSym, int variableSymType, bool pointerIsOnStack, bool writing, bool writingThisTime, bool isProperty, bool mustBeWritable, bool getJustTheAddressIntoAX, bool isArrayOffset, int currentComponentOffset, bool accessActualPointer, ags::Symbol_t firstVariableSym, ags::Symbol_t firstVariableType, bool isDynamicArray, bool negateLiteral, bool isLastClause, VariableSymlist  variablePath[], size_t vp_idx)
{
    int cachedAxValType = scrip->ax_val_type;

    // if a pointer in use, then its address was pushed on the
    // stack, so restore it here

    if (pointerIsOnStack) scrip->pop_reg(SREG_MAR);

    // in a writing operation, but not doing it just yet -- push
    // AX to save the value to write
    if ((writing) && (!writingThisTime)) scrip->push_reg(SREG_AX);

    int retval = do_variable_memory_access(
        scrip, variableSym, variableSymType,
        isProperty, writingThisTime, mustBeWritable,
        getJustTheAddressIntoAX, isArrayOffset,
        currentComponentOffset, pointerIsOnStack,
        accessActualPointer,
        firstVariableSym, firstVariableType,
        isDynamicArray, negateLiteral);
    if (retval < 0) return retval;

    pointerIsOnStack = false;
    currentComponentOffset = 0;
    isDynamicArray = false;
    firstVariableType = SYM_GLOBALVAR;

    if (!isLastClause)
    {
        if (!isProperty && !getJustTheAddressIntoAX)
        {
            cc_error("Unexpected '%s' in variable path", sym.get_friendly_name(variablePath[vp_idx + 1].syml[0]).c_str());
            return -1;
        }

        // pathing, eg. lstItems.OwningGUI.ID
        // we just read a pointer address, so re-push it for use
        // next time round
        if (writing)
        {
            scrip->ax_val_type = cachedAxValType;
            // the value to write was pushed onto the stack,
            // pop it back into BX
            scrip->pop_reg(SREG_BX);
            // meanwhile push the pointer
            // that was just read into AX onto the stack in its place
            scrip->push_reg(SREG_AX);
            // and then copy the value back into AX
            scrip->write_cmd2(SCMD_REGTOREG, SREG_BX, SREG_AX);
        }
        else
        {
            scrip->push_reg(SREG_AX);
        }
    }
    return 0;
}


int do_variable_ax_CheckAccess(ags::Symbol_t variableSym, VariableSymlist variablePath[], bool writing, bool mustBeWritable, bool write_same_as_read_access, bool isLastClause, size_t vp_idx, bool cannotAssign)
{
    // if one of the struct members in the path is read-only, don't allow it
    if (((writing) || (mustBeWritable)) && (write_same_as_read_access))
    {
        // allow writing to read-only pointers if it's actually
        // a property being accessed
        if ((sym.entries[variableSym].flags & SFLG_POINTER) && (!isLastClause)) {}
        else if (sym.entries[variableSym].flags & SFLG_READONLY)
        {
            cc_error("Variable '%s' is read-only", sym.get_friendly_name(variableSym).c_str());
            return -1;
        }
        else if (sym.entries[variableSym].flags & SFLG_WRITEPROTECTED)
        {
            // write-protected variables can only be written by
            // the this ptr
            if ((vp_idx > 0) && (sym.entries[variablePath[vp_idx - 1].syml[0]].flags & SFLG_THISPTR)) {}
            else
            {
                cc_error("Variable '%s' is write-protected", sym.get_friendly_name(variableSym).c_str());
                return -1;
            }
        }
    }

    if ((writing) && (cannotAssign))
    {
        // an entire array or struct cannot be assigned to
        cc_error("Cannot assign to '%s'", sym.get_friendly_name(variableSym).c_str());
        return -1;
    }

    return 0;
}


// read the various types of values into AX
int do_variable_ax(ccCompiledScript*scrip, ags::SymbolScript_t syml, int syml_len, bool writing, bool mustBeWritable, bool negateLiteral, bool &write_same_as_read_access)
{
    // If this is a reading access, then the scope of AX will be the scope of the thing read
    if (!writing) set_ax_scope(scrip, syml[0]);

    // separate out the variable path, into a variablePath
    // for the bit between each dot
    VariableSymlist variablePath[MAX_VARIABLE_PATH];
    size_t variablePathSize;

    int retval = extractPathIntoParts(variablePath, syml, syml_len, variablePathSize);
    if (retval < 0) return retval;
    if (variablePathSize < 1) return 0;

    // start of the component that is looked up
    // given as an offset from the beginning of the overall structure
    int currentComponentOffset = 0;

    // Symbol and type of the first variable in the list 
    // (since that determines whether this is global/local)
    ags::Symbol_t firstVariableSym = variablePath[0].syml[0];
    ags::Symbol_t firstVariableType = sym.get_type(firstVariableSym);

    bool pointerIsOnStack = false;

    for (size_t vp_idx = 0; vp_idx < variablePathSize; vp_idx++)
    {
        VariableSymlist *thisClause = &variablePath[vp_idx];
        bool isLastClause = (vp_idx == variablePathSize - 1);

        ags::Symbol_t variableSym = thisClause->syml[0];
        int variableSymType = sym.get_type(variableSym);

        bool getJustTheAddressIntoAX = false;
        bool doMemoryAccessNow = false;

        bool cannotAssign = false;
        bool isProperty = false;
        bool accessActualPointer = false;

        // the memory access only wants to write if this is the
        // end of the path, not an intermediate pathing property
        bool writingThisTime = isLastClause && writing;

        // Mark the component as accessed
        sym.entries[variableSym].flags |= SFLG_ACCESSED;

        bool isArrayOffset;
        bool isDynamicArray;
        int retval = do_variable_ax_PrepareComponentAccess(scrip, variableSym, variableSymType, isLastClause, thisClause, writing, mustBeWritable, writingThisTime, firstVariableType, firstVariableSym, currentComponentOffset, getJustTheAddressIntoAX, doMemoryAccessNow, isProperty, isArrayOffset, write_same_as_read_access, isDynamicArray, pointerIsOnStack, accessActualPointer, cannotAssign);
        if (retval < 0) return retval;

        retval = do_variable_ax_CheckAccess(variableSym, variablePath, writing, mustBeWritable, write_same_as_read_access, isLastClause, vp_idx, cannotAssign);
        if (retval < 0) return retval;

        if (!doMemoryAccessNow && !isLastClause) continue;

        retval = do_variable_ax_ActualMemoryAccess(scrip, variableSym, variableSymType, pointerIsOnStack, writing, writingThisTime, isProperty, mustBeWritable, getJustTheAddressIntoAX, isArrayOffset, currentComponentOffset, accessActualPointer, firstVariableSym, firstVariableType, isDynamicArray, negateLiteral, isLastClause, variablePath, vp_idx);
        if (retval < 0) return retval;

        
    }

    // free the VariablePaths
    for (size_t vp_idx = 0; vp_idx < variablePathSize; vp_idx++)
    {
        variablePath[vp_idx].destroy();
    }

    return 0;
}


int read_variable_into_ax(ccCompiledScript*scrip, ags::SymbolScript_t syml, int syml_len, bool negateLiteral)
{
    bool dummy; // ignored parameter
    return do_variable_ax(scrip, syml, syml_len, false, false, negateLiteral, dummy);
}

int read_variable_into_ax(ccCompiledScript*scrip, ags::SymbolScript_t syml, int syml_len, bool negateLiteral, bool mustBeWritable, bool &write_same_as_read_access)
{
    return do_variable_ax(scrip, syml, syml_len, false, mustBeWritable, negateLiteral, write_same_as_read_access);
}

// Get or set a property
int call_property_func(ccCompiledScript *scrip, ags::Symbol_t propSym, int isWrite) 
{
    int numargs = 0;

    // AX contains the struct address

    // Always a struct member -- set OP = AX
    if ((sym.entries[propSym].flags & SFLG_STATIC) == 0)
    {
        scrip->push_reg(SREG_OP);
        scrip->write_cmd1(SCMD_CALLOBJ, SREG_AX);
    }

    if (isWrite)
    {
        if (0 == (sym.entries[propSym].flags & SFLG_IMPORTED))
        {
            cc_error("Internal error: Property is not import");
            return -1;
        }

        // BX contains the new value
        scrip->write_cmd1(SCMD_PUSHREAL, SREG_BX);
        numargs++;
    }

    if (sym.entries[propSym].flags & SFLG_ARRAY)
    {
        if (0 == (sym.entries[propSym].flags & SFLG_IMPORTED))
        {
            cc_error("Internal error: Property is not import");
            return -1;
        }
        
        // array indexer is in DX
        scrip->write_cmd1(SCMD_PUSHREAL, SREG_DX);
        numargs++;
    }

    if (sym.entries[propSym].flags & SFLG_IMPORTED)
    {
        // tell it how many args for this call (nested imported functions
        // causes stack problems otherwise)
        scrip->write_cmd1(SCMD_NUMFUNCARGS, numargs);
    }

    int propFunc;
    if (isWrite)
    {
        propFunc = sym.entries[propSym].get_propset();
    }
    else
    {
        propFunc = sym.entries[propSym].get_propget();
    }

    if (propFunc == 0)
    {
        cc_error("Internal error: Property is in use but not set");
        return -1;
    }

    // AX = Func Address
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, propFunc);

    if (sym.entries[propSym].flags & SFLG_IMPORTED)
    {
        scrip->fixup_previous(FIXUP_IMPORT);
        // do the call
        scrip->write_cmd1(SCMD_CALLEXT, SREG_AX);
        if (numargs > 0) scrip->write_cmd1(SCMD_SUBREALSTACK, numargs);
    }
    else
    {
        scrip->fixup_previous(FIXUP_FUNCTION);
        scrip->write_cmd1(SCMD_CALL, SREG_AX);

        // restore the stack
        if (numargs > 0)
        {
            scrip->cur_sp -= numargs * 4;
            scrip->write_cmd2(SCMD_SUB, SREG_SP, numargs * 4);
        }
    }

    if (!isWrite)
    {
        // function return type
        scrip->ax_val_type = sym.entries[propSym].vartype;
        scrip->ax_val_scope = SYM_LOCALVAR;
        if (sym.entries[propSym].flags & SFLG_DYNAMICARRAY)
            scrip->ax_val_type |= STYPE_DYNARRAY;
        if (sym.entries[propSym].flags & SFLG_POINTER)
            scrip->ax_val_type |= STYPE_POINTER;
        if (sym.entries[propSym].flags & SFLG_CONST)
            scrip->ax_val_type |= STYPE_CONST;
    }

    if ((sym.entries[propSym].flags & SFLG_STATIC) == 0)
    {
        scrip->pop_reg(SREG_OP);
    }

    return 0;
}


int do_variable_memory_access_vartype(ccCompiledScript * scrip, ags::Symbol_t variableSym, bool isProperty, int &gotValType)
{
    // it's a static member property
    if (!isProperty)
    {
        cc_error("Internal error: Static non-property access");
        return -1;
    }
    // just write 0 to AX for ease of debugging if anything
    // goes wrong
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);

    gotValType = sym.entries[variableSym].vartype;
    if (sym.entries[variableSym].flags & SFLG_CONST)
        gotValType |= STYPE_CONST;

    return 0;
}

int do_variable_memory_access_LitOrConst(ccCompiledScript * scrip, int mainVariableType, ags::Symbol_t variableSym, bool writing, bool mustBeWritable, bool negateLiteral, int &gotValType)
{
    if ((writing) || (mustBeWritable))
    {
        if (mainVariableType == SYM_LITERALVALUE)
        {
            cc_error("Cannot write to a literal value");
        }
        else
        {
            cc_error("Cannot write to a constant");
        }

        return -1;
    }

    int varSymValue;
    int retval = accept_literal_or_constant_value(variableSym, varSymValue, negateLiteral, "Error parsing integer value");
    if (retval < 0) return retval;

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, varSymValue);
    gotValType = sym.normalIntSym;

    return 0;
}


int do_variable_memory_access_LitFloat(ccCompiledScript * scrip, ags::Symbol_t variableSym, bool writing, bool mustBeWritable, int &gotValType)
{
    if ((writing) || (mustBeWritable))
    {
        cc_error("Cannot write to a literal value");
        return -1;
    }
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, interpret_float_as_int((float)atof(sym.get_name(variableSym))));
    gotValType = sym.normalFloatSym;
    return 0;
}


// a "normal" variable or a pointer
int do_variable_memory_access_Variable(ccCompiledScript * scrip, ags::Symbol_t mainVariableSym, int mainVariableType, ags::Symbol_t variableSym, bool isPointer, bool &wholePointerAccess, bool addressof, int soffset, bool extraoffset, bool isDynamicArray, bool writing, int &gotValType)
{
    int readwritecmd = get_readwrite_cmd_for_size(sym.entries[variableSym].ssize, writing);

    gotValType = sym.entries[variableSym].vartype;
    if (sym.entries[variableSym].flags & SFLG_CONST)
    {
        gotValType |= STYPE_CONST;
    }

    if (isPointer)
    {
        // the address is already in MAR by the caller
        if ((!wholePointerAccess) && ((!addressof) || (soffset) || (extraoffset)))
            scrip->write_cmd(SCMD_CHECKNULL);
        if (soffset != 0)
            scrip->write_cmd2(SCMD_ADD, SREG_MAR, soffset);
    }
    else if (mainVariableType == SYM_LOCALVAR)
    {
        // a local one
        scrip->write_cmd1(SCMD_LOADSPOFFS, scrip->cur_sp - soffset);
    }
    else // global variable
    {

        if (sym.entries[mainVariableSym].flags & SFLG_IMPORTED)
        {
            // imported variable, so get the import address and then add any offset
            scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, sym.entries[mainVariableSym].soffs);
            scrip->fixup_previous(FIXUP_IMPORT);
            if (soffset != 0)
                scrip->write_cmd2(SCMD_ADD, SREG_MAR, soffset);
        }
        else
        {
            scrip->write_cmd2(SCMD_LITTOREG, SREG_MAR, soffset);
            scrip->fixup_previous(FIXUP_GLOBALDATA);
        }
    }

    if (extraoffset)
    {
        if (isDynamicArray)
        {
            scrip->write_cmd1(SCMD_MEMREADPTR, SREG_MAR);
            scrip->write_cmd(SCMD_CHECKNULL);
            scrip->write_cmd1(SCMD_DYNAMICBOUNDS, SREG_CX);
        }

        scrip->write_cmd2(SCMD_ADDREG, SREG_MAR, SREG_CX);
    }
    else if (isDynamicArray)
    {
        // not accessing an element of it, must be whole thing
        wholePointerAccess = true;
        gotValType |= STYPE_DYNARRAY;
    }

    if (wholePointerAccess)
    {
        scrip->write_cmd1((writing ? SCMD_MEMWRITEPTR : SCMD_MEMREADPTR), SREG_AX);
    }
    else if (addressof)
    {
        scrip->write_cmd2(SCMD_REGTOREG, SREG_MAR, SREG_AX);
    }
    else
    {
        scrip->write_cmd1(readwritecmd, SREG_AX);
    }
    return 0;
}


int do_variable_memory_access_String(ccCompiledScript * scrip, bool writing, int soffset, int &gotValType)
{
    if (writing)
    {
        cc_error("Cannot write to a literal string");
        return -1;
    }

    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, soffset);
    scrip->fixup_previous(FIXUP_STRING);
    gotValType = sym.normalStringSym | STYPE_CONST;

    return 0;
}


int do_variable_memory_access_StructMember(ags::Symbol_t mainVariableSym)
{
    cc_error("Must include parent structure of member '%s'", sym.get_friendly_name(mainVariableSym).c_str());
    return -1;
}


int do_variable_memory_access_Null(ccCompiledScript * scrip, bool writing, int &gotValType)
{
    if (writing)
    {
        cc_error("Invalid use of null");
        return -1;
    }
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    gotValType = sym.nullSym | STYPE_POINTER;

    return 0;
}


int do_variable_memory_access_ActualAccess(ccCompiledScript * scrip, ags::Symbol_t mainVariableSym, int mainVariableType, ags::Symbol_t variableSym, bool writing, bool mustBeWritable, bool negateLiteral, bool isPointer, bool addressof, int soffset, bool extraoffset, bool isDynamicArray, bool isProperty, bool &wholePointerAccess, int &gotValType)
{
    switch (mainVariableType)
    {
    default:
        break;

    case SYM_CONSTANT:
        return do_variable_memory_access_LitOrConst(scrip, mainVariableType, variableSym, writing, mustBeWritable, negateLiteral, gotValType);

    case SYM_GLOBALVAR:
        return do_variable_memory_access_Variable(scrip, mainVariableSym, mainVariableType, variableSym, isPointer, wholePointerAccess, addressof, soffset, extraoffset, isDynamicArray, writing, gotValType);

    case SYM_LITERALFLOAT:
        return do_variable_memory_access_LitFloat(scrip, variableSym, writing, mustBeWritable, gotValType);

    case SYM_LITERALVALUE:
        return do_variable_memory_access_LitOrConst(scrip, mainVariableType, variableSym, writing, mustBeWritable, negateLiteral, gotValType);

    case SYM_LOCALVAR:
        return do_variable_memory_access_Variable(scrip, mainVariableSym, mainVariableType, variableSym, isPointer, wholePointerAccess, addressof, soffset, extraoffset, isDynamicArray, writing, gotValType);

    case SYM_NULL:
        return do_variable_memory_access_Null(scrip, writing, gotValType);

    case SYM_STRING:
        return do_variable_memory_access_String(scrip, writing, soffset, gotValType);

    case SYM_STRUCTMEMBER:
        return do_variable_memory_access_StructMember(mainVariableSym);

    case SYM_VARTYPE:
        return do_variable_memory_access_vartype(scrip, variableSym, isProperty, gotValType);
    }

    // Can't reach this
    cc_error("Internal error: Read/write ax called with non-variable parameter '%s'", sym.get_friendly_name(variableSym).c_str());
    return -99;
}


int do_variable_memory_access(ccCompiledScript *scrip, ags::Symbol_t variableSym,
    int variableSymType, bool isProperty,
    bool writing, bool mustBeWritable,
    bool addressof, bool extraoffset,
    int soffset, bool isPointer,
    bool wholePointerAccess,
    ags::Symbol_t mainVariableSym, int mainVariableType,
    bool isDynamicArray, bool negateLiteral)
{
    int gotValType;
    int retval = do_variable_memory_access_ActualAccess(scrip, mainVariableSym, mainVariableType, variableSym, writing, mustBeWritable, negateLiteral, isPointer, addressof, soffset, extraoffset, isDynamicArray, isProperty, wholePointerAccess, gotValType);
    if (retval < 0) return retval;

    if ((!isProperty && addressof) ||
        (isProperty && ((sym.entries[variableSym].flags & SFLG_POINTER) != 0)))
    {
        gotValType |= STYPE_POINTER;
    }

    if (writing)
    {
        retval = check_type_mismatch(scrip->ax_val_type, gotValType, true);
        if (retval < 0) return retval;
    }

    // Must be set both when reading OR writing
    scrip->ax_val_type = gotValType;

    if (isProperty)
    {
        // process_arrays_and_members will have set addressOf to true,
        // so AX now contains the struct address, and BX
        // contains the new value if this is a Set
        retval = call_property_func(scrip, variableSym, writing);
        if (retval < 0) return retval;
    }

    return 0;
}


int write_ax_to_variable(ccCompiledScript*scrip, ags::SymbolScript_t syml, int syml_len)
{
    bool dummy; // ignored parameter
    return do_variable_ax(scrip, syml, syml_len, true, false, false, dummy);
}


int evaluate_expression_CopyExpression(ccInternalList * source, size_t script_idx, ccInternalList *dest)
{
    size_t source_len = script_idx - source->pos;

    // Reserve memory for destination script and copy source into destination
    dest->script = static_cast<ags::SymbolScript_t>(malloc(source_len * sizeof(ags::Symbol_t)));
    if (!dest->script)
    {
        cc_error("Out of memory");
        return -1;
    }

    // Copy the content over, skipping METAs
    // Doing this with memcpy isn't faster, since the METAs would have to be deleted afterwards, anyway.
    size_t dest_idx = 0;
    for (size_t source_idx = source->pos; source_idx < script_idx; source_idx++)
    {
        if (source->script[source_idx] == SCODE_META)
        {
            source_idx += 2;
            continue;
        }
        dest->script[dest_idx++] = source->script[source_idx];
    }
    dest->length = dest_idx;
    dest->pos = 0;
    dest->allocated = source_len;

    return 0;
}


// evaluate the supplied expression, putting the result into AX
// returns 0 on success or -1 if compile error
// leaves targ pointing to last token in expression, so do getnext() to get the following ; or whatever
int evaluate_expression(ccInternalList *targ, ccCompiledScript*scrip, bool consider_paren_nesting)
{
    ccInternalList expr_script;
    size_t script_idx = 0;
    size_t paren_nesting_depth = 0;
    bool hadMetaOnly = true;

    // "Peek" into the symbols and find the first that is NOT part of the expression
    // [fw] This code can be rewritten with peeknext() -- this would skip METAs automatically
    for (script_idx = targ->pos; (int)script_idx < targ->length; script_idx++)
    {
        // Skip meta commands
        if (targ->script[script_idx] == SCODE_META)
        {
            script_idx += 2;
            continue;
        }

        // parenthesis depth counting
        if (sym.get_type(targ->script[script_idx]) == SYM_OPENPARENTHESIS)
        {
            paren_nesting_depth++;
        }
        else if (sym.get_type(targ->script[script_idx]) == SYM_CLOSEPARENTHESIS && paren_nesting_depth > 0)
        {
            paren_nesting_depth--;
            continue; // This means that the outermost ')' will be part of the expression
        }

        if (((paren_nesting_depth == 0) && !canBePartOfExpression(targ, script_idx)) ||   // The parens are all closed and the expression can't continue
            ((paren_nesting_depth == 0) && consider_paren_nesting) || // the last paren has JUST been closed and this is the deciding factor
            sym.get_type(targ->script[script_idx]) == SYM_CLOSEPARENTHESIS) // all parens had been closed beforehand and there is another ')' pending
        {
            // Here, script_idx is the first symbol that is NOT part of the expression
            if ((script_idx == targ->pos) || hadMetaOnly)
            {
                cc_error("Expression expected and not found at '%s'", sym.get_friendly_name(targ->script[script_idx]).c_str());
                return -1;
            }

            // Copy the expression into expr_script (in order to skip the METAs)
            int retval = evaluate_expression_CopyExpression(targ, script_idx, &expr_script);
            if (retval < 0) return retval;
            break;
        }

        // found a real token, not just metadata
        hadMetaOnly = false;
    }

    if ((int)script_idx >= targ->length)
    {
        cc_error("End of input reached in middle of expression");
        return -1;
    }

    // move the cursor of targ to the symbol after the expression, so that getnext will find it.
    targ->pos = script_idx;

    // we now have the expression in expr_script, parse it
    return parse_subexpr(scrip, expr_script.script, expr_script.length);
}


// We're in an assignment, cursym points to the LHS. Check that the LHS is assignable.
int evaluate_assignment_CheckLHSIsAssignable(ags::Symbol_t cursym, const ags::SymbolScript_t &vnlist, int vnlist_len)
{
    if (sym.entries[cursym].is_loadable_variable()) return 0;

    // Static property
    if ((sym.get_type(cursym) == SYM_VARTYPE) &&
        (vnlist_len > 2) &&
        (sym.entries[vnlist[2]].flags & SFLG_STATIC) > 0) return 0;
    
    cc_error("Variable or constant property required on left of \"%s\" assignment", sym.get_name(cursym));
    return -1;
}


// We are processing an assignment. vn_list[] contains a variable or a struct selector. 
// If it is a (static or dynamic) array, then check whether the assignment is allowed.
int evaluate_assignment_ArrayChecks(ags::Symbol_t cursym, ags::Symbol_t nextsym, size_t vnlist_len)
{
    // [fw] This may not be good enough. What if the array is in a struct? 
    //      Then the checks won't run.
    if ((sym.entries[cursym].flags & SFLG_DYNAMICARRAY) != 0) 
    {
        // Can only assign to entire dynamic arrays, e.g., allocate the memory
        if ((vnlist_len < 2) && (sym.get_type(nextsym) != SYM_ASSIGN))
        {
            cc_error("Cannot use operator \"%s\" with an entire dynamic array", sym.get_name(nextsym));
            return -1;
        }
        return 0;
    }

    if ((sym.entries[cursym].flags & SFLG_ARRAY) != 0)
    {
        if (vnlist_len < 2)
        {
            cc_error("Cannot assign a value to an entire static array");
            return -1;
        }
    }
    return 0;
}


// We compile something like "a += b"
int evaluate_assignment_MAssign(ccCompiledScript * scrip, ags::Symbol_t ass_symbol, const ags::SymbolScript_t & vnlist, int vnlist_len)
{
    // Read in and adjust the result
    scrip->push_reg(SREG_AX);
    int varTypeRHS = scrip->ax_val_type;

    int retval = read_variable_into_ax(scrip, vnlist, vnlist_len, false);
    if (retval < 0) return retval;

    retval = check_type_mismatch(varTypeRHS, scrip->ax_val_type, true);
    if (retval < 0) return retval;

    int cpuOp = sym.entries[ass_symbol].ssize;
    if (get_operator_valid_for_type(varTypeRHS, scrip->ax_val_type, cpuOp))
        return -1;

    scrip->pop_reg(SREG_BX);
    scrip->write_cmd2(cpuOp, SREG_AX, SREG_BX);
    
    retval = write_ax_to_variable(scrip, &vnlist[0], vnlist_len);
    if (retval < 0) return retval;
    return 0;
}


int evaluate_assignment_Assign(ccCompiledScript * scrip, int vnlist_len, const ags::SymbolScript_t & vnlist)
{
    // Convert normal literal string into String object
    size_t finalPartOfLHS = vnlist_len - 1;
    if (sym.get_type(vnlist[vnlist_len - 1]) == SYM_CLOSEBRACKET)
    {
        // deal with  a[1] = b
        findOpeningBracketOffs(vnlist_len - 1, vnlist, finalPartOfLHS);
        if (--finalPartOfLHS < 0)
        {
            cc_error("No matching '[' for ']'");
            return -1;
        }
    }
    // If we need a string object ptr but AX contains a normal string, convert AX
    ConvertAXIntoStringObject(scrip, sym.entries[vnlist[finalPartOfLHS]].vartype);
    
    int retval = write_ax_to_variable(scrip, &vnlist[0], vnlist_len);
    if (retval < 0) return retval;
    
    return 0;
}


int evaluate_assignment_SAssign(ccCompiledScript * scrip, ags::Symbol_t ass_symbol, const ags::SymbolScript_t & vnlist, int vnlist_len)
{
    bool write_same_as_read_access;
    int retval = read_variable_into_ax(scrip, &vnlist[0], vnlist_len, false, true, write_same_as_read_access);
    if (retval < 0) return retval;

    // Get the bytecode operator that corresponds to the assignment symbol and type
    int cpuOp = sym.entries[ass_symbol].ssize;
    retval = get_operator_valid_for_type(scrip->ax_val_type, 0, cpuOp);
    if (retval < 0) return retval;

    scrip->write_cmd2(cpuOp, SREG_AX, 1);

    if (write_same_as_read_access)
    {
        // since the MAR won't have changed, we can directly write
        // the value back to it without re-calculating the offset
        // [fw] Passing info around through a global variable: That is a HUGE code smell.
        scrip->write_cmd1(get_readwrite_cmd_for_size(readcmd_lastcalledwith, true), SREG_AX);
        return 0;
    }

    // copy the result (currently in AX) into the variable
    retval = write_ax_to_variable(scrip, &vnlist[0], vnlist_len);
    if (retval < 0) return retval;

    return 0;
}


int evaluate_assignment_DoAssignment(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t ass_symbol, const ags::SymbolScript_t &vnlist, int vnlist_len)
{
    switch (sym.get_type(ass_symbol))
    {
    default: // can't happen
    {
        cc_error("Internal error: Illegal assignment symbol found");
        return -99;
    }

    case SYM_ASSIGN:
    {
        // Get RHS
        int retval = evaluate_expression(targ, scrip, false);
        if (retval < 0) return retval;

        // Do assignment
        return evaluate_assignment_Assign(scrip, vnlist_len, vnlist);
    }

    case SYM_MASSIGN:
    {
        // Get RHS
        int retval = evaluate_expression(targ, scrip, false);
        if (retval < 0) return retval;

        // Do assignment
        return evaluate_assignment_MAssign(scrip, ass_symbol, vnlist, vnlist_len);
    }

    case SYM_SASSIGN:
    {
        // "++" or "--". There isn't any RHS to read in. Do assignment.
        return evaluate_assignment_SAssign(scrip, ass_symbol, vnlist, vnlist_len);
    }
    }

    return 0;
}


// We've read a variable or selector of a struct into vn_list[], the last identifying component is in cursym.
// An assignment symbol is following. Compile the assignment.
int evaluate_assignment(ccInternalList *targ, ccCompiledScript *scrip, ags::Symbol_t cursym, ags::Symbol_t statementEndSymbol, ags::SymbolScript_t vnlist, int vnlist_len)
{
    // Check that the LHS is a loadable variable or a static property. 
    int retval = evaluate_assignment_CheckLHSIsAssignable(cursym, vnlist, vnlist_len);
    if (retval < 0) return retval;

    // Checks to do for the LHS if it is a (dynamic or static) array
    retval = evaluate_assignment_ArrayChecks(cursym, targ->peeknext(), vnlist_len);
    if (retval < 0) return retval;

    if (sym.entries[cursym].flags & SFLG_ISSTRING)
    {
        cc_error("cannot assign to string; Use Str* functions instead");
        return -1;
    }

    // Do the assignment
    retval = evaluate_assignment_DoAssignment(targ, scrip, targ->getnext(), vnlist, vnlist_len);
    if (retval < 0) return retval;

    // Gobble the statement end symbol (usually ';', can be ')')
    if (sym.get_type(targ->getnext()) != statementEndSymbol)
    {
        cc_error("Expected '%s'", sym.get_name(statementEndSymbol));
        return -1;
    }

    return 0;
}


// true if the symbol is "int" and the like.
inline bool sym_is_predef_typename(ags::Symbol_t symbl)
{
    return (symbl >= 0 && symbl <= sym.normalFloatSym);
}


int parse_var_decl_InitialValAssignment_ToLocal(ccInternalList *targ, ccCompiledScript * scrip, int completeVarType)
{
    // Parse and compile the expression
    int retval = evaluate_expression(targ, scrip, false);
    if (retval < 0) return retval;

    // If we need a string object ptr but AX contains a normal string, convert AX
    ConvertAXIntoStringObject(scrip, completeVarType);

    // Check whether the types match
    retval = check_type_mismatch(scrip->ax_val_type, completeVarType, true);
    if (retval < 0) return retval;
    return 0;
}


int parse_var_decl_InitialValAssignment_ToGlobalFloat(ccInternalList * targ, bool is_neg, void *& initial_val_ptr)
{
    // initialize float
    if (sym.get_type(targ->peeknext()) != SYM_LITERALFLOAT)
    {
        cc_error("Expected floating point value after '='");
        return -1;
    }

    float float_init_val = static_cast<float>(atof(sym.get_name(targ->getnext())));
    if (is_neg) float_init_val = -float_init_val;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }

    // Interpret the float as an int; move that into the allocated space
    (static_cast<long *>(initial_val_ptr))[0] = interpret_float_as_int(float_init_val);

    return 0;
}


int parse_var_decl_InitialValAssignment_ToGlobalNonFloat(ccInternalList * targ, bool is_neg, void *& initial_val_ptr)
{
    // Initializer for an integer value
    int int_init_val;
    int retval = accept_literal_or_constant_value(targ->getnext(), int_init_val, is_neg, "Expected integer value after '='");
    if (retval < 0) return retval;

    // Allocate space for one long value
    initial_val_ptr = malloc(sizeof(long));
    if (!initial_val_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }
    // Convert int to long; move that into the allocated space
    (reinterpret_cast<long *>(initial_val_ptr))[0] = int_init_val;

    return 0;
}


// if initial_value is non-null, it returns malloc'd memory that must be free
int parse_var_decl_InitialValAssignment_ToGlobal(ccInternalList *targ, long varname, void * &initial_val_ptr)
{
    initial_val_ptr = nullptr;

    if ((sym.entries[varname].flags & SFLG_POINTER) != 0)
    {
        cc_error("Cannot assign an initial value to a global pointer");
        return -1;
    }

    if ((sym.entries[varname].flags & SFLG_DYNAMICARRAY) != 0)
    {
        cc_error("Cannot assign an initial value to a dynamic array");
        return -1;
    }

    // [fw] This check will probably fail to work for structs that contain just 1 int.
    if (sym.entries[varname].ssize > 4)
    {
        cc_error("Cannot initialize struct type");
        return -1;
    }

    // accept leading '-' if present
    bool is_neg = false;
    if (sym.get_name(targ->peeknext())[0] == '-')
    {
        is_neg = true;
        targ->getnext();
    }

    // Do actual assignment
    if (sym.entries[varname].vartype == sym.normalFloatSym)
    {
        return parse_var_decl_InitialValAssignment_ToGlobalFloat(targ, is_neg, initial_val_ptr);
    }
    return parse_var_decl_InitialValAssignment_ToGlobalNonFloat(targ, is_neg, initial_val_ptr);
}


// We have accepted something like "int var" and we are reading "= val"
int parse_var_decl_InitialValAssignment(ccInternalList *targ, ccCompiledScript * scrip, int next_type, Globalness isglobal, long varname, int type_of_defn, void * &initial_val_ptr, FxFixupType &need_fixup)
{
    targ->getnext();  // skip the '='

    initial_val_ptr = nullptr; // there is no initial value
    if (isglobal == GlGlobalImport)
    {
        cc_error("Cannot set initial value of imported variables");
        return -1;
    }
    if ((sym.entries[varname].flags & (SFLG_ARRAY | SFLG_DYNAMICARRAY)) == SFLG_ARRAY)
    {
        cc_error("Cannot assign a value to an array");
        return -1;
    }
    if (sym.entries[varname].flags & SFLG_ISSTRING)
    {
        cc_error("Cannot assign a value to a string, use StrCopy");
        return -1;
    }

    int completeVarType = type_of_defn;
    if (sym.entries[varname].flags & SFLG_POINTER)      completeVarType |= STYPE_POINTER;
    if (sym.entries[varname].flags & SFLG_DYNAMICARRAY) completeVarType |= STYPE_DYNARRAY;

    if (isglobal == GlLocal)
    {
        // accept an expression of the appropriate type
        // This is compiled as an assignment, so there is no initial value to return here
        initial_val_ptr = nullptr;
        int retval = parse_var_decl_InitialValAssignment_ToLocal(targ, scrip, completeVarType);
        if (retval < 0) return retval;

        need_fixup = FxFixupType2;
    }
    else // global var
    {
        // accept a literal or constant of the appropriate type
        int retval = parse_var_decl_InitialValAssignment_ToGlobal(targ, varname, initial_val_ptr);
        if (retval < 0) return retval;
    }

    return 0;
}


// Move variable information into the symbol table
void parse_var_decl_Var2SymTable(int var_name, Globalness is_global, bool is_pointer, int size_of_defn, int type_of_defn)
{
    sym.entries[var_name].extends = 0;
    sym.entries[var_name].stype = (is_global == GlLocal) ? SYM_LOCALVAR : SYM_GLOBALVAR;
    sym.entries[var_name].ssize = size_of_defn;
    sym.entries[var_name].arrsize = 1;
    sym.entries[var_name].vartype = type_of_defn;
    if (is_pointer)
        sym.entries[var_name].flags |= SFLG_POINTER;
}


// we have accepted something like "int a" and we're expecting "["
int parse_var_decl_ArrayDecl(ccInternalList *targ, int var_name, int type_of_defn, int &array_size, int &size_of_defn)
{
    // an array
    targ->getnext();  // skip the [

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
        sym.entries[var_name].flags |= SFLG_DYNAMICARRAY;
        array_size = 0;
        size_of_defn = 4;
    }
    else
    {
        if (sym.entries[type_of_defn].flags & SFLG_HASDYNAMICARRAY)
        {
            cc_error("Cannot declare an array of a type containing dynamic array(s)");
            return -1;
        }

        ags::Symbol_t nextt = targ->getnext();

        int retval = accept_literal_or_constant_value(nextt, array_size, false, "Array size must be constant value");
        if (retval < 0) return retval;

        if (array_size < 1)
        {
            cc_error("Array size must be >= 1");
            return -1;
        }

        size_of_defn *= array_size;
    }
    sym.entries[var_name].flags |= SFLG_ARRAY;
    sym.entries[var_name].arrsize = array_size;

    if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
    {
        cc_error("Expected ']'");
        return -1;
    }

    return 0;
}


int parse_var_decl_StringDecl_GlobalNoImport(ccCompiledScript * scrip, void *&initial_value_ptr, FxFixupType &fixup_needed)
{
    // Reserve space for the string in globaldata; 
    // the initial value is the offset to the newly reserved space

    int offset_of_init_string = scrip->add_global(STRING_LENGTH, NULL);
    if (offset_of_init_string < 0)
    {
        cc_error("Out of memory");
        return -1;
    }

    initial_value_ptr = malloc(sizeof(int));
    if (!initial_value_ptr)
    {
        cc_error("Out of memory");
        return -1;
    }

    reinterpret_cast<int *>(initial_value_ptr)[0] = offset_of_init_string;
    fixup_needed = FxFixupDataData;
    return 0;
}


int parse_var_decl_StringDecl_Local(ccCompiledScript * scrip, ags::Symbol_t var_name, void * &initial_value_ptr)
{
    // Note: We can't use scrip->cur_sp since we don't know if we'll be in a nested function call at the time
    initial_value_ptr = nullptr;

    sym.entries[var_name].flags |= SFLG_STRBUFFER; // Note in the symbol table that this var is a stringbuffer
    scrip->cur_sp += STRING_LENGTH; // reserve STRING_LENGTH bytes for the var on the stack

                                    // CX will contain the address of the new memory, which will be added to the stack
    scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_CX); // Copy current stack pointer to CX
    scrip->write_cmd2(SCMD_ADD, SREG_SP, STRING_LENGTH); // write code for reserving STRING LENGTH bytes 
    return 0;
}


int parse_var_decl_StringDecl(ccCompiledScript * scrip, ags::Symbol_t var_name, Globalness is_global, void * &initial_value_ptr, FxFixupType &fixup_needed)
{
    if (ccGetOption(SCOPT_OLDSTRINGS) == 0)
    {
        cc_error("Type 'string' is no longer supported; use String instead");
        return -1;
    }

    if (sym.entries[var_name].flags & SFLG_DYNAMICARRAY)
    {
        cc_error("Arrays of old-style strings are not supported");
        return -1;
    }

    if (is_global == GlGlobalImport)
    {
        // cannot import, because string is really char*, and the pointer won't resolve properly
        cc_error("Cannot import string; use char[] instead");
        return -1;
    }

    initial_value_ptr = nullptr;
    fixup_needed = FxNoFixup;

    sym.entries[var_name].flags |= SFLG_ISSTRING;

    switch (is_global)
    {
    default: // This cannot happen
    {
        cc_error("Internal error: Wrong value for globalness");
        return 99;
    }

    case GlGlobalNoImport:
        return parse_var_decl_StringDecl_GlobalNoImport(scrip, initial_value_ptr, fixup_needed);

    case GlLocal:
        return parse_var_decl_StringDecl_Local(scrip, var_name, initial_value_ptr);
    }
}


// We've parsed a definition of a local variable, provide the code for it
void parse_var_decl_CodeForDefnOfLocal(ccCompiledScript * scrip, int var_name, FxFixupType fixup_needed, int size_of_defn, void * initial_value)
{
    scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR); // MAR = SP

    // code for the initial assignment or the initialization to zeros
    if (fixup_needed == FxFixupType2)
    {
        // expression worked out into ax
        if ((sym.entries[var_name].flags & (SFLG_POINTER | SFLG_DYNAMICARRAY)) != 0)
        {
            scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
        }
        else
        {
            scrip->write_cmd1(get_readwrite_cmd_for_size(size_of_defn, true), SREG_AX);
        }
    }
    else if (initial_value == NULL)
    {
        // local string, so the memory chunk pointer needs to be written
        scrip->write_cmd1(SCMD_MEMWRITE, SREG_CX); // memory[MAR] = CX
    }
    else
    {
        // local variable without initial value -- zero it
        scrip->write_cmd1(SCMD_ZEROMEMORY, size_of_defn); // memory[MAR+0, 1... ] = 0;
    }

    if (fixup_needed == FxFixupDataData)
    {
        sym.entries[var_name].flags |= SFLG_STRBUFFER;
        scrip->fixup_previous(FIXUP_STACK);
    }

    // Allocate space on the stack
    if (size_of_defn > 0)
    {
        scrip->cur_sp += size_of_defn;
        scrip->write_cmd2(SCMD_ADD, SREG_SP, size_of_defn);
    }
}


int parse_var_decl_CheckIllegalCombis(int var_name, int type_of_defn, bool is_pointer, Globalness is_global)
{
    if (sym.get_type(var_name) != 0)
    {
        cc_error("Symbol '%s' is already defined");
        return -1;
    }

    if ((sym.entries[type_of_defn].flags & SFLG_MANAGED) && (!is_pointer) && (is_global != GlGlobalImport))
    {
        // managed structs must be allocated via ccRegisterObject,
        // and cannot be declared normally in the script (unless imported)
        cc_error("Cannot declare local instance of managed type");
        return -1;
    }

    if (type_of_defn == sym.normalVoidSym)
    {
        cc_error("'void' not a valid variable type");
        return -1;
    }

    if (((sym.entries[type_of_defn].flags & SFLG_MANAGED) == 0) && (is_pointer) && (is_global != GlGlobalImport))
    {
        // can only point to managed structs
        cc_error("Cannot declare pointer to non-managed type");
        return -1;
    }

    return 0;
}


int parse_var_decl0(
    ccInternalList *targ,
    ccCompiledScript * scrip,
    int var_name,
    int type_of_defn,  // i.e. "void" or "int"
    int next_type, // type of the following symbol
    Globalness is_global,
    bool is_pointer,
    bool &another_var_follows,
    void *initial_value_ptr)
{
    int retval = parse_var_decl_CheckIllegalCombis(var_name, type_of_defn, is_pointer, is_global);
    if (retval < 0) return retval;

    // this will become true iff we gobble a "," after the defn and expect another var of the same type
    another_var_follows = false;

    // will contain the initial value of the var being declared
    initial_value_ptr = nullptr;
    FxFixupType fixup_needed = FxNoFixup;
    int array_size = 1;

    int size_of_defn = sym.entries[type_of_defn].ssize;
    if (is_pointer) size_of_defn = 4;

    // Enter the variable into the symbol table
    parse_var_decl_Var2SymTable(var_name, is_global, is_pointer, size_of_defn, type_of_defn);

    // Default assignment
    if (next_type == SYM_OPENBRACKET)
    {
        // Parse the bracketed expression; determine whether it is dynamic; if not, determine the size
        int retval = parse_var_decl_ArrayDecl(targ, var_name, type_of_defn, array_size, size_of_defn);
        if (retval < 0) return retval;

        next_type = sym.get_type(targ->peeknext());
        initial_value_ptr = calloc(1, size_of_defn + 1);
    }
    else if (size_of_defn > 4)
    {
        // initialize the struct to all zeros
        initial_value_ptr = calloc(1, size_of_defn + 1);
    }
    else if (strcmp(sym.get_name(type_of_defn), "string") == 0)
    {
        int retval = parse_var_decl_StringDecl(scrip, var_name, is_global, initial_value_ptr, fixup_needed);
        if (retval < 0) return retval;
    }
    else
    {
        // a kind of atomic value (char, int, long, float, pointer) -- initialize to 0
        initial_value_ptr = calloc(1, sizeof(long));
    }

    // initial assignment, i.e. a clause "= value" following the definition
    if (next_type == SYM_ASSIGN)
    {
        if (initial_value_ptr) free(initial_value_ptr);
        int retval = parse_var_decl_InitialValAssignment(targ, scrip, next_type, is_global, var_name, type_of_defn, initial_value_ptr, fixup_needed);
        if (retval < 0) return retval;
        next_type = sym.get_type(targ->peeknext());
    }


    switch (is_global)
    {
    default:
        cc_error("Internal error: Wrong value '%d' of is_global", is_global);
        return -99;

    case GlGlobalImport:
        sym.entries[var_name].soffs = scrip->add_new_import(sym.get_name(var_name));
        sym.entries[var_name].flags |= SFLG_IMPORTED;
        if (sym.entries[var_name].soffs == -1)
        {
            cc_error("Internal error: Import table overflow");
            return -1;
        }
        break;

    case GlGlobalNoImport:
        sym.entries[var_name].soffs = scrip->add_global(size_of_defn, reinterpret_cast<const char *>(initial_value_ptr));
        if (sym.entries[var_name].soffs < 0) return -1;
        if (fixup_needed == FxFixupDataData) scrip->add_fixup(sym.entries[var_name].soffs, FIXUP_DATADATA);
        break;

    case GlLocal:
        sym.entries[var_name].soffs = scrip->cur_sp;

        // Output the code for defining the local and initializing it
        parse_var_decl_CodeForDefnOfLocal(scrip, var_name, fixup_needed, size_of_defn, initial_value_ptr);
    }

    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }
    if (next_type == SYM_COMMA || next_type == SYM_SEMICOLON)
    {
        targ->getnext();  // skip the comma or semicolon
        another_var_follows = (next_type == SYM_COMMA);
        return 0;
    }

    cc_error("Expected ',' or ';', not '%s'", sym.get_friendly_name(next_type).c_str());
    return -1;
}

// wrapper around parse_var_decl0() to prevent memory leakage
inline int parse_var_decl(
    ccInternalList *targ,
    ccCompiledScript * scrip,
    int var_name,
    int type_of_defn,  // i.e. "void" or "int"
    int next_type, // type of the following symbol
    Globalness is_global,
    bool is_pointer,
    bool &another_var_follows)
{
    void *initial_value_ptr = nullptr;

    int retval = parse_var_decl0(targ, scrip, var_name, type_of_defn, next_type, is_global, is_pointer, another_var_follows, initial_value_ptr);

    if (initial_value_ptr != nullptr) free(initial_value_ptr);

    return retval;
}


void cs_parser_handle_openbrace_FuncBody(ccCompiledScript * scrip, ags::Symbol_t inFuncSym, int isMemberFunction, bool is_noloopcheck, ags::NestingStack * nesting_stack)
{
    // write base address of function for any relocation needed later
    scrip->write_cmd1(SCMD_THISBASE, scrip->codesize);
    if (is_noloopcheck) scrip->write_cmd(SCMD_LOOPCHECKOFF);

    // loop through all parameters and check whether they are pointers
    // the first entry is the return value, so skip that
    for (size_t pa = 1; pa <= sym.entries[inFuncSym].get_num_args(); pa++)
    {
        if (sym.entries[inFuncSym].funcparamtypes[pa] & (STYPE_POINTER | STYPE_DYNARRAY))
        {
            // pointers are passed in on the stack with the real
            // memory address -- convert this to the mem handle
            // since params are pushed backwards, this works
            // the +1 is to deal with the return address
            scrip->write_cmd1(SCMD_LOADSPOFFS, 4 * (pa + 1));
            scrip->write_cmd1(SCMD_MEMREAD, SREG_AX);
            scrip->write_cmd1(SCMD_MEMINITPTR, SREG_AX);
        }
    }

    // non-static member function -- declare "this" ptr
    if ((isMemberFunction) && ((sym.entries[inFuncSym].flags & SFLG_STATIC) == 0))
    {
        const ags::Symbol_t thisSym = sym.find("this");
        if (thisSym > 0)
        {
            int varsize = 4;
            // declare "this" inside member functions
            sym.entries[thisSym].stype = SYM_LOCALVAR;
            sym.entries[thisSym].vartype = isMemberFunction;
            sym.entries[thisSym].ssize = varsize; // pointer to struct
            sym.entries[thisSym].sscope = static_cast<short>(nesting_stack->Depth() - 1);
            sym.entries[thisSym].flags = SFLG_READONLY | SFLG_ACCESSED | SFLG_POINTER | SFLG_THISPTR;
            // declare as local variable
            sym.entries[thisSym].soffs = scrip->cur_sp;
            scrip->write_cmd2(SCMD_REGTOREG, SREG_SP, SREG_MAR);
            // first of all, write NULL to the pointer so that
            // it doesn't try and free it in the following call
            scrip->write_cmd2(SCMD_WRITELIT, varsize, 0);
            // write the OP location into the variable
            //scrip->write_cmd1(SCMD_MEMINITPTR, SREG_OP);
            // the "this" ptr is allocated a space on the stack,
            // even though it's not used (since accesses go directly
            // via the OP)
            scrip->cur_sp += varsize;
            scrip->write_cmd2(SCMD_ADD, SREG_SP, varsize);
        }
    }
}


int cs_parser_handle_openbrace(
    ccCompiledScript * scrip,
    ags::NestingStack *nesting_stack,
    int in_func,
    ags::Symbol_t inFuncSym,
    int isMemberFunction,
    bool is_noloopcheck)
{
    if (in_func < 0)
    {
        cc_error("Unexpected '{'");
        return -1;
    }

    if (nesting_stack->IsUnbraced())
    {
        cc_error("Internal compiler error in openbrace");
        return -1;
    }

    // Assume a brace without special reason as a default
    int retval = nesting_stack->Push(ags::NestingStack::NTNothing);
    if (retval < 0) return retval;

    if (nesting_stack->Depth() == 2)
    {
        // In this case, the braces are around a function body
        nesting_stack->SetType(ags::NestingStack::NTFunction);
        cs_parser_handle_openbrace_FuncBody(scrip, inFuncSym, isMemberFunction, is_noloopcheck, nesting_stack);
    }

    return 0;
}


int cs_parser_handle_closebrace(ccInternalList *targ, ccCompiledScript *scrip, ags::NestingStack *nesting_stack,  int &in_func, ags::Symbol_t &inFuncSym, ags::Symbol_t &isMemberFunction)
{
    size_t nesting_level = nesting_stack->Depth() - 1;    
    
    if (nesting_level == 0 || nesting_stack->IsUnbraced())
    {
        cc_error("Unexpected '}'");
        return -1;
    }



    if (nesting_level == 1)
    {
        // Code  trace reaches end of a function
        // Emit code that returns 0
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }

    free_pointers_of_locals(scrip, nesting_level - 1);
    int totalsub = stacksize_of_locals(nesting_level - 1);
    if (totalsub > 0)
    {
        // Reduce the "high point" of the stack appropriately, 
        // write code for popping the bytes from the stack
        scrip->cur_sp -= totalsub;
        scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    }

    // All the local variables that were defined within the braces become invalid
    remove_locals_from_symtable(nesting_level);
    
    if (nesting_level == 1)
    {
        // We've just finished the body of the current function.
        in_func = -1;
        inFuncSym = -1;
        isMemberFunction = 0;

        // Write code to return from the function.
        // This pops the return address from the stack, 
        // so adjust the "high point" of stack allocation appropriately
        scrip->write_cmd(SCMD_RET);
        scrip->cur_sp -= 4;

        nesting_stack->Pop();
        return 0;
    }

    // Deal with actions that need to be done at the end 
    int retval;
    switch (nesting_stack->Type())
    {
    default: 
        nesting_stack->Pop();
        return 0;

    case ags::NestingStack::NTBracedDo:
        retval = deal_with_end_of_do(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case ags::NestingStack::NTBracedElse:
    case ags::NestingStack::NTBracedThen:
        {
            bool if_turned_into_else;
            int retval = deal_with_end_of_ifelse(targ, scrip, nesting_stack, if_turned_into_else);
            if (retval < 0) return retval;
            if (if_turned_into_else) return 0;
        }
        break;

    case ags::NestingStack::NTSwitch:
        retval = deal_with_end_of_switch(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;
    }


    // loop round doing all the end of elses, but break once an IF
    // has been turned into an ELSE
    while (nesting_stack->IsUnbraced())
    {
        if (nesting_stack->Type() == ags::NestingStack::NTUnbracedDo)
        {
            int retval = deal_with_end_of_do(targ, scrip, nesting_stack);
            if (retval < 0) return retval;
        }

        bool if_turned_into_else;
        int retval = deal_with_end_of_ifelse(targ, scrip, nesting_stack, if_turned_into_else);
        if (retval < 0) return retval;
        if (if_turned_into_else) break;
    }
    return 0;
}

void cs_parser_struct_SetTypeInSymboltable(SymbolTableEntry &entry, bool struct_is_managed, bool struct_is_builtin, bool struct_is_autoptr)
{
    entry.extends = 0;
    entry.stype = SYM_VARTYPE;
    entry.flags |= SFLG_STRUCTTYPE;
    entry.ssize = 0;

    if (struct_is_managed)
    {
        entry.flags |= SFLG_MANAGED;
    }

    if (struct_is_builtin)
    {
        entry.flags |= SFLG_BUILTIN;
    }

    if (struct_is_autoptr)
    {
        entry.flags |= SFLG_AUTOPTR;
    }
}


// We have accepted something like "struct foo" and are waiting for "extends"
int cs_parser_struct_ExtendsClause(ccInternalList *targ, int stname, ags::Symbol_t &extendsWhat, int &size_so_far)
{
    targ->getnext(); // gobble "extends"
    extendsWhat = targ->getnext(); // name of the extended struct
    if (sym.get_type(extendsWhat) != SYM_VARTYPE)
    {
        cc_error("Expected a struct type here");
        return -1;
    }
    SymbolTableEntry & struct_entry = sym.entries[stname];
    SymbolTableEntry & extends_entry = sym.entries[extendsWhat];

    if ((extends_entry.flags & SFLG_STRUCTTYPE) == 0)
    {
        cc_error("Must extend a struct type");
        return -1;
    }
    if ((extends_entry.flags & SFLG_MANAGED) == 0 && (struct_entry.flags & SFLG_MANAGED))
    {
        cc_error("Managed struct cannot extend the unmanaged struct '%s'", sym.get_name(extendsWhat));
        return -1;
    }
    if ((extends_entry.flags & SFLG_MANAGED) && (struct_entry.flags & SFLG_MANAGED) == 0)
    {
        cc_error("Unmanaged struct cannot extend the managed struct '%s'", sym.get_name(extendsWhat));
        return -1;
    }
    if ((extends_entry.flags & SFLG_BUILTIN) && (struct_entry.flags & SFLG_BUILTIN) == 0)
    {
        cc_error("The built-in type '%s' cannot be extended by a concrete struct. Use extender methods instead", sym.get_name(extendsWhat));
        return -1;
    }
    size_so_far = extends_entry.ssize;
    struct_entry.extends = extendsWhat;

    return 0;
}


int cs_paerser_struct_ParseMemberQualifiers(
    ccInternalList *targ,
    ags::Symbol_t &cursym,
    bool &is_readonly,
    Importness &is_import,
    bool &is_property,
    bool &is_static,
    bool &is_protected,
    bool &is_writeprotected)
{
    // [fw] Check that each qualifier is used exactly once?
    while (true)
    {
        cursym = targ->getnext();

        switch (sym.get_type(cursym))
        {
        default: break;
        case SYM_IMPORT:         is_import = ImImportType1; continue;
        case SYM_PROPERTY:       is_property = true;       continue;
        case SYM_PROTECTED:      is_protected = true;      continue;
        case SYM_READONLY:       is_readonly = true;       continue;
        case SYM_STATIC:         is_static = true;         continue;
        case SYM_WRITEPROTECTED: is_writeprotected = true; continue;
        }
        break;
    };

    if (is_protected && is_writeprotected)
    {
        cc_error("Field cannot be both protected and write-protected.");
        return -1;
    }
    return 0;
}

int cs_parser_struct_IsMemberTypeIllegal(ccInternalList *targ, int stname, ags::Symbol_t cursym, bool member_is_pointer, Importness member_is_import)
{
    // must either have a type of a struct here.
    if ((sym.get_type(cursym) != SYM_VARTYPE) &&
        (sym.get_type(cursym) != SYM_UNDEFINEDSTRUCT))
    {
        // Complain about non-type
        std::string type_name = sym.get_name(cursym);
        std::string prefix = sym.get_name(stname);
        prefix += "::";
        if (type_name.substr(0, prefix.length()) == prefix)
        {
            // The tokenizer has mangled the symbol, undo that.
            type_name = type_name.substr(prefix.length(), type_name.length());
        }
        cc_error("Expected a variable type instead of '%s'", type_name.c_str());
        return -1;
    }

    // [fw] Where's the problem?
    if (cursym == sym.normalStringSym)
    {
        cc_error("'string' not allowed inside struct");
        return -1;
    }

    if (targ->peeknext() < 0)
    {
        cc_error("Invalid syntax near '%s'", sym.get_friendly_name(cursym).c_str());
        return -1;
    }

    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    if (sym.get_type(cursym) == SYM_UNDEFINEDSTRUCT)
    {
        cc_error("Invalid use of forward-declared struct");
        return -1;
    }

    // [fw] Where is the problem?
    if ((sym.entries[cursym].flags & SFLG_STRUCTTYPE) && (member_is_pointer == 0))
    {
        cc_error("Member variable cannot be struct");
        return -1;
    }
    if ((member_is_pointer) && (sym.entries[stname].flags & SFLG_MANAGED) && (member_is_import == ImNoImport))
    {
        cc_error("Member variable of managed struct cannot be pointer");
        return -1;
    }
    else if ((sym.entries[cursym].flags & SFLG_MANAGED) && (!member_is_pointer))
    {
        cc_error("Cannot declare non-pointer of managed type");
        return -1;
    }
    else if (((sym.entries[cursym].flags & SFLG_MANAGED) == 0) && (member_is_pointer))
    {
        cc_error("Cannot declare pointer to non-managed type");
        return -1;
    }
    return 0;
}


int cs_parser_struct_CheckMemberNotInInheritedStruct(
    ags::Symbol_t vname,
    const char * memberExt,
    ags::Symbol_t extendsWhat)
{
    // check that we haven't already inherited a member
    // with the same name
    ags::Symbol_t member = vname;
    if (memberExt == nullptr)
    {
        cc_error("Internal compiler error dbc");
        return -1;
    }
    // skip the colons
    memberExt += 2;
    // find the member-name-only sym
    member = sym.find(memberExt);
    // if it's never referenced it won't exist, so create it
    if (member < 1)  member = sym.add_ex(memberExt, static_cast<ags::Symbol_t>(0), 0);

    if (find_member_sym(extendsWhat, member, true) >= 0)
    {
        cc_error("'%s' already defined by inherited class", sym.get_friendly_name(member).c_str());
        return -1;
    }
    // not found -- a good thing, but find_member_sym will
    // have errored. Clear the error
    ccError = 0;
    return 0;
}

// We have accepted something like "struct foo extends bar { const int".
// We're waiting for the name of the member.
int cs_parser_struct_MemberDefnVarOrFuncOrArray(
    ccInternalList *targ,
    ccCompiledScript * scrip,
    ags::Symbol_t extendsWhat,
    ags::Symbol_t stname,
    int in_func, // [fw] ONLY used for funcs in structs
    int nested_level,
    int curtype,
    bool type_is_property,
    bool type_is_readonly,
    Importness type_is_import,
    bool type_is_protected,
    bool type_is_writeprotected,
    bool type_is_pointer,
    bool type_is_static,
    int &size_so_far)
{
    // [fw] "in_func" is only used for funcs that are part of structs: The function body must not
    //      be included in a struct declaration. But this is true irrespective of whether the struct
    //      is declared within a function or not. We can probably do without this variable.

        ags::Symbol_t vname = targ->getnext(); // normally variable name, array name, or function name, but can be [ too
    bool isDynamicArray = false;

    // Check whether "[]" is behind the type. 
    // [fw] Is this meant to accept "struct foo { const [] bar; }" ??! 
    if (sym.get_type(vname) == SYM_OPENBRACKET && sym.get_type(targ->peeknext()) == SYM_CLOSEBRACKET)
    {
        isDynamicArray = true;
        targ->getnext(); // Eat "]"
        vname = targ->getnext();
    }

    const char *memberExt = sym.get_name(vname);
    memberExt = strstr(memberExt, "::");

    bool isFunction = sym.get_type(targ->peeknext()) == SYM_OPENPARENTHESIS;

    // If this is a member variable of the struct, then change the symbol to the fully qualified name.
    if (!isFunction && sym.get_type(vname) == SYM_VARTYPE && !sym_is_predef_typename(vname) && memberExt == NULL)
    {
        const char *new_name = get_member_full_name(stname, vname);
        vname = sym_find_or_add(sym, new_name);
    }

    // If, OTOH, this is a member that already has a type which is not VARTYPE or which is below sym.normalFloatSym,
    // then complain.
    if (sym.get_type(vname) != 0 &&
        (sym.get_type(vname) != SYM_VARTYPE || sym_is_predef_typename(vname)))
    {
        cc_error("'%s' is already defined", sym.get_friendly_name(vname).c_str());
        return -1;
    }

    // If this is an extension of another type, then we must make sure that names don't clash. 
    if (extendsWhat > 0)
    {
        int retval = cs_parser_struct_CheckMemberNotInInheritedStruct(vname, memberExt, extendsWhat);
        if (retval < 0) return retval;
    }

    if (isFunction)
    {
        if (type_is_import == ImNoImport)
        {
            cc_error("Function in a struct requires the import keyword");
            return -1;
        }
        if (type_is_writeprotected)
        {
            cc_error("'writeprotected' does not apply to functions");
            return -1;
        }

        int retval = process_function_decl_CheckForIllegalCombis(type_is_readonly, in_func, nested_level);
        if (retval < 0) return retval;
        {
            ags::Symbol_t throwaway_value = 0;
            int retval =
                process_function_declaration(targ, scrip, vname, curtype, type_is_pointer, isDynamicArray,
                    type_is_static, type_is_import, stname,
                    in_func, throwaway_value, nullptr);
        }
        if (retval < 0) return retval;

        if (type_is_protected)
            sym.entries[vname].flags |= SFLG_PROTECTED;

        if (in_func >= 0)
        {
            cc_error("Cannot declare struct member function inside a function body");
            return -1;
        }

    }
    else if (isDynamicArray)
    {
        // Someone tried to declare the function syntax for a dynamic array
        // But there was no function declaration
        cc_error("Expected '('");
        return -1;
    }
    else if ((type_is_import != ImNoImport) && (!type_is_property))
    {
        // member variable cannot be an import
        cc_error("Only struct member functions may be declared with 'import'");
        return -1;
    }
    else if ((type_is_static) && (!type_is_property))
    {
        cc_error("Static variables not supported");
        return -1;
    }
    else if ((curtype == stname) && (!type_is_pointer))
    {
        // cannot do  struct A { A a; }
        // since we don't know the size of A, recursiveness
        cc_error("Struct '%s' cannot be a member of itself", sym.get_friendly_name(curtype).c_str());
        return -1;
    }
    else
    {
        // member variable
        sym.entries[vname].stype = SYM_STRUCTMEMBER;
        sym.entries[vname].extends = stname;  // save which struct it belongs to
        sym.entries[vname].ssize = sym.entries[curtype].ssize;
        sym.entries[vname].soffs = size_so_far;
        sym.entries[vname].vartype = (short)curtype;
        if (type_is_readonly)
            sym.entries[vname].flags |= SFLG_READONLY;
        if (type_is_property)
            sym.entries[vname].flags |= SFLG_PROPERTY;
        if (type_is_pointer)
        {
            sym.entries[vname].flags |= SFLG_POINTER;
            sym.entries[vname].ssize = 4;
        }
        if (type_is_static)
            sym.entries[vname].flags |= SFLG_STATIC;
        if (type_is_protected)
            sym.entries[vname].flags |= SFLG_PROTECTED;
        else if (type_is_writeprotected)
            sym.entries[vname].flags |= SFLG_WRITEPROTECTED;

        if (type_is_property)
        {
            if (type_is_import == ImNoImport)
            {
                cc_error("Property must be import");
                return -1;
            }

            sym.entries[vname].flags |= SFLG_IMPORTED;

            const char *namePrefix = "";

            if (sym.get_type(targ->peeknext()) == SYM_OPENBRACKET)
            {
                // An indexed property!
                targ->getnext();  // skip the [
                if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
                {
                    cc_error("Cannot specify array size for property");
                    return -1;
                }

                sym.entries[vname].flags |= SFLG_ARRAY;
                sym.entries[vname].arrsize = 0;
                namePrefix = "i";
            }
            // the variable name will have been jibbled with
            // the struct name added to it -- strip it back off
            const char *memberPart = strstr(sym.get_name(vname), "::");
            if (memberPart == NULL)
            {
                cc_error("Internal error: Property has no struct name");
                return -1;
            }
            // seek to the actual member name
            memberPart += 2;

            // declare the imports for the Get and Setters
            char propFuncName[200];
            sprintf(propFuncName, "%s::get%s_%s", sym.get_name(stname), namePrefix, memberPart);

            int propGet = scrip->add_new_import(propFuncName);
            int propSet = 0;
            if (!type_is_readonly)
            {
                // setter only if it's not read-only
                sprintf(propFuncName, "%s::set%s_%s", sym.get_name(stname), namePrefix, memberPart);
                propSet = scrip->add_new_import(propFuncName);
            }
            sym.entries[vname].set_propfuncs(propGet, propSet);
        }
        else if (sym.get_type(targ->peeknext()) == SYM_OPENBRACKET)
        {
            // An array!
            targ->getnext();  // skip the [
            ags::Symbol_t nextt = targ->getnext();
            int array_size;

            if (sym.get_type(nextt) == SYM_CLOSEBRACKET)
            {
                if ((sym.entries[stname].flags & SFLG_MANAGED))
                {
                    cc_error("Member variable of managed struct cannot be dynamic array");
                    return -1;
                }
                sym.entries[stname].flags |= SFLG_HASDYNAMICARRAY;
                sym.entries[vname].flags |= SFLG_DYNAMICARRAY;
                array_size = 0;
                size_so_far += 4;
            }
            else
            {
                if (accept_literal_or_constant_value(nextt, array_size, false, "Array size must be constant value") < 0)
                {
                    return -1;
                }

                if (array_size < 1)
                {
                    cc_error("Array size cannot be less than 1");
                    return -1;
                }

                size_so_far += array_size * sym.entries[vname].ssize;

                if (sym.get_type(targ->getnext()) != SYM_CLOSEBRACKET)
                {
                    cc_error("Expected ']'");
                    return -1;
                }
            }
            sym.entries[vname].flags |= SFLG_ARRAY;
            sym.entries[vname].arrsize = array_size;
        }
        else
        {
            size_so_far += sym.entries[vname].ssize;
        }
    }

    // both functions and variables have this set
    sym.entries[vname].flags |= SFLG_STRUCTMEMBER;

    return 0;
}

int cs_parser_struct_MemberDefn(
    ccInternalList *targ,
    ccCompiledScript * scrip,
    ags::Symbol_t stname,
    int in_func,
    int nested_level,
    ags::Symbol_t extendsWhat,
    int &size_so_far)
{

    bool type_is_readonly = false;
    Importness type_is_import = ImNoImport;
    bool type_is_property = false;
    bool type_is_pointer = false;
    bool type_is_static = false;
    bool type_is_protected = false;
    bool type_is_writeprotected = false;

    ags::Symbol_t curtype; // the type of the current members being defined, given as a symbol

    // parse qualifiers of the member ("import" etc.), set booleans accordingly
    int retval = cs_paerser_struct_ParseMemberQualifiers(
        targ,
        curtype,
        type_is_readonly,
        type_is_import,
        type_is_property,
        type_is_static,
        type_is_protected,
        type_is_writeprotected);
    if (retval < 0) return retval;

    // curtype can now be: typename or typename *

    // A member defn. is a pointer if it is AUTOPOINTER or it has an explicit "*"
    if (sym.entries[curtype].flags & SFLG_AUTOPTR)
    {
        type_is_pointer = true;
    }
    else if (strcmp(sym.get_name(targ->peeknext()), "*") == 0)
    {
        type_is_pointer = true;
        targ->getnext();
    }

    // Certain types of members are not allowed in structs; check this
    retval = cs_parser_struct_IsMemberTypeIllegal(
        targ,
        stname,
        curtype,
        type_is_pointer,
        type_is_import);
    if (retval < 0) return retval;

    // [fw] "struct foo { int * a, b, c;}"
    //      This declares b to be an int pointer; but in C or C++, b would be an int
    //      Bug?

    // run through all variables declared on this member defn.
    while (true)
    {
        if (cs_parser_struct_MemberDefnVarOrFuncOrArray(
            targ,
            scrip,
            extendsWhat,        // Parent struct
            stname,             // struct
            in_func,
            nested_level,
            curtype,             // core type
            type_is_property,
            type_is_readonly,
            type_is_import,
            type_is_protected,
            type_is_writeprotected,
            type_is_pointer,
            type_is_static,
            size_so_far) < 0) return -1;

        if (sym.get_type(targ->peeknext()) == SYM_COMMA)
        {
            targ->getnext(); // gobble the comma
            continue;
        }
        break;
    }

    // line must end with semicolon
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }

    return 0;
}

// Handle a "struct" definition clause
int cs_parser_handle_struct(
    ccInternalList *targ,
    ccCompiledScript * scrip,
    bool struct_is_managed,
    bool struct_is_builtin,
    bool struct_is_autoptr,
    bool struct_is_stringstruct,
    ags::Symbol_t cursym,
    int &in_func,
    size_t &nested_level)
{
    // get token for name of struct
    ags::Symbol_t stname = targ->getnext();
    if ((sym.get_type(stname) != 0) &&
        (sym.get_type(stname) != SYM_UNDEFINEDSTRUCT))
    {
        cc_error("'%s' is already defined", sym.get_friendly_name(stname).c_str());
        return -1;
    }

    // Sums up the size of the struct
    int size_so_far = 0;

    // If the struct extends another struct, the token of the other struct's name
    ags::Symbol_t extendsWhat = 0;

    // Write the type of stname into the symbol table
    cs_parser_struct_SetTypeInSymboltable(
        sym.entries[stname],
        struct_is_managed,
        struct_is_builtin,
        struct_is_autoptr);

    // [fw] stringstruct (i.e., the word "internalstring") is a special case.
    // [fw] Evidently, "internalstring struct FOO {...}" sets the name of a string struct.
    // [fw] No further documentation.
    if (struct_is_stringstruct)
    {
        sym.stringStructSym = stname;
    }

    // forward-declaration of struct type
    if (sym.get_type(targ->peeknext()) == SYM_SEMICOLON)
    {
        targ->getnext(); // gobble the ";"
        sym.entries[stname].stype = SYM_UNDEFINEDSTRUCT;
        sym.entries[stname].ssize = 4;
        return 0;
    }

    // So we are in the "real" declaration.
    // optional "extends" clause
    if (sym.get_type(targ->peeknext()) == SYM_EXTENDS)
    {
        // [fw] At this point, it might be better to copy all the parent elements into this child
        // We need extendsWhat later on.
        cs_parser_struct_ExtendsClause(targ, stname, extendsWhat, size_so_far);
    }

    // mandatory "{"
    if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
    {
        cc_error("Expected '{'");
        return -1;
    }

    // Process every member of the struct in turn
    while (sym.get_type(targ->peeknext()) != SYM_CLOSEBRACE)
    {
        int retval = cs_parser_struct_MemberDefn(targ, scrip, stname, in_func, nested_level, extendsWhat, size_so_far);
        if (retval < 0) return retval;
    }

    // align struct on 4-byte boundary in keeping with compiler
    if ((size_so_far % 4) != 0) size_so_far += 4 - (size_so_far % 4);
    sym.entries[stname].ssize = size_so_far;

    // gobble the "}"
    targ->getnext();

    // mandatory ";" after struct defn.
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}


// We've accepted something like "enum foo { bar"; '=' follows
int cs_parser_enum_accept_assigned_value(ccInternalList * targ, int &currentValue)
{
    targ->getnext(); // eat "="

    // Get the value of the item
    ags::Symbol_t item_value = targ->getnext(); // may be '-', too
    bool is_neg = false;
    if (item_value == sym.find("-"))
    {
        is_neg = true;
        item_value = targ->getnext();
    }

    if (accept_literal_or_constant_value(item_value, currentValue, is_neg, "Expected integer or integer constant after '='") < 0)
    {
        return -1;
    }

    return 0;
}

void cs_parser_enum_item_2_symtable(int enum_name, int item_name, int currentValue)
{
    sym.entries[item_name].stype = SYM_CONSTANT;
    sym.entries[item_name].ssize = 4;
    sym.entries[item_name].arrsize = 1;
    sym.entries[item_name].vartype = enum_name;
    sym.entries[item_name].sscope = 0;
    sym.entries[item_name].flags = SFLG_READONLY;
    // soffs is unused for a constant, so in a gratuitous
    // hack we use it to store the enum's value
    sym.entries[item_name].soffs = currentValue;
}

int cs_parser_enum_name_2_symtable(int enumName)
{
    if (sym.get_type(enumName) != 0)
    {
        cc_error("'%s' is already defined", sym.get_friendly_name(enumName).c_str());
        return -1;
    }
    sym.entries[enumName].stype = SYM_VARTYPE;
    sym.entries[enumName].ssize = 4; // standard int size
    sym.entries[enumName].vartype = sym.normalIntSym;

    return 0;
}

// enum eEnumName { value1, value2 }
int cs_parser_handle_enum_inner(ccInternalList *targ, int in_func)
{
    if (in_func >= 0)
    {
        cc_error("Enum declaration not allowed within a function body");
        return -1;
    }

    // Get name of the enum, enter it into the symbol table
    int enum_name = targ->getnext();
    int retval = cs_parser_enum_name_2_symtable(enum_name);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
    {
        cc_error("Expected '{'");
        return -1;
    }


    int currentValue = 0;

    while (true)
    {
        if (ReachedEOF(targ))
        {
            cc_error("Unexpected end of input");
            return -1;
        }

        ags::Symbol_t item_name = targ->getnext();
        if (sym.get_type(item_name) == SYM_CLOSEBRACE) break; // item list empty or ends with trailing ','

        if (sym.get_type(item_name) != 0)
        {
            cc_error("Expected '}' or an unused identifier, found '%s' instead", sym.get_name_string(item_name).c_str());
            return -1;
        }

        // increment the value of the enum entry
        currentValue++;

        ags::Symbol_t tnext = sym.get_type(targ->peeknext());
        if (tnext != SYM_ASSIGN && tnext != SYM_COMMA && tnext != SYM_CLOSEBRACE)
        {
            cc_error("Expected '=' or ',' or '}'");
            return -1;
        }

        if (tnext == SYM_ASSIGN)
        {
            // the value of this entry is specified explicitly
            int retval = cs_parser_enum_accept_assigned_value(targ, currentValue);
            if (retval < 0) return retval;
        }

        // Enter this enum item as a constant int into the sym table
        cs_parser_enum_item_2_symtable(enum_name, item_name, currentValue);

        ags::Symbol_t comma_or_brace = targ->getnext();
        if (sym.get_type(comma_or_brace) == SYM_CLOSEBRACE) break;
        if (sym.get_type(comma_or_brace) == SYM_COMMA) continue;

        cc_error("Expected ',' or '}'");
        return -1;
    }
    return 0;
}

// enum eEnumName { value1, value2 };
int cs_parser_handle_enum(ccInternalList *targ, int in_func)
{
    int retval = cs_parser_handle_enum_inner(targ, in_func);
    if (retval < 0) return retval;

    // Force a semicolon after the declaration
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}


int cs_parse_handle_import(ccInternalList *targ, int in_func, ags::Symbol_t cursym, Importness &next_is_import)
{
    if (in_func >= 0)
    {
        cc_error("'import' not allowed inside function body");
        return -1;
    }

    next_is_import = ImImportType1;
    if (strcmp(sym.get_name(cursym), "_tryimport") == 0)
    {
        next_is_import = ImImportType2;
    }


    if ((sym.get_type(targ->peeknext()) != SYM_VARTYPE) &&
        (sym.get_type(targ->peeknext()) != SYM_READONLY))
    {
        cc_error("Expected a type or 'readonly' after 'import', not '%s'", sym.get_friendly_name(targ->peeknext()).c_str());
        return -1;
    }
    return 0;
}

int cs_parser_handle_static(ccInternalList *targ, int in_func, bool &next_is_static)
{
    if (in_func >= 0)
    {
        cc_error("'static' not allowed inside function body");
        return -1;
    }
    next_is_static = 1;
    if ((sym.get_type(targ->peeknext()) != SYM_VARTYPE) &&
        (sym.get_type(targ->peeknext()) != SYM_READONLY))
    {
        cc_error("Expected a type or 'readonly' after 'static'");
        return -1;
    }
    return 0;
}

int cs_parser_handle_protected(ccInternalList *targ, int in_func, bool &next_is_protected)
{
    if (in_func >= 0)
    {
        cc_error("'protected' not allowed inside a function body");
        return -1;
    }
    next_is_protected = 1;
    if ((sym.get_type(targ->peeknext()) != SYM_VARTYPE) &&
        (sym.get_type(targ->peeknext()) != SYM_STATIC) &&
        (sym.get_type(targ->peeknext()) != SYM_READONLY))
    {
        cc_error("Expected a type, 'static' or 'readonly' after 'protected'");
        return -1;
    }
    return 0;
}

int cs_parser_handle_export(ccInternalList *targ, ccCompiledScript * scrip, ags::Symbol_t &cursym)
{
    // export specified symbol
    cursym = targ->getnext();
    while (sym.get_type(cursym) != SYM_SEMICOLON)
    {
        int nextype = sym.get_type(cursym);
        if (nextype == 0)
        {
            cc_error("Can only export global variables and functions, not '%s'", sym.get_friendly_name(cursym).c_str());
            return -1;
        }
        if ((nextype != SYM_GLOBALVAR) && (nextype != SYM_FUNCTION))
        {
            cc_error("Invalid export symbol '%s'", sym.get_friendly_name(cursym).c_str());
            return -1;
        }
        if (sym.entries[cursym].flags & SFLG_IMPORTED)
        {
            cc_error("Cannot export an import");
            return -1;
        }
        if (sym.entries[cursym].flags & SFLG_ISSTRING)
        {
            cc_error("Cannot export string; use char[200] instead");
            return -1;
        }
        // if all functions are being exported anyway, don't bother doing
        // it now
        if ((ccGetOption(SCOPT_EXPORTALL) != 0) && (nextype == SYM_FUNCTION));
        else if (scrip->add_new_export(sym.get_name(cursym),
            (nextype == SYM_GLOBALVAR) ? EXPORT_DATA : EXPORT_FUNCTION,
            sym.entries[cursym].soffs, sym.entries[cursym].sscope) == -1)
        {
            return -1;
        }
        if (ReachedEOF(targ))
        {
            cc_error("Unexpected end of input");
            return -1;
        }
        cursym = targ->getnext();
        if (sym.get_type(cursym) == SYM_SEMICOLON) break;
        if (sym.get_type(cursym) != SYM_COMMA)
        {
            cc_error("Expected ',' instead of '%s'", sym.get_friendly_name(cursym).c_str());
            return -1;
        }
        cursym = targ->getnext();
    }

    return 0;
}

int cs_parser_handle_vartype_GetVarName(ccInternalList * targ, ags::Symbol_t & varname, ags::Symbol_t & struct_of_member_fct)
{
    struct_of_member_fct = 0;

    varname = targ->getnext();

    if (sym.get_type(targ->peeknext()) != SYM_MEMBERACCESS) return 0; // done
    // We are accepting "struct::member"; so varname isn't the var name yet: it's the struct name.

    struct_of_member_fct = varname;
    targ->getnext(); // gobble "::"
    ags::Symbol_t member_of_member_function = targ->getnext();

    // change varname to be the full function name
    const char *full_name_str = get_member_full_name(struct_of_member_fct, member_of_member_function);
    varname = sym.find(full_name_str);
    if (varname < 0)
    {
        cc_error("'%s' does not contain a function '%s'",
            sym.get_friendly_name(struct_of_member_fct).c_str(),
            sym.get_friendly_name(member_of_member_function).c_str());
        return -1;
    }

    return 0;
}

int cs_parser_handle_vartype_CheckForIllegalContext(ags::NestingStack *nesting_stack)
{
    if (nesting_stack->IsUnbraced())
    {
        cc_error("A variable or function declaration cannot be the sole body of an 'if', 'else' or loop clause");
        return -1;
    }
    if (nesting_stack->Type() == ags::NestingStack::NTSwitch)
    {
        cc_error("This variable declaration may be skipped by case label. Use braces to limit its scope or move it outside the switch statement block");
        return -1;
    }
    return 0;
}

int cs_parser_handle_vartype_GetPointerStatus(ccInternalList * targ, int type_of_defn, bool &isPointer)
{
    isPointer = false;
    if (targ->peeknext() == sym.find("*"))
    {
        // only allow pointers to structs
        if ((sym.entries[type_of_defn].flags & SFLG_STRUCTTYPE) == 0)
        {
            cc_error("Cannot create pointer to basic type");
            return -1;
        }
        if (sym.entries[type_of_defn].flags & SFLG_AUTOPTR)
        {
            cc_error("Invalid use of '*'");
            return -1;
        }
        isPointer = true;
        targ->getnext();
    }

    if (sym.entries[type_of_defn].flags & SFLG_AUTOPTR)
        isPointer = true;

    return 0;
}

// there was a forward declaration -- check that the real declaration matches it
int cs_parser_handle_vartype_CheckThatForwardDeclMatches(
    Globalness isglobal, // isglobal is int 0 = local, 1 = global, 2 = global and import
    SymbolTableEntry &oldDefinition,
    ags::Symbol_t cursym)
{

    ccError = 0;
    if (isglobal == GlLocal) // 0 == local
    {
        cc_error("Local variable cannot have the same name as an import");
        return -1;
    }

    if (oldDefinition.stype != sym.entries[cursym].stype)
    {
        cc_error("Type of identifier differs from original declaration");
        return -1;
    }

    if (oldDefinition.flags != (sym.entries[cursym].flags & ~SFLG_IMPORTED))
    {
        cc_error("Attributes of identifier do not match prototype");
        return -1;
    }

    if (oldDefinition.ssize != sym.entries[cursym].ssize)
    {
        cc_error("Size of identifier does not match prototype");
        return -1;
    }

    if ((sym.entries[cursym].flags & SFLG_ARRAY) && (oldDefinition.arrsize != sym.entries[cursym].arrsize))
    {
        cc_error("Array size '%d' of identifier does not match prototype which is '%d'", sym.entries[cursym].arrsize, oldDefinition.arrsize);
        return -1;
    }

    if (oldDefinition.stype != SYM_FUNCTION) return 0;

    // Following checks pertain to functions
    if (oldDefinition.sscope != sym.entries[cursym].sscope)
    {
        cc_error("Function declaration has wrong number of arguments to prototype");
        return -1;
    }

    // this is <= because the return type is the first one
    for (int ii = 0; ii <= sym.entries[cursym].get_num_args(); ii++)
    {
        if (oldDefinition.funcparamtypes.at(ii) != sym.entries[cursym].funcparamtypes.at(ii))
            cc_error("Parameter type does not match prototype");

        // copy the default values from the function prototype
        sym.entries[cursym].funcParamDefaultValues.push_back(oldDefinition.funcParamDefaultValues.at(ii));
        sym.entries[cursym].funcParamHasDefaultValues.push_back(oldDefinition.funcParamHasDefaultValues.at(ii));
    }

    return 0;
}

int cs_parser_handle_vartype_CheckIllegalCombis(bool is_static, bool is_member_definition, bool is_function, bool is_protected, bool loopCheckOff, bool is_import)
{
    if (is_static && (!is_function || !is_member_definition))
    {
        cc_error("'static' only applies to member functions");
        return -1;
    }

    if (!is_function && is_protected)
    {
        cc_error("'protected' not valid in this context");
        return -1;
    }

    if (!is_function && loopCheckOff)
    {
        cc_error("'noloopcheck' not valid in this context");
        return -1;
    }

    if (!is_function && is_static)
    {
        cc_error("Invalid use of 'static'");
        return -1;
    }

    if (is_function && loopCheckOff && is_import)
    {
        cc_error("'noloopcheck' cannot be applied to imported functions");
        return -1;
    }
    return 0;
}

int cs_parser_handle_vartype_FuncDef(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t &func_name, bool is_readonly, int & idx_of_current_func, int type_of_defn, bool isPointer, bool isDynamicArray, bool is_static, Importness is_import, ags::Symbol_t & struct_of_current_func, SymbolTableEntry &oldDefinition, bool is_member_function_definition, bool is_protected, ags::Symbol_t & name_of_current_func, bool loopCheckOff)
{
    // Don't allow functions within functions or readonly functions
    int retval = process_function_declaration(
        targ, scrip, func_name, type_of_defn, isPointer, isDynamicArray,
        is_static, is_import, struct_of_current_func,
        idx_of_current_func, struct_of_current_func, &oldDefinition);
    if (retval < 0) return retval;

    // restore flags, since remove_any_imports() zeros them out
    if (is_member_function_definition) sym.entries[func_name].flags |= SFLG_STRUCTMEMBER;
    if (is_protected)                  sym.entries[func_name].flags |= SFLG_PROTECTED;

    // If we've started a function, remember what it is.
    if (idx_of_current_func >= 0) name_of_current_func = func_name;

    return 0;
}

// Call out an error if this identifier is in use
int cs_parser_handle_vartype_CheckWhetherInUse(ags::Symbol_t var_or_func_name, bool is_function, bool is_member_definition)
{
    if (sym.get_type(var_or_func_name) == 0) return 0; // not in use

    if (is_function && !is_member_definition) // if member defn, it has been _declared_, not defined
    {
        cc_error("Function '%s' is already defined", sym.get_friendly_name(var_or_func_name).c_str());
        return -1;
    }

    if (sym.get_type(var_or_func_name) == SYM_VARTYPE || sym_is_predef_typename(var_or_func_name))
    {
        cc_error("'%s' is already in use as a type name", sym.get_friendly_name(var_or_func_name).c_str());
        return -1;
    }
    return 0;
}


int cs_parser_handle_vartype_VarDef(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t &var_name, Globalness is_global, int nested_level, bool is_readonly, int type_of_defn, int next_type, bool isPointer, bool &another_var_follows)
{

    if (is_global == GlLocal)  // is_global is int 0 = local
    {
        sym.entries[var_name].sscope = nested_level;
    }
    if (is_readonly)
    {
        sym.entries[var_name].flags |= SFLG_READONLY;
    }

    // parse the definition
    int retval = parse_var_decl(targ, scrip, var_name, type_of_defn, next_type, is_global, isPointer, another_var_follows);
    if (retval < 0) return retval;
    return 0;
}


// We accepted a variable type such as "int", so what follows is a function or variable definition
int cs_parser_handle_vartype(
    ccInternalList *targ,
    ccCompiledScript *scrip,
    ags::Symbol_t type_of_defn,           // e.g., "int"
    ags::NestingStack *nesting_stack,
    Importness is_import,             // can be 0 or 1 or 2 
    bool is_readonly,
    bool is_static,
    bool is_protected,
    int &idx_of_current_func,
    ags::Symbol_t &name_of_current_func,
    ags::Symbol_t &struct_of_current_func, // 0 if _not_ a member function
    bool &loopCheckOff)
{
    if (ReachedEOF(targ))
    {
        cc_error("Unexpected end of input");
        return -1;
    }

    // Don't define variable or function where illegal in context.
    int retval = cs_parser_handle_vartype_CheckForIllegalContext(nesting_stack);
    if (retval < 0) return retval;

    // Calculate whether this is a pointer definition, gobbling "*" if present
    bool isPointer = false;
    retval = cs_parser_handle_vartype_GetPointerStatus(targ, type_of_defn, isPointer);
    if (retval < 0) return retval;

    // Look for "[]"; if present, gobble it and call this a dynamic array.
    // "int [] func(...)"
    int dynArrayStatus = param_list_param_DynArrayMarker(targ, type_of_defn, isPointer);
    if (dynArrayStatus < 0) return -1;
    bool isDynamicArray = (dynArrayStatus > 0);

    // Look for "noloopcheck"; if present, gobble it and set the indicator
    // "TYPE noloopcheck foo(...)"
    loopCheckOff = false;
    if (sym.get_type(targ->peeknext()) == SYM_LOOPCHECKOFF)
    {
        targ->getnext();
        loopCheckOff = true;
    }

    Globalness is_global = GlLocal;
    if (idx_of_current_func < 0)
    {
        is_global = (is_import == ImNoImport) ? GlGlobalNoImport : GlGlobalImport;
    }

    bool another_ident_follows = false; // will become true when we gobble a "," after a var defn
    // We've accepted a type expression and are now reading vars or one func that should have this type.
    do
    {
        if (ReachedEOF(targ))
        {
            cc_error("Unexpected end of input");
            return -1;
        }

        // Get the variable or function name.
        ags::Symbol_t var_or_func_name = -1;
        retval = cs_parser_handle_vartype_GetVarName(targ, var_or_func_name, struct_of_current_func);
        if (retval < 0) return retval;

        // Check whether var or func is being defined
        int next_type = sym.get_type(targ->peeknext());
        bool is_function = (sym.get_type(targ->peeknext()) == SYM_OPENPARENTHESIS);
        bool is_member_definition = (struct_of_current_func >= 0); // [fw] "> 0" would be right

        // certains modifiers, such as "static" only go with certain kinds of definitions.
        retval = cs_parser_handle_vartype_CheckIllegalCombis(is_static, is_member_definition, is_function, is_protected, loopCheckOff, (is_import != ImNoImport));
        if (retval < 0) return retval;

        // Check whether the var or func is already defined or in use otherwise
        retval = cs_parser_handle_vartype_CheckWhetherInUse(var_or_func_name, is_function, is_member_definition);
        if (retval < 0) return retval;

        // If there has been a forward declaration, its type will go here
        SymbolTableEntry oldDefinition;
        oldDefinition.stype = 0;

        if (is_import != ImImportType1) // [fw] that is, no import or import of type 2
        {

            // Copy so that the forward declaration can be compared afterwards to the real one 
            int retval = scrip->copy_import_symbol_table_entry(var_or_func_name, &oldDefinition);
            if (retval < 0) return retval;
            oldDefinition.flags &= ~SFLG_IMPORTED; // Strip import flag, since the real defn won't be exported

            // Check whether the import has been referenced or whether imports may not be overridden;
            // if so, complain; 
            // remove the import flags
            retval = scrip->just_remove_any_import(var_or_func_name);
            if (retval < 0) return retval;
        }

        if (is_function) // function defn
        {
            int retval = process_function_decl_CheckForIllegalCombis(is_readonly, idx_of_current_func, nesting_stack->Depth());
            if (retval < 0) return retval;

            retval = cs_parser_handle_vartype_FuncDef(targ, scrip, var_or_func_name, is_readonly, idx_of_current_func, type_of_defn, isPointer, isDynamicArray, is_static, is_import, struct_of_current_func, oldDefinition, is_member_definition, is_protected, name_of_current_func, loopCheckOff);
            if (retval < 0) return retval;
            another_ident_follows = false; // Can't join another func or var with ','
        }
        else // variable defn
        {
            int retval = cs_parser_handle_vartype_VarDef(targ, scrip, var_or_func_name, is_global, nesting_stack->Depth() - 1, is_readonly, type_of_defn, next_type, isPointer, another_ident_follows);
            if (retval < 0) return retval;
        }

        // If we've got a forward declaration, check whether it matches the actual one.
        if (oldDefinition.stype != 0)
        {
            int retval = cs_parser_handle_vartype_CheckThatForwardDeclMatches(is_global, oldDefinition, var_or_func_name);
            if (retval < 0) return retval;
        }

    } while (another_ident_follows);

    return 0;
}

int error_undef_token_inside_func(ags::Symbol_t cursym)
{
    char ascii_explanation[20] = "";
    const char *symname = sym.get_name(cursym);
    if ((symname[0] <= 32) || (symname[0] >= 128))
    {
        sprintf(ascii_explanation, "(ASCII index %02X)", symname[0]);
    }

    cc_error("Undefined token '%s' %s", symname, ascii_explanation);
    return -1;
}


int evaluate_funccall(ccInternalList *targ, ccCompiledScript * scrip, int offset_of_funcname, int targPosWas)
{
    // calling a function
    if (offset_of_funcname > 0)
    {
        // member function -- wind back to process whole expression
        targ->pos = targPosWas;
    }

    targ->pos--;

    int retval = evaluate_expression(targ, scrip, false);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }

    return 0;
}

int compile_funcbodycode_EndOfDoIfElse(ccInternalList * targ, ccCompiledScript * scrip, ags::NestingStack *nesting_stack)
{
    // Unravel else ... else ... chains
    while (nesting_stack->IsUnbraced())
    {
        if (nesting_stack->Type() == ags::NestingStack::NTUnbracedDo)
        {
            int retval = deal_with_end_of_do(targ, scrip, nesting_stack);
            if (retval < 0) return retval;
            continue;
        }

        bool else_after_then;
        int retval = deal_with_end_of_ifelse(targ, scrip, nesting_stack, else_after_then);
        if (retval < 0) return retval;
        // If an else follows a then clause, it has been changed into an
        // else clause that has just started so this clause has NOT ended yet.
        // So the else ... else chain is broken at this point, so we break out of the loop.
        if (else_after_then) break;
    }
    return 0;
}

int evaluate_return(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t inFuncSym)
{
    int functionReturnType = sym.entries[inFuncSym].funcparamtypes[0];

    if (sym.get_type(targ->peeknext()) != SYM_SEMICOLON)
    {
        if (functionReturnType == sym.normalVoidSym)
        {
            cc_error("Cannot return value from void function");
            return -1;
        }

        // parse what is being returned
        int retval = evaluate_expression(targ, scrip, false);
        if (retval < 0) return retval;

        // If we need a string object ptr but AX contains a normal string, convert AX
        ConvertAXIntoStringObject(scrip, functionReturnType);

        // check return type is correct
        retval = check_type_mismatch(scrip->ax_val_type, functionReturnType, true);
        if (retval < 0) return retval;

        if ((is_string(scrip->ax_val_type)) &&
            (scrip->ax_val_scope == SYM_LOCALVAR))
        {
            cc_error("Cannot return local string from function");
            return -1;
        }
    }
    else if ((functionReturnType != sym.normalIntSym) && (functionReturnType != sym.normalVoidSym))
    {
        cc_error("Must return a '%s' value from function", sym.get_friendly_name(functionReturnType).c_str());
        return -1;
    }
    else
    {
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0);
    }

    int cursym = sym.get_type(targ->getnext());
    if (cursym != SYM_SEMICOLON)
    {
        cc_error("Expected ';' instead of '%s'", sym.get_name(cursym));
        return -1;
    }

    // count total space taken by all local variables
    free_pointers_of_locals(scrip, 1);

    int totalsub = stacksize_of_locals(0);
    if (totalsub > 0) scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    scrip->write_cmd(SCMD_RET);
    // We don't alter cur_sp since there can be code after the RETURN

    return 0;
}


// Evaluate the head of an "if" clause, e.g. "if (i < 0)".
int evaluate_if(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t cursym, ags::NestingStack *nesting_stack)
{
    // Get expression, must be in parentheses
    if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }

    int retval = evaluate_expression(targ, scrip, true);
    if (retval < 0) return retval;

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    scrip->write_cmd1(SCMD_JZ, 0);
    ags::CodeLoc_t jump_dest_loc = scrip->codesize - 1;

    // Assume unbraced as a default
    retval = nesting_stack->Push(
        ags::NestingStack::NTUnbracedThen, // Type
        0, // Start
        jump_dest_loc); // Info
    
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
    {
        targ->getnext();
        nesting_stack->SetType(ags::NestingStack::NTBracedThen); // change to braced
    }

    return 0;
}


// Evaluate the head of a "while" clause, e.g. "while (i < 0)" 
int evaluate_while(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t cursym, ags::NestingStack *nesting_stack)
{
    // Get expression, must be in parentheses
    if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }

    // point to the start of the code that evaluates the condition
    ags::CodeLoc_t condition_eval_loc = scrip->codesize;

    int retval = evaluate_expression(targ, scrip, true);
    if (retval < 0) return retval;

    // Now the code that has just been generated has put the result of the check into AX
    // Generate code for "if (AX == 0) jumpto X", where X will be determined later on.
    scrip->write_cmd1(SCMD_JZ, 0);
    ags::CodeLoc_t jump_dest_loc = scrip->codesize - 1;

    // Assume unbraced as a default
    retval = nesting_stack->Push(
        ags::NestingStack::NTUnbracedThen, // Type
        condition_eval_loc, // Start
        jump_dest_loc); // Info
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
    {
        targ->getnext();
        nesting_stack->SetType(ags::NestingStack::NTBracedElse); // change to braced
    }
    return 0;
}

int evaluate_do(ccInternalList * targ, ccCompiledScript * scrip, ags::NestingStack *nesting_stack)
{
    // We need a jump at a known location for the break command to work:
    scrip->write_cmd1(SCMD_JMP, 2); // Jump past the next jump :D
    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the end of the loop
    // This points to the address we have to patch with a jump past the end of the loop
    ags::CodeLoc_t jump_dest_loc = scrip->codesize - 1;
    
    // Assume an unbraced DO as a default
    int retval = nesting_stack->Push(
        ags::NestingStack::NTUnbracedDo, // Type
        scrip->codesize, // Start
        jump_dest_loc);  // Info
    if (retval < 0) return retval;
    
    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
    {
        targ->getnext();
        // Change to braced DO
        nesting_stack->SetType(ags::NestingStack::NTBracedDo);
    }     

    return 0;
}

int evaluate_for_InitClause(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t & cursym, const ags::SymbolScript_t & vnlist, size_t & vnlist_len, int & offset_of_funcname, char is_protected, char is_static, size_t nested_level, char is_readonly)
{
    // Check for empty init clause
    if (sym.get_type(cursym) == SYM_SEMICOLON) return 0;
    
    int retval = read_var_or_funccall(targ, cursym, vnlist, vnlist_len, offset_of_funcname);
    if (retval < 0) return retval;
    if (sym.get_type(cursym) == SYM_VARTYPE)
    {
        int vtwas = cursym;

        bool isPointer = false;

        if (strcmp(sym.get_name(targ->peeknext()), "*") == 0)
        {
            // only allow pointers to structs
            if ((sym.entries[vtwas].flags & SFLG_STRUCTTYPE) == 0)
            {
                cc_error("Cannot create pointer to basic type");
                return -1;
            }
            if (sym.entries[vtwas].flags & SFLG_AUTOPTR)
            {
                cc_error("Invalid use of '*'");
                return -1;
            }
            isPointer = true;
            targ->getnext();
        }

        if (sym.entries[vtwas].flags & SFLG_AUTOPTR)
            isPointer = true;

        if (sym.get_type(targ->peeknext()) == SYM_LOOPCHECKOFF)
        {
            cc_error("'noloopcheck' is not applicable in this context");
            return -1;
        }

        // FIXME: This duplicates common variable declaration parsing at/around the "startvarbit" label
        bool another_var_follows = false;
        do
        {
            cursym = targ->getnext();
            if (cursym == SCODE_META)
            {
                // eg. "int" was the last word in the file
                currentline = targ->lineAtEnd;
                cc_error("Unexpected end of file");
                return -1;
            }

            int next_type = sym.get_type(targ->peeknext());
            if (next_type == SYM_MEMBERACCESS || next_type == SYM_OPENPARENTHESIS)
            {
                cc_error("Function declaration not allowed in for loop initialiser");
                return -1;
            }
            else if (sym.get_type(cursym) != 0)
            {
                cc_error("Variable '%s' is already defined", sym.get_name(cursym));
                return -1;
            }
            else if (is_protected)
            {
                cc_error("'protected' not valid in this context");
                return -1;
            }
            else if (is_static)
            {
                cc_error("Invalid use of 'static'");
                return -1;
            }
            else
            {
                // variable declaration
                sym.entries[cursym].sscope = static_cast<short>(nested_level);
                if (is_readonly) sym.entries[cursym].flags |= SFLG_READONLY;

                // parse the declaration
                int varsize = sym.entries[vtwas].ssize;
                int retval = parse_var_decl(targ, scrip, cursym, vtwas, next_type, GlLocal, isPointer, another_var_follows);
                if (retval < 0) return retval;
            }
        } while (another_var_follows);
    }
    else
    {
        retval = evaluate_assignment(targ, scrip, cursym, SYM_SEMICOLON, vnlist, vnlist_len);
        if (retval < 0) return retval;
    }
    return 0;
}

int evaluate_for_WhileClause(ccInternalList * targ, ccCompiledScript * scrip)
{
    // Check for empty while clause
    if (sym.get_type(targ->peeknext()) == SYM_SEMICOLON)
    {
        // Not having a while clause is tantamount to the while condition "true".
        // So let's write "true" to the AX register.
        scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 1);
        targ->getnext();
        return 0;
    }

    int retval = evaluate_expression(targ, scrip, false);
    if (retval < 0) return retval;

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }
    return 0;
}

int evaluate_for_IterateClause(ccInternalList * targ, ccCompiledScript * scrip, const ags::SymbolScript_t & vnlist, size_t & vnlist_len, int & offset_of_funcname, ags::Symbol_t & cursym)
{
    // Check for empty interate clause
    if (sym.get_type(cursym) == SYM_CLOSEPARENTHESIS) return 0;

    int retval = read_var_or_funccall(targ, cursym, vnlist, vnlist_len, offset_of_funcname);
    if (retval < 0) return retval;

    retval = evaluate_assignment(targ, scrip, cursym, SYM_CLOSEPARENTHESIS, vnlist, vnlist_len);
    if (retval < 0) return retval;

    return 0;
}


int evaluate_for(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t &cursym, ags::NestingStack *nesting_stack, const ags::SymbolScript_t &vnlist, size_t &vnlist_len, int &offset_of_funcname, char is_protected, char is_static, char is_readonly)
{
    // "for (I; E; C) { ...}" is equivalent to "{ I; while (E) { ...; C} }"
    // We implement this with TWO levels of the nesting stack.
    // The outer level contains "I"
    // The inner level contains "while (E) { ...; C}"

    // Outer level
    int retval = nesting_stack->Push(ags::NestingStack::NTFor);
    if (retval < 0) return retval;
    
    // '(' must follow
    cursym = targ->getnext();
    if (sym.get_type(cursym) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }
    // Even if clauses are empty, we still need their ';'
    cursym = targ->getnext();
    if (sym.get_type(cursym) == SYM_CLOSEPARENTHESIS)
    {
        cc_error("Empty parentheses \"()\" aren't allowed after \"for\" (write \"for(;;)\" instead");
        return -1;
    }
    
    // Generate the initialization clause (I)
    retval = evaluate_for_InitClause(targ, scrip, cursym, vnlist, vnlist_len, offset_of_funcname, is_protected, is_static, nesting_stack->Depth() - 1, is_readonly);
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == SYM_CLOSEPARENTHESIS)
    {
        cc_error("Missing ';' inside for loop declaration");
        return -1;
    }

    // Remember where the code of the while condition starts.
    ags::CodeLoc_t while_cond_loc = scrip->codesize;

    retval = evaluate_for_WhileClause(targ, scrip);
    if (retval < 0) return retval;
    
    // Remember where the code of the iterate clause starts.
    ags::CodeLoc_t iterate_clause_loc = scrip->codesize;
    size_t pre_fixup_count = scrip->numfixups;
    cursym = targ->getnext();

    retval = evaluate_for_IterateClause(targ, scrip, vnlist, vnlist_len, offset_of_funcname, cursym);
    if (retval < 0) return retval;

    // Inner nesting level - assume unbraced as a default
    retval = nesting_stack->Push(
        ags::NestingStack::NTUnbracedElse, // Type
        while_cond_loc, // Start
        0); // Info
    if (retval < 0) return retval;

    if (sym.get_type(targ->peeknext()) == SYM_OPENBRACE)
    {
        targ->getnext();
        // Set type "braced" instead of "unbraced"
        nesting_stack->SetType(ags::NestingStack::NTBracedElse);
    }

    // We've just generated code for getting to the next loop iteration.
    // But we don't need that code right here; we need it at the bottom of the loop.
    // So rip it out of the bytecode base and save it into our nesting stack.
    nesting_stack->YankChunk(scrip, iterate_clause_loc, pre_fixup_count);
           
    // Code for "If the expression we just evaluated is false, jump over the loop body."
    // the 0 will be fixed to a proper offset later
    scrip->write_cmd1(SCMD_JZ, 0);
    nesting_stack->SetJumpOutLoc(scrip->codesize - 1); // the address to fix

    return 0;
}


int evaluate_switch(ccInternalList * targ, ccCompiledScript * scrip, ags::NestingStack *nesting_stack)
{
    // Get the switch expression, must be in parentheses
    if (sym.get_type(targ->peeknext()) != SYM_OPENPARENTHESIS)
    {
        cc_error("Expected '('");
        return -1;
    }

    int retval = evaluate_expression(targ, scrip, true); 
    if (retval < 0) return retval; 

    // Remember the type of this expression to enforce it later
    int switch_expr_type = scrip->ax_val_type;

    // Copy the result to the BX register, ready for case statements
    scrip->write_cmd2(SCMD_REGTOREG, SREG_AX, SREG_BX);
    scrip->flush_line_numbers();

    // Remember the start of the lookup table
    ags::CodeLoc_t lookup_table_start = scrip->codesize;

    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to the lookup table
    scrip->write_cmd1(SCMD_JMP, 0); // Placeholder for a jump to beyond the switch statement (for break)
    
    // There's no such thing as an unbraced SWITCH, so '{' must follow
    if (sym.get_type(targ->getnext()) != SYM_OPENBRACE)
    {
        cc_error("Expected '{'");
        return -1;
    }

    retval = nesting_stack->Push(
        ags::NestingStack::NTSwitch, // Type
        lookup_table_start, // Start
        switch_expr_type); // Info
    if (retval < 0) return retval;
    nesting_stack->SetDefaultLabelLoc(-1);

    // Check that "default" or "case" follows
    if (ReachedEOF(targ))
    {
        currentline = targ->lineAtEnd;
        cc_error("Unexpected end of input");
        return -1;
    }
    if (sym.get_type(targ->peeknext()) != SYM_CASE && sym.get_type(targ->peeknext()) != SYM_DEFAULT && sym.get_type(targ->peeknext()) != SYM_CLOSEBRACE)
    {
        cc_error("Invalid keyword '%s' in switch statement block", sym.get_name(targ->peeknext()));
        return -1;
    }
    return 0;
}

int evaluate_casedefault(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t cursym, ags::NestingStack *nesting_stack)
{
    if (nesting_stack->Type() != ags::NestingStack::NTSwitch)
    {
        cc_error("Case label not valid outside switch statement block");
        return -1;
    }

    if (sym.get_type(cursym) == SYM_DEFAULT)
    {
        if (nesting_stack->DefaultLabelLoc() != -1)
        {
            cc_error("This switch statement block already has a \"default:\" label");
            return -1;
        }
        nesting_stack->SetDefaultLabelLoc(scrip->codesize);
    }
    else // "case"
    {
        ags::CodeLoc_t start_of_code_loc = scrip->codesize;
        int numfixups_at_start_of_code = scrip->numfixups;
        // Push the switch variable onto the stack
        scrip->push_reg(SREG_BX);

        // get an expression
        int retval = evaluate_expression(targ, scrip, false);
        if (retval < 0) return retval;  // case n: label expression, result is in AX

        // check that the types of the "case" expression and the "switch" expression match
        retval = check_type_mismatch(scrip->ax_val_type, nesting_stack->SwitchExprType(), false);
        if (retval < 0) return retval;

        // Pop the switch variable, ready for comparison
        scrip->pop_reg(SREG_BX);

        // get the right equality operator for the type
        int eq_op = SCMD_ISEQUAL;
        retval = get_operator_valid_for_type(scrip->ax_val_type, nesting_stack->SwitchExprType(), eq_op);
        if (retval < 0) return retval;

        // [fw] Comparison operation may be missing here.

        // rip out the already generated code for the case/switch and store it with the switch
        nesting_stack->YankChunk(scrip, start_of_code_loc, numfixups_at_start_of_code);
    }

    // expect and gobble the ':'
    if (sym.get_type(targ->getnext()) != SYM_LABEL)
    {
        cc_error("Expected ':'");
        return -1;
    }

    return 0;
}

int evaluate_break(ccInternalList * targ, ccCompiledScript * scrip, ags::NestingStack *nesting_stack)
{
    // Find the (level of the) looping construct to which the break applies
    size_t loop_level = nesting_stack->Depth() - 1;
    while (loop_level > 0 && nesting_stack->StartLoc(loop_level) == 0) loop_level--;

    if (loop_level == 0)
    {
        cc_error("Break only valid inside a loop or switch statement block");
        return -1;
    }

    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // If locals contain pointers, free them
    free_pointers_of_locals(scrip, loop_level - 1);

    // Pop local variables from the stack
    int totalsub = stacksize_of_locals(loop_level - 1);
    if (totalsub > 0) scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    scrip->flush_line_numbers();

    // The jump out of the loop, below, may be a conditional jump.
    // So clear AX to make sure that the jump is executed.
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0); 

    // Jump to a jump to the end of the loop
    if (nesting_stack->Type(loop_level) == ags::NestingStack::NTSwitch)
    {
        scrip->write_cmd1(SCMD_JMP, -(scrip->codesize - nesting_stack->StartLoc(loop_level))); // Jump to the known break point
    }
    else
    {
        scrip->write_cmd1(SCMD_JMP, -(scrip->codesize - nesting_stack->JumpOutLoc(loop_level) + 3)); // Jump to the known break point
    }
    return 0;
}

int evaluate_continue(ccInternalList * targ, ccCompiledScript * scrip, ags::NestingStack *nesting_stack)
{
    // Find the (level of the) looping construct to which the break applies
    size_t loop_level = nesting_stack->Depth() - 1;
    while (loop_level > 0 && nesting_stack->StartLoc(loop_level) == 0) loop_level--;

    if (loop_level == 0)
    {
        cc_error("Continue not valid outside a loop");
        return -1;
    }
   
    if (sym.get_type(targ->getnext()) != SYM_SEMICOLON)
    {
        cc_error("Expected ';'");
        return -1;
    }

    // If locals contain pointers, free them
    free_pointers_of_locals(scrip, loop_level);

    // Pop local variables from the stack
    int totalsub = stacksize_of_locals(loop_level);
    if (totalsub > 0) scrip->write_cmd2(SCMD_SUB, SREG_SP, totalsub);
    scrip->flush_line_numbers();

    // if it's a for loop, drop the yanked chunk (loop increment) back in
    if (nesting_stack->ChunksExist(loop_level))
    {
        nesting_stack->WriteChunk(scrip, loop_level, 0);
    }
    scrip->flush_line_numbers();

    // original comment "The jump below may be a conditional jump."
    //  [fw] Nooo? Leave it in, anyway, so that we have byte identical code
    // original comment "So clear AX to make sure that the jump is executed."
    scrip->write_cmd2(SCMD_LITTOREG, SREG_AX, 0); 

    // Jump to the start of the loop
    scrip->write_cmd1(SCMD_JMP, -((scrip->codesize + 2) - nesting_stack->StartLoc(loop_level))); 
    return 0;
}

// We're compiling function body code; the code does not start with a keyword or type.
// Thus, we should be at the start of an assignment. Compile it.
int compile_funcbodycode_Assignment(ccInternalList * targ, ccCompiledScript * scrip, ags::Symbol_t cursym, const ags::SymbolScript_t &vnlist, size_t &vnlist_len)
{
    // Check whether the next symbol is an assignment symbol
    ags::Symbol_t nextsym = targ->peeknext();
    int nexttype = sym.get_type(nextsym);
    if (nexttype != SYM_ASSIGN && nexttype != SYM_MASSIGN && nexttype != SYM_SASSIGN)
    {
        cc_error("Parse error at '%s'", sym.get_friendly_name(cursym).c_str());
        return -1;
    }
    return evaluate_assignment(targ, scrip, cursym, SYM_SEMICOLON, vnlist, vnlist_len);
}


int compile_funcbodycode(
    ccInternalList *targ,
    ccCompiledScript * scrip,
    ags::Symbol_t cursym,
    int offset_of_funcname,
    int index_of_expression_start,
    ags::SymbolScript_t  vnlist,
    size_t vnlist_len,
    ags::Symbol_t inFuncSym,
    ags::NestingStack *nesting_stack,
    char is_protected,
    char is_static,
    char is_readonly)
{
    // Force current symbol for functions
    if (offset_of_funcname > 0) cursym = sym.find("function");

    int retval = 0; // for error msg
    switch (sym.get_type(cursym))
    {
    default:
        // Should be an assignment -- or else it is an error
        retval = compile_funcbodycode_Assignment(targ, scrip, cursym, vnlist, vnlist_len);
        if (retval < 0) return retval;
        break;

    case SYM_BREAK:
        retval = evaluate_break(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_CASE:
        retval = evaluate_casedefault(targ, scrip, cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_CONTINUE:
        retval = evaluate_continue(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_DEFAULT:
        retval = evaluate_casedefault(targ, scrip, cursym, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_DO:
        return evaluate_do(targ, scrip, nesting_stack);

    case SYM_FOR:
        return evaluate_for(targ, scrip, cursym, nesting_stack, vnlist, vnlist_len, offset_of_funcname, is_protected, is_static, is_readonly);

    case SYM_FUNCTION:
        retval = evaluate_funccall(targ, scrip, offset_of_funcname, index_of_expression_start);
        if (retval < 0) return retval;
        break;

    case SYM_IF:
        return evaluate_if(targ, scrip, cursym, nesting_stack);

    case SYM_RETURN:
        retval = evaluate_return(targ, scrip, inFuncSym);
        if (retval < 0) return retval;
        break;

    case SYM_SWITCH:
        retval = evaluate_switch(targ, scrip, nesting_stack);
        if (retval < 0) return retval;
        break;

    case SYM_WHILE:
        return evaluate_while(targ, scrip, cursym, nesting_stack);
    }

    // sort out jumps when a single-line if or else has finished
    return compile_funcbodycode_EndOfDoIfElse(targ, scrip, nesting_stack);
}


int cc_compile_HandleLinesAndMeta(ccInternalList & targ, ccCompiledScript * scrip, ags::Symbol_t cursym, int &currentlinewas)
{
    if (cursym < 0) return 0; // end of stream was reached.
    if (currentline == -10) return 0; // end of stream was reached
    // [fw] DEBUGGING Start
    if (currentline == 11)
    {
        int i = 0; // set breakpoint here
    }
    // [fw]DEBUGGING END

    if ((currentline != currentlinewas) && (ccGetOption(SCOPT_LINENUMBERS) != 0))
    {
        scrip->set_line_number(currentline);
        currentlinewas = currentline;
    }

    if (cursym == SCODE_INVALID)
    {
        cc_error("Internal compiler error: Invalid symbol found");
        return -1;
    }

    if (cursym == SCODE_META)
    {
        long metatype = targ.getnext();
        if (metatype == SMETA_END) return 0; // Another way of ending a compilation

        if (metatype == SMETA_LINENUM)
        {
            cc_error("Internal compiler error: Unexpected 'meta linenum' token found in input");
            return -1;
        }

        cc_error("Internal compiler error: Invalid meta token found in input");
        return -1;
    }

    return 0;
}

int cc_compile_ParseTokens(ccInternalList &targ, ccCompiledScript * scrip, size_t &nested_level, int idx_of_current_func, ags::Symbol_t name_of_current_func)
{
    ags::Symbol_t struct_of_current_func = 0; // non-zero only when a struct member function is open
    
    ags::NestingStack nesting_stack = ags::NestingStack();

    // These are indicators that are switched on when keywords are encountered and switched off after they are used.
    // Thus, they are called "NEXT_IS_..." although they are sometimes _used_ with the meaning "THIS_IS...".
    bool next_is_managed = false;
    bool next_is_builtin = false;
    bool next_is_autoptr = false;
    bool next_is_stringstruct = false;
    bool next_is_readonly = false;
    bool next_is_static = false;
    bool next_is_protected = false;
    bool next_is_noloopcheck = false;

    // This is NOT an indicator proper since it can have values from 0 to 2.
    // But 0 seems to mean "next is NOT an import".
    Importness next_is_import = ImNoImport; // NOT a bool, can be 0 or 1 or 2

    // Go through the list of tokens one by one. We start off in the global data
    // part - no code is allowed until a function definition is started
    currentline = 1; // This is a global variable. cc_internallist.cpp, cs_internallist_test.cpp, cs_parser.cpp
    int currentlinewas = 0;

    // This 'for' will typically run for much less than targ->length times
    for (size_t aa = 0; aa < static_cast<const size_t>(targ.length); aa++)
    {
        if (ReachedEOF(&targ)) break;

        ags::Symbol_t cursym = targ.getnext();
        int retval = cc_compile_HandleLinesAndMeta(targ, scrip, cursym, currentlinewas);
        if (retval < 0) return retval;

        // Handling new sections
        // These section are denoted by a string that begins with NEW_SCRIPT_TOKEN_PREFIX;
        // the section name follows.
        if (strncmp(sym.get_name(cursym), NEW_SCRIPT_TOKEN_PREFIX, 18) == 0)
        {
            // scriptNameBuffer is a static C string. 
            // We can't replace it by a std::string  because it is referenced 
            // externally through ccCurScriptName (defined in cc_error.cpp)
            strncpy(scriptNameBuffer, sym.get_name(cursym) + 18, 236);
            scriptNameBuffer[strlen(scriptNameBuffer) - 1] = 0;  // strip closing quote
            ccCurScriptName = scriptNameBuffer;
            scrip->start_new_section(scriptNameBuffer);
            currentline = 0;
            continue;
        }


        int symType = sym.get_type(cursym);

        switch (symType) // please keep the cases alphabetical
        {
        default: break;

        case  SYM_AUTOPTR:
        {
            next_is_autoptr = true;
            int type_of_next_sym = sym.get_type(targ.peeknext());

            if (type_of_next_sym != SYM_MANAGED && type_of_next_sym != SYM_BUILTIN)
            {
                cc_error("Expected 'managed' or 'builtin' after 'autoptr'");
                return -1;
            }
            continue;
        }

        case SYM_BUILTIN:
        {
            next_is_builtin = true;
            int type_of_next_sym = sym.get_type(targ.peeknext());
            if ((type_of_next_sym != SYM_MANAGED) && (type_of_next_sym != SYM_STRUCT))
            {
                cc_error("'builtin' can only be used with 'struct' or 'managed struct'");
                return -1;
            }
            continue;
        }

        case  SYM_CLOSEBRACE:
        {
            int retval = cs_parser_handle_closebrace(&targ, scrip, &nesting_stack, idx_of_current_func, name_of_current_func, struct_of_current_func);
            if (retval < 0) return retval;
            continue;
        }

        case SYM_CONST:
        {
            cc_error("'const' is only valid for function parameters (use 'readonly' instead)");
            return -1;
        }

        case SYM_ENUM:
        {
            int retval = cs_parser_handle_enum(&targ, idx_of_current_func);
            if (retval < 0) return retval;

            continue;
        }

        case SYM_EXPORT:
        {
            int retval = cs_parser_handle_export(&targ, scrip, cursym);
            if (retval < 0) return retval;

            continue;
        }

        case SYM_IMPORT:
        {
            int retval = cs_parse_handle_import(&targ, idx_of_current_func, cursym, next_is_import);
            if (retval < 0) return retval;
            continue;
        }

        case  SYM_MANAGED:
        {
            next_is_managed = true;
            if (sym.get_type(targ.peeknext()) != SYM_STRUCT)
            {
                cc_error("'managed' can only be used with 'struct'");
                return -1;
            }

            continue;
        }

        case SYM_OPENBRACE:
        {
            // This begins a compound statement (which is only legal if we are within a func body)
            int retval = cs_parser_handle_openbrace(scrip, &nesting_stack, idx_of_current_func, name_of_current_func, struct_of_current_func, next_is_noloopcheck);
            if (retval < 0) return retval;

            // The "noloopcheck" mode has just been used up. 
            // This is true although the function that uses "noloopcheck" is still
            // being compiled: "Noloopcheck" mode is translated into a Bytecode
            // that is generated as part of a function header. Seemingly, this switches
            // the mode on in the virtual machine until the virtual machine detects 
            // the end of the function in some way.
            next_is_noloopcheck = false;
            continue;
        }

        case SYM_PROTECTED:
        {
            int retval = cs_parser_handle_protected(&targ, idx_of_current_func, next_is_protected);
            if (retval < 0) return retval;
            continue;
        }

        case SYM_READONLY:
        {
            next_is_readonly = true;
            if (sym.get_type(targ.peeknext()) != SYM_VARTYPE)
            {
                cc_error("Expected type after 'readonly'");
                return -1;
            }
            continue;
        }

        case SYM_STATIC:
        {
            int retval = cs_parser_handle_static(&targ, idx_of_current_func, next_is_static);
            if (retval < 0) return retval;
            continue;
        }

        case SYM_STRINGSTRUCT:
        {
            next_is_stringstruct = true;
            if (sym.stringStructSym > 0)
            {
                cc_error("stringstruct already defined");
                return -1;
            }
            if (sym.get_type(targ.peeknext()) != SYM_AUTOPTR)
            {
                cc_error("Expected 'autoptr' after 'stringstruct'");
                return -1;
            }
            continue;
        }

        case  SYM_STRUCT:
        {
            int retval = cs_parser_handle_struct(&targ, scrip, next_is_managed, next_is_builtin, next_is_autoptr, next_is_stringstruct, cursym, idx_of_current_func, nested_level);
            if (retval < 0) return retval;

            next_is_managed = false;
            next_is_builtin = false;
            next_is_autoptr = false;
            next_is_stringstruct = false;
            continue;
        }

        case SYM_VARTYPE:
        {
            if (sym.get_type(targ.peeknext()) == SYM_DOT)
            {
                // We're looking at "int ." or similar; treated below the switch
                break;
            }
            // func or variable definition
            int retval = cs_parser_handle_vartype(&targ, scrip, cursym, &nesting_stack, next_is_import, next_is_readonly, next_is_static, next_is_protected, idx_of_current_func, name_of_current_func, struct_of_current_func, next_is_noloopcheck);
            if (retval < 0) return retval;
            next_is_import = ImNoImport;
            next_is_readonly = false;
            next_is_static = false;
            next_is_protected = false;
            continue;
        }

        } // switch (symType)

          // Guard against strange input first 
        if (symType == 0) return error_undef_token_inside_func(cursym);

        // Here when we haven't found any of the declaration starting keywords.
        // This only makes sense if we are within a function call.
        if (idx_of_current_func < 0)
        {
            cc_error("'%s' is illegal outside a function", sym.get_friendly_name(cursym).c_str());
            return -1;
        }

        // This will contain buffered code.
        ags::Symbol_t vnlist[TEMP_SYMLIST_LENGTH];
        size_t vnlist_len;
        int offset_of_funcname; // -1 means: No function call

        // Read and buffer a leading variable or funccall, if present. If not, return without error.
        retval = read_var_or_funccall(&targ, cursym, vnlist, vnlist_len, offset_of_funcname);
        if (retval < 0) return retval;

        retval = compile_funcbodycode(&targ, scrip, cursym, offset_of_funcname, targ.pos, vnlist, vnlist_len, name_of_current_func, &nesting_stack, next_is_protected, next_is_static, next_is_readonly);
        if (retval < 0) return retval;
    } // for

    return 0;
}

// compile the code in the INPL parameter into code in the scrip structure,
// but don't reset anything because more files could follow
int cc_compile(const char * inpl, ccCompiledScript * scrip)
{
    // Note: cc_preprocess() used to be called here once

    // Scan the program code.
    ccInternalList targ;
    int retval = cc_tokenize(inpl, &targ, scrip);
    if (retval < 0) return retval;

    targ.startread();

    size_t nested_level = 0;
    int idx_of_current_func = -1;
    ags::Symbol_t name_of_current_func = -1;
    retval = cc_compile_ParseTokens(targ, scrip, nested_level, idx_of_current_func, name_of_current_func);
    if (retval < 0) return retval;

    // Here when the tokens have been exhausted
    if (idx_of_current_func >= 0)
    {
        currentline = targ.lineAtEnd;
        std::string func_identification = "current function";
        if (name_of_current_func != 0)
        {
            func_identification = "function '&1'";
            func_identification.replace(func_identification.find("&1"), 2, sym.get_name_string(name_of_current_func));
        }
        cc_error("At end of input, but body of '%s' has not been closed", func_identification.c_str());
        return -1;
    }

    if (nested_level == 0) return 0;

    currentline = targ.lineAtEnd;
    cc_error("At end of input, but an open '{' is missing its '}'");
    return -1;
}

