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
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILESTREAM_H
#define __AGS_CN_UTIL__FILESTREAM_H

#include <stdio.h>
#include "util/datastream.h"
#include "util/file.h"

namespace AGS
{
namespace Common
{

class FileStream : public DataStream
{
public:
    FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
        DataEndianess stream_endianess = kLittleEndian);
    virtual ~FileStream();

    virtual void    Close();
    virtual bool    Flush();

    // TODO
    // Temporary solution for cases when the code can't live without
    // having direct access to FILE pointer
    inline FILE     *GetHandle() const
    {
        return _file;
    }

    // Is stream valid (underlying data initialized properly)
    virtual bool    IsValid() const;
    // Is end of stream
    virtual bool    EOS() const;
    // Total length of stream (if known)
    virtual size_t  GetLength() const;
    // Current position (if known)
    virtual size_t  GetPosition() const;
    virtual bool    CanRead() const;
    virtual bool    CanWrite() const;
    virtual bool    CanSeek() const;

    virtual size_t  Read(void *buffer, size_t size);
    virtual int32_t ReadByte();
    virtual size_t  Write(const void *buffer, size_t size);
    virtual int32_t WriteByte(uint8_t b);

    virtual size_t  Seek(int offset, StreamSeek origin);

protected:
    void            Open(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode);

private:
    FILE                *_file;
    const FileOpenMode  _openMode;
    const FileWorkMode  _workMode;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__FILESTREAM_H
