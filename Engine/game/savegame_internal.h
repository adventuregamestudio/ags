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

#ifndef __AGS_EE_GAME__SAVEGAMEINTERNAL_H
#define __AGS_EE_GAME__SAVEGAMEINTERNAL_H

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)

#include "media/audio/audiodefines.h"


namespace AGS
{
namespace Engine
{

// PreservedParams keeps old values of particular gameplay
// parameters that are saved before the save restoration
// and either applied or compared to new values after
// loading save data
struct PreservedParams
{
    // Whether speech and audio packages available
    int SpeechVOX;
    int MusicVOX;
    // Script global data sizes
    int GlScDataSize;
    std::vector<int> ScMdDataSize;

    PreservedParams();
};

// RestoredData keeps certain temporary data to help with
// the restoration process
struct RestoredData
{
    int                     FPS;
    // Unserialized bitmaps for dynamic surfaces
    std::vector<Bitmap*>    DynamicSurfaces;
    // Scripts global data
    struct ScriptData
    {
        stdtr1compat::shared_ptr<char> Data;
        size_t              Len;

        ScriptData();
    };
    ScriptData              GlobalScript;
    std::vector<ScriptData> ScriptModules;
    // Room data (has to be be preserved until room is loaded)
    Bitmap                 *RoomBkgScene[MAX_BSCENE];
    short                   RoomLightLevels[MAX_REGIONS];
    int                     RoomTintLevels[MAX_REGIONS];
    short                   RoomZoomLevels1[MAX_WALK_AREAS + 1];
    short                   RoomZoomLevels2[MAX_WALK_AREAS + 1];
    int                     RoomVolume;
    // Mouse cursor parameters
    int                     CursorID;
    int                     CursorMode;
    // General audio
    struct ChannelInfo
    {
        int ClipID;
        int Pos;
        int Priority;
        int Repeat;
        int Vol;
        int VolAsPercent;
        int Pan;
        int PanAsPercent;
        int Speed;
    };
    ChannelInfo             AudioChans[MAX_SOUND_CHANNELS + 1];
    // Ambient sounds
    int                     DoAmbient[MAX_SOUND_CHANNELS];

    RestoredData();
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAMEINTERNAL_H
