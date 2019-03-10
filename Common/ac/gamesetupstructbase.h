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
//
//
//=============================================================================
#ifndef __AGS_CN_AC__GAMESETUPSTRUCTBASE_H
#define __AGS_CN_AC__GAMESETUPSTRUCTBASE_H

#include "ac/game_version.h"
#include "ac/gamestructdefines.h"
#include "util/string.h"
#include "util/wgt2allg.h" // color (allegro RGB)

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct WordsDictionary;
struct CharacterInfo;
struct ccScript;


struct GameSetupStructBase {
    static const int  GAME_NAME_LENGTH = 50;
    static const int  MAX_OPTIONS = 100;
    static const int  NUM_INTS_RESERVED = 17;

    char              gamename[GAME_NAME_LENGTH];
    int               options[MAX_OPTIONS];
    unsigned char     paluses[256];
    color             defpal[256];
    int               numviews;
    int               numcharacters;
    int               playercharacter;
    int               totalscore;
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
    char             *messages[MAXGLOBALMES];
    WordsDictionary  *dict;
    char             *globalscript;
    CharacterInfo    *chars;
    ccScript         *compiled_script;
    Size              size;                 // native game size in pixels
    Size              altsize;              // alternate, lesser, game size for letterbox-by-design games

    int             *load_messages;
    bool             load_dictionary;
    bool             load_compiled_script;
    // [IKM] 2013-03-30
    // NOTE: it looks like nor 'globalscript', not 'compiled_script' are used
    // to store actual script data anytime; 'ccScript* gamescript' global
    // pointer is used for that instead.

    GameSetupStructBase();
    virtual ~GameSetupStructBase();
    void Free();
    void SetDefaultResolution(GameResolutionType resolution_type);
    void SetCustomResolution(Size game_res);
    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);

    inline GameResolutionType GetDefaultResolution() const
    {
        return _defResolution;
    }

    inline bool IsHiRes() const
    {
        if (_defResolution == kGameResolution_Custom)
            return (size.Width * size.Height) > (320 * 240);
        return ::IsHiRes(_defResolution);
    }

    // Get game resolution in logical (script) units
    const Size &GetNativeSize() const { return _nativeSize; }
    // Get game data-->final game resolution coordinate multiplier
    inline int GetUpscaleMult() const { return _upscaleMult; }

    inline bool IsLegacyLetterbox() const
    {
        return options[OPT_LETTERBOX] != 0;
    }

    // Test if the game is built around old audio system
    inline bool IsLegacyAudioSystem() const
    {
        return loaded_game_file_version < kGameVersion_320;
    }
    // Returns the expected filename of a digital audio package
    inline AGS::Common::String GetAudioVOXName() const
    {
        return IsLegacyAudioSystem() ? "music.vox" : "audio.vox";
    }

private:
    void OnSetGameResolution();

    GameResolutionType _defResolution; // game size identifier
    // Determines the game's size in "native" units, used to convert coordinate
    // arguments in game data and scripts to screen coordinates.
    // Equals real game size by default, which results in 1:1 conversion.
    // (used mainly for backwards-compatibility in high-res games that wanted
    // to keep coordinates in low-resolution range in scripts)
    Size _nativeSize;
    // Game logic to game resolution coordinate factor
    int _upscaleMult;
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCTBASE_H