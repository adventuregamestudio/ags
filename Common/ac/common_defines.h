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
#ifndef __AC_DEFINES_H
#define __AC_DEFINES_H

#include "core/platform.h"

// Some arbitrary return values, should be replaced with either
// simple boolean, or HError
#define EXIT_NORMAL 0
#define EXIT_CRASH  92
#define EXIT_ERROR  93

// Script name length limit for some game objects
#define LEGACY_MAX_SCRIPT_NAME_LEN 20
// Max inventory items
#define MAX_INV   301
// Number of state-saved rooms
#define MAX_ROOMS 300
// Max number of old-style global interaction variables
#define MAX_INTERACTION_VARIABLES 100
// An offset for local room's interaction variable IDs
#define LOCAL_INTER_VAR_OFFSET 10000


// Room object flags
// (design-time values are now limited to 16-bit by the game file format)
#define OBJF_NOINTERACT     0x0001  // not clickable
#define OBJF_NOWALKBEHINDS  0x0002  // ignore walk-behinds
#define OBJF_HASTINT        0x0004  // the tint_* members are valid
#define OBJF_USEREGIONTINTS 0x0008  // obey region tints/light areas
#define OBJF_USEROOMSCALING 0x0010  // obey room scaling areas
#define OBJF_SOLID          0x0020  // blocks characters from moving
#define OBJF_LEGACY_LOCKED  0x0040  // object position is locked in the editor (OBSOLETE since 3.5.0)
#define OBJF_HASLIGHT       0x0080  // the tint_light is valid and treated as brightness
#define OBJF_TINTLIGHTMASK  (OBJF_HASTINT | OBJF_HASLIGHT | OBJF_USEREGIONTINTS)
#define OBJF_ENABLED        0x0100
#define OBJF_VISIBLE        0x0200

// Animation flow mode
// NOTE: had to move to common_defines, because used by CharacterInfo
// Animates once and stops at the *last* frame
#define ANIM_ONCE              0
// Animates infinitely until stopped by command
#define ANIM_REPEAT            1
// Animates once and stops, resetting to the very first frame
#define ANIM_ONCERESET         2

#endif // __AC_DEFINES_H
