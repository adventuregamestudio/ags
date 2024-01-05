//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_CN_AC__OLDGAMESETUPSTRUCT_H
#define __AGS_CN_AC__OLDGAMESETUPSTRUCT_H

#if defined (OBSOLETE)

#include "ac/characterinfo.h"       // OldCharacterInfo, CharacterInfo
#include "ac/eventblock.h"       // EventBlock
#include "ac/interfaceelement.h"    // InterfaceElement
#include "ac/inventoryiteminfo.h"   // InventoryItemInfo
#include "ac/mousecursor.h"      // MouseCursor
#include "ac/wordsdictionary.h"  // WordsDictionary
#include "script/cc_script.h"           // ccScript

struct OriGameSetupStruct {
    char              gamename[30];
    char              options[20];
    unsigned char     paluses[256];
    RGB               defpal[256];
    InterfaceElement  iface[10];
    int               numiface;
    int               numviews;
    MouseCursor       mcurs[10];
    char              *globalscript;
    int               numcharacters;
    OldCharacterInfo     *chars;
#if defined (OBSOLETE)
    EventBlock        __charcond[50];
    EventBlock        __invcond[100];
#endif
    ccScript          *compiled_script;
    int               playercharacter;
    unsigned char     __old_spriteflags[2100];
    int               totalscore;
    short             numinvitems;
    InventoryItemInfo invinfo[100];
    int               numdialog, numdlgmessage;
    int               numfonts;
    int               color_depth;              // in bytes per pixel (ie. 1 or 2)
    int               target_win;
    int               dialog_bullet;            // 0 for none, otherwise slot num of bullet point
    short             hotdot, hotdotouter;   // inv cursor hotspot dot
    int               uniqueid;    // random key identifying the game
    int               reserved[2];
    short             numlang;
    char              langcodes[MAXLANGUAGE][3];
    char              *messages[MAXGLOBALMES];
};

struct OriGameSetupStruct2 : public OriGameSetupStruct {
    unsigned char   fontflags[10];
    char            fontoutline[10];
    int             numgui;
    WordsDictionary *dict;
    int             reserved2[8];
};

struct OldGameSetupStruct : public OriGameSetupStruct2 {
    unsigned char spriteflags[LEGACY_MAX_SPRITES_V25];
};

#endif // OBSOLETE

#endif // __AGS_CN_AC__OLDGAMESETUPSTRUCT_H
