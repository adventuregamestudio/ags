
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALFILE_H
#define __AGS_EE_AC__GLOBALFILE_H

#include "util/file.h"

namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

Common::DataStream *FileOpen(const char*fnmm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
void  FileClose(Common::DataStream *hha);
void  FileWrite(Common::DataStream *haa, const char *towrite);
void  FileWriteRawLine(Common::DataStream *haa, const char*towrite);
void  FileRead(Common::DataStream *haa,char*toread);
int   FileIsEOF (Common::DataStream *haa);
int   FileIsError(Common::DataStream *haa);
void  FileWriteInt(Common::DataStream *haa,int into);
int   FileReadInt(Common::DataStream *haa);
char  FileReadRawChar(Common::DataStream *haa);
int   FileReadRawInt(Common::DataStream *haa);
void  FileWriteRawChar(Common::DataStream *haa, int chartoWrite);

#endif // __AGS_EE_AC__GLOBALFILE_H
