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

#include "ac/characterinfo.h"       // OldCharacterInfo, CharacterInfo
#include "ac/wordsdictionary.h"  // WordsDictionary
#include "ac/gamestructdefines.h"
#include "script/cc_script.h"           // ccScript
#include "util/wgt2allg.h" // color (allegro RGB)

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct GameSetupStructBase {
    static const int  MAX_OPTIONS = 100;

    char              gamename[50];
    int32             options[MAX_OPTIONS];
    unsigned char     paluses[256];
    color             defpal[256];
    int32             numviews;
    int32             numcharacters;
    int32             playercharacter;
    int32             totalscore;
    short             numinvitems;
    int32             numdialog, numdlgmessage;
    int32             numfonts;
    int32             color_depth;          // in bytes per pixel (ie. 1 or 2)
    int32             target_win;
    int32             dialog_bullet;        // 0 for none, otherwise slot num of bullet point
    unsigned short    hotdot, hotdotouter;  // inv cursor hotspot dot
    int32             uniqueid;    // random key identifying the game
    int32             numgui;
    int32             numcursors;
    int32             default_lipsync_frame; // used for unknown chars
    int32             invhotdotsprite;
    int32             reserved[17];
    char             *messages[MAXGLOBALMES];
    WordsDictionary  *dict;
    char             *globalscript;
    CharacterInfo    *chars;
    ccScript         *compiled_script;
    Size              size;                 // native game size in pixels

    int32_t          *load_messages;
    bool             load_dictionary;
    bool             load_compiled_script;
    // [IKM] 2013-03-30
    // NOTE: it looks like nor 'globalscript', not 'compiled_script' are used
    // to store actual script data anytime; 'ccScript* gamescript' global
    // pointer is used for that instead.

    GameSetupStructBase();
    virtual ~GameSetupStructBase();
    void SetDefaultResolution(GameResolutionType resolution_type);
    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);

    inline GameResolutionType GetDefaultResolution() const
    {
        return default_resolution;
    }
    inline bool IsHiRes() const
    {
        return ::IsHiRes(default_resolution);
    }

private:
    GameResolutionType default_resolution; // game size identifier
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCTBASE_H