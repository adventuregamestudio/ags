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
#include "util/file.h"
#include "util/wgt2allg.h"

// Forward declaration
namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

// This struct is written directly to the disk file // [IKM] not really anymore
// The GameSetupStruct subclass parts are written individually
//
// [IKM] Do not change the order of variables in this struct!
// Until the serialization and script referencing methods are improved
// game execution will depend on actual object addresses in memory
//
struct GameSetupStructBase {
    char              gamename[50];
    int32             options[100];
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
    int32             default_resolution; // 0=undefined, 1=320x200, 2=320x240, 3=640x400 etc
    int32             default_lipsync_frame; // used for unknown chars
    int32             invhotdotsprite;
    int32             reserved[17];
    char             *messages[MAXGLOBALMES];
    WordsDictionary  *dict;
    char             *globalscript;
    CharacterInfo    *chars;
    ccScript         *compiled_script;

    void ReadFromFile(Common::DataStream *in);
    void WriteToFile(Common::DataStream *out);
};

#endif // __AGS_CN_AC__GAMESETUPSTRUCTBASE_H