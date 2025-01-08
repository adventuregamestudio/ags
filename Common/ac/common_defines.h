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
#ifndef __AC_DEFINES_H
#define __AC_DEFINES_H

#include "core/platform.h"

// Some arbitrary return values, should be replaced with either
// simple boolean, or HError
#define EXIT_NORMAL 0
#define EXIT_CRASH  92
#define EXIT_ERROR  93

// Legacy (UNSUPPORTED!) interaction script constants
//
// NUMCONDIT : whataction[0]:  Char walks off left
//                       [1]:  Char walks off right
//                       [2]:  Char walks off bottom
//                       [3]:  Char walks off top
//			 [4]:  First enters screen
//                       [5]:  Every time enters screen
//                       [6]:  execute every loop
//                [5]...[19]:  Char stands on lookat type
//               [20]...[35]:  Look at type
//               [36]...[49]:  Action on type
//               [50]...[65]:  Use inv on type
//               [66]...[75]:  Look at object
//               [76]...[85]:  Action on object
//               [86]...[95]:  Speak to object
//		[96]...[105]:  Use inv on object
//             [106]...[124]:  Misc conditions 1-20

// game ver     whataction[]=
// v1.00              0  :  Go to screen
//                    1  :  Don't do anything
//                    2  :  Can't walk
//                    3  :  Man dies
//                    4  :  Run animation
//                    5  :  Display message
//                    6  :  Remove an object (set object.on=0)
//		                7  :  Remove object & add Val2 to inventory
//                    8  :  Add Val1 to inventory (Val2=num times)
//                    9  :  Run a script
// v1.00 SR-1        10  :  Run graphical script
// v1.1              11  :  Play sound effect SOUND%d.WAV
// v1.12             12  :  Play FLI/FLC animation FLIC%d.FLC or FLIC%d.FLI
//                   13  :  Turn object on
// v2.00             14  :  Run conversation
#if defined (OBSOLETE)
#define NUM_MISC      20
#define NUMOTCON      7                 // number of conditions before standing on
#define NUM_CONDIT    (120 + NUMOTCON)
#define MISC_COND     (MAX_WALK_BEHINDS * 4 + NUMOTCON + MAX_ROOM_OBJECTS * 4)
#define NUMRESPONSE   14
#define NUMCOMMANDS   15
#define GO_TO_SCREEN  0
#define NO_ACTION     1
#define NO_WALK       2
#define MAN_DIES      3
#define RUN_ANIMATE   4
#define SHOW_MESSAGE  5
#define OBJECT_OFF    6
#define OBJECT_INV    7
#define ADD_INV       8
#define RUNSCRIPT     9
#define GRAPHSCRIPT   10
#define PLAY_SOUND    11
#define PLAY_FLI      12
#define OBJECT_ON     13
#define RUN_DIALOG    14
#endif

// Script name length limit for some game objects
#define LEGACY_MAX_SCRIPT_NAME_LEN 20
// Number of state-saved rooms
#define MAX_ROOMS 300
// Some obsolete room data, likely pre-2.5
#define MAX_LEGACY_ROOM_FLAGS 15
// Old object name limit
#define LEGACY_MAXOBJNAMELEN 30
// Max number of sprites in older versions
#define LEGACY_MAX_SPRITES_V25  6000
#define LEGACY_MAX_SPRITES      30000

// Max number of old-style global interaction variables
#define MAX_INTERACTION_VARIABLES 100
// An offset for local room's interaction variable IDs
#define LOCAL_INTER_VAR_OFFSET 10000

// The game to screen coordinate conversion multiplier, was used in older high-res games
#define HIRES_COORD_MULTIPLIER 2

// Room object flags (currently limited by a byte)
#define OBJF_NOINTERACT     0x01  // not clickable
#define OBJF_NOWALKBEHINDS  0x02  // ignore walk-behinds
#define OBJF_HASTINT        0x04  // the tint_* members are valid
#define OBJF_USEREGIONTINTS 0x08  // obey region tints/light areas
#define OBJF_USEROOMSCALING 0x10  // obey room scaling areas
#define OBJF_SOLID          0x20  // blocks characters from moving
#define OBJF_LEGACY_LOCKED  0x40  // object position is locked in the editor (OBSOLETE since 3.5.0)
#define OBJF_HASLIGHT       0x80  // the tint_light is valid and treated as brightness
#define OBJF_TINTLIGHTMASK  (OBJF_HASTINT | OBJF_HASLIGHT | OBJF_USEREGIONTINTS)

// Animation flow mode
// NOTE: had to move to common_defines, because used by CharacterInfo
// Animates once and stops at the *last* frame
#define ANIM_ONCE              0
// Animates infinitely until stopped by command
#define ANIM_REPEAT            1
// Animates once and stops, resetting to the very first frame
#define ANIM_ONCERESET         2

// An identifier of a "null font", a pseudo font used when you don't want a text to be drawn
#define FONT_NULL (-1)

#endif // __AC_DEFINES_H
