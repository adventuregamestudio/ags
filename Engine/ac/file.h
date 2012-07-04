
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__FILE_H
#define __AGS_EE_AC__FILE_H

#include "ac/dynobj/scriptfile.h"

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
int		check_valid_file_handle(FILE*hann, char*msg);
bool	validate_user_file_path(const char *fnmm, char *output, bool currentDirOnly);

#endif // __AGS_EE_AC__FILE_H
