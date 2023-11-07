//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
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
struct ccScript;


struct GameSetupStructBase
{
    static const int  GAME_NAME_LENGTH = 50;
    static const int  MAX_OPTIONS = 100;
    static const int  NUM_INTS_RESERVED = 17;
    // TODO: this is left purely to load older format version, revise later
    static const int  NUM_LEGACY_GLOBALMES = 500;

    char              gamename[GAME_NAME_LENGTH];
    int               options[MAX_OPTIONS];
    unsigned char     paluses[256];
    RGB               defpal[256];
    int               numviews;
    int               numcharacters;
    int               playercharacter;
    short             numinvitems;
    int               numdialog, numdlgmessage;
    int               numfonts;
    int               color_depth;          // in bytes per pixel (ie. 1 or 2)
    int               target_win;
    int               dialog_bullet;        // 0 for none, otherwise slot num of bullet point
    unsigned short    hotdot, hotdotouter;  // inv cursor hotspot dot color
    int               uniqueid;    // random key identifying the game
    int               numgui;
    int               numcursors;
    int               default_lipsync_frame; // used for unknown chars
    int               invhotdotsprite;
    int               reserved[NUM_INTS_RESERVED];
    std::unique_ptr<WordsDictionary> dict;
    std::vector<CharacterInfo> chars;

    GameSetupStructBase();
    GameSetupStructBase(GameSetupStructBase &&gss) = default;
    ~GameSetupStructBase();

    GameSetupStructBase &operator =(GameSetupStructBase &&gss) = default;

    void Free();

    // Tells whether the serialized game data contains certain components
    struct SerializeInfo
    {
        bool HasCCScript = false;
        bool HasWordsDict = false;
        // NOTE: Global messages are cut out, but we still have to check them
        // so long as we keep support of loading an older game data
        std::array<int, NUM_LEGACY_GLOBALMES> HasMessages{};
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

    // Returns the expected filename of a digital audio package
    inline AGS::Common::String GetAudioVOXName() const
    {
        return "audio.vox";
    }

    // Returns a list of game options that are forbidden to change at runtime
    inline static std::array<int, 17> GetRestrictedOptions()
    {
        return std::array<int,17> {{
            OPT_DEBUGMODE, OPT_OBSOLETE_LETTERBOX, OPT_OBSOLETE_HIRES_FONTS, OPT_SPLITRESOURCES,
            OPT_OBSOLETE_STRICTSCRIPTING, OPT_OBSOLETE_LEFTTORIGHTEVAL, OPT_COMPRESSSPRITES, OPT_OBSOLETE_STRICTSTRINGS,
            OPT_OBSOLETE_NATIVECOORDINATES, OPT_OBSOLETE_SAFEFILEPATHS, OPT_DIALOGOPTIONSAPI, OPT_BASESCRIPTAPI,
            OPT_SCRIPTCOMPATLEV, OPT_OBSOLETE_RELATIVEASSETRES, OPT_GAMETEXTENCODING, OPT_KEYHANDLEAPI,
            OPT_CUSTOMENGINETAG
        }};
    }

private:
    void OnResolutionSet();

    // Determines game's actual resolution.
    Size _gameResolution;
    // Multiplier for various UI drawin sizes, meant to keep UI elements readable
    int _relativeUIMult;
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCTBASE_H