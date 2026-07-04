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
#ifndef __AC_RUNTIMEDEFINES_H
#define __AC_RUNTIMEDEFINES_H

#include <array>
#include "ac/common_defines.h"
#include "ac/game_version.h"

// Max old-style script string length
#define MAX_MAXSTRLEN 200
#define MAXGLOBALVARS 50

#define INVALID_X  30000
#define MAXGSVALUES 500
#define MAXGLOBALSTRINGS 51
#define LEGACY_MAX_CHAR_INVENTORY 500
// Max character inventory capacity is limited by SCR_NO_VALUE,
// because that was AddInventory's index parameter's default value.
// May up to INT32_MAX in AGS 4.x, but keep here for backwards compatibility.
#define MAX_CHAR_INVENTORY 31998
#define LEGACY_MAX_TIMERS 21
#define MAX_PARSED_WORDS 15

#define MAX_QUEUED_MUSIC 10
#define GLED_INTERACTION 1
#define GLED_EFFECTS     2 
#define QUEUED_MUSIC_REPEAT 10000

// Legacy (pre 3.5.0) alignment types used in the script API
enum LegacyScriptAlignment
{
    kLegacyScAlignLeft      = 1,
    kLegacyScAlignCentre    = 2,
    kLegacyScAlignRight     = 3
};

const int LegacyMusicMasterVolumeAdjustment = 60;
const int LegacyRoomVolumeFactor            = 30;

// Common command arguments
// HISTORICAL NOTE: These numbers were chosen arbitrarily -- the idea is
// to make sure that the user gets the parameters the right way round
// Walk (pathfinding) modes
#define ANYWHERE       304
#define WALKABLE_AREAS 305
// Blocking / non-blocking action
#define BLOCKING       919
#define IN_BACKGROUND  920
// Direction of animation
#define FORWARDS       1062
#define BACKWARDS      1063
// Stop / don't stop when changing a view
#define STOP_MOVING    1
#define KEEP_MOVING    0

#define SCR_NO_VALUE   31998
#define SCR_COLOR_TRANSPARENT -1


#define TXT_SCOREBAR        29
#define MAXSCORE play.totalscore

#define FONT_STATUSBAR  0
#define FONT_NORMAL     play.normal_font
//#define FONT_SPEECHBACK 1
#define FONT_SPEECH     play.speech_font

// Standard interaction verbs (aka cursor modes)
#define MODE_NONE      -1
#define MODE_WALK       0
#define MODE_LOOK       1
#define MODE_HAND       2
#define MODE_TALK       3
#define MODE_USE        4
#define MODE_PICKUP     5
// aka MODE_POINTER
#define CURS_ARROW      6
// aka MODE_WAIT
#define CURS_WAIT       7
#define MODE_CUSTOM1    8
#define MODE_CUSTOM2    9
#define NUM_STANDARD_VERBS 10

// Fixed Overlay IDs
#define OVER_TEXTMSG  1
#define OVER_COMPLETE 2
#define OVER_PICTURE  3
#define OVER_TEXTSPEECH 4
#define OVER_FIRSTFREE 5
#define OVER_CUSTOM   -1
// Overlay parameters
// FIXME: this constant is assigned to the Overlay's x coordinate,
// which is ugly and prone to mistakes, store as Overlay's flag instead
#define OVR_AUTOPLACE 30000

// These are possibly actions scheduled to run after "wait";
// but only FOR_EXITLOOP is used currently;
// other FOR_* types are deprecated since at least v2.5.
// Judging by the old comment in code, FOR_SCRIPT is for v2.1 and earlier.
#define FOR_ANIMATION 1
#define FOR_SCRIPT    2
#define FOR_EXITLOOP  3

// an actsps index offset for characters
#define ACTSP_OBJSOFF (MAX_ROOM_OBJECTS)
// a 1-based movelist index offset for characters
#define CHMLSOFFS (1 + MAX_ROOM_OBJECTS)
#define MAX_SCRIPT_AT_ONCE 10

#define EVENT_NONE       0
#define EVENT_INPROGRESS 1
#define EVENT_CLAIMED    2

// Values for GameState::stop_dialog_at_end;
// tell what to do with the current Dialog after returning from the option script
#define DIALOG_NONE      0
#define DIALOG_RUNNING   1
#define DIALOG_STOP      2
#define DIALOG_NEWROOM   100
#define DIALOG_NEWTOPIC  12000

// Internal skip style flags, for speech/display, wait;
// theoretically correspond to InputType in script (with a 24-bit shift)
#define SKIP_NONE       0x00
#define SKIP_AUTOTIMER  0x01
#define SKIP_KEYPRESS   0x02
#define SKIP_MOUSECLICK 0x04
// Bit shift for packing skip type into result
#define SKIP_RESULT_TYPE_SHIFT 24
// Bit mask for packing skip key/button data into result
#define SKIP_RESULT_DATA_MASK  0x00FFFFFF

// The index base for characters, used in legacy AnimateObject script function;
// if passed ID is eq or gt than this, then a Character is animated instead
#define LEGACY_ANIMATE_CHARIDBASE 100

#define STD_BUFFER_SIZE 2048

#define LOCTYPE_NOTHING 0
#define LOCTYPE_HOTSPOT 1
#define LOCTYPE_CHAR    2
#define LOCTYPE_OBJ     3
// LOCTYPE_INVITEM is used only internally, not exported to script in AGS 3.*
#define LOCTYPE_INVITEM 4

#define MAX_DYNAMIC_SURFACES 20

// legacy max number of saves which may be filled into the ListBox
#define LEGACY_MAXSAVEGAMES 50
// topmost save index to be listed with a FillSaveGameList command
#define LEGACY_TOP_LISTEDSAVESLOT 50
// topmost save index to be listed with a Save/RestoreGameDialog command
#define LEGACY_TOP_BUILTINDIALOGSAVESLOT 20
// topmost supported save slot index
#define TOP_SAVESLOT 999
// save slot reserved for the "restart point"
#define RESTART_POINT_SAVE_GAME_NUMBER 999

// Script API SortStyle
enum ScriptSortStyle
{
    kScNotSorted     = 0,
    kScSorted        = 1,
};

// Script API SortDirection
enum ScriptSortDirection
{
    kScSortNone       = 0,
    kScSortAscending  = 1,
    kScSortDescending = 2,
};

// Script API StringCompareStyle
enum ScriptStringComparison
{
    // NOTE: these constants are made as a combination of flags,
    // for easier comparison mode selection
    kScCaseSensitiveFlag = 0x1,
    kScLocaleAwareFlag   = 0x2,

    kScCaseInsensitive = 0,
    kScCaseSensitive   = 1,
    kScCaseInsensitiveLocaleAware = 2,
    kScCaseSensitiveLocaleAware = 3,
};

// Script API FileSortStyle
enum ScriptFileSortStyle
{
    kScFileSort_None = 0, // undefined order
    kScFileSort_Name = 1, // by file name
    kScFileSort_Time = 2, // by last write time
};

// Script API SaveGameSortStyle
enum ScriptSaveGameSortStyle
{
    kScSaveGameSort_None        = 0, // undefined order
    kScSaveGameSort_Number      = 1, // by slot number
    kScSaveGameSort_Time        = 2, // by last write time
    kScSaveGameSort_Description = 3, // by save description
};

// Script API GUIButtonColorStyle
enum ScriptButtonColorStyle
{
    eGUIButtonDefault           = 0,
    eGUIButtonDynamic           = 1,
    eGUIButtonDynamicFlat       = 2
};

// Determines a set of bit-flags for hit-test functions, such as
// GetAtScreenXY and GetAtRoomXY.
enum ScriptHitTestOptions
{
    kHit_Any                    = 0,
    kHit_Interactable           = 0x0001
};

// Determines which order of character turning is chosen when
// both clockwise and counter-clockwise directions are equally short
enum ScriptTurnOrderPriority
{
    kScTurnOrder_Clockwise          = 0, // historically is default
    kScTurnOrder_CounterClockwise   = 1,
    kScTurnOrder_Random             = 2, // cw or ccw, chosen random each time
    kScTurnOrder_FaceDown           = 3, // try to face down (towards player) more
};

// Determines under which condition the TextBox control claims
// the received key / text input events
enum ScriptTextBoxClaimStyle
{
    kScTextBoxClaim_Always          = 0, // historically is default
    kScTextBoxClaim_Never           = 1,
    kScTextBoxClaim_Handled         = 2
};

enum eScriptSystemOSID
{
    eOS_Unknown = 0,
    eOS_DOS,
    eOS_Win,
    eOS_Linux,
    eOS_Mac,
    eOS_Android,
    eOS_iOS,
    eOS_PSP,
    eOS_Web,
    eOS_FreeBSD,
    eNumOS
};

// Plugin system event IDs;
// must correspond to the declarations in plugin API.
// These event ids are defined as flags to let plugins combine them into
// a flag set when subscribing for events.
enum PluginEventID
{
    kPluginEvt_KeyPress         = 0x00000001,
    kPluginEvt_MouseClick       = 0x00000002,
    kPluginEvt_PostScreenDraw   = 0x00000004,
    kPluginEvt_PreScreenDraw    = 0x00000008,
    kPluginEvt_SaveGame         = 0x00000010,
    kPluginEvt_RestoreGame      = 0x00000020,
    kPluginEvt_PreGUIDraw       = 0x00000040,
    kPluginEvt_LeaveRoom        = 0x00000080,
    kPluginEvt_EnterRoom        = 0x00000100,
    kPluginEvt_TransitionIn     = 0x00000200,
    kPluginEvt_TransitionOut    = 0x00000400,
    kPluginEvt_FinalScreenDraw  = 0x00000800,
    kPluginEvt_TranslateText    = 0x00001000,
    kPluginEvt_ScriptDebug      = 0x00002000,
    // 0x00004000 - unused, was AUDIODECODE, no longer supported
    kPluginEvt_SpriteLoad       = 0x00008000,
    kPluginEvt_PreRender        = 0x00010000,
    kPluginEvt_PreSaveGame      = 0x00020000,
    kPluginEvt_PostRestoreGame  = 0x00040000,
    kPluginEvt_PostRoomDraw     = 0x00080000,
};

// Runtime behavior switches.
// The purpose is differentiating between modern and backwards-compatible behavior
// on a per-operation basis.
enum RuntimeBehaviorSwitch
{
    // Is here to have at least one valid constant
    kRBS_Dummy = 0,
    // Enables smooth transition between two consecutive walk orders
    kRBO_SmoothWalkTransition,
    // Apply text direction for GUI controls (except Labels, where it's always applied)
    kRBO_ApplyGUITextDirection,
    // Apply text direction for dialog options
    kRBO_ApplyDialogOptionTextDirection,
    kNum_RBS
};

extern std::array<const char*, kNum_RBS> RBSwitchNames;

// Data format version of the loaded game
// TODO: get rid of this global variable, use one from GameSetupStruct
extern GameDataVersion loaded_game_file_version;

#endif // __AC_RUNTIMEDEFINES_H
