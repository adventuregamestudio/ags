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
//
// GameSetupStructBase is a base class for main game data.
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESETUPSTRUCTBASE_H
#define __AGS_CN_AC__GAMESETUPSTRUCTBASE_H
#include <array>
#include <memory>
#include <vector>
#include <allegro.h> // RGB
#include "ac/game_version.h"
#include "ac/gamestructdefines.h"
#include "ac/wordsdictionary.h"
#include "util/string.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct CharacterInfo;


struct GameSetupStructBase
{
    static const int  LEGACY_GAME_NAME_LENGTH = 50;
    static const int  MAX_OPTIONS = 100;
    static const int  NUM_INTS_RESERVED = 14;
    // TODO: this is left purely to load older format version, revise later
    static const int  NUM_LEGACY_GLOBALMES = 500;

    Common::String    gamename;
    int               options[MAX_OPTIONS] = { 0 };
    uint8_t           paluses[256] = { 0 };
    RGB               defpal[256] = {};
    int               numviews = 0;
    int               numcharacters = 0;
    int               playercharacter = -1;
    int               numinvitems = 0;
    int               numdialog = 0;
    int               numdlgmessage = 0;    // [DEPRECATED]
    int               numfonts = 0;
    int               color_depth = 0;      // in bytes per pixel (ie. 1, 2, 4)
    int               target_win = 0;
    int               dialog_bullet = 0;    // 0 for none, otherwise slot num of bullet point
    int               hotdot = 0;           // inv cursor hotspot dot color
    int               hotdotouter = 0;      // inv cursor hotspot cross color
    int               uniqueid = 0;         // random key identifying the game
    int               numgui = 0;
    int               numcursors = 0;
    int               default_lipsync_frame = 0; // used for unknown chars
    int               invhotdotsprite = 0;
    int               reserved[NUM_INTS_RESERVED] = { 0 };
    std::unique_ptr<WordsDictionary> dict;
    std::vector<CharacterInfo> chars;

    GameSetupStructBase() = default;
    GameSetupStructBase(GameSetupStructBase &&gss) = default;
    ~GameSetupStructBase() = default;

    GameSetupStructBase &operator =(GameSetupStructBase &&gss) = default;

    // Tells whether the serialized game data contains certain components
    struct SerializeInfo
    {
        bool HasCCScript = false;
        bool HasWordsDict = false;
        // NOTE: Global messages are cut out, but we still have to check them
        // so long as we keep support of loading an older game data
        std::array<int, NUM_LEGACY_GLOBALMES> HasMessages = {};
        // File offset at which game data extensions begin
        uint32_t ExtensionOffset = 0u;
    };

    void ReadFromFile(Common::Stream *in, GameDataVersion game_ver, SerializeInfo &info);
    void WriteToFile(Common::Stream *out, const SerializeInfo &info) const;

    // Game resolution is a size of a native game screen in pixels.
    // This is the "game resolution" that developer sets up in AGS Editor.
    // It is in the same units in which sprite and font sizes are defined.
    //
    // Graphic renderer may scale and stretch game's frame as requested by
    // player or system, which will not affect native coordinates in any way.
    void SetGameResolution(Size game_res);
    const Size &GetGameRes() const { return _gameResolution; }
    // Get game's native color depth (bits per pixel)
    inline int GetColorDepth() const { return color_depth * 8; }

    // Returns the expected filename of a digital audio package
    inline AGS::Common::String GetAudioVOXName() const
    {
        return "audio.vox";
    }

    // Returns a list of game options that are forbidden to change at runtime
    inline static std::array<int, 18> GetRestrictedOptions()
    {
        return std::array<int, 18> {{
            OPT_DEBUGMODE, OPT_OBSOLETE_LETTERBOX, OPT_OBSOLETE_HIRES_FONTS, OPT_SPLITRESOURCES,
            OPT_OBSOLETE_STRICTSCRIPTING, OPT_OBSOLETE_LEFTTORIGHTEVAL, OPT_COMPRESSSPRITES, OPT_OBSOLETE_STRICTSTRINGS,
            OPT_OBSOLETE_NATIVECOORDINATES, OPT_OBSOLETE_SAFEFILEPATHS, OPT_DIALOGOPTIONSAPI, OPT_BASESCRIPTAPI,
            OPT_SCRIPTCOMPATLEV, OPT_OBSOLETE_RELATIVEASSETRES, OPT_GAMETEXTENCODING, OPT_KEYHANDLEAPI,
            OPT_CUSTOMENGINETAG, OPT_VOICECLIPNAMERULE
        }};
    }

    // Returns a list of game options that must be preserved when restoring a save;
    // NOTE: restricted options are always preserved, so no need to mention them here
    inline static std::array<int, 1> GetPreservedOptions()
    {
        return std::array<int, 1> {{
            OPT_SAVECOMPONENTSIGNORE
        }};
    }

private:
    void OnResolutionSet();

    // Determines game's actual resolution.
    Size _gameResolution;
    // Multiplier for various UI drawin sizes, meant to keep UI elements readable
    int _relativeUIMult = 1;
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCTBASE_H
