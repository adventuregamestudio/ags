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
#include <allegro.h> // RGB
#include "ac/game_version.h"
#include "ac/gamestructdefines.h"
#include "util/string.h"

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
    RGB               defpal[256];
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
    Common::String    messages[MAXGLOBALMES];
    WordsDictionary  *dict;
    char             *globalscript;
    CharacterInfo    *chars;
    ccScript         *CompiledScript;

    // TODO: refactor to not have this as struct members
    int             *load_messages;
    bool             load_dictionary;
    bool             load_compiled_script;
    // [IKM] 2013-03-30
    // NOTE: it looks like nor 'globalscript', not 'CompiledScript' are used
    // to store actual script data anytime; 'ccScript* gamescript' global
    // pointer is used for that instead.

    GameSetupStructBase();
    ~GameSetupStructBase();
    void Free();
    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);

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

private:
    void OnResolutionSet();

    // Determines game's actual resolution.
    Size _gameResolution;
    // Multiplier for various UI drawin sizes, meant to keep UI elements readable
    int _relativeUIMult;
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCTBASE_H