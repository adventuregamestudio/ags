
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_UTIL__FILESTREAM_H
#define __AGS_CN_UTIL__FILESTREAM_H

#include "util/agsstream.h"
#include "util/file.h"

namespace AGS
{
namespace Common
{

class CFileStream : public CAGSStream
{
public:
    CFileStream(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode);
    CFileStream(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode,
        DataEndianess caller_endianess, DataEndianess stream_endianess);
    virtual ~CFileStream();

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

    virtual int     Seek(StreamSeek seek, int pos);

    virtual int8_t  ReadInt8();
    virtual int16_t ReadInt16();
    virtual int32_t ReadInt32();
    virtual int64_t ReadInt64();
    virtual int     Read(void *buffer, int size);
    virtual CString ReadString(int max_chars = 5000000);

    virtual void    WriteInt8(int8_t val);
    virtual void    WriteInt16(int16_t val);
    virtual void    WriteInt32(int32_t val);
    virtual void    WriteInt64(int64_t val);
    virtual int     Write(const void *buffer, int size);
    virtual void    WriteString(const CString &str);

protected:
    virtual int     ReadAndConvertArrayOfInt16(int16_t *buffer, int count);
    virtual int     ReadAndConvertArrayOfInt32(int32_t *buffer, int count);
    virtual int     ReadAndConvertArrayOfInt64(int64_t *buffer, int count);
    virtual int     WriteAndConvertArrayOfInt16(const int16_t *buffer, int count);
    virtual int     WriteAndConvertArrayOfInt32(const int32_t *buffer, int count);
    virtual int     WriteAndConvertArrayOfInt64(const int64_t *buffer, int count);

    void            Open(const CString &file_name, FileOpenMode open_mode, FileWorkMode work_mode);

private:
    FILE                *_file;
    const FileOpenMode  _openMode;
    const FileWorkMode  _workMode;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__FILESTREAM_H
