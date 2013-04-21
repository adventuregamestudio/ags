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
// GameSetup, a class of game configuration
//
//=============================================================================
#ifndef __AGS_EE_GAME__GAMESETUP_H
#define __AGS_EE_GAME__GAMESETUP_H

#include "util/string.h"

namespace AGS
{
namespace Engine
{

using Common::String;

class GameSetup
{
public:
    GameSetup();
    ~GameSetup();

    void InitDefaults();

    // TODO: all members are currently public; hide them later
public:
    int32_t DigitalSoundCard;
    int32_t MidiSoundCard;
    bool    ModPlayer;
    bool    Mp3Player;  // CHECKME: not used anywhere
    int32_t TextHeight;
    bool    WantLetterbox;
    int32_t Windowed;
    int32_t BaseWidth;
    int32_t BaseHeight;
    int16_t RefreshRate;
    bool    NoSpeechPack;
    bool    EnableAntiAliasing;
    bool    ForceHicolorMode;
    bool    DisableExceptionHandling;
    bool    EnableSideBorders;
    String  DataFilesDir;
    String  MainDataFilename;
    String  Translation;
    String  GfxFilterID;
    String  GfxDriverID;
};

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__GAMESETUP_H
