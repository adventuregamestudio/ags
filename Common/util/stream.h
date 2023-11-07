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
// Base stream class.
//
// Provides default implementation for a few helper methods.
// 
// Only streams with uncommon behavior should be derived directly from Stream.
// Most I/O devices should inherit DataStream instead.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__STREAM_H
#define __AGS_CN_UTIL__STREAM_H

#include "util/iagsstream.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

// TODO: merge with FileWorkMode (historical mistake)
enum StreamWorkMode
{
    kStream_Read,
    kStream_Write
};


class Stream : public IAGSStream
{
public:
    Stream() = default;
    Stream(const String &path)
        : _path(path) {}

    // Returns an optional path of a stream's source, such as a filepath;
    // primarily for diagnostic purposes
    const String &GetPath() const { return _path; }
    // Tells if the stream has errors
    virtual bool HasErrors() const { return false; }
    // Flush stream buffer to the underlying device
    virtual bool Flush() = 0;

    //-----------------------------------------------------
    // Helper methods
    //-----------------------------------------------------
    bool ReadBool() override
    {
        return ReadInt8() != 0;
    }

    size_t WriteBool(bool val) override
    {
        return WriteInt8(val ? 1 : 0);
    }

    // Practically identical to Read() and Write(), these two helpers' only
    // meaning is to underline the purpose of data being (de)serialized
    size_t ReadArrayOfInt8(int8_t *buffer, size_t count) override
    {
        return Read(buffer, count);
    }
    size_t WriteArrayOfInt8(const int8_t *buffer, size_t count) override
    {
        return Write(buffer, count);
    }

    // Fill the requested number of bytes with particular value
    size_t WriteByteCount(uint8_t b, size_t count);

protected:
    String _path; // optional name of the stream's source (e.g. filepath)
};

// Copies N bytes from one stream into another;
// returns number of bytes actually written
soff_t CopyStream(Stream *in, Stream *out, soff_t length);

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__STREAM_H
