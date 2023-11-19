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
// Script File API implementation.
//
//=============================================================================
#ifndef __AGS_EE_AC__FILE_H
#define __AGS_EE_AC__FILE_H

#include <memory>
#include "ac/dynobj/scriptfile.h"
#include "ac/runtime_defines.h"
#include "util/stream.h"

int		File_Exists(const char *fnmm);
int		File_Delete(const char *fnmm);
void	*sc_OpenFile(const char *fnmm, int mode);
const char *File_ResolvePath(const char *fnmm);
void	File_Close(sc_File *fil);
void	File_WriteString(sc_File *fil, const char *towrite);
void	File_WriteInt(sc_File *fil, int towrite);
void	File_WriteRawChar(sc_File *fil, int towrite);
void	File_WriteRawInt(sc_File *fil, int towrite);
void	File_WriteRawLine(sc_File *fil, const char *towrite);
void	File_ReadRawLine(sc_File *fil, char* buffer);
const char* File_ReadRawLineBack(sc_File *fil);
void	File_ReadString(sc_File *fil, char *toread);
const char* File_ReadStringBack(sc_File *fil);
int		File_ReadInt(sc_File *fil);
int		File_ReadRawChar(sc_File *fil);
int		File_ReadRawInt(sc_File *fil);
int     File_Seek(sc_File *fil, int offset, int origin);
int		File_GetEOF(sc_File *fil);
int		File_GetError(sc_File *fil);
int     File_GetPosition(sc_File *fil);

//=============================================================================

namespace AGS
{
namespace Engine
{
    // IManagedStream interface is a contract for the plugin API,
    // matching IAGSStream interface there. Having this here is mostly an
    // issue of code organization. Perhaps this may be reviewed if we change
    // how plugin API is declared and reused within the engine itself.
    // NOTE: we cannot use our utility IStream for this, because:
    //    1) different, extended expectation for Close function;
    //    2) different types in args or return values: we cannot use
    //       some of them in plugin API.
    class IManagedStream
    {
    public:
        // Flushes and closes the stream, deallocates the stream object.
        // After calling this the IAGSStream pointer becomes INVALID.
        virtual void   Close() = 0;
        // Returns an optional stream's source description.
        // This may be a file path, or a resource name, or anything of that kind.
        virtual const char *GetPath() = 0;
        // Reads number of bytes into the provided buffer
        virtual size_t Read(void *buffer, size_t len) = 0;
        // Writes number of bytes from the provided buffer
        virtual size_t Write(void *buffer, size_t len) = 0;
        // Returns the total stream's length in bytes
        virtual int64_t GetLength() = 0;
        // Returns stream's position
        virtual int64_t GetPosition() = 0;
        // Tells whether the stream's position is at its end
        virtual bool   EOS() = 0;
        // Seeks to offset from the origin, returns new position in stream,
        // or -1 on error.
        virtual int64_t Seek(int64_t offset, int origin) = 0;
        // Flushes stream, forcing it to write any buffered data to the
        // underlying device. Note that the effect may depend on implementation.
        virtual void   Flush() = 0;
    protected:
        IManagedStream() = default;
        virtual ~IManagedStream() = default;
    };
} // namespace Engine
} // namespace AGS

// Managed file streams: for script and plugin use
int32_t add_file_stream(std::unique_ptr<AGS::Common::Stream> &&stream, const char *operation_name);
void    close_file_stream(int32_t fhandle, const char *operation_name);
AGS::Common::Stream *get_file_stream(int32_t fhandle, const char *operation_name);
AGS::Engine::IManagedStream *get_file_stream_iface(int32_t fhandle, const char *operation_name);
int32_t find_file_stream_handle(AGS::Engine::IManagedStream *iface);
AGS::Common::Stream *release_file_stream(int32_t fhandle, const char *operation_name);
void    close_all_file_streams();

#endif // __AGS_EE_AC__FILE_H
