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

#ifndef __AC_DEFINES_H
#define __AC_DEFINES_H

#define EXIT_NORMAL 91
#define EXIT_CRASH  92

#define MAX_INIT_SPR  40
#define MAX_OBJ       16  // max walk-behinds
#define NUM_MISC      20
#define MAXMESS       100
#define NUMOTCON      7                 // number of conditions before standing on
#define NUM_CONDIT    (120 + NUMOTCON)
#define MAX_HOTSPOTS  50   // v2.62 increased from 20 to 30; v2.8 to 50
#define MAX_REGIONS   16

// careful with this - the shadinginfo[] array needs to be
// MAX_WALK_AREAS + 1 if this gets changed
#define MAX_WALK_AREAS 15

#define MAX_SCRIPT_NAME_LEN 20

//const int MISC_COND = MAX_OBJ * 4 + NUMOTCON + MAX_INIT_SPR * 4;

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


#define MAX_ROOMS 300

#define MAXANIMS      10
#define MAX_FLAGS     15
#define LEGACY_MAXOBJNAMELEN 30
#define MAX_BSCENE    5   // max number of frames in animating bg scene

#define MAX_SPRITES         30000
#define MAX_CURSOR          20

#ifndef int32
#define int32 int
#endif

#ifdef WINDOWS_VERSION
#define AGS_INLINE inline
#else
// the linux compiler won't allow extern inline
#define AGS_INLINE
#endif

// object flags (currently only a char)
#define OBJF_NOINTERACT        1  // not clickable
#define OBJF_NOWALKBEHINDS     2  // ignore walk-behinds
#define OBJF_HASTINT           4  // the tint_* members are valid
#define OBJF_USEREGIONTINTS    8  // obey region tints/light areas
#define OBJF_USEROOMSCALING 0x10  // obey room scaling areas
#define OBJF_SOLID          0x20  // blocks characters from moving
#define OBJF_LOCKED         0x40  // object position is locked in the editor
#define OBJF_HASLIGHT       0x80  // the tint_light is valid and treated as brightness

#endif // __AC_DEFINES_H
