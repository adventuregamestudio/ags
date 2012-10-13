//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// String class.
//
// [IKM] 2012-08-17: basic variant of string class
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STRING_H
#define __AGS_CN_UTIL__STRING_H

#include "core/types.h"

namespace AGS
{
namespace Common
{

class String
{
public:
    String();
    String(const String&);
    String(const char *cstr);
    String(const char *cstr, int length);
    String(char c, int count);
    ~String();

    inline const char   *GetCStr() const
    {
        return _cstr;
    }

    inline int          GetLength() const
    {
        return _length;
    }

    inline bool         IsEmpty() const
    {
        return _length == 0;
    }

    char                *GetBuffer(int length = -1);
    void                ReleaseBuffer(int set_length = -1);

    int                 Compare(const char *cstr) const;
    int                 CompareNoCase(const char *cstr) const;
    int                 CompareLeft(const char *cstr) const;

    int                 FindChar(char c) const;
    int                 FindCharReverse(char c) const;

    void                FillString(char c, int count);
    void                Append(const char *cstr);
    void                AppendChar(char c);

    char                GetAt(int index) const;
    void                SetAt(int index, char c);

    String              ToLower() const;
    String              ToUpper() const;

    String              Left(int count) const;
    String              Mid(int from) const;
    String              Mid(int from, int count) const;
    String              Right(int count) const;

    void                Format(const char *fcstr, ...);
    static String       MakeString(const char *fcstr, ...);

    int                 ToInt() const;

    inline operator const char *() const
    {
        return _cstr;
    }
    String &operator=(const String&);
    String &operator=(const char *cstr);
    inline char operator[](int index) const
    {
        return GetAt(index);
    }
    inline bool operator==(const String &str)
    {
        return Compare(str) == 0;
    }
    inline bool operator==(const char *cstr)
    {
        return cstr ? (Compare(cstr) == 0) : IsEmpty();
    }

protected:
    void                SetLength(int new_length);

private:
    char    *_cstr;
    int     _length;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STRING_H
