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

#include <string.h>
#include "debug/rawfileoutputtarget.h"
#include "debug/out.h"

namespace AGS
{
namespace Engine
{
namespace Out
{
//
// TODO:
// Use advanced utility classes instead of C-style strings and arrays.      
//

//
// TODO: filepath parameter here may be actually used as a pattern
// or prefix, while the actual filename could be made by combining
// this prefix with current date, game name, and similar additional
// useful information. Whether this is to be determined here or on
// high-level side remains a question.
//
CRawFileOutputTarget::CRawFileOutputTarget(const char *sz_filepath)
{
    File = NULL;

    int len = strlen(sz_filepath);
    FilePath = new char[len + 1];
    strcpy(FilePath, sz_filepath);

    DidWriteOnce = false;
}

CRawFileOutputTarget::~CRawFileOutputTarget()
{
    CloseFile();
    delete [] FilePath;
}

void CRawFileOutputTarget::Out(const char *sz_fullmsg)
{
    if (!OpenFile()) {
        return;
    }

    fprintf(File, "%s\n", sz_fullmsg);
    CloseFile();
}

bool CRawFileOutputTarget::OpenFile()
{
    char *open_mode;
    if (DidWriteOnce) {
        open_mode = "at";
    }
    else {
        open_mode = "wt";
    }

    File = fopen(FilePath, open_mode);
    if (!File) {
        // TODO: make emergency call
        return false;
    }

    DidWriteOnce = true;
    return true;
}

void CRawFileOutputTarget::CloseFile()
{
    if (!File) {
        return;
    }

    fclose(File);
    File = NULL;
}

} // namespace Out
} // namespace Engine
} // namespace AGS
