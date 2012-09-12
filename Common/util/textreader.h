
//=============================================================================
//
// Specialized interface for reading plain text from the underlying source
//
//=============================================================================
#ifndef __AGS_CN_UTIL__TEXTREADER_H
#define __AGS_CN_UTIL__TEXTREADER_H

#include "util/string.h"

namespace AGS
{
namespace Common
{

class TextReader
{
public:
    virtual ~TextReader(){}

    virtual bool IsValid() const            = 0;

    // Read single character
    virtual char    ReadChar()              = 0;
    // Read defined number of characters
    virtual String ReadString(int length)  = 0;
    // Read till line break
    virtual String ReadLine()              = 0;
    // Read till end of available data
    virtual String ReadAll()               = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAM_H
