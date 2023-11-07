//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// IAGSStream is a contract for stream class, provided by engine to plugin
// on the need, such as saving/restoring the game.
// The user is advised to use advanced helper methods, such as Read/WriteX
// and Read/WriteArrayOfX to allow the stream implementation properly control
// endianness conversions and data padding, when needed.
//
//=============================================================================
#ifndef __AGS_CN_API__IAGSSTREAM_H
#define __AGS_CN_API__IAGSSTREAM_H

// TODO: it would probably be better to not include core definition headers
// in API class headers, but make separate core headers specifically for
// plugins, and let plugin developers include them manually in plugin sources.
#include "core/types.h"

namespace AGS
{
namespace Common
{

enum StreamSeek
{
   kSeekBegin,
   kSeekCurrent,
   kSeekEnd
};

class IAGSStream
{
public:
    virtual ~IAGSStream() = default;

    virtual void        Close() = 0;

    virtual bool        IsValid() const = 0;
    virtual bool        EOS() const = 0;
    virtual soff_t      GetLength() const = 0;
    virtual soff_t      GetPosition() const = 0;
    virtual bool        CanRead() const = 0;
    virtual bool        CanWrite() const = 0;
    virtual bool        CanSeek() const = 0;

    // Reads number of bytes in the provided buffer
    virtual size_t      Read(void *buffer, size_t size) = 0;
    // ReadByte conforms to fgetc behavior:
    // - if stream is valid, then returns an *unsigned char* packed in the int
    // - if EOS, then returns -1
    virtual int32_t     ReadByte() = 0;
    // Writes number of bytes from the provided buffer
    virtual size_t      Write(const void *buffer, size_t size) = 0;
    // WriteByte conforms to fputc behavior:
    // - on success, returns the unsigned char packed in the int
    // - on failure, returns -1
    virtual int32_t     WriteByte(uint8_t b) = 0;

    // Convenience methods for reading values of particular size
    virtual int8_t      ReadInt8() = 0;
    virtual int16_t     ReadInt16() = 0;
    virtual int32_t     ReadInt32() = 0;
    virtual int64_t     ReadInt64() = 0;
    virtual float       ReadFloat32() = 0;
    virtual bool        ReadBool() = 0;
    virtual size_t      ReadArray(void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      ReadArrayOfInt8(int8_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt16(int16_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt32(int32_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfInt64(int64_t *buffer, size_t count) = 0;
    virtual size_t      ReadArrayOfFloat32(float *buffer, size_t count) = 0;

    // Convenience methods for writing values of particular size
    virtual size_t      WriteInt8(int8_t val) = 0;;
    virtual size_t      WriteInt16(int16_t val) = 0;
    virtual size_t      WriteInt32(int32_t val) = 0;
    virtual size_t      WriteInt64(int64_t val) = 0;
    virtual size_t      WriteFloat32(float val) = 0;
    virtual size_t      WriteBool(bool val) = 0;
    virtual size_t      WriteArray(const void *buffer, size_t elem_size, size_t count) = 0;
    virtual size_t      WriteArrayOfInt8(const int8_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt16(const int16_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt32(const int32_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfInt64(const int64_t *buffer, size_t count) = 0;
    virtual size_t      WriteArrayOfFloat32(const float *buffer, size_t count) = 0;

    virtual bool        Seek(soff_t offset, StreamSeek origin = kSeekCurrent) = 0;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_API__IAGSSTREAM_H
