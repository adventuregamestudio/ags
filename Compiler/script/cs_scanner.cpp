#include <string>
#include <sstream>
#include "cc_internallist.h"    // ccInternalList
#include "cs_parser_common.h"

#include "cs_scanner.h"

AGS::Scanner::Scanner()
    : _lineno(1)
    , _tokenList(0)
    , _lastError("")
{
}

AGS::Scanner::Scanner(std::string const &input, std::size_t lineno, ::ccInternalList *token_list)
    : _lineno(lineno)
    , _tokenList(token_list)
    , _lastError("")
{
    SetInput(input);
}

void AGS::Scanner::SetInput(const std::string &input)
{
    _inputStream.str(input);
}

void AGS::Scanner::SetLineno(std::size_t lineno)
{
    _lineno = lineno;
}

std::size_t AGS::Scanner::GetLineno()
{
    return _lineno;
}

void AGS::Scanner::SetTokenList(ccInternalList *token_list)
{
    _tokenList = token_list;
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
        scan_type = kSct_IntLiteral;
        return;
    }

    // Identifier or keyword
    if (isupper(next_char) || islower(next_char) || (next_char == '_'))
    {
        ReadInIdentifier(symstring, eof_encountered, error_encountered);
        scan_type = kSct_Identifier;
        return;
    }

    // String literal
    if (next_char == '"')
    {
        ReadInStringLit(symstring, eof_encountered, error_encountered);
        scan_type = kSct_StringLiteral;
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
    _lastError = "The character '&1' is not legal in this context";
    std::string chstring(1, next_char);
    _lastError.replace(_lastError.find("&1"), 2, chstring);
    return;
}

const std::string AGS::Scanner::GetLastError()
{
    return this->_lastError;
}


void AGS::Scanner::SkipWhitespace(bool &eof_encountered, bool &error_encountered)
{
    while (true)
    {
        int ch = _inputStream.get();
        eof_encountered = _inputStream.eof();
        if (eof_encountered)
            return;
        error_encountered = _inputStream.fail();
        if (error_encountered)
        {
            _lastError = "Error whilst skipping whitespace (file corrupt?)";
            return;
        }

        if (!isspace(ch))
        {
            _inputStream.putback(ch);
            return;
        }

        // Gobble the CR of a CRLF combination
        if ((ch == '\r') && (_inputStream.peek() == '\n'))
            continue;

        if (ch == '\n')
        {
            // Write pseudocode for increased line number
            WriteNewLinenoMeta(++_lineno);
        }
    }
}

void AGS::Scanner::ReadInNumberLit(std::string &symstring, ScanType &scan_type, bool &eof_encountered, bool &error_encountered)
{
    bool decimal_point_encountered = false;

    scan_type = kSct_IntLiteral;
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
            _lastError = "Error encountered while scanning a number literal (file corrupt?)";
            return;
        }
        if (eof_encountered || error_encountered)
            return;

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
                scan_type = kSct_FloatLiteral;
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
    int lit_char;

    do // exactly 1 time
    {
        // Opening '\''
        _inputStream.get(); // Eat '\''

        // The character inside
        lit_char = _inputStream.get();
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

        if (lit_char == '\\')
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
        int ch = _inputStream.get();
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
            std::string wrong_letter_as_string(1, ch);
            _lastError = "Expected apostrophe, but found '&1' instead";
            _lastError.replace(_lastError.find("&1"), 2, wrong_letter_as_string);
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
        int digit = _inputStream.peek();
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
    if (isdigit(ch))
        return OctChar2Char(ch);

    switch (ch)
    {
    default:
        error_encountered = true;
        _lastError = "Found unknown escape sequence '\\&' in string.";
        _lastError.replace(_lastError.find_first_of('&'), 1, 1, ch);
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

void AGS::Scanner::ReadInStringLit(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    symstring = "\"";
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
            int ch = _inputStream.get();
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

        symstring.push_back(ch);

        // End of string
        if (ch == '"')
            return;
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

        if (isupper(ch) || islower(ch) || isdigit(ch) || (ch == '_'))
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
        _lastError = "Unexpected error in input (file corrupt?";
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

void AGS::Scanner::ReadInDotCombi(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    ReadIn1or2Char(".", symstring, eof_encountered, error_encountered);
    if (eof_encountered || error_encountered)
        return;

    if (symstring == ".")
        return;

    if (_inputStream.peek() == '.')
    {
        symstring.push_back(_inputStream.get());
        return;
    }

    _lastError = "Must either use '.' or '...'";
    error_encountered = true;
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

