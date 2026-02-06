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
#ifndef __AC_CHARACTERINFO_H
#define __AC_CHARACTERINFO_H

#include <algorithm>
#include <vector>
#include "core/types.h"
#include "ac/common_defines.h" // constants
#include "ac/game_version.h"
#include "util/bbop.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// Max inventory items in character's inventory
#define MAX_INV             301
// Character flags
// NOTE: flag meaning is inconsistent: some of them have positive (DO) meaning,
// some older ones have negative (DON'T). TODO: bring them to consistency someday,
// but remember that this involves updating game file formats and converting
// after loading game data and restoring older saves.
#define CHF_MANUALSCALING   1        // Use explicit scaling property rather than area parameters
#define CHF_FIXVIEW         2        // View locked
#define CHF_NOINTERACT      4        // Non-interactable (non-clickable)
#define CHF_NODIAGONAL      8        // Don't use diagonal walking loops
#define CHF_ALWAYSIDLE      0x10     // [UNUSED] meaning unknown
#define CHF_NOLIGHTING      0x20     // Ignore Region lighting
#define CHF_NOTURNWHENWALK  0x40     // Do not turn step-by-step when walking
#define CHF_NOWALKBEHINDS   0x80     // Ignore walk-behinds (always draw above)
#define CHF_FLIPSPRITE      0x100    // [UNUSED] meaning unknown
#define CHF_NOBLOCKING      0x200    // Not solid
#define CHF_SCALEMOVESPEED  0x400    // Scale move speed with character scaling
#define CHF_NOBLINKANDTHINK 0x800    // Don't do blink animation when "thinking"
#define CHF_SCALEVOLUME     0x1000   // Scale animation volume with character scaling
#define CHF_HASTINT         0x2000   // Use explicit tint rather than region tint
#define CHF_BEHINDSHEPHERD  0x4000   // [INTERNAL] z-sort behind leader when following another char
#define CHF_AWAITINGMOVE    0x8000   // [INTERNAL] (meaning not clear, investigate)
#define CHF_MOVENOTWALK     0x10000  // [INTERNAL] do not play walking animation while moving
#define CHF_ANTIGLIDE       0x20000  // Link movement to animation
#define CHF_HASLIGHT        0x40000  // Use explicit lighting rather than region lighting
#define CHF_TINTLIGHTMASK   (CHF_NOLIGHTING | CHF_HASTINT | CHF_HASLIGHT)
#define CHF_TURNWHENFACE    0x80000  // Turn step-by-step when changing standing direction
// Pre-v2.5 bit mask for when the speechcolor was stored in character flags
#define OCHF_SPEECHCOL      0xff000000
#define OCHF_SPEECHCOLSHIFT 24
// Value of CharacterInfo::walkspeed_y that tells to use walkspeed_x
#define UNIFORM_WALK_SPEED  0
// Value of CharacterInfo::followinfo that tells to keep follower z-sorted above the leading char
#define FOLLOW_ALWAYSONTOP  0x7ffe

// Length of deprecated character name field, in bytes
#define LEGACY_MAX_CHAR_NAME_LEN 40

// Character's internal flags, packed in CharacterInfo::animating
#define CHANIM_MASK         0xFF
#define CHANIM_ON           0x01
#define CHANIM_REPEAT       0x02
#define CHANIM_BACKWARDS    0x04

// These flags are merged with the CharacterInfo's MoveList index;
// but this means that the number of MoveList users will be limited by 1000
#define TURNING_AROUND      1000
#define TURNING_BACKWARDS   10000


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
    kCharSvgVersion_36205   = 3060205, // 32-bit "following" parameters
    kCharSvgVersion_36304   = 3060304, // blocking x,y
};


// Predeclare a design-time Character extension
struct CharacterInfo2;


// CharacterInfo is a design-time Character data.
// Contains original set of character fields.
// ----------------------------------------------------------------------------
// IMPORTANT: exposed to script API, and plugin API as AGSCharacter!
// For older script compatibility the struct also has to maintain its size,
// and be stored in a plain array to keep the relative memory address offsets
// between the Character script objects!
// ----------------------------------------------------------------------------
// Do not add or change existing fields, unless planning breaking compatibility.
// Prefer to use CharacterInfo2 struct for extended fields that are setup at
// design-time and CharacterExtras for runtime only extended fields.
//
// TODO: must refactor, some parts of it should be in a runtime Character class.
// TODO: as a idea, we might have a data struct for serialization, which has all
// design-time Character fields, and then a specialized runtime struct for
// exporting into script API / plugin API.
struct CharacterInfo
{
    int     defview     = 0;
    int     talkview    = 0;
    int     view        = 0;
    int     room        = 0;
    int     prevroom    = 0;
    int     x           = 0;
    int     y           = 0;
    int     wait        = 0;
    int     flags       = 0;  // CHF_* flags
    int16_t legacy_following = -1;  // deprecated 16-bit values that store follow params
    int16_t legacy_followinfo = 0;  // -- left for the script and plugin compatibility only
    int     idleview    = 0;  // the loop will be randomly picked
    int16_t idletime    = 0;
    int16_t idleleft    = 0; // num seconds idle before playing anim
    int16_t transparency = 0; // "incorrect" alpha (in legacy 255-range units)
    int16_t baseline    = -1;
    int     activeinv   = -1; // selected inventory item
    int     talkcolor   = 0;
    int     thinkview   = 0;
    int16_t blinkview   = 0;
    int16_t blinkinterval = 0;
    int16_t blinktimer  = 0;
    int16_t blinkframe  = 0;
    int16_t walkspeed_y = 0;
    int16_t pic_yoffs   = 0; // this is fixed in screen coordinates
    int     z           = 0; // z-location, for flying etc
    int     walkwait    = 0;
    int16_t speech_anim_speed = 0;
    int16_t idle_anim_speed = 0;
    int16_t blocking_width = 0;
    int16_t blocking_height = 0;
    int     index_id    = 0; // this character's numeric ID
    int16_t pic_xoffs   = 0; // this is fixed in screen coordinates
    int16_t walkwaitcounter = 0;
    uint16_t loop       = 0;
    uint16_t frame      = 0;
    int16_t walking     = 0; // stores movelist index, optionally +TURNING_AROUND
    int16_t animating   = 0; // stores CHANIM_* flags in lower byte and delay in upper byte
    int16_t walkspeed   = 0;
    int16_t animspeed   = 0;
    int16_t inv[MAX_INV] = { 0 }; // quantities of each inventory item in game
    int16_t actx        = 0;
    int16_t acty        = 0;
    // These two name fields are deprecated, but must stay here
    // for compatibility with old scripts and plugin API
    char    name[LEGACY_MAX_CHAR_NAME_LEN] = { 0 };
    char    scrname[LEGACY_MAX_SCRIPT_NAME_LEN] = { 0 };
    int8_t  on          = 0; // is character enabled

    int get_baseline() const;        // return baseline, or Y if not set
    int get_blocking_top() const;    // return Y - BlockingHeight/2
    int get_blocking_bottom() const; // return Y + BlockingHeight/2

    // Returns effective x/y walkspeeds for this character
    void get_effective_walkspeeds(int &walk_speed_x, int &walk_speed_y) const
    {
        walk_speed_x = walkspeed;
        walk_speed_y = ((walkspeed_y == UNIFORM_WALK_SPEED) ? walkspeed : walkspeed_y);
    }

    // Gets current character's movelist id
    inline int  get_movelist_id()    const { return walking % TURNING_AROUND; }
    // Tells if the character is performing a move, that is - either moving along
    // a path or turning around; the latter may be either turning to face something,
    // or turning between move path segments.
    inline bool is_moving() const { return walking > 0; }
    // Tells if the character is turning; this may be either just turning on spot,
    // or turning between move path segments.
    inline bool is_turning() const { return walking >= TURNING_AROUND; }
    // Tells if the character has a valid move order, meaning it's actually
    // moving along the path (and not e.g. turning on spot to face something).
    inline bool is_moving_onpath() const { return get_movelist_id() > 0; }
    // Is moving *and* playing walking animation
    // FIXME: would be proper to also test is_moving_onpath() in is_moving_walkanim()
    // and in is_moving_no_anim(), but it breaks animation logic;
    // some further adjustments would be required in the engine code.
    inline bool is_moving_walkanim() const { return (flags & CHF_MOVENOTWALK) == 0; }
    // Is moving, but *not* playing walking animation
    inline bool is_moving_no_anim()  const { return (flags & CHF_MOVENOTWALK) != 0; }

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

    inline bool is_idling()          const { return (idleleft < 0); }

    // Gets legacy follow distance, limited to a 8-bit unsigned value
    inline int get_follow_distance() const
    {
        return (legacy_followinfo == FOLLOW_ALWAYSONTOP) ? FOLLOW_ALWAYSONTOP : (legacy_followinfo >> 8);
    }
    // Gets legacy follow eagerness, limited to a 8-bit unsigned value
    inline int get_follow_eagerness() const
    {
        return (legacy_followinfo == FOLLOW_ALWAYSONTOP) ? 0 : (legacy_followinfo & 0xFF);
    }
    inline bool get_follow_sort_behind() const
    {
        return (flags & CHF_BEHINDSHEPHERD) != 0;
    }

    // Sets "following" flags;
    // sets legacy "followinfo" values: this is for plugin API and old script API compatibility
    void set_following(int16_t follow_whom, int dist = 0, int eagerness = 0, bool sort_behind = false)
    {
        legacy_following = follow_whom;
        if (dist == FOLLOW_ALWAYSONTOP)
        {
            flags = (flags & ~CHF_BEHINDSHEPHERD) | (CHF_BEHINDSHEPHERD * sort_behind);
            legacy_followinfo = FOLLOW_ALWAYSONTOP;
        }
        else
        {
            flags = (flags & ~CHF_BEHINDSHEPHERD);
            legacy_followinfo = (std::min<int>(UINT8_MAX, dist) << 8) | std::min<int>(UINT8_MAX, eagerness);
        }
    }

    void ReadFromFile(CharacterInfo2 &chinfo2, Common::Stream *in, GameDataVersion data_ver);
    void WriteToFile(const CharacterInfo2 &chinfo2, Common::Stream *out) const;
    // TODO: move to runtime-only class (?)
    void ReadFromSavegame(CharacterInfo2 &chinfo2, Common::Stream *in, CharacterSvgVersion save_ver);
    void WriteToSavegame(const CharacterInfo2 &chinfo2, Common::Stream *out) const;
};


// Design-time Character extended fields.
// This struct has to be separate from CharacterInfo, because CharacterInfo
// is exported to script and plugin APIs, and therefore has size constraints.
// See comments to CharacterInfo for more details.
struct CharacterInfo2
{
    // Unrestricted scriptname and name fields
    AGS::Common::String scrname_new;
    AGS::Common::String name_new;

    // relative offset of the blocking rect
    int blocking_x = 0;
    int blocking_y = 0;
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