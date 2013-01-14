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
// Script Editor run-time engine component (c) 1998 Chris Jones
// script chunk format:
// 00h  1 dword  version - should be 2
// 04h  1 dword  sizeof(scriptblock)
// 08h  1 dword  number of ScriptBlocks
// 0Ch  n STRUCTs ScriptBlocks
//
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include "util/wgt2allg.h"
#include "ac/roomstruct.h"
#include "util/filestream.h"
#include "script/cc_instance.h"
#include "script/cc_error.h"

using AGS::Common::Stream;

char *scripteditruntimecopr = "Script Editor v1.2 run-time component. (c) 1998 Chris Jones";

#define SCRIPT_CONFIG_VERSION 1
extern void quit(char *);
extern int currentline; // in script/script_common

void cc_error_at_line(char *buffer, const char *error_msg)
{
    if (ccInstance::GetCurrentInstance() == NULL) {
        sprintf(ccErrorString, "Error (line %d): %s", currentline, error_msg);
    }
    else {
        sprintf(ccErrorString, "Error: %s\n", error_msg);
        ccInstance::GetCurrentInstance()->GetCallStack(ccErrorCallStack, 5);
    }
}

void save_script_configuration(Stream *out)
{
    quit("ScriptEdit: run-time version can't save");
}

void load_script_configuration(Stream *in)
{
    int aa;
    if (in->ReadInt32() != SCRIPT_CONFIG_VERSION)
        quit("ScriptEdit: invalid config version");

    int numvarnames = in->ReadInt32();
    for (aa = 0; aa < numvarnames; aa++) {
        int lenoft = in->ReadByte();
        in->Seek(Common::kSeekCurrent, lenoft);
    }
}

void save_graphical_scripts(Stream *out, roomstruct * rss)
{
    quit("ScriptEdit: run-time version can't save");
}

char *scripttempn = "~acsc%d.tmp";

void load_graphical_scripts(Stream *in, roomstruct * rst)
{
    int32_t ct;

    while (1) {
        ct = in->ReadInt32();
        if ((ct == -1) | (in->EOS() != 0))
            break;

        int32_t lee;
        lee = in->ReadInt32();

        char thisscn[20];
        sprintf(thisscn, scripttempn, ct);
        Stream *te = Common::File::CreateFile(thisscn);

        char *scnf = (char *)malloc(lee);
        // MACPORT FIX: swap size and nmemb
        in->Read(scnf, lee);
        te->Write(scnf, lee);
        delete te;

        free(scnf);
    }
}
