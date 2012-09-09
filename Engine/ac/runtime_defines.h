#ifndef __AC_RUNTIMEDEFINES_H
#define __AC_RUNTIMEDEFINES_H

// Max script string length
#define MAX_MAXSTRLEN 200
#define MAXGLOBALVARS 50

#define MAX_SCRIPT_MODULES 50

#define INVALID_X  30000
#define MAXGSVALUES 500
#define MAXGLOBALSTRINGS 51
#define MAX_INVORDER 500
#define SCALIGN_LEFT     1
#define SCALIGN_CENTRE   2
#define SCALIGN_RIGHT    3
#define DIALOG_NONE      0
#define DIALOG_RUNNING   1
#define DIALOG_STOP      2
#define DIALOG_NEWROOM   100
#define DIALOG_NEWTOPIC  12000
#define MAX_TIMERS       21
#define MAX_PARSED_WORDS 15
#define MAXSAVEGAMES     50
#define MAX_QUEUED_MUSIC 10
#define GLED_INTERACTION 1
#define GLED_EFFECTS     2 
#define QUEUED_MUSIC_REPEAT 10000
#define PLAYMP3FILE_MAX_FILENAME_LEN 50
#define MAX_AUDIO_TYPES  30

// These numbers were chosen arbitrarily -- the idea is
// to make sure that the user gets the parameters the right way round
#define ANYWHERE       304
#define WALKABLE_AREAS 305
#define BLOCKING       919
#define IN_BACKGROUND  920
#define FORWARDS       1062
#define BACKWARDS      1063

#define SCR_NO_VALUE   31998
#define SCR_COLOR_TRANSPARENT -1



#define NUM_DIGI_VOICES     16
#define NUM_MOD_DIGI_VOICES 12

#define DEBUG_CONSOLE_NUMLINES 6
#define TXT_SCOREBAR        29
#define MAXSCORE play.totalscore
#define CHANIM_REPEAT    2
#define CHANIM_BACKWARDS 4
#define ANIM_BACKWARDS 10
#define ANIM_ONCE      1
#define ANIM_REPEAT    2
#define ANIM_ONCERESET 3
#define FONT_STATUSBAR  0
#define FONT_NORMAL     play.normal_font
//#define FONT_SPEECHBACK 1
#define FONT_SPEECH     play.speech_font
#define MODE_WALK 0
#define MODE_LOOK 1
#define MODE_HAND 2
#define MODE_TALK 3
#define MODE_USE  4
#define MODE_PICKUP 5
#define CURS_ARROW  6
#define CURS_WAIT   7
#define MODE_CUSTOM1 8
#define MODE_CUSTOM2 9

#define OVER_TEXTMSG  1
#define OVER_COMPLETE 2
#define OVER_PICTURE  3
#define OVER_CUSTOM   100
#define OVR_AUTOPLACE 30000
#define FOR_ANIMATION 1
#define FOR_SCRIPT    2
#define FOR_EXITLOOP  3
#define opts usetup
#define CHMLSOFFS (MAX_INIT_SPR+1)    // reserve this many movelists for objects & stuff
#define MAX_SCREEN_OVERLAYS 20
#define abort_all_conditions restrict_until
#define MAX_SCRIPT_AT_ONCE 10
#define EVENT_NONE       0
#define EVENT_INPROGRESS 1
#define EVENT_CLAIMED    2

#define SKIP_AUTOTIMER  1
#define SKIP_KEYPRESS   2
#define SKIP_MOUSECLICK 4

#define UNTIL_ANIMEND   1
#define UNTIL_MOVEEND   2
#define UNTIL_CHARIS0   3
#define UNTIL_NOOVERLAY 4
#define UNTIL_NEGATIVE  5
#define UNTIL_INTIS0    6
#define UNTIL_SHORTIS0  7
#define UNTIL_INTISNEG  8
#define MANOBJNUM 99

#define STD_BUFFER_SIZE 3000

#define TURNING_AROUND     1000
#define TURNING_BACKWARDS 10000

#define MAX_PLUGIN_OBJECT_READERS 50

#define NEXT_ITERATION() play.gamestep++

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#define HWND long
#endif

#define BASEWIDTH usetup.base_width
#define BASEHEIGHT usetup.base_height
#define TRANS_ALPHA_CHANNEL 20000
#define TRANS_OPAQUE        20001
#define TRANS_RUN_PLUGIN    20002


#define LOCTYPE_HOTSPOT 1
#define LOCTYPE_CHAR 2
#define LOCTYPE_OBJ  3

#define MAX_DYNAMIC_SURFACES 20

#define MAX_ANIMATING_BUTTONS 15
#define RESTART_POINT_SAVE_GAME_NUMBER 999

#define MAX_OPEN_SCRIPT_FILES 10

#include "ac/common_defines.h"

#endif // __AC_RUNTIMEDEFINES_H
