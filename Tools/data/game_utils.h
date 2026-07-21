//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__GAMEUTIL_H
#define __AGS_TOOL_DATA__GAMEUTIL_H

#include <vector>
#include "ac/gamestructdefines.h"
#include "util/ini_util.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::String;

// EntityRef is a parent struct for a game object data;
// contains common fields such as a numeric ID (aka index) and script name.
struct EntityRef
{
    String TypeName; // name of type, for the reference when necessary
    int ID = -1;
    String ScriptName;
};

typedef EntityRef CharacterRef;

// DialogRef contains only Dialog data strictly necessary for generating scripts.
// NOTE: replace with full Dialog struct later if appears necessary
struct DialogRef : EntityRef
{
    int OptionCount = 0;
};

// GUIRef contains only GUI data strictly necessary for generating scripts.
// NOTE: replace with full GUI struct later if appears necessary
struct GUIRef : EntityRef
{
    std::vector<EntityRef> Controls;
};

// Game variable (for variables defined in the game project)
struct Variable
{
    String Type;
    String Name;
    String Value;
};

// Game settings
struct GameSettings
{
    // TODO: following struct is a stub, listing only few fields used so far. Expand later.

    bool DebugMode = false;
    RenderAtScreenRes RenderAtScreenResolution = ::kRenderAtScreenRes_UserDefined;
    String SayFunction; // Custom speech function name
    String NarrateFunction; // Custom narrate function name
};

struct RuntimeSetup
{
    // TODO: consider turning enum-like string fields into actual enum fields? see notes below
    String GraphicsDriver;
    bool Windowed = false;
    bool FullscreenDesktop = false;
    String FullscreenGameScaling; // enum-like field
    String WindowGameScaling; // enum-like field
    int GameScalingMultiplier = 0;
    String GraphicsFilter;
    bool VSync = false;
    bool AAScaledSprites = false;
    bool RenderAtScreenResolution = false;
    String Rotation; // enum-like field
    String AudioDriver; // enum-like field
    bool UseVoicePack = false;
    String Translation;
    bool AutoLockMouse = false;
    float MouseSpeed = 0.f;
    String TouchToMouseEmulation; // enum-like field
    String TouchToMouseMotionMode; // enum-like field
    bool ShowFPS = false;
    int SpriteCacheSize = 0;
    int TextureCacheSize = 0;
    int SoundCacheSize = 0;
    bool CompressSaves = false;
    bool UseCustomSavePath = false;
    String CustomSavePath;
    bool UseCustomAppDataPath = false;
    String CustomAppDataPath;
    String TitleText;
};

// GameRef contains only game data strictly necessary for generating scripts.
// NOTE: replace with full Game struct later if appears necessary
struct GameRef
{
    std::vector<EntityRef> AudioClips;
    std::vector<EntityRef> AudioTypes;
    std::vector<CharacterRef> Characters;
    std::vector<EntityRef> Cursors;
    std::vector<DialogRef> Dialogs;
    std::vector<EntityRef> Fonts;
    std::vector<GUIRef>    GUI;
    std::vector<EntityRef> Inventory;
    std::vector<EntityRef> Views;

    std::vector<Variable>  GlobalVars;

    GameSettings           Settings;
};

// Fills ConfigTree with contents of RuntimeSetup, in accordance to the AGS config format.
// Optionally uses GameSettings to adjust certain config options.
void WriteConfig(const RuntimeSetup &setup, const GameSettings *settings, AGS::Common::ConfigTree &cfg);

} // namespace DataUtil
} // namespace AGS

#endif // __AGS_TOOL_DATA__GAMEUTIL_H
