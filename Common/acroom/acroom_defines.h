
#ifndef __CROOM_DEFINES_H
#define __CROOM_DEFINES_H

#define EXIT_NORMAL 91
#define EXIT_CRASH  92

#define ROOM_FILE_VERSION 29
/* room file versions history
8:  final v1.14 release
9:  intermediate v2 alpha releases
10:  v2 alpha-7 release
11:  final v2.00 release
12:  v2.08, to add colour depth byte
13:  v2.14, add walkarea light levels
14:  v2.4, fixed so it saves walkable area 15
15:  v2.41, supports NewInteraction
16:  v2.5
17:  v2.5 - just version change to force room re-compile for new charctr struct
18:  v2.51 - vector scaling
19:  v2.53 - interaction variables
20:  v2.55 - shared palette backgrounds
21:  v2.55 - regions
22:  v2.61 - encrypt room messages
23:  v2.62 - object flags
24:  v2.7  - hotspot script names
25:  v2.72 - game id embedded
26:  v3.0 - new interaction format, and no script source
27:  v3.0 - store Y of bottom of object, not top
28:  v3.0.3 - remove hotspot name length limit
29:  v3.0.3 - high-res coords for object x/y, edges and hotspot walk-to point
*/
#define MAX_INIT_SPR  40
#define MAX_OBJ       16  // max walk-behinds
#define NUM_MISC      20
#define MAXMESS       100
#define NUMOTCON      7                 // number of conditions before standing on
#define NUM_CONDIT    (120 + NUMOTCON)
#define MAX_HOTSPOTS  50   // v2.62 increased from 20 to 30; v2.8 to 50
#define MAX_REGIONS   16

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


#ifdef DJGPP
#include <unistd.h>
#endif

#ifdef _MSC_VER
#undef VTA_LEFT
#undef VTA_RIGHT
#endif

#ifdef DJGPP
#define PCKD __attribute__((packed))
#else
#define PCKD
#endif

#define int32 int

#endif
