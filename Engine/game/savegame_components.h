//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_GAME__SAVEGAMECOMPONENTS_H
#define __AGS_EE_GAME__SAVEGAMECOMPONENTS_H

#include "game/savegame.h"
#include "util/stream.h"

namespace AGS
{

namespace Common { struct Interaction; }

namespace Engine
{

using Common::Stream;
using Common::Interaction;

struct PreservedParams;
struct RestoredData;

namespace SavegameComponents
{
    // SaveCmpSelection flags tell which save components to restore, and which to skip
    enum SaveCmpSelection
    {
        kSaveCmp_None           = 0,
        kSaveCmp_GameState      = 0x00000001,
        kSaveCmp_Audio          = 0x00000002,
        kSaveCmp_Characters     = 0x00000004,
        kSaveCmp_Dialogs        = 0x00000008,
        kSaveCmp_GUI            = 0x00000010,
        kSaveCmp_InvItems       = 0x00000020,
        kSaveCmp_Cursors        = 0x00000040,
        kSaveCmp_Views          = 0x00000080,
        kSaveCmp_DynamicSprites = 0x00000100,
        kSaveCmp_Overlays       = 0x00000200,
        kSaveCmp_DynamicSurfaces= 0x00000400,
        kSaveCmp_Scripts        = 0x00000800,
        kSaveCmp_Rooms          = 0x00001000,
        kSaveCmp_ThisRoom       = 0x00002000,
        kSaveCmp_Plugins        = 0x00004000,
        kSaveCmp_All            = 0xFFFFFFFF
    };


    // Reads all available components from the stream
    HSaveError    ReadAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp,
        const PreservedParams &pp, RestoredData &r_data);
    // Writes a full list of common components to the stream
    HSaveError    WriteAllCommon(Stream *out, SaveCmpSelection select_cmp);

    // Utility functions for reading and writing legacy interactions,
    // or their "times run" counters separately.
    void ReadTimesRun272(Interaction &intr, Stream *in);
    HSaveError ReadInteraction272(Interaction &intr, Stream *in);
    void WriteTimesRun272(const Interaction &intr, Stream *out);
    void WriteInteraction272(const Interaction &intr, Stream *out);
}

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAMECOMPONENTS_H
