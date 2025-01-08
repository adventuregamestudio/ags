//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
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
    // Reads all available components from the stream
    HSaveError    ReadAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp,
        const PreservedParams &pp, RestoredData &r_data);
    // Prescans all components, gathering data counts and asserting data match;
    // does *not* keep any actual game data
    HSaveError    PrescanAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp,
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
