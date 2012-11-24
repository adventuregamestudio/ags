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
// Base stream interface
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STREAM_H
#define __AGS_CN_UTIL__STREAM_H

#include "core/types.h"

namespace AGS
{
namespace Common
{

enum StreamSeek
{
    kSeekBegin,
    kSeekEnd,
    kSeekCurrent
};

class Stream
{
public:
    virtual ~Stream(){}

    // Is stream valid (underlying data initialized properly)
    virtual bool    IsValid() const                     = 0;
    // Is end of stream
    virtual bool    EOS() const                         = 0;
    // Total length of stream (if known)
    virtual size_t  GetLength() const                   = 0;
    // Current position (if known)
    virtual size_t  GetPosition() const                 = 0;
    virtual bool    CanRead() const                     = 0;
    virtual bool    CanWrite() const                    = 0;
    virtual bool    CanSeek() const                     = 0;

    virtual void    Close()                             = 0;

    // Returns number of bytes read, or -1 if the end of stream was reached
    virtual size_t  Read(void *buffer, size_t size)        = 0;
    // Returns a value of next (unsigned) byte read from stream cast to int or,
    // or -1 if the end of stream was reached
    virtual int     ReadByte()                          = 0;
    // Returns number of bytes written, or -1 if the end of stream was reached
    virtual size_t  Write(const void *buffer, size_t size) = 0;
    // Returns value of byte written or -1 if the end of stream was reached
    virtual int     WriteByte(uint8_t b)                = 0;
    virtual size_t  Seek(StreamSeek seek, int pos)      = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
