
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

class Stream;

class TextStreamWriter : public TextWriter
{
public:
    // TODO: use shared ptr
    TextStreamWriter(Stream *stream);
    virtual ~TextStreamWriter();

    virtual bool    IsValid() const;
    const Stream   *GetStream() const;
    // TODO: use shared ptr instead
    void            ReleaseStream();

    bool            EOS() const;

    // Write single character
    virtual void    WriteChar(char c);
    // Write string as a plain text (without null-terminator)
    virtual void    WriteString(const String &str);
    // Write string and add line break at the end
    virtual void    WriteLine(const String &str);
    // Write formatted string (see *printf)
    virtual void    WriteFormat(const char *fmt, ...);
    virtual void    WriteLineBreak();

private:
    Stream *_stream;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAMWRITER_H
