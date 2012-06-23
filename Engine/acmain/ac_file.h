#ifndef __AC_FILE_H
#define __AC_FILE_H

#include "ac/dynobj/scriptfile.h"

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
long int filelength(int fhandle);
#endif

void get_current_dir_path(char* buffer, const char *fileName);
int check_valid_file_handle(FILE*hann, char*msg);
bool validate_user_file_path(const char *fnmm, char *output, bool currentDirOnly);
FILE* FileOpen(const char*fnmm, const char* mode);
void FileClose(FILE*hha);
void FileWrite(FILE*haa, const char *towrite);
void FileWriteRawLine(FILE*haa, const char*towrite);
void FileRead(FILE*haa,char*toread);
int FileIsEOF (FILE *haa);
int FileIsError(FILE *haa);
void FileWriteInt(FILE*haa,int into);
int FileReadInt(FILE*haa);
char FileReadRawChar(FILE*haa);
int FileReadRawInt(FILE*haa);
void FileWriteRawChar(FILE *haa, int chartoWrite);

int File_Exists(const char *fnmm);
int File_Delete(const char *fnmm);
void *sc_OpenFile(const char *fnmm, int mode);
void File_Close(sc_File *fil);
void File_WriteString(sc_File *fil, const char *towrite);
void File_WriteInt(sc_File *fil, int towrite);
void File_WriteRawChar(sc_File *fil, int towrite);
void File_WriteRawLine(sc_File *fil, const char *towrite);
void File_ReadRawLine(sc_File *fil, char* buffer);
const char* File_ReadRawLineBack(sc_File *fil);
void File_ReadString(sc_File *fil, char *toread);
const char* File_ReadStringBack(sc_File *fil);
int File_ReadInt(sc_File *fil);
int File_ReadRawChar(sc_File *fil);
int File_ReadRawInt(sc_File *fil);
int File_GetEOF(sc_File *fil);
int File_GetError(sc_File *fil);
#ifdef __cplusplus
extern "C" {
#endif
extern char* game_file_name;
#ifdef __cplusplus
}
#endif
#endif // __AC_FILE_H
