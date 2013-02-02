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
#ifndef __AGS_EE_AC__FILE_H
#define __AGS_EE_AC__FILE_H

#include "ac/dynobj/scriptfile.h"
#include "ac/runtime_defines.h"

int		File_Exists(const char *fnmm);
int		File_Delete(const char *fnmm);
void	*sc_OpenFile(const char *fnmm, int mode);
void	File_Close(sc_File *fil);
void	File_WriteString(sc_File *fil, const char *towrite);
void	File_WriteInt(sc_File *fil, int towrite);
void	File_WriteRawChar(sc_File *fil, int towrite);
void	File_WriteRawLine(sc_File *fil, const char *towrite);
void	File_ReadRawLine(sc_File *fil, char* buffer);
const char* File_ReadRawLineBack(sc_File *fil);
void	File_ReadString(sc_File *fil, char *toread);
const char* File_ReadStringBack(sc_File *fil);
int		File_ReadInt(sc_File *fil);
int		File_ReadRawChar(sc_File *fil);
int		File_ReadRawInt(sc_File *fil);
int		File_GetEOF(sc_File *fil);
int		File_GetError(sc_File *fil);

void	get_current_dir_path(char* buffer, const char *fileName);
bool	validate_user_file_path(const char *fnmm, char *output, bool currentDirOnly);

struct ScriptFileHandle
{
    Common::DataStream  *stream;
    int32_t     handle;
};
extern ScriptFileHandle valid_handles[MAX_OPEN_SCRIPT_FILES + 1];
extern int num_open_script_files;

ScriptFileHandle *check_valid_file_handle_ptr(Common::DataStream *stream_ptr, const char *operation_name);
ScriptFileHandle *check_valid_file_handle_int32(int32_t handle, const char *operation_name);
Common::DataStream *get_valid_file_stream_from_handle(int32_t handle, const char *operation_name);

#endif // __AGS_EE_AC__FILE_H
