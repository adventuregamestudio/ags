
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALFILE_H
#define __AGS_EE_AC__GLOBALFILE_H

#include "platform/file.h"

FILE* FileOpen(const char*fnmm, const char* mode);
void  FileClose(FILE*hha);
void  FileWrite(FILE*haa, const char *towrite);
void  FileWriteRawLine(FILE*haa, const char*towrite);
void  FileRead(FILE*haa,char*toread);
int   FileIsEOF (FILE *haa);
int   FileIsError(FILE *haa);
void  FileWriteInt(FILE*haa,int into);
int   FileReadInt(FILE*haa);
char  FileReadRawChar(FILE*haa);
int   FileReadRawInt(FILE*haa);
void  FileWriteRawChar(FILE *haa, int chartoWrite);

#endif // __AGS_EE_AC__GLOBALFILE_H
