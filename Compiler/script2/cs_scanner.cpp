#include <string>
#include <sstream>
#include <iomanip>

#include "cc_internallist.h"    // SrcList
#include "cs_parser_common.h"

#include "cs_scanner.h"

std::string const AGS::Scanner::new_section_lit_prefix  = "__NEWSCRIPTSTART_";

AGS::Scanner::Scanner(std::string const &input, SrcList &token_list, struct ::ccCompiledScript &string_collector, ::SymbolTable &symt)
    : _ocMatcher(token_list)
    , _lineno(1)
    , _section("")
    , _tokenList(token_list)
    , _lastError("")
    , _sym(symt)
    , _stringCollector(string_collector)
{
    _inputStream.str(input);
}

void AGS::Scanner::Scan(bool &error_encountered)
{
    while (true)
    {
        bool eof_encountered = false;
        Symbol const symbol = GetNextSymbol(eof_encountered, error_encountered);
        if (eof_encountered || error_encountered)
            return;
        _tokenList.Append(symbol);
    }        
}

void AGS::Scanner::NewLine(size_t lineno)
{
    _lineno = lineno;
    _tokenList.NewLine(_lineno);
}

void AGS::Scanner::NewSection(std::string const section)
{
    _section = section;
    _tokenList.NewSection(section);
    NewLine(0);
}

void AGS::Scanner::GetNextSymstring(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered)
{
    eof_encountered = false;
    error_encountered = false;

    SkipWhitespace(eof_encountered, error_encountered);
    if (eof_encountered || error_encountered)
        return;

    int next_char = _inputStream.peek();
    eof_encountered = _inputStream.eof();
    if (eof_encountered)
        return;
    error_encountered = _inputStream.fail();
    if (error_encountered)
        return;

    // Integer or float literal
    if (IsDigit(next_char))
    {
        symstring = ""; 
        ReadInNumberLit(symstring, scan_type, eof_encountered, error_encountered);
        return;
    }

    // Character literal
    if ('\'' == next_char)
    {
        // Note that this converts the literal to an equivalent integer string "'A'" >>-> "65"
        ReadInCharLit(symstring, eof_encountered, error_encountered);
        scan_type = kSct_IntLiteral;
        return;
    }

    // Identifier or keyword
    if (IsUpper(next_char) || IsLower(next_char) || ('_' == next_char))
    {
        ReadInIdentifier(symstring, eof_encountered, error_encountered);
        scan_type = kSct_Identifier;
        return;
    }

    // String literal
    if ('"' == next_char)
    {
        ReadInStringLit(symstring, eof_encountered, error_encountered);
        scan_type = kSct_StringLiteral;
        size_t const len = new_section_lit_prefix.length();
        if (new_section_lit_prefix == symstring.substr(0, len))
        {
            symstring = symstring.substr(len);
            scan_type = kSct_SectionChange;
        }
        return;
    }

    // Non-char symstrings, such as "*="
    scan_type = kSct_NonChar;
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
        // Note, this can overwrite scan_type.
    case '.': ReadInDotCombi(symstring, scan_type, eof_encountered, error_encountered); return;
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
    _lastError = "The character '&char&' is not legal in this context";
    std::string chstring(1, next_char);
    ReplaceToken(_lastError, "&char&", chstring);
    return;
}

AGS::Symbol AGS::Scanner::GetNextSymbol(bool &eof_encountered, bool &error_encountered)
{
    Symbol symbol = -1;
    std::string symstring;
    ScanType scan_type;
    
    while (true)
    {
        GetNextSymstring(symstring, scan_type, eof_encountered, error_encountered);
        if (eof_encountered || error_encountered)
            return symbol;

        if (kSct_SectionChange != scan_type)
            break;
        NewSection(symstring);
    }

    SymstringToSym(symstring, scan_type, symbol, eof_encountered, error_encountered);
    if (!eof_encountered && !error_encountered)
        CheckMatcherNesting(symbol, error_encountered);
    return symbol;
}

void AGS::Scanner::SkipWhitespace(bool &eof_encountered, bool &error_encountered)
{
    while (true)
    {
        int const ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered)
            return;
        error_encountered = _inputStream.fail();
        if (error_encountered)
        {
            _lastError = "Error whilst skipping whitespace (file corrupt?)";
            return;
        }

        if (!IsSpace(ch))
        {
            _inputStream.putback(ch);
            return;
        }

        // Gobble the CR of a CRLF combination
        if ('\r' == ch && '\n' == _inputStream.peek())
            continue;

        if ('\n' == ch)
            NewLine(_lineno + 1);
    }
}

void AGS::Scanner::ReadInNumberLit(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered)
{
    bool decimal_point_encountered = (std::string::npos != symstring.find('.'));
    bool e_encountered = false;
    bool eminus_encountered = false;

    scan_type = (decimal_point_encountered)? kSct_FloatLiteral : kSct_IntLiteral;
    symstring.push_back(_inputStream.get());

    while (true)
    {
        int const ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered)
            return;
        error_encountered = _inputStream.fail();
        if (error_encountered)
        {
            _lastError = "Error encountered while scanning a number literal (file corrupt?)";
            return;
        }
        if (eof_encountered || error_encountered)
            return;

        if (IsDigit(ch))
        {
            symstring.push_back(ch);
            continue;
        }
        if ('.' == ch)
        {
            if (!decimal_point_encountered)
            {
                decimal_point_encountered = true;
                scan_type = kSct_FloatLiteral;
                symstring.push_back(ch);
                continue;
            }
        }
        if ('e' == ch || 'E' == ch)
        {
            if (!e_encountered)
            {
                decimal_point_encountered = true;
                e_encountered = true;
                scan_type = kSct_FloatLiteral;
                symstring.push_back(ch);
                continue;
            }
        }
        if ('-' == ch)
        {
            if (e_encountered && !eminus_encountered)
            {
                eminus_encountered = true;
                symstring.push_back(ch);
                continue;
            }

        }
        _inputStream.putback(ch); // no longer part of the number literal, so put it back
        break;
    }
}

void AGS::Scanner::ReadInCharLit(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    symstring = "";

    do // exactly 1 time
    {
        // Opening '\''
        _inputStream.get(); // Eat '\''

        // The character inside
        int lit_char = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered)
        {
            error_encountered = true;
            _lastError = "Expected a character and an apostrophe, but input ended instead";
            return;
        }
        error_encountered = _inputStream.fail();
        if (error_encountered)
            break; // to error processing

        if ('\\' == lit_char)
        {
            // The next char is escaped, whatever it may be.
            lit_char = _inputStream.get();
            eof_encountered = _inputStream.eof();
            error_encountered = _inputStream.fail();
            if (eof_encountered)
            {
                error_encountered = true;
                _lastError = "The input ended inmidst of an escape sequence";
                return;
            }
            if ('[' == lit_char)
            {
                // "\\[" is equivalent to two characters, so can't be used as a single character
                _lastError = "\\[ not allowed in single quotes, use '[' instead";
                return;
            }
            if (error_encountered)
                break; // to error processing
            lit_char = EscapedChar2Char(lit_char, error_encountered);
            if (error_encountered)
                return;
        }

        // Closing '\''
        int const ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered)
        {
            error_encountered = true;
            _lastError = "Expected an apostrophe, but input ended instead";
            return;
        }
        error_encountered = _inputStream.fail();
        if (error_encountered)
            break; // to error processing
        if (ch != '\'')
        {
            error_encountered = true;
            std::string wrong_char_as_string(1, ch);
            _lastError = "Expected apostrophe, but found '&char&' instead";
            ReplaceToken(_lastError, "&char&", wrong_char_as_string);
            return;
        }
        // Convert the char literal to an int literal
        // Note: We do NOT return the char literal, 
        // we return an equivalent integer literal, instead.
        symstring = std::to_string(lit_char);
        return;
    }
    while (false);

    // Here when we got a read error
    error_encountered = true;
    _lastError = "Could not read the input (corrupt file?)";
    return;
}

int AGS::Scanner::OctChar2Char(int first_digit_char)
{
    int ret = first_digit_char - '0';
    for (size_t count = 0; count < 2; ++count)
    {
        int const digit = _inputStream.peek();
        if (digit < '0' || digit > '8')
            return ret;
        int new_value = 8 * ret + (digit - '0');
        if (new_value > 255)
            return ret;
        ret = new_value;
        _inputStream.get(); // Eat the digit
    }
    return ret;
}

int AGS::Scanner::HexChar2Char()
{
    int ret = 0;
    for (size_t count = 0; count < 2; ++count)
    {
        int hexdigit = _inputStream.peek();
        //convert a..f to A..F
        if (hexdigit > 'a')
            hexdigit = hexdigit - 'a' + 'A'; 
        if (hexdigit < '0' || (hexdigit > '9' && hexdigit < 'A') || hexdigit > 'F' )
            return ret;
        hexdigit -= '0';
        if (hexdigit > 9)
            hexdigit -= ('@' - '9');
        ret = 16 * ret + hexdigit;
        _inputStream.get(); // Eat the hexdigit
    }
    return ret;
}

int AGS::Scanner::EscapedChar2Char(int ch, bool &error_encountered)
{
    if (IsDigit(ch))
        return OctChar2Char(ch);

    switch (ch)
    {
    default:
        error_encountered = true;
        _lastError = "Found unknown escape sequence '\\&char&' in string.";
        ReplaceToken(_lastError, "&char&", std::string{ 1, static_cast<char>(ch) });
        return 0;
    case '\'':
    case '\"':
        return ch;
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'e':
        return 27; // escape char
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case 'x':
        return HexChar2Char();
    }
    return 0; // can't be reached
}

std::string AGS::Scanner::MakeStringPrintable(std::string const &inp)
{
    std::ostringstream out;
    out.put('"');

    for (auto it = inp.begin(); it != inp.end(); ++it)
    {
        if (*it >= 32 && *it <= 127)
            out.put(*it);
        else // force re-interpretation of *it as an UNSIGNED char, then treat it as int       
            out << "\\x" << std::hex << std::setw(2) << std::setfill('0') << +*reinterpret_cast<const unsigned char *>(&*it);
    }

    out.put('"');
    return out.str();
}

void AGS::Scanner::ReadInStringLit(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    symstring = "";
    _inputStream.get(); // Eat '"'
    while (true)
    {
        int ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        error_encountered = _inputStream.fail();
        if (eof_encountered || error_encountered || ch == '\n' || ch == '\r')
            break; // to error msg

        if (ch == '\\')
        {
            ch = _inputStream.get();
            eof_encountered = _inputStream.eof(); // This is an error, too
            error_encountered = _inputStream.fail();
            if (eof_encountered || error_encountered || ch == '\n' || ch == '\r')
                break; // to error msg
            if ('[' == ch)
                symstring.append("\\[");
            else
                symstring.push_back(EscapedChar2Char(ch, error_encountered));
            if (error_encountered)
                return;
            continue;
        }

        // End of string
        if (ch == '"')
            return;

        symstring.push_back(ch);
    }

    // Here when an error or eof occurs.
    if (eof_encountered)
        _lastError = "End of input encountered in an unclosed string literal";
    else 
        _lastError = "String literal may not contain any line breaks (use '[' instead)";
    error_encountered = true;
    return;
}


void AGS::Scanner::ReadInIdentifier(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    symstring.assign(1, _inputStream.get());

    while (true)
    {
        int ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered)
            return;
        error_encountered = _inputStream.fail();
        if (error_encountered)
        {
            _lastError = "Error encountered while scanning an identifier (file corrupt?)";
            return;
        }

        if (IsUpper(ch) || IsLower(ch) || IsDigit(ch) || (ch == '_'))
        {
            symstring.push_back(ch);
            continue;
        }
        // That last char doesn't belong to the literal, so put it back.
        _inputStream.putback(ch);
        return;
    }
}


void AGS::Scanner::ReadIn1or2Char(const std::string &possible_second_chars, std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    symstring.assign(1, _inputStream.get());
    int second_char = _inputStream.peek();
    eof_encountered = _inputStream.eof();
    error_encountered = _inputStream.fail();
    if (error_encountered)
        _lastError = "Unexpected error in input (file corrupt)?";
    if (eof_encountered || error_encountered)
        return;

    if (possible_second_chars.find(second_char) != std::string::npos)
    {
        _inputStream.get(); // Gobble the character that was peek()ed
        symstring.push_back(second_char);
    }
}

void AGS::Scanner::ReadIn1Char(std::string &symstring)
{
    symstring.assign(1, _inputStream.get());
}

void AGS::Scanner::ReadInDotCombi(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered)
{
    symstring.assign(1, _inputStream.get());
    int const second_char = _inputStream.peek();
    eof_encountered = _inputStream.eof();
    error_encountered = _inputStream.fail();
    if (error_encountered)
        _lastError = "Unexpected error in input (file corrupt)?";
    if (eof_encountered || error_encountered)
        return;

    if (IsDigit(second_char))
    {
        ReadInNumberLit(symstring, scan_type, eof_encountered, error_encountered);
        return;
    }

    if ('.' != second_char)
        return;

    symstring.push_back(_inputStream.get());

    if ('.' != _inputStream.get())
    {
        _lastError = "Must either use '.' or '...'";
        error_encountered = true;
    }

    symstring.push_back('.');
}

void AGS::Scanner::ReadInLTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    ReadIn1or2Char("<=", symstring, eof_encountered, error_encountered);
    if (eof_encountered || error_encountered)
        return;

    if ((symstring == "<<") && (_inputStream.peek() == '='))
        symstring.push_back(_inputStream.get());
}

void AGS::Scanner::ReadInGTCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    ReadIn1or2Char(">=", symstring, eof_encountered, error_encountered);
    if (eof_encountered || error_encountered)
        return;

    if ((symstring == ">>") && (_inputStream.peek() == '='))
        symstring.push_back(_inputStream.get());
}

void AGS::Scanner::SymstringToSym(std::string const &symstring, ScanType scan_type, Symbol &symb, bool eof_encountered, bool error_encountered)
{
    std::string name =
        (kSct_StringLiteral == scan_type) ? MakeStringPrintable(symstring) : symstring;
    symb = _sym.FindOrAdd(name.c_str());
    if (symb < 0)
    {
        error_encountered = true;
        _lastError = "Symbol table overflow - could not add new symbol";
        return;
    }

    switch (scan_type)
    {
    default:
        return;

    case  Scanner::kSct_StringLiteral:
        _sym[symb].SType = kSYM_LiteralString;
        _sym[symb].Vartype = _sym.GetOldStringSym();
        _sym[symb].SOffset = _stringCollector.add_string(symstring.c_str());
        return;

    case Scanner::kSct_IntLiteral:
        _sym[symb].SType = kSYM_LiteralInt;
        return;

    case Scanner::kSct_FloatLiteral:
        _sym[symb].SType = kSYM_LiteralFloat;
        return;
    }
}

void AGS::Scanner::OpenCloseMatcher::Reset()
{
    _lastError = "";
    _openInfoStack.clear();
}


AGS::Scanner::OpenCloseMatcher::OpenCloseMatcher(SrcList &sectionIdConverter)
    :_sectionIdConverter(sectionIdConverter)
{
    Reset();
}


void AGS::Scanner::OpenCloseMatcher::Push(std::string const &opener, std::string const &expected_closer, size_t opener_pos)
{
    struct OpenInfo oi = { opener, expected_closer, opener_pos };
    _openInfoStack.push_back(oi);
}


void AGS::Scanner::OpenCloseMatcher::PopAndCheck(std::string const &closer, size_t closer_pos, bool &error_encountered)
{
    if (_openInfoStack.empty())
    {
        error_encountered = true;
        _lastError = "There isn't any opening symbol that matches the closing '&closer&'";
        ReplaceToken(_lastError, "&closer&", closer);
        return;
    }

    struct OpenInfo const oi = _openInfoStack.back();
    _openInfoStack.pop_back();
    if (closer == oi.Closer)
        return;

    error_encountered = true;
    size_t const opener_section_id = _sectionIdConverter.GetSectionIdAt(oi.Pos);
    std::string const &opener_section = _sectionIdConverter.SectionId2Section(opener_section_id);
    size_t const opener_lineno = _sectionIdConverter.GetLinenoAt(oi.Pos);
    size_t const closer_section_id = _sectionIdConverter.GetSectionIdAt(oi.Pos);
    std::string const &closer_section = _sectionIdConverter.SectionId2Section(opener_section_id);
    size_t const closer_lineno = _sectionIdConverter.GetLinenoAt(closer_pos);

    _lastError = "Found '&closer&', this does not match the '&opener&' in &section&, line &lineno&";
    if (0 == closer_section.compare(opener_section))
        _lastError = "Found '&closer&', this does not match the '&opener&' on line &lineno&";
    if (closer_lineno == opener_lineno)
        _lastError = "Found '&closer&', this does not match the '&opener&' on this line";
    ReplaceToken(_lastError, "&closer&", closer);
    ReplaceToken(_lastError, "&opener&", oi.Opener);
    if (_lastError.npos != _lastError.find("&lineno&"))
        ReplaceToken(_lastError, "&lineno&", std::to_string(opener_lineno));
    if (_lastError.npos != _lastError.find("&section&"))
        ReplaceToken(_lastError, "&section&", opener_section);
}

// Check the nesting of () [] {}, error if mismatch
void AGS::Scanner::CheckMatcherNesting(Symbol token, bool &error_encountered)
{
    switch (_sym.GetSymbolType(token))
    {
    default:
        return;

    case kSYM_CloseBrace:
    case kSYM_CloseBracket:
    case kSYM_CloseParenthesis:
        _ocMatcher.PopAndCheck(_sym[token].SName, _tokenList.Length(), error_encountered);
        if (error_encountered)
            _lastError = _ocMatcher.GetLastError();
        return;

    case kSYM_OpenBrace:
        _ocMatcher.Push("{", "}", _tokenList.Length());
        return;

    case kSYM_OpenBracket:
        _ocMatcher.Push("[", "]", _tokenList.Length());
        return;

    case kSYM_OpenParenthesis:
        _ocMatcher.Push("(", ")", _tokenList.Length());
        return;
    }
}
