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
