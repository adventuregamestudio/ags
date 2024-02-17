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
#ifndef __AC_CHARACTERINFO_H
#define __AC_CHARACTERINFO_H

#include <vector>
#include "core/types.h"
#include "ac/common_defines.h" // constants
#include "ac/game_version.h"
#include "util/bbop.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MAX_INV             301
// Character flags
#define CHF_MANUALSCALING   1
#define CHF_FIXVIEW         2     // between SetCharView and ReleaseCharView
#define CHF_NOINTERACT      4
#define CHF_NODIAGONAL      8
#define CHF_ALWAYSIDLE      0x10
#define CHF_NOLIGHTING      0x20
#define CHF_NOTURNING       0x40
#define CHF_NOWALKBEHINDS   0x80
#define CHF_FLIPSPRITE      0x100  // ?? Is this used??
#define CHF_NOBLOCKING      0x200
#define CHF_SCALEMOVESPEED  0x400
#define CHF_NOBLINKANDTHINK 0x800
#define CHF_SCALEVOLUME     0x1000
#define CHF_HASTINT         0x2000   // engine only
#define CHF_BEHINDSHEPHERD  0x4000   // engine only
#define CHF_AWAITINGMOVE    0x8000   // engine only
#define CHF_MOVENOTWALK     0x10000   // engine only - do not do walk anim
#define CHF_ANTIGLIDE       0x20000
#define CHF_HASLIGHT        0x40000
#define CHF_TINTLIGHTMASK   (CHF_NOLIGHTING | CHF_HASTINT | CHF_HASLIGHT)
// Speechcol is no longer part of the flags as of v2.5
#define OCHF_SPEECHCOL      0xff000000
#define OCHF_SPEECHCOLSHIFT 24
#define UNIFORM_WALK_SPEED  0
#define FOLLOW_ALWAYSONTOP  0x7ffe

// Length of deprecated character name field, in bytes
#define LEGACY_MAX_CHAR_NAME_LEN 40

// Character's internal flags, packed in CharacterInfo::animating
#define CHANIM_MASK         0xFF
#define CHANIM_ON           0x01
#define CHANIM_REPEAT       0x02
#define CHANIM_BACKWARDS    0x04


// Converts character flags (CHF_*) to matching RoomObject flags (OBJF_*)
inline int CharFlagsToObjFlags(int chflags)
{
    using namespace AGS::Common;
    return 
        FlagToFlag(chflags,   CHF_NOINTERACT,       OBJF_NOINTERACT) |
        FlagToFlag(chflags,   CHF_NOWALKBEHINDS,    OBJF_NOWALKBEHINDS) |
        FlagToFlag(chflags,   CHF_HASTINT,          OBJF_HASTINT) |
        FlagToFlag(chflags,   CHF_HASLIGHT,         OBJF_HASLIGHT) |
        // following flags are inverse
        FlagToNoFlag(chflags, CHF_NOLIGHTING,       OBJF_USEREGIONTINTS) |
        FlagToNoFlag(chflags, CHF_MANUALSCALING,    OBJF_USEROOMSCALING) |
        FlagToNoFlag(chflags, CHF_NOBLOCKING,       OBJF_SOLID);
}


enum CharacterSvgVersion
{
    kCharSvgVersion_Initial = 0, // [UNSUPPORTED] from 3.5.0 pre-alpha
    kCharSvgVersion_350     = 1, // new movelist format (along with pathfinder)
    kCharSvgVersion_36025   = 2, // animation volume
    kCharSvgVersion_36109   = 3, // removed movelists, save externally
    kCharSvgVersion_36115   = 4, // no limit on character name's length
};


// Predeclare a design-time Character extension
struct CharacterInfo2;
// Predeclare a runtime Character extension (TODO: refactor and remove this from here)
struct CharacterExtras;


// CharacterInfo is a design-time Character data.
// Contains original set of character fields.
// IMPORTANT: exposed to script API, and plugin API as AGSCharacter!
// For older script compatibility the struct also has to maintain its size,
// and be stored in a plain array to keep the relative memory address offsets
// between the Character objects!
// Do not add or change existing fields, unless planning breaking compatibility.
// Prefer to use CharacterInfo2 and CharacterExtras structs for any extensions.
//
// TODO: must refactor, some parts of it should be in a runtime Character class.
struct CharacterInfo
{
    int   defview;
    int   talkview;
    int   view;
    int   room, prevroom;
    int   x, y, wait;
    int   flags;
    short following;
    short followinfo;
    int   idleview;           // the loop will be randomly picked
    short idletime, idleleft; // num seconds idle before playing anim
    short transparency;       // if character is transparent
    short baseline;
    int   activeinv;
    int   talkcolor;
    int   thinkview;
    short blinkview, blinkinterval; // design time
    short blinktimer, blinkframe;   // run time
    short walkspeed_y;
    short pic_yoffs; // this is fixed in screen coordinates
    int   z;    // z-location, for flying etc
    int   walkwait;
    short speech_anim_speed, idle_anim_speed;
    short blocking_width, blocking_height;
    int   index_id;  // used for object functions to know the id
    short pic_xoffs; // this is fixed in screen coordinates
    short walkwaitcounter;
    uint16_t loop, frame;
    short walking; // stores movelist index, optionally +TURNING_AROUND
    short animating; // stores CHANIM_* flags in lower byte and delay in upper byte
    short walkspeed, animspeed;
    short inv[MAX_INV];
    short actx, acty;
    // These two name fields are deprecated, but must stay here
    // for compatibility with old scripts and plugin API
    char  name[LEGACY_MAX_CHAR_NAME_LEN];
    char  scrname[LEGACY_MAX_SCRIPT_NAME_LEN];
    char  on;

    int get_baseline() const;        // return baseline, or Y if not set
    int get_blocking_top() const;    // return Y - BlockingHeight/2
    int get_blocking_bottom() const; // return Y + BlockingHeight/2

    // Returns effective x/y walkspeeds for this character
    void get_effective_walkspeeds(int &walk_speed_x, int &walk_speed_y) const
    {
        walk_speed_x = walkspeed;
        walk_speed_y = ((walkspeed_y == UNIFORM_WALK_SPEED) ? walkspeed : walkspeed_y);
    }

    inline bool has_explicit_light() const { return (flags & CHF_HASLIGHT) != 0; }
    inline bool has_explicit_tint()  const { return (flags & CHF_HASTINT) != 0; }
    inline bool is_animating()       const { return (animating & CHANIM_ON) != 0; }
    inline int  get_anim_repeat()    const { return (animating & CHANIM_REPEAT) ? ANIM_REPEAT : ANIM_ONCE; }
    inline bool get_anim_forwards()  const { return (animating & CHANIM_BACKWARDS) == 0; }
    inline int  get_anim_delay()     const { return (animating >> 8) & 0xFF; }
    inline void set_animating(bool repeat, bool forwards, int delay)
    {
        animating = CHANIM_ON |
            (CHANIM_REPEAT * repeat) |
            (CHANIM_BACKWARDS * !forwards) |
            ((delay & 0xFF) << 8);
    }

	// [IKM] 2012-06-28: I still have to pass char_index to some of those functions
	// either because they use it to set some variables with it,
	// or because they pass it further to other functions, that are called from various places
	// and it would be too much to change them all simultaneously
    //
    // [IKM] 2016-08-26: these methods should NOT be in CharacterInfo class,
    // bit in distinct runtime character class!
	void UpdateMoveAndAnim(int &char_index, CharacterExtras *chex, std::vector<int> &followingAsSheep);
	void UpdateFollowingExactlyCharacter();

    bool update_character_turning(CharacterExtras *chex);
	void update_character_moving(int &char_index, CharacterExtras *chex, int &doing_nothing);
	bool update_character_animating(int &char_index, int &doing_nothing);
	void update_character_idle(CharacterExtras *chex, int &doing_nothing);
	void update_character_follower(int &char_index, std::vector<int> &followingAsSheep, int &doing_nothing);

    void ReadFromFile(Common::Stream *in, CharacterInfo2 &chinfo2, GameDataVersion data_ver);
    void WriteToFile(Common::Stream *out) const;
    // TODO: move to runtime-only class (?)
    void ReadFromSavegame(Common::Stream *in, CharacterInfo2 &chinfo2, CharacterSvgVersion save_ver);
    void WriteToSavegame(Common::Stream *out, const CharacterInfo2 &chinfo2) const;

private:
    // Helper functions that read and write first data fields,
    // common for both game file and save.
    void ReadBaseFields(Common::Stream *in);
    void WriteBaseFields(Common::Stream *out) const;
};


// Design-time Character extended fields
struct CharacterInfo2
{
    // Unrestricted scriptname and name fields
    AGS::Common::String scrname_new;
    AGS::Common::String name_new;
};


#if defined (OBSOLETE)
struct OldCharacterInfo {
    int   defview;
    int   talkview;
    int   view;
    int   room, prevroom;
    int   x, y, wait;
    int   flags;
    short following;
    short followinfo;
    int   idleview;           // the loop will be randomly picked
    short idletime, idleleft; // num seconds idle before playing anim
    short transparency;       // if character is transparent
    short baseline;
    int   activeinv;          // this is an INT to support SeeR (no signed shorts)
    short loop, frame;
    short walking, animating;
    short walkspeed, animspeed;
    short inv[100];
    short actx, acty;
    char  name[30];
    char  scrname[16];
    char  on;
};

void ConvertOldCharacterToNew (OldCharacterInfo *oci, CharacterInfo *ci);
#endif // OBSOLETE

#endif // __AC_CHARACTERINFO_H