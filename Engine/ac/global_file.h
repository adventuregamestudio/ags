
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALFILE_H
#define __AGS_EE_AC__GLOBALFILE_H

#include "util/file.h"

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

Common::CDataStream *FileOpen(const char*fnmm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
void  FileClose(Common::CDataStream *hha);
void  FileWrite(Common::CDataStream *haa, const char *towrite);
void  FileWriteRawLine(Common::CDataStream *haa, const char*towrite);
void  FileRead(Common::CDataStream *haa,char*toread);
int   FileIsEOF (Common::CDataStream *haa);
int   FileIsError(Common::CDataStream *haa);
void  FileWriteInt(Common::CDataStream *haa,int into);
int   FileReadInt(Common::CDataStream *haa);
char  FileReadRawChar(Common::CDataStream *haa);
int   FileReadRawInt(Common::CDataStream *haa);
void  FileWriteRawChar(Common::CDataStream *haa, int chartoWrite);

#endif // __AGS_EE_AC__GLOBALFILE_H
