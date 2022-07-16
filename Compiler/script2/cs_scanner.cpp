#include <string>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cstdarg>

#include "cc_internallist.h"    // SrcList
#include "cs_parser_common.h"

#include "cs_scanner.h"

//                                                      123456789a1234567
std::string const AGS::Scanner::kNewSectionLitPrefix = "__NEWSCRIPTSTART_";
size_t const AGS::Scanner::kNewSectionLitPrefixSize = 17u;

AGS::Scanner::Scanner(std::string const &input, SrcList &token_list, ccCompiledScript &string_collector, SymbolTable &symt, MessageHandler &messageHandler)
    : _ocMatcher(*this)
    , _lineno(1u)
    , _tokenList(token_list)
    , _msgHandler(messageHandler)
    , _sym(symt)
    , _stringCollector(string_collector)
{
    _section = token_list.SectionId2Section(token_list.GetSectionIdAt(0));
    _inputStream.str(input);
}

void AGS::Scanner::Scan()
{
    try
    {
        while (!EOFReached() && !Failed())
        {
            Symbol symbol;
            GetNextSymbol(symbol);
            if (kKW_NoSymbol != symbol)
                _tokenList.Append(symbol);
        }
        _ocMatcher.EndOfInputCheck();
    }
    catch (ScanningError &)
    {
        // Message handler already has the error, can simply continue
    }
    catch (std::exception const &e)
    {
        std::string msg = "Exception encountered: ";
        msg += e.what();
        _msgHandler.AddMessage(
            MessageHandler::kSV_InternalError,
            _section,
            _lineno,
            msg);
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

void AGS::Scanner::GetNextSymstring(std::string &symstring, ScanType &scan_type, CodeCell &value)
{
    symstring = "";
    scan_type = kSct_Unspecified;
    value = 0;

    SkipWhitespace();
    if (EOFReached()) return;

    int const next_char = Peek();
    if (EOFReached()) return;
    if (Failed())
        UserError("Error reading a character (file corrupt?)");

    // Integer or float literal
    if (IsDigit(next_char))
    {
        symstring = "";
        return ReadInNumberLit(symstring, scan_type, value);
    }

    if ('.' == next_char)
    {
        Get();
        int nextnext = Peek();
        if (IsDigit(nextnext))
        {
            symstring = ".";
            return ReadInNumberLit(symstring, scan_type, value);
        }
        UnGet();
    }

    // Character literal
    if ('\'' == next_char)
    {
        scan_type = kSct_IntLiteral;
        return ReadInCharLit(symstring, value);
    }

    // Identifier or keyword
    if (IsAlpha(next_char) || '_' == next_char)
    {
        scan_type = kSct_Identifier;
        return ReadInIdentifier(symstring);
    }

    // String literal
    if ('"' == next_char)
    {
        std::string valstring;
        ReadInStringLit(symstring, valstring);
        
        size_t const len = kNewSectionLitPrefix.length();
        if (kNewSectionLitPrefix == valstring.substr(0, len))
        {
            symstring = valstring.substr(len);
            scan_type = kSct_SectionChange;
            value = 0;
            return;
        }

        scan_type = kSct_StringLiteral;
        value = _stringCollector.AddString(valstring.c_str());
        return;
    }

    // Non-char symstrings, such as "*="
    scan_type = kSct_NonAlphanum;
    switch (next_char)
    {
    default:  break;
    case '!': return ReadIn1or2Char("=", symstring);
    case '%': return ReadIn1or2Char("=", symstring);
    case '&': return ReadIn1or2Char("&=", symstring);
    case '(': return ReadIn1Char(symstring);
    case ')': return ReadIn1Char(symstring);
    case '*': return ReadIn1or2Char("=", symstring);
    case '+': return ReadIn1or2Char("+=", symstring);
    case ',': return ReadIn1Char(symstring);
    case '-': return ReadIn1or2Char("-=>", symstring);
        // Note, this can overwrite scan_type.
    case '.': return ReadInDotCombi(symstring, scan_type);
        // Note that the input is pre-processed,
        // so it cannot contain comments of the form //...EOL or /*...*/
    case '/': return ReadIn1or2Char("=", symstring);
    case ':': return ReadIn1or2Char(":", symstring);
    case ';': return ReadIn1Char(symstring);
    case '<': return ReadInLTCombi(symstring);
    case '=': return ReadIn1or2Char("=", symstring);
    case '>': return ReadInGTCombi(symstring);
    case '?': return ReadIn1Char(symstring);
    case '[': return ReadIn1Char(symstring);
    case ']': return ReadIn1Char(symstring);
    case '^': return ReadIn1or2Char("=", symstring);
    case '{': return ReadIn1Char(symstring);
    case '|': return ReadIn1or2Char("=|", symstring);
    case '}': return ReadIn1Char(symstring);
    case '~': return ReadIn1Char(symstring);
    }

    // Here when we don't know how to process the next char to be read
    UserError("The character '%c' is not legal in this context", next_char);
}

int AGS::Scanner::GetNextSymstringT(std::string &symstring, ScanType &scan_type, CodeCell &value)
{
    try
    {
        GetNextSymstring(symstring, scan_type, value);
    }
    catch (...)
    {
        return -1;
    }
    return 0;
}

void AGS::Scanner::GetNextSymbol(Symbol &symbol)
{
    symbol = kKW_NoSymbol;
    std::string symstring;
    ScanType scan_type;
    CodeCell value;

    while (true)
    {
        GetNextSymstring(symstring, scan_type, value);

        if (symstring.empty())
        {
            symbol = kKW_NoSymbol;
            return _ocMatcher.EndOfInputCheck();
        }

        if (kSct_SectionChange != scan_type)
            break;

        _ocMatcher.EndOfInputCheck();

        NewSection(symstring);
    }

    SymstringToSym(symstring, scan_type, value, symbol);
    CheckMatcherNesting(symbol);
}

void AGS::Scanner::SkipWhitespace()
{
    if (_eofReached)
        return;
    while (true)
    {
        int const ch = Get();
        if (EOFReached())
            return;
        if (Failed())
            UserError("Error whilst skipping whitespace (file corrupt?)");

        if (!IsSpace(ch))
        {
            UnGet();
            return;
        }

        // Gobble the CR of a CRLF combination
        if ('\r' == ch && '\n' == Peek())
            continue;

        if ('\n' == ch)
            NewLine(_lineno + 1);
    }
}

long long AGS::Scanner::StringToLongLong(std::string const &valstring, bool &conversion_successful) const
{
    errno = 0;
    char *endptr;
    int base = 0;
    if (valstring.length() > 1 &&
        valstring[0] == '0' &&
        IsDigit(valstring[1]))
    {
        // Force interpreting the integer as decimal instead of octal
        base = 10;
    }
    long long retval = std::strtoll(valstring.c_str(), &endptr, base);
    conversion_successful =
        (errno == 0 || errno == ERANGE) &&                  // ignore range error here
        valstring.length() == endptr - valstring.c_str();   // ensure that all chars were used up in the conversion

    return retval;
}

void AGS::Scanner::ReadInNumberLit(std::string &symstring, ScanType &scan_type, CodeCell &value)
{
    static std::string const exponent_leadin = "EePp";
    static std::string const exponent_follow = "0123456789-+";

    // Collect all the characters into symstring.
    // Collect those characters that are part of the number proper and have a meaning into valstring
    // Extend the strings as far as possible within the constraint that they still can be interpreted as long or double.
    // Let std::strtoll and std::strtod figure out just what that is.

    symstring.push_back(Get());
    std::string valstring = symstring;

    while (true)
    {
        int const ch = Get();
        if (!EOFReached() && Failed())
            UserError(
                "Error whilst reading the number literal starting with '%s' (file corrupt?)",
                valstring.c_str());

        symstring.push_back(ch);
        if ('\'' == ch)
        {
            if (!valstring.empty() && IsDigit(valstring.back()) && IsDigit(Peek()))
                continue;

            if (valstring.length() > 1 &&
                '0' == valstring[0] &&
                ('x' == valstring[1] || 'X' == valstring[1]) &&
                IsHexDigit(valstring.back()) &&
                IsHexDigit(Peek()))
                continue; 
        }
        valstring.push_back(ch);

        if (2 == valstring.length() &&
            ("0x" == valstring || "0X" == valstring) &&
            IsHexDigit(Peek()))
            continue; // Is neither an int nor a float yet but will become a number in later loop traversals

        if (std::string::npos != exponent_leadin.find(ch) &&
            std::string::npos != exponent_follow.find(Peek()))
            continue; // Is neither an int nor a float yet but will become a number in later loop traversals

        if (('-' == ch || '+' == ch) &&
            IsDigit(Peek()) &&
            valstring.length() > 1 &&
            std::string::npos != exponent_leadin.find(valstring[valstring.length() - 2]))
            continue; // Is neither an int nor a float yet but will become a number in later loop traversals

        // Test convert to a long long (!) so that -LONG_MIN is still within the range that we must allow.
        bool can_be_an_integer;
        StringToLongLong(valstring, can_be_an_integer);
        if (can_be_an_integer)
            continue;

        errno = 0;
        char *endptr;
        std::strtof(valstring.c_str(), &endptr);
        bool const can_be_a_floating_point =
            (errno == 0 || errno == ERANGE) &&                  // range errors will be treated below
            (valstring.length() == endptr - valstring.c_str()); // ensure that all chars are used up in the conversion
        if (can_be_a_floating_point)
            continue;

        // So this last char can't belong to the number
        symstring.pop_back();
        valstring.pop_back();
        UnGet();
        break;
    }

    // We've read in the number completely: Figure out what it is

    bool can_be_an_integer;
    // Convert to a long long (!) so that -LONG_MIN is still within the range that we must allow.
    long long longlong_value = StringToLongLong(valstring.c_str(), can_be_an_integer);
    if (can_be_an_integer)
    {
        if (valstring.length() > 1 && '0' == valstring[0] && IsDigit(valstring[2]))
            Warning("'%s' is interpreted as a number in decimal notation", symstring.c_str());

        if (longlong_value > LONG_MAX)
        {
            if (valstring.length() > 2 &&
                IsDigit(valstring[0]) &&
                IsDigit(valstring[1]) &&
                longlong_value == -static_cast<long long>(LONG_MIN))
            {
                // Special case. This is out-of-range for integers,
                // but might still be allowed when preceded by a _unary_ minus.
                // Only the parser can decide whether a minus is unary,
                // so let this through here. A code cell is too small to hold
                // this value, so return a dedicated scan type for this situation
                scan_type = kSct_OnePastLongMax;
                value = 0;
                return;
            }

            if (longlong_value >= 0x80000000 &&
                valstring.length() > 1 &&
                valstring[0] == '0' &&
                (valstring[1] == 'x' || valstring[1] == 'X'))
            {
                // Large hexadecimal
                if (longlong_value > 0xFFFFFFFF)
                    UserError(
                        "Too many significant hex digits in '%s' (at most 8 significant digits allowed)",
                        (symstring.length() <= 20? symstring : symstring.substr(0, 20) + "...").c_str());
                // 'strtoll()' has converted this hexadecimal into a value that
                // is too large for a long. However, this is still legal and
                // yields a negative long number.
                longlong_value = longlong_value - (0xFFFFFFFFLL + 1LL);
            }
            else
            {
                UserError(
                    "Literal integer '%s' is out of bounds (maximum is '%d')",
                    symstring.length() <= 20 ? symstring.c_str() : (symstring.substr(0, 20) + "...").c_str(),
                    LONG_MAX);
            }
        }

        scan_type = kSct_IntLiteral;
        value = static_cast<long>(longlong_value);
        return;
    }

    errno = 0;
    float float_value = std::strtof(valstring.c_str(), nullptr);
    if (ERANGE == errno)
        UserError(
            "Literal float '%s' is out of bounds (maximum is '%.3G')",
            symstring.c_str(),
            static_cast<double>(std::numeric_limits<float>::max()));
    if (errno != 0)
        UserError("Expected a number literal, found '%s' instead", symstring.c_str());   

    scan_type = kSct_FloatLiteral;
    value = *reinterpret_cast<CodeCell *>(&float_value);
    return;
}

void AGS::Scanner::ReadInCharLit(std::string &symstring, CodeCell &value)
{
    symstring = "";

    do // exactly 1 time
    {
        // Opening '\''
        symstring.push_back(Get());

        // The character inside
        int lit_char = Get();
        symstring.push_back(lit_char);
        if (EOFReached())
            UserError("Expected a character after the quote mark but input ended instead");
        if (Failed())
            break; // to error processing

        if ('\\' == lit_char)
        {
            // The next char is escaped
            lit_char = Get();
            symstring.push_back(lit_char);
            if (EOFReached())
                UserError("Expected a character after the backslash but input ended instead");
            if (Failed())
                break; // to error processing

            if ('[' == lit_char)
                // "\\[" is equivalent to two characters, so can't be used as a single character
                UserError("'\\[' is not allowed in single quotes, use '[' instead");

            EscapedChar2Char(lit_char, symstring, lit_char);
        }

        // Closing '\''
        int const closer = Get();
        symstring.push_back(closer);
        if (EOFReached())
            UserError("Expected a quote mark but input ended instead");
        if (Failed())
            break; // to error processing

        if ('\'' != closer)
            UserError("Expected a quote mark but found '%c' instead", closer);
        value = lit_char;
        return;
    }
    while (false);

    UserError("Read error while scanning a char literal (file corrupt?)");
}

int AGS::Scanner::OctDigits2Char(int first_digit_char, std::string &symstring)
{
    int ret = first_digit_char - '0';
    for (size_t digit_idx = 0; digit_idx < 2; ++digit_idx)
    {
        int const digit = Peek() - '0';
        if (digit < 0 || digit >= 8)
            break;
        int new_value = 8 * ret + digit;
        if (new_value > 255)
            break;
        ret = new_value;
        symstring.push_back(Get()); // Eat the digit char
    }
    return ret - 256 * (ret > 127); // convert unsigned to signed
}

int AGS::Scanner::HexDigits2Char(std::string &symstring)
{
    int ret = 0;
    for (size_t digit_idx = 0; digit_idx < 2; ++digit_idx)
    {
        int hexdigit = Peek();
        //convert a..f to A..F
        if (hexdigit >= 'a')
            hexdigit = hexdigit - 'a' + 'A';
        if (hexdigit < '0' || (hexdigit > '9' && hexdigit < 'A') || hexdigit > 'F')
            break;
        hexdigit -= '0';
        if (hexdigit > 9)
            hexdigit -= ('@' - '9');
        ret = 16 * ret + hexdigit;
        symstring.push_back(Get()); // Eat the hexdigit
    }
    return ret - 256 * (ret > 127); // convert unsigned to signed
}

void AGS::Scanner::EscapedChar2Char(int first_char_after_backslash, std::string &symstring, int &converted)
{
    if ('0' <= first_char_after_backslash && first_char_after_backslash < '8')
    {
        converted = OctDigits2Char(first_char_after_backslash, symstring);
        return;
    }
    if ('x' == first_char_after_backslash)
    {
        int hexdigit = Peek();
        if (!(('0' <= hexdigit && hexdigit <= '9') ||
            ('A' <= hexdigit && hexdigit <= 'F') ||
            ('a' <= hexdigit && hexdigit <= 'f')))
            UserError("Expected a hex digit to follow '\\x' in a string or char literal, found '%c' instead", hexdigit);

        converted = HexDigits2Char(symstring);
        return;
    }

    switch (first_char_after_backslash)
    {
    default:
        break;

    case '\'': 
    case '"': 
    case '?': 
    case '\\':
        converted = first_char_after_backslash;
        return;

    case 'a': converted = '\a'; return;
    case 'b': converted = '\b'; return;
    case 'e': converted = 27;   return; // escape char
    case 'f': converted = '\f'; return;
    case 'n': converted = '\n'; return;
    case 'r': converted = '\r'; return;
    case 't': converted = '\t'; return;
    case 'v': converted = '\v'; return;
    }
    UserError("Unrecognized '\\%c' in character or string literal", first_char_after_backslash);
}

void AGS::Scanner::ReadInStringLit(std::string &symstring, std::string &valstring)
{
    symstring = "\"";
    valstring = "";

    Get(); // Eat '"';

    while (true)
    {
        int ch = Get();
        symstring.push_back(ch);
        if (EOFReached() || Failed() || '\n' == ch || '\r' == ch)
            break; // to error msg

        if ('\\' == ch)
        {
            ch = Get();
            symstring.push_back(ch);
            if (EOFReached() || Failed() || '\n' == ch || '\r' == ch)
                break; // to error msg
            if ('[' == ch)
            {
                valstring.append("\\[");
            }
            else
            {
                int converted;
                EscapedChar2Char(ch, symstring, converted);
                valstring.push_back(converted);
            }
            continue;
        }

        if (ch == '"')
        {
            // Except for string literals that are new section markers really,
            // if whitespace and another string literal follows, the literals must be concatenated.
            // However, if not, then the whitespace must not be consumed at this point.
            // Implement this by undoing the SkipWhitespace() in that case.
            // Save what may need to be undone.
            std::streampos pos_before_skip = _inputStream.tellg();
            size_t lineno_before_skip = _lineno;

            SkipWhitespace();

            if ('"' != Peek() ||
                kNewSectionLitPrefix == valstring.substr(0, kNewSectionLitPrefix.length()))
            {
                _inputStream.seekg(pos_before_skip);
                _lineno = lineno_before_skip;
                return;
            }

            // Another string literal follows
            // Tentatively read ahead to check whether it's a new section marker
            pos_before_skip = _inputStream.tellg();
            lineno_before_skip = _lineno;
            Get(); // Eat leading '"'
            char tbuffer[kNewSectionLitPrefixSize + 1];
            _inputStream.get(tbuffer, kNewSectionLitPrefixSize + 1, 0);
            _eofReached |= _inputStream.eof();
            _failed |= _inputStream.fail();
            // Undo the reading
            _inputStream.seekg(pos_before_skip);
            _lineno = lineno_before_skip;

            if (kNewSectionLitPrefix == tbuffer)
                return; // do not concatenate this new section marker

            // Concatenate
            symstring.pop_back(); // Delete quote
            Get(); // Eat quote
            continue;
        }           

        valstring.push_back(ch);
    }

    // Here when an error or eof occurs.
    if (EOFReached())
        UserError("Input ended within a string literal (did you forget a '\"\'?)");
    else if (Failed())
        UserError("Read error while scanning a string literal (file corrupt?)");
    else  
        UserError("Line ended within a string literal, this isn't allowed (use '[' for newline)");
}

void AGS::Scanner::ReadInIdentifier(std::string &symstring)
{
    symstring.assign(1, Get());

    while (true)
    {
        int ch = Get();
        if (EOFReached()) return;
        if (Failed())
            UserError("Read error while scanning an identifier (file corrupt?)");

        if (IsAlpha(ch) || IsDigit(ch) || '_' == ch)
        {
            symstring.push_back(ch);
            continue;
        }
        // That last char doesn't belong to the literal, so put it back.
        UnGet();
        return;
    }
}

void AGS::Scanner::ReadIn1or2Char(const std::string &possible_second_chars, std::string &symstring)
{
    symstring.assign(1, Get());
    int const second_char = Peek();
    if (EOFReached()) return;
    if (Failed())
        UserError("Read error (file corrupt?)");

    if (std::string::npos != possible_second_chars.find(second_char))
    {
        Get(); // Gobble the character that was peek()ed
        symstring.push_back(second_char);
    }
    return;
}

void AGS::Scanner::ReadIn1Char(std::string &symstring)
{
    symstring.assign(1, Get());
    return;
}

void AGS::Scanner::ReadInDotCombi(std::string &symstring, ScanType &scan_type)
{
    symstring.assign(1, Get());
    int const second_char = Peek();
    if (EOFReached()) return;
    if (Failed())
        UserError("Read error (file corrupt?)");
    if ('.' != second_char)
        return;

    symstring.push_back(Get());

    if ('.' != Get())
        UserError("Must either use '.' or '...'");

    symstring.push_back('.');
    return;
}

void AGS::Scanner::ReadInLTCombi(std::string &symstring)
{
    ReadIn1or2Char("<=", symstring);
    if (EOFReached()) return;

    if (("<<" == symstring) && ('=' == Peek()))
        symstring.push_back(Get());
    return;
}

void AGS::Scanner::ReadInGTCombi(std::string &symstring)
{
    ReadIn1or2Char(">=", symstring);
    if (EOFReached()) return;

    if ((symstring == ">>") && (Peek() == '='))
        symstring.push_back(Get());
    return;
}

void AGS::Scanner::SymstringToSym(std::string const &symstring, ScanType scan_type, CodeCell value, Symbol &symb)
{
    static Symbol const const_string_vartype = _sym.VartypeWith(VTT::kConst, kKW_String);
    static const char *const one_past_long_max_string = "2147483648";

    symb = _sym.FindOrAdd(symstring);
    if (symb < 0)
        InternalError("Could not add new symbol to symbol table");

    switch (scan_type)
    {
    default:
        return;

    case Scanner::kSct_StringLiteral:
        _sym[symb].LiteralD = new SymbolTableEntry::LiteralDesc;
        _sym[symb].LiteralD->Vartype = const_string_vartype;
        _sym[symb].LiteralD->Value = value;
        return;

    case Scanner::kSct_IntLiteral:
        _sym[symb].LiteralD = new SymbolTableEntry::LiteralDesc;
        _sym[symb].LiteralD->Vartype = kKW_Int;
        _sym[symb].LiteralD->Value = value;
        return;

    case Scanner::kSct_OnePastLongMax:  // 1 plus largest signed integer
        symb = _sym.FindOrAdd(one_past_long_max_string);
        return;

    case Scanner::kSct_FloatLiteral:
        _sym[symb].LiteralD = new SymbolTableEntry::LiteralDesc;
        _sym[symb].LiteralD->Vartype = kKW_Float;
        _sym[symb].LiteralD->Value = value;
        return;
    }
    // Can't reach.
}

AGS::Scanner::OpenCloseMatcher::OpenCloseMatcher(Scanner &scanner)
    :_scanner(scanner)
{
}

void AGS::Scanner::OpenCloseMatcher::Push(Symbol opener, size_t opener_pos)
{
    _openInfoStack.push_back(OpenInfo{ opener, opener_pos });
}

void AGS::Scanner::OpenCloseMatcher::PopAndCheck(Symbol closer, size_t closer_pos)
{
    if (_openInfoStack.empty())
        _scanner.UserError("There isn't any opening symbol that matches the closing '%s'", _scanner._sym.GetName(closer).c_str());

    struct OpenInfo const oi = _openInfoStack.back();
    _openInfoStack.pop_back();
    if (closer == _scanner._sym[oi.Opener].DelimeterD->Partner)
        return;

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
    ReplaceToken(error_msg, "&closer&", _scanner._sym.GetName(closer));
    ReplaceToken(error_msg, "&opener&", _scanner._sym.GetName(oi.Opener));
    if (std::string::npos != error_msg.find("&lineno&"))
        ReplaceToken(error_msg, "&lineno&", std::to_string(opener_lineno));
    if (std::string::npos != error_msg.find("&section&"))
        ReplaceToken(error_msg, "&section&", opener_section);
    _scanner.UserError(error_msg.c_str());
}

void AGS::Scanner::OpenCloseMatcher::EndOfInputCheck()
{
    if (_openInfoStack.empty())
        return;

    struct OpenInfo const oi = _openInfoStack.back();
    size_t const opener_section_id = _scanner._tokenList.GetSectionIdAt(oi.Pos);
    _scanner._section = _scanner._tokenList.SectionId2Section(opener_section_id);
    _scanner._lineno = _scanner._tokenList.GetLinenoAt(oi.Pos);

    std::string error_msg = "The '&opener&' on this line isn't closed.";
    ReplaceToken(error_msg, "&opener&", _scanner._sym.GetName(oi.Opener));
    _scanner.UserError(error_msg.c_str());
}

// Check the nesting of () [] {}, error if mismatch
void AGS::Scanner::CheckMatcherNesting(Symbol token)
{
    switch (token)
    {
    default:
        return;

    case kKW_CloseBrace:
    case kKW_CloseBracket:
    case kKW_CloseParenthesis:
        return _ocMatcher.PopAndCheck(token, _tokenList.Length());

    case kKW_OpenBrace:
    case kKW_OpenBracket:
    case kKW_OpenParenthesis:
        _ocMatcher.Push(token, _tokenList.Length());
        return;
    }
    // Can't reach
}

void AGS::Scanner::Error(bool is_internal, std::string const &message)
{
    _msgHandler.AddMessage(
        is_internal ? MessageHandler::kSV_InternalError : MessageHandler::kSV_UserError,
        _section,
        _lineno,
        is_internal ? "Internal error: " + message :  message);

    // Set a breakpoint here to stop the scanner as soon as an error happens,
    // before the call stack has unwound:
    throw ScanningError(message);
}

void AGS::Scanner::UserError(char const *descr ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len, '\0');
    vsprintf(&message[0u], descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    Error(false, &message[0u]);
}

void AGS::Scanner::InternalError(char const *descr ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(&message[0u], descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    Error(true, &message[0u]);
}

void AGS::Scanner::Warning(char const *descr ...)
{
    // Convert the parameters into message
    va_list vlist1, vlist2;
    va_start(vlist1, descr);
    va_copy(vlist2, vlist1);
    size_t const needed_len = vsnprintf(nullptr, 0u, descr, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(&message[0u], descr, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    _msgHandler.AddMessage(
        MessageHandler::kSV_Warning,
        _section,
        _lineno,
        &message[0u]);
}
