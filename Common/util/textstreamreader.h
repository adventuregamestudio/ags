
//=============================================================================
//
// Class for reading plain text from the stream
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TEXTSTREAMREADER_H
#define __AGS_CN_UTIL__TEXTSTREAMREADER_H

#include "util/textreader.h"

namespace AGS
{
namespace Common
{

class IStream;

class CTextStreamReader : public ITextReader
{
public:
    // TODO: use shared ptr
    CTextStreamReader(IStream *stream);
    virtual ~CTextStreamReader();

    virtual bool    IsValid() const;
    const IStream   *GetStream() const;
    // TODO: use shared ptr instead
    void            ReleaseStream();

    bool            EOS() const;

    // Read single character
    virtual char    ReadChar();
    // Read defined number of characters
    virtual CString ReadString(int length);
    // Read till line break
    virtual CString ReadLine();
    // Read till end of available data
    virtual CString ReadAll();

private:
    IStream *_stream;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAM_H
