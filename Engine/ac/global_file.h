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

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

Common::Stream *FileOpen(const char*fnmm, Common::FileOpenMode open_mode, Common::FileWorkMode work_mode);
// NOTE: FileOpenCMode is a backwards-compatible replacement for old-style global script function FileOpen
Common::Stream *FileOpenCMode(const char*fnmm, const char* cmode);
void  FileClose(Common::Stream *hha);
void  FileWrite(Common::Stream *haa, const char *towrite);
void  FileWriteRawLine(Common::Stream *haa, const char*towrite);
void  FileRead(Common::Stream *haa,char*toread);
int   FileIsEOF (Common::Stream *haa);
int   FileIsError(Common::Stream *haa);
void  FileWriteInt(Common::Stream *haa,int into);
int   FileReadInt(Common::Stream *haa);
char  FileReadRawChar(Common::Stream *haa);
int   FileReadRawInt(Common::Stream *haa);
void  FileWriteRawChar(Common::Stream *haa, int chartoWrite);

#endif // __AGS_EE_AC__GLOBALFILE_H
