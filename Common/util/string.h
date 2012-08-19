
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

class CString
{
public:
    CString();
    CString(const CString&);
    CString(const char *cstr);
    CString(const char *cstr, int length);
    CString(char c, int count);
    ~CString();

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

    char                *GetBuffer(int length);
    void                ReleaseBuffer(int set_length = -1);

    int                 Compare(const char *cstr) const;
    inline int          Compare(const CString &str) const
    {
        return Compare(str.GetCStr());
    }
    int                 CompareNoCase(const char *cstr) const;
    inline int          CompareNoCase(const CString &str) const
    {
        return CompareNoCase(str.GetCStr());
    }

    void                FillString(char c, int count);
    void                Append(const CString &str);
    void                Append(const char *cstr);
    void                AppendChar(char c);

    char                GetAt(int index) const;
    void                SetAt(int index, char c);

    CString             Left(int count) const;
    CString             Mid(int from) const;
    CString             Mid(int from, int count) const;
    CString             Right(int count) const;

    void                Format(const char *fcstr, ...);
    static CString      MakeString(const char *fcstr, ...);

    //operator const char *() const;
    CString &operator=(const CString&);
    CString &operator=(const char *cstr);
    inline char operator[](int index) const
    {
        return GetAt(index);
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
