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
// RoomInfo implementation specific to Engine runtime
//
//=============================================================================

#include <stdio.h>
#include "ac/common.h"
#include "game/roominfo.h"
#include "util/file.h"
#include "util/stream.h"

namespace AGS
{
namespace Common
{

#define SCRIPT_CONFIG_VERSION 1

/* static */ bool RoomInfo::IsVersionSupported(int16_t version)
{
  return version > kRoomVersion_250b && version <= kRoomVersion_Current;
}

void RoomInfo::LoadScriptConfiguration(Stream *in)
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

void RoomInfo::SaveScriptConfiguration(Stream *out)
{
    quit("ScriptEdit: run-time version can't save");
}

char *scripttempn = "~acsc%d.tmp";

void RoomInfo::LoadGraphicalScripts(Stream *in)
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

void RoomInfo::SaveGraphicalScripts(Stream *out)
{
    quit("ScriptEdit: run-time version can't save");
}

} // namespace Common
} // namespace AGS
