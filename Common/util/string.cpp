
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "util/string.h"
#include "util/math.h"

namespace AGS
{
namespace Common
{

CString::CString()
    : _cstr     (NULL)
    , _length   (0)
{
}

CString::CString(const CString &str)
    : _cstr     (NULL)
    , _length   (0)
{
    *this = str;
}

CString::CString(const char *cstr)
    : _cstr     (NULL)
    , _length   (0)
{
    *this = cstr;
}

CString::CString(const char *cstr, int length)
    : _cstr     (NULL)
    , _length   (0)
{
    _length = length;
    _cstr = new char[_length + 1];
    memcpy(_cstr, cstr, sizeof(char) * _length);
    _cstr[_length] = 0;
}

CString::CString(char c, int count)
    : _cstr     (NULL)
    , _length   (0)
{
    FillString(c, count);
}

CString::~CString()
{
    delete [] _cstr;
}

char *CString::GetBuffer(int length)
{
    if (_length < length)
    {
        SetLength(length);
    }

    // Actually should return a writable buffer and copy buffer
    // contents to _cstr when ReleaseBuffer is called
    return _cstr;
}

void CString::ReleaseBuffer(int set_length)
{
    // Should copy buffer contents to _cstr here
}

int CString::Compare(const char *cstr) const
{
    return strcmp(_cstr, cstr);
}

int CString::CompareNoCase(const char *cstr) const
{
    return stricmp(_cstr, cstr);
}

void CString::FillString(char c, int count)
{
    SetLength(count);
    memset(_cstr, c, count);
}

void CString::Append(const CString &str)
{
    char *newcstr = new char[_length + str._length + 1];
    memcpy(newcstr, _cstr, sizeof(char) * _length);
    memcpy(newcstr + sizeof(char) * _length, str._cstr, sizeof(char) * str._length);
    _length += str._length;
    newcstr[_length] = 0;
    delete [] _cstr;
    _cstr = newcstr;
}

void CString::Append(const char *cstr)
{
    int add_length = strlen(cstr);
    char *newcstr = new char[_length + add_length + 1];
    memcpy(newcstr, _cstr, sizeof(char) * _length);
    memcpy(newcstr + sizeof(char) * _length, _cstr, sizeof(char) * add_length);
    _length += add_length;
    newcstr[_length] = 0;
    delete [] _cstr;
    _cstr = newcstr;
}

void CString::AppendChar(char c)
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

char CString::GetAt(int index) const
{
    if (index < 0 || index >= _length)
    {
        return 0;
    }

    return _cstr[index];
}

void CString::SetAt(int index, char c)
{
    if (index < 0 || index >= _length)
    {
        return;
    }

    _cstr[index] = c;
}

CString CString::Left(int count) const
{
    count = Math::Min(count, _length);
    return CString(_cstr, count);
}

CString CString::Mid(int from) const
{
    int count = _length;
    Math::ClampLength(0, _length, from, count);
    return CString(_cstr + from, count);
}

CString CString::Mid(int from, int count) const
{
    Math::ClampLength(0, _length, from, count);
    return CString(_cstr + from, count);
}

CString CString::Right(int count) const
{
    count = Math::Min(count, _length);
    int from = _length - count;
    return CString(_cstr + from, count);
}

void CString::Format(const char *fcstr, ...)
{
    delete [] _cstr;
    va_list argptr;
    va_start(argptr, fcstr);
    _length = _vscprintf(fcstr, argptr);
    _cstr = new char[_length + 1];
    vsprintf(_cstr, fcstr, argptr);
    _cstr[_length] = 0;
    va_end(argptr);
}

/*static*/ CString CString::MakeString(const char *fcstr, ...)
{
    CString s;
    va_list argptr;
    va_start(argptr, fcstr);
    s._length = _vscprintf(fcstr, argptr);
    s._cstr = new char[s._length + 1];
    vsprintf(s._cstr, fcstr, argptr);
    s._cstr[s._length] = 0;
    va_end(argptr);
    return s;
}

/*
CString::operator const char *() const
{
    return _cstr;
}
*/

CString &CString::operator=(const CString& str)
{
    delete [] _cstr;
    _length = str._length;
    _cstr = new char[_length + 1];
    memcpy(_cstr, str._cstr, sizeof(char) * _length);
    _cstr[_length] = 0;
    return *this;
}

CString &CString::operator=(const char *cstr)
{
    delete [] _cstr;
    _length = strlen(cstr);
    _cstr = new char[_length + 1];
    memcpy(_cstr, cstr, sizeof(char) * _length);
    _cstr[_length] = 0;
    return *this;
}

void CString::SetLength(int new_length)
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
