
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/string.h"
#include "util/string_utils.h"
#include "util/math.h"


namespace AGS
{
namespace Common
{

String::String()
    : _cstr     (NULL)
    , _length   (0)
{
}

String::String(const String &str)
    : _cstr     (NULL)
    , _length   (0)
{
    *this = str;
}

String::String(const char *cstr)
    : _cstr     (NULL)
    , _length   (0)
{
    *this = cstr;
}

String::String(const char *cstr, int length)
    : _cstr     (NULL)
    , _length   (0)
{
    _length = length;
    _cstr = new char[_length + 1];
    if (cstr)
    {
        memcpy(_cstr, cstr, sizeof(char) * _length);
    }    
    _cstr[_length] = 0;
}

String::String(char c, int count)
    : _cstr     (NULL)
    , _length   (0)
{
    FillString(c, count);
}

String::~String()
{
    delete [] _cstr;
}

char *String::GetBuffer(int length)
{
    if (_length < length)
    {
        SetLength(length);
    }

    // TODO
    // Actually should return a writable buffer and copy buffer
    // contents to _cstr when ReleaseBuffer is called
    return _cstr;
}

void String::ReleaseBuffer(int set_length)
{
    // TODO
    // Should copy buffer contents to _cstr here
    if (set_length < 0)
    {
        _length = strlen(_cstr);
    }
    else
    {
        _length = set_length;
    }
}

int String::Compare(const char *cstr) const
{
    if (!_cstr) return -1; // CHECKME
    return strcmp(_cstr, cstr);
}

int String::CompareNoCase(const char *cstr) const
{
    if (!_cstr) return -1; // CHECKME
    return stricmp(_cstr, cstr);
}

int String::CompareLeft(const char *cstr) const
{
    if (!_cstr) return -1; // CHECKME
    int count = strlen(cstr);
    return strncmp(_cstr, cstr, count);
}

int String::FindChar(char c) const
{
    const char * p_str = strchr(_cstr, c);
    if (!p_str)
    {
        return -1;
    }
    return p_str - _cstr;
}

int String::FindCharReverse(char c) const
{
    const char * p_str = strrchr(_cstr, c);
    if (!p_str)
    {
        return -1;
    }
    return p_str - _cstr;
}

void String::FillString(char c, int count)
{
    SetLength(count);
    memset(_cstr, c, count);
}

void String::Append(const char *cstr)
{
    int add_length = strlen(cstr);
    char *newcstr = new char[_length + add_length + 1];
    memcpy(newcstr, _cstr, sizeof(char) * _length);
    memcpy(newcstr + sizeof(char) * _length, cstr, sizeof(char) * add_length);
    _length += add_length;
    newcstr[_length] = 0;
    delete [] _cstr;
    _cstr = newcstr;
}

void String::AppendChar(char c)
{
    // Yeah yeah that's totally not optimal, but that's "basic version", remember?
    // (At this point only interface is important)
    char *newcstr = new char[_length + 1 + 1];
    memcpy(newcstr, _cstr, sizeof(char) * _length);
    newcstr[_length] = c;
    _length++;
    newcstr[_length] = 0;
    delete [] _cstr;
    _cstr = newcstr;
}

char String::GetAt(int index) const
{
    if (index < 0 || index >= _length)
    {
        return 0;
    }

    return _cstr[index];
}

void String::SetAt(int index, char c)
{
    if (index < 0 || index >= _length)
    {
        return;
    }

    _cstr[index] = c;
}

String String::ToLower() const
{
    String s(*this);
    if (s._cstr)
    {
        strlwr(s._cstr);
    }
    return s;
}

String String::ToUpper() const
{
    String s(*this);
    if (s._cstr)
    {
        strupr(s._cstr);
    }
    return s;    
}

String String::Left(int count) const
{
    count = Math::Min(count, _length);
    return String(_cstr, count);
}

String String::Mid(int from) const
{
    int count = _length;
    Math::ClampLength(0, _length, from, count);
    return String(_cstr + from, count);
}

String String::Mid(int from, int count) const
{
    Math::ClampLength(0, _length, from, count);
    return String(_cstr + from, count);
}

String String::Right(int count) const
{
    count = Math::Min(count, _length);
    int from = _length - count;
    return String(_cstr + from, count);
}

void String::Format(const char *fcstr, ...)
{
    delete [] _cstr;
    va_list argptr;
    va_start(argptr, fcstr);
    _length = vsnprintf(NULL, 0, fcstr, argptr);
    va_start(argptr, fcstr); // Reset argptr
    _cstr = new char[_length + 1];
    vsprintf(_cstr, fcstr, argptr);
    _cstr[_length] = 0;
    va_end(argptr);
}

/*static*/ String String::MakeString(const char *fcstr, ...)
{
    String s;
    va_list argptr;
    va_start(argptr, fcstr);
    s._length = vsnprintf(NULL, 0, fcstr, argptr);
    va_start(argptr, fcstr); // Reset argptr
    s._cstr = new char[s._length + 1];
    vsprintf(s._cstr, fcstr, argptr);
    s._cstr[s._length] = 0;
    va_end(argptr);
    return s;
}

int String::ToInt() const
{
    if (!_length)
    {
        return 0;
    }

    return atoi(_cstr);
}

String &String::operator=(const String& str)
{
    delete [] _cstr;
    _length = str._length;
    _cstr = new char[_length + 1];
    if (str._cstr)
    {
        memcpy(_cstr, str._cstr, sizeof(char) * _length);
    }
    _cstr[_length] = 0;
    return *this;
}

String &String::operator=(const char *cstr)
{
    delete [] _cstr;
    _length = cstr ? strlen(cstr) : 0;
    _cstr = new char[_length + 1];
    if (cstr)
    {
        memcpy(_cstr, cstr, sizeof(char) * _length);
    }
    _cstr[_length] = 0;
    return *this;
}

void String::SetLength(int new_length)
{
    if (_length == new_length)
    {
        return;
    }

    char *newcstr = new char[new_length + 1];
    int copy_length = Math::Min(_length, new_length);
    memcpy(newcstr, _cstr, sizeof(char) * copy_length);
    memset(newcstr + copy_length, 0, new_length - copy_length + 1);
    _length = new_length;
    delete [] _cstr;
    _cstr = newcstr;
}

} // namespace Common
} // namespace AGS
