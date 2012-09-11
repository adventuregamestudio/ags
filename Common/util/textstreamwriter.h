
//=============================================================================
//
// Class for writing plain text to the stream
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TEXTSTREAMWRITER_H
#define __AGS_CN_UTIL__TEXTSTREAMWRITER_H

#include "util/textwriter.h"

namespace AGS
{
namespace Common
{

class IStream;

class CTextStreamWriter
{
public:
    // TODO: use shared ptr
    CTextStreamWriter(IStream *stream);
    virtual ~CTextStreamWriter();

    virtual bool    IsValid() const;
    const IStream   *GetStream() const;
    // TODO: use shared ptr instead
    void            ReleaseStream();

    bool            EOS() const;

    // Write single character
    virtual void    WriteChar(char c);
    // Write string as a plain text (without null-terminator)
    virtual void    WriteString(const CString &str);
    // Write string and add line break at the end
    virtual void    WriteLine(const CString &str);
    // Write formatted string (see *printf)
    virtual void    WriteFormat(const CString &fmt, ...);
    virtual void    WriteLineBreak();

private:
    IStream *_stream;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAMWRITER_H
