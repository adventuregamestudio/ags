//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

// Managed file streams: for script and plugin use
int32_t add_file_stream(std::unique_ptr<AGS::Common::Stream> &&stream, const char *operation_name);
void    close_file_stream(int32_t fhandle, const char *operation_name);
AGS::Common::Stream *get_file_stream(int32_t fhandle, const char *operation_name);
AGS::Common::Stream *release_file_stream(int32_t fhandle, const char *operation_name);
void    close_all_file_streams();

#endif // __AGS_EE_AC__FILE_H
