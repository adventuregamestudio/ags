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

// Character flags (32-bit)
// Flags marked as "runtime" are marking dynamic character state
#define CHF_MANUALSCALING   0x0001
#define CHF_FIXVIEW         0x0002  // between SetCharView and ReleaseCharView
#define CHF_NOINTERACT      0x0004
#define CHF_NODIAGONAL      0x0008
#define CHF_ALWAYSIDLE      0x0010
#define CHF_NOLIGHTING      0x0020  // TODO: rename this CHF_USEREGIONTINTS with opposite meaning
#define CHF_NOTURNING       0x0040
#define CHF_NOWALKBEHINDS   0x0080  // [DEPRECATED], forbidden as breaks draw order
#define CHF_FLIPSPRITE      0x0100  // [DEPRECATED], ancient
#define CHF_NOBLOCKING      0x0200
#define CHF_SCALEMOVESPEED  0x0400
#define CHF_NOBLINKANDTHINK 0x0800
#define CHF_SCALEVOLUME     0x1000
#define CHF_HASTINT         0x2000  // has applied individual tint
#define CHF_BEHINDSHEPHERD  0x4000  // draw character behind followed (CHECKME: logic unclear)
#define CHF_AWAITINGMOVE    0x8000  // runtime
#define CHF_MOVENOTWALK     0x00010000  // runtime - do not do walk anim
#define CHF_ANTIGLIDE       0x00020000
#define CHF_HASLIGHT        0x00040000
#define CHF_TINTLIGHTMASK   (CHF_NOLIGHTING | CHF_HASTINT | CHF_HASLIGHT)
#define CHF_ENABLED         0x00080000
#define CHF_VISIBLE         0x00100000

// Value of walk speed indicating that X and Y speed is the same
#define UNIFORM_WALK_SPEED  0
// Value of "followinfo" field that tells to draw follower char above followed
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
    kCharSvgVersion_400     = 4000000, // extended graphic effects (blend, rotate,...)
    kCharSvgVersion_400_03  = 4000003, // compat with kCharSvgVersion_36115
};

struct CharacterExtras;

// Design-time Character data.
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
    AGS::Common::String scrname; // script name
    AGS::Common::String name; // regular name (aka description)

    int get_baseline() const;        // return baseline, or Y if not set
    int get_blocking_top() const;    // return Y - BlockingHeight/2
    int get_blocking_bottom() const; // return Y + BlockingHeight/2

    // Tells if the "enabled" flag is set
    inline bool is_enabled() const { return (flags & CHF_ENABLED) != 0; }
    // Tells if the "visible" flag is set
    inline bool is_visible() const { return (flags & CHF_VISIBLE) != 0; }
    // Tells if the character is actually meant to be displayed on screen;
    // this combines both "enabled" and "visible" factors.
    inline bool is_displayed() const { return is_enabled() && is_visible(); }
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
    inline void set_enabled(bool on) { flags = (flags & ~CHF_ENABLED) | (CHF_ENABLED * on); }
    inline void set_visible(bool on) { flags = (flags & ~CHF_VISIBLE) | (CHF_VISIBLE * on); }
    inline void set_animating(bool repeat, bool forwards, int delay)
    {
        animating = CHANIM_ON |
            (CHANIM_REPEAT * repeat) |
            (CHANIM_BACKWARDS * !forwards) |
            ((delay & 0xFF) << 8);
    }

	// TODO: these methods should NOT be in CharacterInfo class,
    // bit in distinct runtime character class! were moved to CharacterInfo by mistake.
    // NOTE: char_index had still be passed to some of those functions
	// either because they use it to set some variables with it,
	// or because they pass it further to other functions, that are called from various places
	// and it would be too much to change them all simultaneously.
	void UpdateMoveAndAnim(int &char_index, CharacterExtras *chex, std::vector<int> &followingAsSheep);
	void UpdateFollowingExactlyCharacter();

    bool update_character_turning(CharacterExtras *chex);
	void update_character_moving(int &char_index, CharacterExtras *chex, int &doing_nothing);
	bool update_character_animating(int &char_index, int &doing_nothing);
	void update_character_idle(CharacterExtras *chex, int &doing_nothing);
	void update_character_follower(int &char_index, std::vector<int> &followingAsSheep, int &doing_nothing);

    void ReadFromFile(Common::Stream *in, GameDataVersion data_ver);
    void WriteToFile(Common::Stream *out) const;
    // TODO: move to runtime-only class (?)
    void ReadFromSavegame(Common::Stream *in, CharacterSvgVersion save_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    // Helper functions that read and write first data fields,
    // common for both game file and save.
    void ReadBaseFields(Common::Stream *in);
    void WriteBaseFields(Common::Stream *out) const;
};

#endif // __AC_CHARACTERINFO_H
