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
#include "util/string.h"

using AGS::Common::String;

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

// Filepath tokens, which are replaced by platform-specific directory names
extern const String UserSavedgamesRootToken;
extern const String GameSavedgamesDirToken;
extern const String GameDataDirToken;

inline const char *PathOrCurDir(const char *path)
{
    return path ? path : ".";
}

// Subsitutes illegal characters with '_'. This function uses illegal chars array
// specific to current platform.
void FixupFilename(char *filename);
// Checks if there is a slash after special token in the beginning of the
// file path, and adds one if it is missing. If no token is found, string is
// returned unchanged.
String FixSlashAfterToken(const String &path);
// Creates a directory path by combining absolute path to special directory with
// custom game's directory name.
// If the path is relative, keeps it unmodified (no extra subdir added).
String MakeSpecialSubDir(const String &sp_dir);
// Resolves a file path provided by user (e.g. script) into actual file path,
// by substituting special keywords with actual platform-specific directory names.
// Sets a primary and alternate paths; the latter is for backwards compatibility only.
// Returns 'true' on success, and 'false' if either path is impossible to resolve
// or if the file path is forbidden to be accessed in current situation.
bool ResolveScriptPath(const String &sc_path, bool read_only, String &path, String &alt_path);

String  get_current_dir();
void	get_current_dir_path(char* buffer, const char *fileName);

struct ScriptFileHandle
{
    Common::Stream  *stream;
    int32_t         handle;
};
extern ScriptFileHandle valid_handles[MAX_OPEN_SCRIPT_FILES + 1];
extern int num_open_script_files;

ScriptFileHandle *check_valid_file_handle_ptr(Common::Stream *stream_ptr, const char *operation_name);
ScriptFileHandle *check_valid_file_handle_int32(int32_t handle, const char *operation_name);
Common::Stream   *get_valid_file_stream_from_handle(int32_t handle, const char *operation_name);

#endif // __AGS_EE_AC__FILE_H
