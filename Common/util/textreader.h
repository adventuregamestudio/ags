
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

class ITextReader
{
public:
    virtual ~ITextReader(){}

    virtual bool IsValid() const            = 0;

    // Read single character
    virtual char    ReadChar()              = 0;
    // Read defined number of characters
    virtual CString ReadString(int length)  = 0;
    // Read till line break
    virtual CString ReadLine()              = 0;
    // Read till end of available data
    virtual CString ReadAll()               = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__TEXTSTREAM_H
