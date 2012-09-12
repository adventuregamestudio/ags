
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILESTREAM_H
#define __AGS_CN_UTIL__FILESTREAM_H

#include "util/datastream.h"
#include "util/file.h"

namespace AGS
{
namespace Common
{

class FileStream : public DataStream
{
public:
    FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode);
    FileStream(const String &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
        DataEndianess caller_endianess, DataEndianess stream_endianess);
    virtual ~FileStream();

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
    virtual int     GetLength() const;
    // Current position (if known)
    virtual int     GetPosition() const;
    virtual bool    CanRead() const;
    virtual bool    CanWrite() const;
    virtual bool    CanSeek() const;

    virtual void    Close();

    virtual int     ReadByte();
    virtual int16_t ReadInt16();
    virtual int32_t ReadInt32();
    virtual int64_t ReadInt64();
    virtual int     Read(void *buffer, int size);

    virtual int     WriteByte(uint8_t b);
    virtual void    WriteInt16(int16_t val);
    virtual void    WriteInt32(int32_t val);
    virtual void    WriteInt64(int64_t val);
    virtual int     Write(const void *buffer, int size);

    virtual int     Seek(StreamSeek seek, int pos);

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
