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
        _inputStream.get();

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
            // Note that AGS doesn't follow C syntax here:
            // In C, '\n' is a newline; in AGS, it is the letter 'n'.
            lit_char = _inputStream.get();
            eof_encountered = _inputStream.eof();  // This is an error
            if (eof_encountered)
            {
                error_encountered = true;
                _lastError = "Expected a character and an apostrophe, but input ended instead";
                return;
            }
            error_encountered = _inputStream.fail();
            if (error_encountered)
                break; // to error processing
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

void AGS::Scanner::ReadInStringLit(std::string &symstring, bool &eof_encountered, bool &error_encountered)
{
    symstring = "\"";
    _inputStream.get(); // We know that this is a '"'
    while (true)
    {
        int ch = _inputStream.get();
        eof_encountered = _inputStream.eof(); // This is an error, too
        error_encountered = _inputStream.fail();
        if (eof_encountered || error_encountered || (strchr("\r\n", ch) != 0))
            break; // to error msg

        symstring.push_back(ch);

        // End of string
        if (ch == '"')
            return;

        if (ch == '\\')
        {
            // Now some character MUST follow; any one is allowed, but no line changes 
            // Note that AGS doesn't follow C syntax here.
            // In C, "\n" is a newline, but in AGS, "\n" is "n".
            int ch = _inputStream.get();
            eof_encountered = _inputStream.eof(); // This is an error, too
            error_encountered = _inputStream.fail();
            if (eof_encountered || error_encountered || (strchr("\r\n", ch) != 0))
                break; // to error msg
            symstring.push_back(ch);
        }
    }
    // Here whenever an error or EOF or LF came before the string was terminated
    error_encountered = true;
    _lastError = "Incorrectly terminated string literal (Use '[' for line feed)";
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

