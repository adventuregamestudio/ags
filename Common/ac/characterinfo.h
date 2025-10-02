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
#ifndef __AC_CHARACTERINFO_H
#define __AC_CHARACTERINFO_H

#include <algorithm>
#include <vector>
#include "core/types.h"
#include "ac/common_defines.h" // constants
#include "ac/game_version.h"
#include "game/scripteventtable.h"
#include "util/bbop.h"
#include "util/string_types.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

// Character flags (32-bit)
// Flags marked as "INTERNAL" are marking dynamic character state set by the engine
// TODO: move internal flags to a separate 32-bit field in a runtime character class.
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
#define CHF_NOWALKBEHINDS   0x80     // [DEPRECATED], forbidden as breaks draw order
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
// reserve (skip) 4 bits for compatibility with 3.* branch (CHECKME revise some time later)
#define CHF_ENABLED         0x01000000
#define CHF_VISIBLE         0x02000000

// Value of walk speed indicating that X and Y speed is the same
// Value of CharacterInfo::walkspeed_y that tells to use walkspeed_x
#define UNIFORM_WALK_SPEED  0
// Value of "followinfo" field that tells to draw follower char above followed
// Value of CharacterInfo::followinfo that tells to keep follower z-sorted above the leading char
#define FOLLOW_ALWAYSONTOP  0x7ffe

// Length of deprecated character name field, in bytes
#define LEGACY_MAX_CHAR_NAME_LEN 40

// Legacy Character's animation flags, packed in a byte
#define LEGACY_CHANIM_MASK         0xFF
#define LEGACY_CHANIM_ON           0x01
#define LEGACY_CHANIM_REPEAT       0x02
#define LEGACY_CHANIM_BACKWARDS    0x04

// These deprecated values were **added** to CharacterInfo's MoveList index;
// TURNING_AROUND was added times the character must turn,
// TURNING_BACKWARDS was added if the character is turning counter-clockwise;
// which meant that the number of MoveList users will be limited by 1000,
// and the number of possible turns limited to 9.
#define LEGACY_CHAR_TURNING_AROUND      1000
#define LEGACY_CHAR_TURNING_BACKWARDS   10000


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
    kCharSvgVersion_400     = 4000000, // extended graphic effects (blend, rotate,...)
    kCharSvgVersion_400_03  = 4000003, // compat with kCharSvgVersion_36115
    kCharSvgVersion_400_09  = 4000009, // 32-bit color properties
    kCharSvgVersion_400_13  = 4000013, // compat with kCharSvgVersion_36205
    kCharSvgVersion_400_14  = 4000014, // proper ARGB color properties
    kCharSvgVersion_400_16  = 4000016, // motion path exposed to script, separate runtime flags, turns
    kCharSvgVersion_400_18  = 4000018, // shaders
    kCharSvgVersion_400_20  = 4000020, // expanded and bit more consistent anim params serialization
};

// Character event indexes
enum CharacterEventID
{
    // an interaction with any cursor mode that normally has a event
    kCharacterEvent_AnyClick = 0
};

// Design-time Character data.
// TODO: must refactor, some parts of it should be in a runtime Character class.
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
    int     idleview    = 0;  // the loop will be randomly picked
    int16_t idletime    = 0;
    int16_t idleleft    = 0; // num seconds idle before playing anim
    int16_t transparency = 0; // level of transparency (0 - 100)
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
    int     walkwait    = 0; // number of frames to wait before advancing a move;
                             // also used as a turning counter
    int16_t speech_anim_speed = 0;
    int16_t idle_anim_speed = 0;
    int16_t blocking_width = 0;
    int16_t blocking_height = 0;
    int     index_id    = 0; // this character's numeric ID
    int16_t pic_xoffs   = 0; // this is fixed in screen coordinates
    int16_t walkwaitcounter = 0;
    uint16_t loop       = 0;
    uint16_t frame      = 0;
    int16_t walking     = 0; // stores movelist index
    int16_t walkspeed   = 0;
    int16_t animspeed   = 0;
    int16_t inv[MAX_INV] = { 0 }; // quantities of each inventory item in game
    AGS::Common::String scrname = {}; // script name
    AGS::Common::String name = {}; // regular name (aka description)
    // Interaction events (cursor-based)
    AGS::Common::ScriptEventHandlers interactions = {};
    // Common events
    AGS::Common::ScriptEventTable Events = AGS::Common::ScriptEventTable(&CharacterInfo::_eventSchema);

    // Gets a events schema corresponding to this object's type
    static const AGS::Common::ScriptEventSchema &GetEventSchema() { return CharacterInfo::_eventSchema; }

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

    // Gets current character's movelist id
    inline int  get_movelist_id()    const { return walking; }
    // Tells if the character is performing a move, that is - either moving along
    // a path or turning between move path segments.
    inline bool is_moving()          const { return walking > 0; }
    // Is moving *and* playing walking animation
    // FIXME: would be proper to also test is_moving() in is_moving_walkanim()
    // and in is_moving_no_anim(), but it breaks animation logic;
    // some further adjustments would be required in the engine code.
    inline bool is_moving_walkanim() const { return (flags & CHF_MOVENOTWALK) == 0; }
    // Is moving, but *not* playing walking animation
    inline bool is_moving_no_anim()  const { return (flags & CHF_MOVENOTWALK) != 0; }
    inline bool has_explicit_light() const { return (flags & CHF_HASLIGHT) != 0; }
    inline bool has_explicit_tint()  const { return (flags & CHF_HASTINT) != 0; }
    inline void set_enabled(bool on) { flags = (flags & ~CHF_ENABLED) | (CHF_ENABLED * on); }
    inline void set_visible(bool on) { flags = (flags & ~CHF_VISIBLE) | (CHF_VISIBLE * on); }

    inline bool is_idling()          const { return (idleleft < 0); }

    // Gets if character follows another, while being drawn behind
    inline bool get_follow_sort_behind() const
    {
        return (flags & CHF_BEHINDSHEPHERD) != 0;
    }
    // Sets "following sort behind" flag
    void set_following_sortbehind(bool sort_behind = false)
    {
        flags = (flags & ~CHF_BEHINDSHEPHERD) | (CHF_BEHINDSHEPHERD * sort_behind);
    }

    CharacterInfo() = default;
    CharacterInfo(const CharacterInfo&) = default;
    CharacterInfo(CharacterInfo &&) = default;

    CharacterInfo &operator = (const CharacterInfo&) = default;
    CharacterInfo &operator = (CharacterInfo&&) = default;

    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

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

    // Script events schema
    static AGS::Common::ScriptEventSchema _eventSchema;
};

#endif // __AC_CHARACTERINFO_H
