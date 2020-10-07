#include <string>
#include <sstream>
#include <iomanip>
#include <cstdarg>

#include "cc_internallist.h"    // SrcList
#include "cs_parser_common.h"

#include "cs_scanner.h"

std::string const AGS::Scanner::kNewSectionLitPrefix = "__NEWSCRIPTSTART_";

AGS::Scanner::Scanner(std::string const &input, SrcList &token_list, ::ccCompiledScript &string_collector, ::SymbolTable &symt, MessageHandler &messageHandler)
    : _ocMatcher(*this)
    , _lineno(1u)
    , _tokenList(token_list)
    , _messageHandler(messageHandler)
    , _sym(symt)
    , _stringCollector(string_collector)
{
    _section = token_list.SectionId2Section(token_list.GetSectionIdAt(0));
    _inputStream.str(input);
}

ErrorType AGS::Scanner::Scan()
{
    while (true)
    {
        bool eof_encountered = false;
        bool error_encountered = false;
        Symbol symbol;
        ErrorType retval = GetNextSymbol(symbol, eof_encountered);
        if (retval < 0) return retval;
        if (eof_encountered)
            return _ocMatcher.EndOfInputCheck();
        _tokenList.Append(symbol);
    }
}

void AGS::Scanner::NewLine(size_t lineno)
{
    _lineno = lineno;
    _tokenList.NewLine(_lineno);
}

void AGS::Scanner::NewSection(std::string const &section)
{
    _section = section;
    _tokenList.NewSection(section);
    NewLine(0);
}

ErrorType AGS::Scanner::GetNextSymstring(std::string &symstring, ScanType &scan_type, bool &eof_encountered)
{
    eof_encountered = false;

    ErrorType retval = SkipWhitespace(eof_encountered);
    if (retval < 0) return kERR_UserError;
    if (eof_encountered) return kERR_None;

    int next_char = _inputStream.peek();
    eof_encountered = _inputStream.eof();
    if (eof_encountered) return kERR_None;
    if (_inputStream.fail())
    {
        Error("Error reading a character (file corrupt?)");
        return kERR_UserError;
    }

    // Integer or float literal
    if (IsDigit(next_char))
    {
        symstring = "";
        return ReadInNumberLit(symstring, scan_type, eof_encountered);
    }

    // Character literal
    if ('\'' == next_char)
    {
        // Note that this converts the literal to an equivalent integer string "'A'" >>-> "65"
        scan_type = kSct_IntLiteral;
        return ReadInCharLit(symstring, eof_encountered);
    }

    // Identifier or keyword
    if (IsUpper(next_char) || IsLower(next_char) || ('_' == next_char))
    {
        scan_type = kSct_Identifier;
        return ReadInIdentifier(symstring, eof_encountered);
    }

    // String literal
    if ('"' == next_char)
    {
        scan_type = kSct_StringLiteral;
        ErrorType retval = ReadInStringLit(symstring, eof_encountered);
        if (retval < 0) return retval;

        size_t const len = kNewSectionLitPrefix.length();
        if (kNewSectionLitPrefix == symstring.substr(0, len))
        {
            symstring = symstring.substr(len);
            scan_type = kSct_SectionChange;
        }
        return kERR_None;
    }

    // Non-char symstrings, such as "*="
    scan_type = kSct_NonChar;
    switch (next_char)
    {
    default:  break;
    case '!': return ReadIn1or2Char("=", symstring, eof_encountered);
    case '%': return ReadIn1or2Char("=", symstring, eof_encountered);
    case '&': return ReadIn1or2Char("&=", symstring, eof_encountered);
    case '(': return ReadIn1Char(symstring);
    case ')': return ReadIn1Char(symstring);
    case '*': return ReadIn1or2Char("=", symstring, eof_encountered);
    case '+': return ReadIn1or2Char("+=", symstring, eof_encountered);
    case ',': return ReadIn1Char(symstring);
    case '-': return ReadIn1or2Char("-=>", symstring, eof_encountered);
        // Note, this can overwrite scan_type.
    case '.': return ReadInDotCombi(symstring, scan_type, eof_encountered);
        // Note that the input is pre-processed,
        // so it cannot contain comments of the form //...EOL or /*...*/
    case '/': return ReadIn1or2Char("=", symstring, eof_encountered);
    case ':': return ReadIn1or2Char(":", symstring, eof_encountered);
    case ';': return ReadIn1Char(symstring);
    case '<': return ReadInLTCombi(symstring, eof_encountered);
    case '=': return ReadIn1or2Char("=", symstring, eof_encountered);
    case '>': return ReadInGTCombi(symstring, eof_encountered);
    case '?': return ReadIn1Char(symstring);
    case '[': return ReadIn1Char(symstring);
    case ']': return ReadIn1Char(symstring);
    case '^': return ReadIn1or2Char("=", symstring, eof_encountered);
    case '{': return ReadIn1Char(symstring);
    case '|': return ReadIn1or2Char("=|", symstring, eof_encountered);
    case '}': return ReadIn1Char(symstring);
    case '~': return ReadIn1Char(symstring);
    }

    // Here when we don't know how to process the next char to be read
    Error("The character '&c' is not legal in this context", next_char);
    return kERR_UserError;
}

ErrorType AGS::Scanner::GetNextSymbol(Symbol &symbol, bool &eof_encountered)
{
    symbol = -1;
    std::string symstring;
    ScanType scan_type;

    while (true)
    {
        ErrorType retval = GetNextSymstring(symstring, scan_type, eof_encountered);
        if (retval < 0) return retval;
        if (eof_encountered) return kERR_None;

        if (kSct_SectionChange != scan_type)
            break;
        retval = _ocMatcher.EndOfInputCheck();
        if (retval < 0) return retval;

        NewSection(symstring);
    }

    ErrorType retval = SymstringToSym(symstring, scan_type, symbol, eof_encountered);
    if (retval < 0) return retval;
    if (eof_encountered) return kERR_None;

    return CheckMatcherNesting(symbol);
}

ErrorType AGS::Scanner::SkipWhitespace(bool &eof_encountered)
{
    while (true)
    {
        int const ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered) return kERR_None;
        if (_inputStream.fail())
        {
            Error("Error whilst skipping whitespace (file corrupt?)");
            return kERR_UserError;
        }

        if (!IsSpace(ch))
        {
            _inputStream.putback(ch);
            return kERR_None;
        }

        // Gobble the CR of a CRLF combination
        if ('\r' == ch && '\n' == _inputStream.peek())
            continue;

        if ('\n' == ch)
            NewLine(_lineno + 1);
    }
}

ErrorType AGS::Scanner::ReadInNumberLit(std::string &symstring, ScanType &scan_type, bool &eof_encountered)
{
    bool decimal_point_encountered = (std::string::npos != symstring.find('.'));
    bool e_encountered = false;
    bool eminus_encountered = false;

    scan_type = (decimal_point_encountered) ? kSct_FloatLiteral : kSct_IntLiteral;
    symstring.push_back(_inputStream.get());

    while (true)
    {
        int const ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered) return kERR_None;
        if (_inputStream.fail())
        {
            Error("Read error encountered while scanning a number literal (file corrupt?)");
            return kERR_UserError;
        }

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
    return kERR_None;
}

ErrorType AGS::Scanner::ReadInCharLit(std::string &symstring, bool &eof_encountered)
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
            Error("Expected a character after the quote mark but input ended instead");
            return kERR_UserError;
        }
        if (_inputStream.fail())
            break; // to error processing

        if ('\\' == lit_char)
        {
            // The next char is escaped
            lit_char = _inputStream.get();
            if (_inputStream.eof())
            {
                Error("Expected a character after the '\\' but input ended instead");
                return kERR_UserError;
            }
            if (_inputStream.fail())
                break; // to error processing

            if ('[' == lit_char)
            {
                // "\\[" is equivalent to two characters, so can't be used as a single character
                Error("'\\[' is not allowed in single quotes, use '[' instead");
                return kERR_UserError;
            }

            ErrorType retval = EscapedChar2Char(lit_char, lit_char);
            if (retval < 0) return retval;
        }

        // Closing '\''
        int const ch = _inputStream.get();
        if (_inputStream.eof())
        {
            Error("Expected an apostrophe but input ended instead");
            return kERR_UserError;
        }
        if (_inputStream.fail())
            break; // to error processing

        if (ch != '\'')
        {
            Error("Expected apostrophe but found '%c' instead", ch);
            return kERR_UserError;
        }
        // Convert the char literal to an int literal
        // Note: We do NOT return the char literal, 
        // we return an equivalent integer literal, instead.
        symstring = std::to_string(lit_char);
        return kERR_None;
    }
    while (false);

    // Here when we got a read error
    Error("Read error encountered while scanning a char literal (file corrupt?)");
    return kERR_UserError;
}

int AGS::Scanner::OctDigits2Char(int first_digit_char)
{
    int ret = first_digit_char - '0';
    for (size_t digit_idx = 0; digit_idx < 2; ++digit_idx)
    {
        int const digit = _inputStream.peek() - '0';
        if (digit < 0 || digit >= 8)
            break;
        int new_value = 8 * ret + digit;
        if (new_value > 255)
            break;
        ret = new_value;
        _inputStream.get(); // Eat the digit char
    }
    return ret - 256 * (ret > 127); // convert unsigned to signed
}

int AGS::Scanner::HexDigits2Char()
{
    int ret = 0;
    for (size_t digit_idx = 0; digit_idx < 2; ++digit_idx)
    {
        int hexdigit = _inputStream.peek();
        //convert a..f to A..F
        if (hexdigit >= 'a')
            hexdigit = hexdigit - 'a' + 'A';
        if (hexdigit < '0' || (hexdigit > '9' && hexdigit < 'A') || hexdigit > 'F')
            break;
        hexdigit -= '0';
        if (hexdigit > 9)
            hexdigit -= ('@' - '9');
        ret = 16 * ret + hexdigit;
        _inputStream.get(); // Eat the hexdigit
    }
    return ret - 256 * (ret > 127); // convert unsigned to signed
}

ErrorType AGS::Scanner::EscapedChar2Char(int first_char_after_backslash, int &converted)
{
    if ('0' <= first_char_after_backslash && first_char_after_backslash < '8')
    {
        converted = OctDigits2Char(first_char_after_backslash);
        return kERR_None;
    }
    if ('x' == first_char_after_backslash)
    {
        int hexdigit = _inputStream.peek();
        if (!(('0' <= hexdigit && hexdigit <= '9') ||
            ('A' <= hexdigit && hexdigit <= 'F') ||
            ('a' <= hexdigit && hexdigit <= 'f')))
        {
            Error("Expected a hex digit to follow '\\x' in a string or char literal, found '%c' instead", hexdigit);
            return kERR_UserError;
        }
        converted = HexDigits2Char();
        return kERR_None;
    }

    switch (first_char_after_backslash)
    {
    default: break;
    case '\'': converted = '\''; return kERR_None;
    case '\"': converted = '\"'; return kERR_None;
    case '?': converted = '\?'; return kERR_None;
    case 'a': converted = '\a'; return kERR_None;
    case 'b': converted = '\b'; return kERR_None;
    case 'e': converted = 27; return kERR_None; // escape char
    case 'f': converted = '\f'; return kERR_None;
    case 'n': converted = '\n'; return kERR_None;
    case 'r': converted = '\r'; return kERR_None;
    case 't': converted = '\t'; return kERR_None;
    case 'v': converted = '\v'; return kERR_None;
    }
    Error("Unrecognized '\\%c' in character or string literal", first_char_after_backslash);
    return kERR_UserError;
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

ErrorType AGS::Scanner::ReadInStringLit(std::string &symstring, bool &eof_encountered)
{
    symstring = "";
    bool error_encountered = false;
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
            eof_encountered = _inputStream.eof();
            error_encountered = _inputStream.fail();
            if (eof_encountered || error_encountered || ch == '\n' || ch == '\r')
                break; // to error msg
            if ('[' == ch)
            {
                symstring.append("\\[");
            }
            else
            {
                int converted;
                ErrorType retval = EscapedChar2Char(ch, converted);
                if (retval < 0) return retval;
                symstring.push_back(converted);
            }
            continue;
        }

        if (ch == '"')
            return kERR_None; // End of string

        symstring.push_back(ch);
    }

    // Here when an error or eof occurs.
    if (eof_encountered)
        Error("End of input encountered when scanning a string literal (did you forget a '\"\'?)");
    else if (error_encountered)
        Error("Read error encountered while scanning a string literal (file corrupt?)");
    else  
        Error("End of line encountered when scanning a string literal, this isn't allowed (use '[' for newline)");
    return kERR_UserError;
}

ErrorType AGS::Scanner::ReadInIdentifier(std::string &symstring, bool &eof_encountered)
{
    symstring.assign(1, _inputStream.get());

    while (true)
    {
        int ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered) return kERR_None;
        if (_inputStream.fail())
        {
            Error("Read error encountered while scanning an identifier (file corrupt?)");
            return kERR_UserError;
        }

        if (IsUpper(ch) || IsLower(ch) || IsDigit(ch) || (ch == '_'))
        {
            symstring.push_back(ch);
            continue;
        }
        // That last char doesn't belong to the literal, so put it back.
        _inputStream.putback(ch);
        return kERR_None;
    }
}

ErrorType AGS::Scanner::ReadIn1or2Char(const std::string &possible_second_chars, std::string &symstring, bool &eof_encountered)
{
    symstring.assign(1, _inputStream.get());
    int const second_char = _inputStream.peek();
    eof_encountered = _inputStream.eof();
    if (eof_encountered) return kERR_None;
    if (_inputStream.fail())
    {
        Error("Read error encountered (file corrupt?)");
        return kERR_UserError;
    }

    if (std::string::npos != possible_second_chars.find(second_char))
    {
        _inputStream.get(); // Gobble the character that was peek()ed
        symstring.push_back(second_char);
    }
    return kERR_None;
}

ErrorType AGS::Scanner::ReadIn1Char(std::string &symstring)
{
    symstring.assign(1, _inputStream.get());
    return kERR_None;
}

ErrorType AGS::Scanner::ReadInDotCombi(std::string &symstring, ScanType &scan_type, bool &eof_encountered)
{
    symstring.assign(1, _inputStream.get());
    int const second_char = _inputStream.peek();
    eof_encountered = _inputStream.eof();
    if (eof_encountered) return kERR_None;
    if (_inputStream.fail())
    {
        Error("Read error encountered (file corrupt?)");
        return kERR_UserError;
    }

    if (IsDigit(second_char))
        return ReadInNumberLit(symstring, scan_type, eof_encountered);

    if ('.' != second_char)
        return kERR_None;

    symstring.push_back(_inputStream.get());

    if ('.' != _inputStream.get())
    {
        Error("Must either use '.' or '...'");
        return kERR_UserError;
    }

    symstring.push_back('.');
    return kERR_None;
}

ErrorType AGS::Scanner::ReadInLTCombi(std::string &symstring, bool &eof_encountered)
{
    ErrorType retval = ReadIn1or2Char("<=", symstring, eof_encountered);
    if (retval < 0) return retval;
    if (eof_encountered) return kERR_None;

    if ((symstring == "<<") && (_inputStream.peek() == '='))
        symstring.push_back(_inputStream.get());
    return kERR_None;
}

ErrorType AGS::Scanner::ReadInGTCombi(std::string &symstring, bool &eof_encountered)
{
    ErrorType retval = ReadIn1or2Char(">=", symstring, eof_encountered);
    if (retval < 0) return retval;
    if (eof_encountered) return kERR_None;

    if ((symstring == ">>") && (_inputStream.peek() == '='))
        symstring.push_back(_inputStream.get());
    return kERR_None;
}

ErrorType AGS::Scanner::SymstringToSym(std::string const &symstring, ScanType scan_type, Symbol &symb, bool eof_encountered)
{
    std::string const name =
        (kSct_StringLiteral == scan_type) ? MakeStringPrintable(symstring) : symstring;
    symb = _sym.FindOrAdd(name);
    if (symb < 0)
    {
        Error("Symbol table overflow - could not add new symbol");
        return kERR_InternalError;
    }

    switch (scan_type)
    {
    default:
        return kERR_None;

    case Scanner::kSct_StringLiteral:
        _sym[symb].SType = kSYM_LiteralString;
        _sym[symb].Vartype = _sym.GetOldStringSym();
        _sym[symb].SOffset = _stringCollector.add_string(symstring.c_str());
        return kERR_None;

    case Scanner::kSct_IntLiteral:
        _sym[symb].SType = kSYM_LiteralInt;
        return kERR_None;

    case Scanner::kSct_FloatLiteral:
        _sym[symb].SType = kSYM_LiteralFloat;
        return kERR_None;
    }
    // Can't reach.
}

AGS::Scanner::OpenCloseMatcher::OpenInfo::OpenInfo(std::string const &opener, std::string const &closer, size_t pos)
    : Opener(opener)
    , Closer(closer)
    , Pos(pos)
{
}

AGS::Scanner::OpenCloseMatcher::OpenCloseMatcher(Scanner &scanner)
    :_scanner(scanner)
{
}

void AGS::Scanner::OpenCloseMatcher::Push(std::string const &opener, std::string const &expected_closer, size_t opener_pos)
{
    _openInfoStack.emplace_back(opener, expected_closer, opener_pos);
}

ErrorType AGS::Scanner::OpenCloseMatcher::PopAndCheck(std::string const &closer, size_t closer_pos)
{
    if (_openInfoStack.empty())
    {
        _scanner.Error("There isn't any opening symbol that matches the closing '%s'", closer.c_str());
        return kERR_UserError;
    }

    struct OpenInfo const oi = _openInfoStack.back();
    _openInfoStack.pop_back();
    if (closer == oi.Closer)
        return kERR_None;

    size_t const opener_section_id = _scanner._tokenList.GetSectionIdAt(oi.Pos);
    std::string const &opener_section = _scanner._tokenList.SectionId2Section(opener_section_id);
    size_t const opener_lineno = _scanner._tokenList.GetLinenoAt(oi.Pos);

    std::string const &closer_section = _scanner._section;
    size_t const closer_lineno = _scanner._lineno;

    std::string error_msg = "Found '&closer&', this does not match the '&opener&' in &section& on line &lineno&";
    if (closer_section == opener_section)
    {
        error_msg = "Found '&closer&', this does not match the '&opener&' on line &lineno&";
        if (closer_lineno == opener_lineno)
            error_msg = "Found '&closer&', this does not match the '&opener&' on this line";
    }
    ReplaceToken(error_msg, "&closer&", closer);
    ReplaceToken(error_msg, "&opener&", oi.Opener);
    if (std::string::npos != error_msg.find("&lineno&"))
        ReplaceToken(error_msg, "&lineno&", std::to_string(opener_lineno));
    if (std::string::npos != error_msg.find("&section&"))
        ReplaceToken(error_msg, "&section&", opener_section);
    _scanner.Error(error_msg.c_str());
    return kERR_UserError;
}

ErrorType AGS::Scanner::OpenCloseMatcher::EndOfInputCheck()
{
    if (_openInfoStack.empty())
        return kERR_None;

    struct OpenInfo const oi = _openInfoStack.back();
    size_t const opener_section_id = _scanner._tokenList.GetSectionIdAt(oi.Pos);
    std::string const &opener_section = _scanner._tokenList.SectionId2Section(opener_section_id);
    size_t const opener_lineno = _scanner._tokenList.GetLinenoAt(oi.Pos);

    std::string const &current_section = _scanner._section;
    size_t const current_lineno = _scanner._lineno;

    std::string error_msg = "The '&opener&' in &section& on line &lineno& has not been closed.";
    if (opener_section == current_section)
    {
        error_msg = "The '&opener&' on line &lineno& has not been closed.";
        if (opener_lineno == current_lineno)
            error_msg = "The '&opener&' on this line has not been closed.";
    }
    ReplaceToken(error_msg, "&opener&", oi.Opener);
    if (std::string::npos != error_msg.find("&lineno&"))
        ReplaceToken(error_msg, "&lineno&", std::to_string(opener_lineno));
    if (std::string::npos != error_msg.find("&section&"))
        ReplaceToken(error_msg, "&section&", opener_section);
    _scanner.Error(error_msg.c_str());
    return kERR_UserError;
}

// Check the nesting of () [] {}, error if mismatch
ErrorType AGS::Scanner::CheckMatcherNesting(Symbol token)
{
    switch (_sym.GetSymbolType(token))
    {
    default:
        return kERR_None;

    case kSYM_CloseBrace:
    case kSYM_CloseBracket:
    case kSYM_CloseParenthesis:
        return _ocMatcher.PopAndCheck(_sym[token].SName, _tokenList.Length());

    case kSYM_OpenBrace:
        _ocMatcher.Push("{", "}", _tokenList.Length());
        return kERR_None;

    case kSYM_OpenBracket:
        _ocMatcher.Push("[", "]", _tokenList.Length());
        return kERR_None;

    case kSYM_OpenParenthesis:
        _ocMatcher.Push("(", ")", _tokenList.Length());
        return kERR_None;
    }
    // Can't reach
}

void AGS::Scanner::Error(char const *msg, ...)
{
    va_list vlist1, vlist2;
    va_start(vlist1, msg);
    va_copy(vlist2, vlist1);
    char *message = new char[vsnprintf(nullptr, 0, msg, vlist1) + 1];
    vsprintf(message, msg, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _messageHandler.AddMessage(MessageHandler::kSV_Error, _section, _lineno, message);

    delete[] message;
}
