/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#ifdef NO_MP3_PLAYER
#define SPECIAL_VERSION "NMP"
#else
#define SPECIAL_VERSION ""
#endif
// Version and build numbers
#define AC_VERSION_TEXT "3.21 "
#define ACI_VERSION_TEXT "3.21.1115"SPECIAL_VERSION
// this needs to be updated if the "play" struct changes
#define LOWEST_SGVER_COMPAT "3.20.1103"SPECIAL_VERSION
//#define THIS_IS_THE_ENGINE   now defined in the VC Project so that it's defined in all files

#define UNICODE
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#ifdef DJGPP
#include <dir.h>
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <process.h>
#endif

// MACPORT FIX: endian support
#include "bigend.h"
#ifdef ALLEGRO_BIG_ENDIAN
struct DialogTopic;
void preprocess_dialog_script(DialogTopic *);
#endif

#ifdef MAC_VERSION
char dataDirectory[512];
char appDirectory[512];
extern "C"
{
   int osx_sys_question(AL_CONST char *msg, AL_CONST char *but1, AL_CONST char *but2);
}
#endif

#include "misc.h"

// This is needed by a couple of headers, so it's at the top
extern "C" {
 extern long cliboffset(char*);
}
extern char lib_file_name[];
/*
extern "C" {
extern void * memcpy_amd(void *dest, const void *src, size_t n);
}
#define memcpyfast memcpy_amd*/
#define memcpyfast memcpy

#define USE_CLIB

#define IS_ANTIALIAS_SPRITES usetup.enable_antialiasing && (play.disable_antialiasing == 0)
extern int our_eip;
#include "wgt2allg.h"
#include "sprcache.h"

// Allegro 4 has switched 15-bit colour to BGR instead of RGB, so
// in this case we need to convert the graphics on load
#if ALLEGRO_DATE > 19991010
#define USE_15BIT_FIX
#endif

#ifdef WINDOWS_VERSION
#include <crtdbg.h>
#include "winalleg.h"
#include <shlwapi.h>

#elif defined(LINUX_VERSION) || defined(MAC_VERSION)

#define HWND long
#define _getcwd getcwd
#define strnicmp strncasecmp

long int filelength(int fhandle)
{
	struct stat statbuf;
	fstat(fhandle, &statbuf);
	return statbuf.st_size;
}

#else   // it's DOS (DJGPP)

#include "sys/exceptn.h"
#define _getcwd getcwd

int sys_getch() {
  return getch();
}

#endif  // WINDOWS_VERSION

#define getr32(xx) ((xx >> _rgb_r_shift_32) & 0xFF)
#define getg32(xx) ((xx >> _rgb_g_shift_32) & 0xFF)
#define getb32(xx) ((xx >> _rgb_b_shift_32) & 0xFF)
#define geta32(xx) ((xx >> _rgb_a_shift_32) & 0xFF)
#define makeacol32(r,g,b,a) ((r << _rgb_r_shift_32) | (g << _rgb_g_shift_32) | (b << _rgb_b_shift_32) | (a << _rgb_a_shift_32))
/*
#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
not needed now that allegro is being built with MSVC solution with no ASM
// The assembler stretch routine seems to GPF
extern "C" {
	void Cstretch_sprite(BITMAP *dst, BITMAP *src, int x, int y, int w, int h);
	void Cstretch_blit(BITMAP *src, BITMAP *dst, int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
}

#define stretch_sprite Cstretch_sprite
#define stretch_blit Cstretch_blit
#endif  // WINDOWS_VERSION || LINUX_VERSION || MAC_VERSION
*/

void draw_sprite_compensate(int,int,int,int);

char *get_translation(const char*);
int   source_text_length = -1;
#define NO_SAVE_FUNCTIONS
#define LOADROOM_DO_POLL
#include "acroom.h"
#include "cscomp.h"

#include <aastr.h>
#include <acdebug.h>

#define INI_READONLY
//#include <myini.H>

#include "agsplugin.h"
#include <apeg.h>

// We need COLOR_DEPTH_24 to allow it to load the preload PCX in hi-col
BEGIN_COLOR_DEPTH_LIST
  COLOR_DEPTH_8
  COLOR_DEPTH_15
  COLOR_DEPTH_16
  COLOR_DEPTH_24
  COLOR_DEPTH_32
END_COLOR_DEPTH_LIST

#if defined(WINDOWS_VERSION) && !defined(_DEBUG)
#define USE_CUSTOM_EXCEPTION_HANDLER
#endif
extern "C" HWND allegro_wnd;

// archive attributes to search for - al_findfirst breaks with 0
#define FA_SEARCH -1

// MACPORT FIX 9/6/5: undef M_PI first
#undef M_PI
#define M_PI 3.14159265358979323846


// Windows Vista Rich Save Games, modified to be platform-agnostic

#define RM_MAXLENGTH    1024
#define RM_MAGICNUMBER  "RGMH"

#pragma pack(push)
#pragma pack(1)
typedef struct _RICH_GAME_MEDIA_HEADER
{
    long       dwMagicNumber;
    long       dwHeaderVersion;
    long       dwHeaderSize;
    long       dwThumbnailOffsetLowerDword;
    long       dwThumbnailOffsetHigherDword;
    long       dwThumbnailSize;
    unsigned char guidGameId[16];
    unsigned short szGameName[RM_MAXLENGTH];
    unsigned short szSaveName[RM_MAXLENGTH];
    unsigned short szLevelName[RM_MAXLENGTH];
    unsigned short szComments[RM_MAXLENGTH];
} RICH_GAME_MEDIA_HEADER;
#pragma pack(pop)

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifdef WINDOWS_VERSION
int wArgc;
LPWSTR *wArgv;
#else
#define wArgc argc
#define wArgv argv
#define LPWSTR char*
#define LPCWSTR const char*
#define WCHAR char
#define StrCpyW strcpy
#endif

// ***** EXTERNS ****
extern "C" {
 extern int  csetlib(char*,char*);
 extern FILE*clibfopen(char*,char*);
 extern int  cfopenpriority;
 }
extern int  minstalled();
extern void mnewcursor(char);
extern void mgetgraphpos();
extern void mloadwcursor(char*);
extern int  misbuttondown(int);
extern int  disable_mgetgraphpos;
extern void msetcallback(IMouseGetPosCallback *gpCallback);
extern void msethotspot(int,int);
extern int  ismouseinbox(int,int,int,int);
extern void print_welcome_text(char*,char*);
extern char currentcursor;
extern int  mousex,mousey;
extern block mousecurs[10];
extern int   hotx, hoty;
extern char*get_language_text(int);
extern void init_language_text(char*);
extern int  loadgamedialog();
extern int  savegamedialog();
extern int  quitdialog();
extern int  cbuttfont;
extern int  acdialog_font;
extern int  enternumberwindow(char*);
extern void enterstringwindow(char*,char*);
extern int  roomSelectorWindow(int currentRoom, int numRooms, int*roomNumbers, char**roomNames);
extern void ccFlattenGlobalData (ccInstance *);
extern void ccUnFlattenGlobalData (ccInstance *);


// CD Player functions
// flags returned with cd_getstatus
#define CDS_DRIVEOPEN    0x0001  // tray is open
#define CDS_DRIVELOCKED  0x0002  // tray locked shut by software
#define CDS_AUDIOSUPPORT 0x0010  // supports audio CDs
#define CDS_DRIVEEMPTY   0x0800  // no CD in drive
// function definitions
extern int  cd_installed();
extern int  cd_getversion();
extern int  cd_getdriveletters(char*);
extern void cd_driverinit(int);
extern void cd_driverclose(int);
extern long cd_getstatus(int);
extern void cd_playtrack(int,int);
extern void cd_stopmusic(int);
extern void cd_resumemusic(int);
extern void cd_eject(int);
extern void cd_uneject(int);
extern int  cd_getlasttrack(int);
extern int  cd_isplayingaudio(int);
extern void QGRegisterFunctions();  // let the QFG module register its own

int eip_guinum, eip_guiobj;
int trans_mode=0;
int engineNeedsAsInt = 100;

int  sc_GetTime(int whatti);
void quitprintf(char*texx, ...);
void replace_macro_tokens(char*,char*);
void wouttext_reverseifnecessary(int x, int y, int font, char *text);
void SetGameSpeed(int newspd);
void SetMultitasking(int mode);
void put_sprite_256(int xxx,int yyy,block piccy);
void construct_virtual_screen(bool fullRedraw);
int initialize_engine_with_exception_handling(int argc,char*argv[]);
int initialize_engine(int argc,char*argv[]);
block recycle_bitmap(block bimp, int coldep, int wid, int hit);

#define WOUTTEXT_REVERSE wouttext_reverseifnecessary
#include "acgui.h"
#include "acruntim.h"
#include "acsound.h"

#define MAX_SCRIPT_MODULES 50


// **** TYPES ****


struct ScriptGUI {
  int id;
  GUIMain *gui;
};

struct ScriptHotspot {
  int id;
  int reserved;
};

struct ScriptRegion {
  int id;
  int reserved;
};




int ExecutingScript::queue_action(PostScriptAction act, int data, const char *aname) {
  if (numPostScriptActions >= MAX_QUEUED_ACTIONS)
    quitprintf("!%s: Cannot queue action, post-script queue full", aname);

  if (numPostScriptActions > 0) {
    // if something that will terminate the room has already
    // been queued, don't allow a second thing to be queued
    switch (postScriptActions[numPostScriptActions - 1]) {
    case ePSANewRoom:
    case ePSARestoreGame:
    case ePSARestoreGameDialog:
    case ePSARunAGSGame:
    case ePSARestartGame:
      quitprintf("!%s: Cannot run this command, since there is a %s command already queued to run", aname, postScriptActionNames[numPostScriptActions - 1]);
      break;
    // MACPORT FIX 9/6/5: added default clause to remove warning
    default:
      break;
    }
  }
  
  postScriptActions[numPostScriptActions] = act;
  postScriptActionData[numPostScriptActions] = data;
  postScriptActionNames[numPostScriptActions] = aname;
  numPostScriptActions++;
  return numPostScriptActions - 1;
}

void ExecutingScript::run_another (char *namm, int p1, int p2) {
  if (numanother < MAX_QUEUED_SCRIPTS)
    numanother++;
  else {
    /*debug_log("Warning: too many scripts to run, ignored %s(%d,%d)",
      script_run_another[numanother - 1], run_another_p1[numanother - 1],
      run_another_p2[numanother - 1]);*/
  }
  int thisslot = numanother - 1;
  strcpy(script_run_another[thisslot], namm);
  run_another_p1[thisslot] = p1;
  run_another_p2[thisslot] = p2;
}

void ExecutingScript::init() {
  inst = NULL;
  forked = 0;
  numanother = 0;
  numPostScriptActions = 0;
}

ExecutingScript::ExecutingScript() {
  init();
}

struct TempEip {
  int oldval;
  TempEip (int newval) {
    oldval = our_eip;
    our_eip = newval;
  }
  ~TempEip () { our_eip = oldval; }
};

struct DebugConsoleText {
  char text[100];
  char script[12];
};

struct CachedActSpsData {
  int xWas, yWas;
  int baselineWas;
  int isWalkBehindHere;
  int valid;
};

struct NonBlockingScriptFunction
{
  const char* functionName;
  int numParameters;
  void* param1;
  void* param2;
  bool roomHasFunction;
  bool globalScriptHasFunction;
  bool moduleHasFunction[MAX_SCRIPT_MODULES];
  bool atLeastOneImplementationExists;

  NonBlockingScriptFunction(const char*funcName, int numParams)
  {
    this->functionName = funcName;
    this->numParameters = numParams;
    atLeastOneImplementationExists = false;
    roomHasFunction = true;
    globalScriptHasFunction = true;

    for (int i = 0; i < MAX_SCRIPT_MODULES; i++)
    {
      moduleHasFunction[i] = true;
    }
  }
};


// **** GLOBALS ****
char *music_file;
char *speech_file;
WCHAR directoryPathBuffer[MAX_PATH];

/*extern int get_route_composition();
extern int routex1;*/
extern char*scripttempn;
#define REC_MOUSECLICK 1
#define REC_MOUSEMOVE  2
#define REC_MOUSEDOWN  3
#define REC_KBHIT      4
#define REC_GETCH      5
#define REC_KEYDOWN    6
#define REC_MOUSEWHEEL 7
#define REC_SPEECHFINISHED 8
#define REC_ENDOFFILE  0x6f
short *recordbuffer = NULL;
int  recbuffersize = 0, recsize = 0;
volatile int switching_away_from_game = 0;

int musicPollIterator; // long name so it doesn't interfere with anything else
#define UPDATE_MP3 \
   while (switching_away_from_game) { }\
   for (musicPollIterator = 0; musicPollIterator <= MAX_SOUND_CHANNELS; musicPollIterator++) { \
     if ((channels[musicPollIterator] != NULL) && (channels[musicPollIterator]->done == 0)) \
       channels[musicPollIterator]->poll();\
   }

//#define UPDATE_MP3 update_polled_stuff();



const char* sgnametemplate = "agssave.%03d";
char saveGameSuffix[MAX_SG_EXT_LENGTH + 1];

SOUNDCLIP *channels[MAX_SOUND_CHANNELS+1];
SOUNDCLIP *cachedQueuedMusic = NULL;
int numSoundChannels = 8;
#define SCHAN_SPEECH  0
#define SCHAN_AMBIENT 1
#define SCHAN_MUSIC   2
#define SCHAN_NORMAL  3
#define AUDIOTYPE_LEGACY_AMBIENT_SOUND 1
#define AUDIOTYPE_LEGACY_MUSIC 2
#define AUDIOTYPE_LEGACY_SOUND 3

#define MAX_ANIMATING_BUTTONS 15
#define RESTART_POINT_SAVE_GAME_NUMBER 999

enum WalkBehindMethodEnum
{
  DrawOverCharSprite,
  DrawAsSeparateSprite,
  DrawAsSeparateCharSprite
};

AGSPlatformDriver *platform = NULL;
// crossFading is >0 (channel number of new track), or -1 (old
// track fading out, no new track)
int crossFading = 0, crossFadeVolumePerStep = 0, crossFadeStep = 0;
int crossFadeVolumeAtStart = 0;
int last_sound_played[MAX_SOUND_CHANNELS + 1];
char *heightTestString = "ZHwypgfjqhkilIK";
block virtual_screen; 
int scrnwid,scrnhit;
int current_screen_resolution_multiplier = 1;
roomstruct thisroom;
GameSetupStruct game;
RoomStatus *roomstats;
RoomStatus troom;    // used for non-saveable rooms, eg. intro
GameState play;
GameSetup usetup;
CharacterExtras *charextra;
int force_letterbox = 0;
int game_paused=0;
int ifacepopped=-1;  // currently displayed pop-up GUI (-1 if none)
color palette[256];
//block spriteset[MAX_SPRITES+1];
//SpriteCache spriteset (MAX_SPRITES+1);
// initially size 1, this will be increased by the initFile function
SpriteCache spriteset(1);
long t1;  // timer for FPS
int cur_mode,cur_cursor;
int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
char saveGameDirectory[260] = "./";
//int abort_all_conditions=0;
int fps=0,display_fps=0;
DebugConsoleText debug_line[DEBUG_CONSOLE_NUMLINES];
int first_debug_line = 0, last_debug_line = 0, display_console = 0;
char *walkBehindExists = NULL;  // whether a WB area is in this column
int *walkBehindStartY = NULL, *walkBehindEndY = NULL;
char noWalkBehindsAtAll = 0;
int walkBehindLeft[MAX_OBJ], walkBehindTop[MAX_OBJ];
int walkBehindRight[MAX_OBJ], walkBehindBottom[MAX_OBJ];
IDriverDependantBitmap *walkBehindBitmap[MAX_OBJ];
int walkBehindsCachedForBgNum = 0;
WalkBehindMethodEnum walkBehindMethod = DrawOverCharSprite;
unsigned long loopcounter=0,lastcounter=0;
volatile unsigned long globalTimerCounter = 0;
char alpha_blend_cursor = 0;
RoomObject*objs;
RoomStatus*croom=NULL;
CharacterInfo*playerchar;
long _sc_PlayerCharPtr = 0;
int offsetx = 0, offsety = 0;
int use_extra_sound_offset = 0;
GUIMain*guis=NULL;
//GUIMain dummygui;
//GUIButton dummyguicontrol;
block *guibg = NULL;
IDriverDependantBitmap **guibgbmp = NULL;
ccScript* gamescript=NULL;
ccScript* dialogScriptsScript = NULL;
ccInstance *gameinst = NULL, *roominst = NULL;
ccInstance *dialogScriptsInst = NULL;
ccInstance *gameinstFork = NULL, *roominstFork = NULL;
IGraphicsDriver *gfxDriver;
IDriverDependantBitmap *mouseCursor = NULL;
IDriverDependantBitmap *blankImage = NULL;
IDriverDependantBitmap *blankSidebarImage = NULL;
IDriverDependantBitmap *debugConsole = NULL;
block debugConsoleBuffer = NULL;
block blank_mouse_cursor = NULL;
bool current_background_is_dirty = false;
int longestline = 0;

PluginObjectReader pluginReaders[MAX_PLUGIN_OBJECT_READERS];
int numPluginReaders = 0;

ccScript *scriptModules[MAX_SCRIPT_MODULES];
ccInstance *moduleInst[MAX_SCRIPT_MODULES];
ccInstance *moduleInstFork[MAX_SCRIPT_MODULES];
char *moduleRepExecAddr[MAX_SCRIPT_MODULES];
int numScriptModules = 0;

ViewStruct*views=NULL;
ScriptMouse scmouse;
COLOR_MAP maincoltable;
ScriptSystem scsystem;
block _old_screen=NULL;
block _sub_screen=NULL;
MoveList *mls = NULL;
DialogTopic *dialog;
block walkareabackup=NULL, walkable_areas_temp = NULL;
ExecutingScript scripts[MAX_SCRIPT_AT_ONCE];
ExecutingScript*curscript = NULL;
AnimatingGUIButton animbuts[MAX_ANIMATING_BUTTONS];
int numAnimButs = 0;
int num_scripts=0, eventClaimed = EVENT_NONE;
int getloctype_index = 0, getloctype_throughgui = 0;
int user_disabled_for=0,user_disabled_data=0,user_disabled_data2=0;
int user_disabled_data3=0;
int is_complete_overlay=0,is_text_overlay=0;
// Sierra-style speech settings
int face_talking=-1,facetalkview=0,facetalkwait=0,facetalkframe=0;
int facetalkloop=0, facetalkrepeat = 0, facetalkAllowBlink = 1;
int facetalkBlinkLoop = 0;
CharacterInfo *facetalkchar = NULL;
// lip-sync speech settings
int loops_per_character, text_lips_offset, char_speaking = -1;
char *text_lips_text = NULL;
SpeechLipSyncLine *splipsync = NULL;
int numLipLines = 0, curLipLine = -1, curLipLinePhenome = 0;
int gameHasBeenRestored = 0;
char **characterScriptObjNames = NULL;
char objectScriptObjNames[MAX_INIT_SPR][MAX_SCRIPT_NAME_LEN + 5];
char **guiScriptObjNames = NULL;

// set to 0 once successful
int working_gfx_mode_status = -1;

int said_speech_line; // used while in dialog to track whether screen needs updating

int restrict_until=0;
int gs_to_newroom=-1;
ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
int proper_exit=0,our_eip=0;
int numscreenover=0;
int scaddr;
int walk_behind_baselines_changed = 0;
int displayed_room=-10,starting_room = -1;
int mouse_on_iface=-1;   // mouse cursor is over this interface
int mouse_on_iface_button=-1;
int mouse_pushed_iface=-1;  // this BUTTON on interface MOUSE_ON_IFACE is pushed
int mouse_ifacebut_xoffs=-1,mouse_ifacebut_yoffs=-1;
int debug_flags=0;
IDriverDependantBitmap* roomBackgroundBmp = NULL;

int use_compiled_folder_as_current_dir = 0;
int editor_debugging_enabled = 0;
int editor_debugging_initialized = 0;
char editor_debugger_instance_token[100];
IAGSEditorDebugger *editor_debugger = NULL;
int break_on_next_script_step = 0;
volatile int game_paused_in_debugger = 0;
HWND editor_window_handle = NULL;

int in_enters_screen=0,done_es_error = 0;
int in_leaves_screen = -1;
int need_to_stop_cd=0;
int debug_15bit_mode = 0, debug_24bit_mode = 0;
int said_text = 0;
int convert_16bit_bgr = 0;
int mouse_z_was = 0;
int bg_just_changed = 0;
int loaded_game_file_version = 0;
volatile char want_exit = 0, abort_engine = 0;
char check_dynamic_sprites_at_exit = 1;
#define DBG_NOIFACE       1
#define DBG_NODRAWSPRITES 2
#define DBG_NOOBJECTS     4
#define DBG_NOUPDATE      8
#define DBG_NOSFX      0x10
#define DBG_NOMUSIC    0x20
#define DBG_NOSCRIPT   0x40
#define DBG_DBGSCRIPT  0x80
#define DBG_DEBUGMODE 0x100
#define DBG_REGONLY   0x200
#define DBG_NOVIDEO   0x400
#define MAXEVENTS 15
EventHappened event[MAXEVENTS+1];
int numevents=0;
#define EV_TEXTSCRIPT 1
#define EV_RUNEVBLOCK 2
#define EV_FADEIN     3
#define EV_IFACECLICK 4
#define EV_NEWROOM    5
#define TS_REPEAT   1
#define TS_KEYPRESS 2
#define TS_MCLICK   3
#define EVB_HOTSPOT 1
#define EVB_ROOM    2
char ac_engine_copyright[]="Adventure Game Studio engine & tools (c) 1999-2000 by Chris Jones.";
int current_music_type = 0;

#define LOCTYPE_HOTSPOT 1
#define LOCTYPE_CHAR 2
#define LOCTYPE_OBJ  3

#define MAX_DYNAMIC_SURFACES 20

#define REP_EXEC_ALWAYS_NAME "repeatedly_execute_always"
#define REP_EXEC_NAME "repeatedly_execute"

char*tsnames[4]={NULL, REP_EXEC_NAME, "on_key_press","on_mouse_click"};
char*evblockbasename;
int evblocknum;
//int current_music=0;
int frames_per_second=40;
int in_new_room=0, new_room_was = 0;  // 1 in new room, 2 first time in new room, 3 loading saved game
int new_room_pos=0;
int new_room_x = SCR_NO_VALUE, new_room_y = SCR_NO_VALUE;
unsigned int load_new_game = 0;
int load_new_game_restore = -1;
int inside_script=0,in_graph_script=0;
int no_blocking_functions = 0; // set to 1 while in rep_Exec_always
int in_inv_screen = 0, inv_screen_newroom = -1;
int mouse_frame=0,mouse_delay=0;
int lastmx=-1,lastmy=-1;
int new_room_flags=0;
#define MAX_SPRITES_ON_SCREEN 76
SpriteListEntry sprlist[MAX_SPRITES_ON_SCREEN];
int sprlistsize=0;
#define MAX_THINGS_TO_DRAW 125
SpriteListEntry thingsToDrawList[MAX_THINGS_TO_DRAW];
int thingsToDrawSize = 0;
int use_cdplayer=0;
bool triedToUseCdAudioCommand = false;
int final_scrn_wid=0,final_scrn_hit=0,final_col_dep=0;
int post_script_cleanup_stack = 0;
// actsps is used for temporary storage of the bitamp image
// of the latest version of the sprite
int actSpsCount = 0;
block *actsps;
IDriverDependantBitmap* *actspsbmp;
// temporary cache of walk-behind for this actsps image
block *actspswb;
IDriverDependantBitmap* *actspswbbmp;
CachedActSpsData* actspswbcache;

CharacterCache *charcache = NULL;
ObjectCache objcache[MAX_INIT_SPR];

ScriptObject scrObj[MAX_INIT_SPR];
ScriptGUI *scrGui = NULL;
ScriptHotspot scrHotspot[MAX_HOTSPOTS];
ScriptRegion scrRegion[MAX_REGIONS];
ScriptInvItem scrInv[MAX_INV];
ScriptDialog scrDialog[MAX_DIALOG];

RGB_MAP rgb_table;  // for 256-col antialiasing
char* game_file_name=NULL;
int want_quit = 0, screen_reset = 0;
block raw_saved_screen = NULL;
block dotted_mouse_cursor = NULL;
block dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
int scrlockWasDown = 0;
// whether there are currently remnants of a DisplaySpeech
int screen_is_dirty = 0;
char replayfile[MAX_PATH] = "record.dat";
int replay_time = 0;
unsigned long replay_last_second = 0;
int replay_start_this_time = 0;
char pexbuf[STD_BUFFER_SIZE];

int pluginsWantingDebugHooks = 0;

const char *replayTempFile = "~replay.tmp";

NonBlockingScriptFunction repExecAlways(REP_EXEC_ALWAYS_NAME, 0);
NonBlockingScriptFunction getDialogOptionsDimensionsFunc("dialog_options_get_dimensions", 1);
NonBlockingScriptFunction renderDialogOptionsFunc("dialog_options_render", 1);
NonBlockingScriptFunction getDialogOptionUnderCursorFunc("dialog_options_get_active", 1);
NonBlockingScriptFunction runDialogOptionMouseClickHandlerFunc("dialog_options_mouse_click", 2);

//  *** FUNCTIONS ****

bool AmbientSound::IsPlaying () {
  if (channel <= 0)
    return false;
  return (channels[channel] != NULL) ? true : false;
}

int  Overlay_GetValid(ScriptOverlay *scover);
void mainloop(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void set_mouse_cursor(int);
void set_default_cursor();
int  run_text_script(ccInstance*,char*);
int  run_text_script_2iparam(ccInstance*,char*,int,int);
int  run_text_script_iparam(ccInstance*,char*,int);
//void run_graph_script(int);
//void run_event_block(EventBlock*,int,int=-1, int=-1);
int  run_interaction_event (NewInteraction *nint, int evnt, int chkAny = -1, int isInv = 0);
int  run_interaction_script(InteractionScripts *nint, int evnt, int chkAny = -1, int isInv = 0);
void new_room(int,CharacterInfo*);
void NewRoom(int);
void AnimateObject(int,int,int,int);
void SetObjectView(int,int);
void GiveScore(int);
void walk_character(int,int,int,int,bool);
void move_object(int,int,int,int,int);
void StopMoving(int);
void MoveCharacterToHotspot(int,int);
int  GetCursorMode();
void GetLocationName(int,int,char*);
void save_game(int,const char*);
int  load_game(int,char*, int*);
void update_music_volume();
int  invscreen();
void process_interface_click(int,int,int);
void DisplayMessage (int);
void do_conversation(int);
void compile_room_script();
int  CreateTextOverlay(int,int,int,int,int,char*,...);
void RemoveOverlay(int);
void stopmusic();
void play_flc_file(int,int);
void SetCharacterView(int,int);
void ReleaseCharacterView(int);
void setevent(int evtyp,int ev1=0,int ev2=-1000,int ev3=0);
void update_events();
void process_event(EventHappened*);
int  GetLocationType(int,int);
int  __GetLocationType(int,int,int);
int  AreCharObjColliding(int charid,int objid);
int  play_speech(int,int);
void stop_speech();
int  play_sound (int);
int  play_sound_priority (int, int);
int  __Rand(int);
int  cd_manager(int,int);
int  DisplaySpeechBackground(int,char*);
void MergeObject(int);
void script_debug(int,int);
void sc_inputbox(const char*,char*);
void ParseText(char*);
void FaceLocation(int,int,int);
void check_debug_keys();
int  IsInterfaceEnabled();
void break_up_text_into_lines(int,int,char*);
void start_game();
void init_game_settings();
void show_preload();
void stop_recording ();
void save_game_data (FILE *, block screenshot);
void setup_script_exports ();
void SetSpeechFont (int);
void SetNormalFont (int);
void tint_image (block source, block dest, int red, int grn, int blu, int light_level, int luminance=255);
void get_message_text (int msnum, char *buffer, char giveErr = 1);
void render_graphics(IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
int  wait_loop_still_valid();
SOUNDCLIP *load_music_from_disk(int mnum, bool repeat);
void play_new_music(int mnum, SOUNDCLIP *music);
int GetGameSpeed();
int check_for_messages_from_editor();
int show_dialog_options(int dlgnum, int sayChosenOption, bool runGameLoopsInBackground);
void add_to_sprite_list(IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind = false);

// MACPORT FIX 9/6/5: undef (was macro) and add prototype
#undef wouttext_outline
void wouttext_outline(int xxp, int yyp, int usingfont, char *texx);
  
#define Random __Rand


#define ALLEGRO_KEYBOARD_HANDLER
// KEYBOARD HANDLER
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
int myerrno;
#else
int errno;
#define myerrno errno
#endif

#ifdef MAC_VERSION
#define EXTENDED_KEY_CODE 0x3f
#else
#define EXTENDED_KEY_CODE 0
#endif

#define AGS_KEYCODE_INSERT 382
#define AGS_KEYCODE_DELETE 383
#define AGS_KEYCODE_ALT_TAB 399
#define READKEY_CODE_ALT_TAB 0x4000

int my_readkey() {
  int gott=readkey();
  int scancode = ((gott >> 8) & 0x00ff);

  if (gott == READKEY_CODE_ALT_TAB)
  {
    // Alt+Tab, it gets stuck down unless we do this
    return AGS_KEYCODE_ALT_TAB;
  }

/*  char message[200];
  sprintf(message, "Scancode: %04X", gott);
  OutputDebugString(message);*/

  /*if ((scancode >= KEY_0_PAD) && (scancode <= KEY_9_PAD)) {
    // fix numeric pad keys if numlock is off (allegro 4.2 changed this behaviour)
    if ((key_shifts & KB_NUMLOCK_FLAG) == 0)
      gott = (gott & 0xff00) | EXTENDED_KEY_CODE;
  }*/

  if ((gott & 0x00ff) == EXTENDED_KEY_CODE) {
    gott = scancode + 300;

    // convert Allegro KEY_* numbers to scan codes
    // (for backwards compatibility we can't just use the
    // KEY_* constants now, it's too late)
    if ((gott>=347) & (gott<=356)) gott+=12;
    // F11-F12
    else if ((gott==357) || (gott==358)) gott+=76;
    // insert / numpad insert
    else if ((scancode == KEY_0_PAD) || (scancode == KEY_INSERT))
      gott = AGS_KEYCODE_INSERT;
    // delete / numpad delete
    else if ((scancode == KEY_DEL_PAD) || (scancode == KEY_DEL))
      gott = AGS_KEYCODE_DELETE;
    // Home
    else if (gott == 378) gott = 371;
    // End
    else if (gott == 379) gott = 379;
    // PgUp
    else if (gott == 380) gott = 373;
    // PgDn
    else if (gott == 381) gott = 381;
    // left arrow
    else if (gott==382) gott=375;
    // right arrow
    else if (gott==383) gott=377;
    // up arrow
    else if (gott==384) gott=372;
    // down arrow
    else if (gott==385) gott=380;
    // numeric keypad
    else if (gott==338) gott=379;
    else if (gott==339) gott=380;
    else if (gott==340) gott=381;
    else if (gott==341) gott=375;
    else if (gott==342) gott=376;
    else if (gott==343) gott=377;
    else if (gott==344) gott=371;
    else if (gott==345) gott=372;
    else if (gott==346) gott=373;
  }
  else
    gott = gott & 0x00ff;

  // Alt+X, abort (but only once game is loaded)
  if ((gott == play.abort_key) && (displayed_room >= 0)) {
    check_dynamic_sprites_at_exit = 0;
    quit("!|");
  }

  //sprintf(message, "Keypress: %d", gott);
  //OutputDebugString(message);

  return gott;
}
//#define getch() my_readkey()
//#undef kbhit
//#define kbhit keypressed
// END KEYBOARD HANDLER


// for external modules to call
void next_iteration() {
  NEXT_ITERATION();
}
void write_record_event (int evnt, int dlen, short *dbuf) {

  recordbuffer[recsize] = play.gamestep;
  recordbuffer[recsize+1] = evnt;

  for (int i = 0; i < dlen; i++)
    recordbuffer[recsize + i + 2] = dbuf[i];
  recsize += dlen + 2;

  if (recsize >= recbuffersize - 100) {
    recbuffersize += 10000;
    recordbuffer = (short*)realloc (recordbuffer, recbuffersize * sizeof(short));
  }

  play.gamestep++;
}
void disable_replay_playback () {
  play.playback = 0;
  if (recordbuffer)
    free (recordbuffer);
  recordbuffer = NULL;
  disable_mgetgraphpos = 0;
}

void done_playback_event (int size) {
  recsize += size;
  play.gamestep++;
  if ((recsize >= recbuffersize) || (recordbuffer[recsize+1] == REC_ENDOFFILE))
    disable_replay_playback();
}

int rec_getch () {
  if (play.playback) {
    if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_GETCH)) {
      int toret = recordbuffer[recsize + 2];
      done_playback_event (3);
      return toret;
    }
    // Since getch() waits for a key to be pressed, if we have no
    // record for it we're out of sync
    quit("out of sync in playback in getch");
  }
  int result = my_readkey();
  if (play.recording) {
    short buff[1] = {result};
    write_record_event (REC_GETCH, 1, buff);
  }

  return result;  
}

int rec_kbhit () {
  if ((play.playback) && (recordbuffer != NULL)) {
    // check for real keypresses to abort the replay
    if (keypressed()) {
      if (my_readkey() == 27) {
        disable_replay_playback();
        return 0;
      }
    }
    // now simulate the keypresses
    if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_KBHIT)) {
      done_playback_event (2);
      return 1;
    }
    return 0;
  }
  int result = keypressed();
  if ((result) && (globalTimerCounter < play.ignore_user_input_until_time))
  {
    // ignoring user input
    my_readkey();
    result = 0;
  }
  if ((result) && (play.recording)) {
    write_record_event (REC_KBHIT, 0, NULL);
  }
  return result;  
}

char playback_keystate[KEY_MAX];

int rec_iskeypressed (int keycode) {

  if (play.playback) {
    if ((recordbuffer[recsize] == play.gamestep)
     && (recordbuffer[recsize + 1] == REC_KEYDOWN)
     && (recordbuffer[recsize + 2] == keycode)) {
      playback_keystate[keycode] = recordbuffer[recsize + 3];
      done_playback_event (4);
    }
    return playback_keystate[keycode];
  }

  int toret = key[keycode];

  if (play.recording) {
    if (toret != playback_keystate[keycode]) {
      short buff[2] = {keycode, toret};
      write_record_event (REC_KEYDOWN, 2, buff);
      playback_keystate[keycode] = toret;
    }
  }

  return toret;
}

int rec_isSpeechFinished () {
  if (play.playback) {
    if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_SPEECHFINISHED)) {
      done_playback_event (2);
      return 1;
    }
    return 0;
  }

  if (!channels[SCHAN_SPEECH]->done) {
    return 0;
  }
  if (play.recording)
    write_record_event (REC_SPEECHFINISHED, 0, NULL);
  return 1;
}

int recbutstate[4] = {-1, -1, -1, -1};
int rec_misbuttondown (int but) {
  if (play.playback) {
    if ((recordbuffer[recsize] == play.gamestep)
     && (recordbuffer[recsize + 1] == REC_MOUSEDOWN)
     && (recordbuffer[recsize + 2] == but)) {
      recbutstate[but] = recordbuffer[recsize + 3];
      done_playback_event (4);
    }
    return recbutstate[but];
  }
  int result = misbuttondown (but);
  if (play.recording) {
    if (result != recbutstate[but]) {
      short buff[2] = {but, result};
      write_record_event (REC_MOUSEDOWN, 2, buff);
      recbutstate[but] = result;
    }
  }
  return result;
}

int pluginSimulatedClick = NONE;
void PluginSimulateMouseClick(int pluginButtonID) {
  pluginSimulatedClick = pluginButtonID - 1;
}

int rec_mgetbutton() {

  if ((play.playback) && (recordbuffer != NULL)) {
    if ((recordbuffer[recsize] < play.gamestep) && (play.gamestep < 32766))
      quit("Playback error: out of sync");
    if (loopcounter >= replay_last_second + 40) {
      replay_time ++;
      replay_last_second += 40;
    }
    if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_MOUSECLICK)) {
      filter->SetMousePosition(recordbuffer[recsize+3], recordbuffer[recsize+4]);
      disable_mgetgraphpos = 0;
      mgetgraphpos ();
      disable_mgetgraphpos = 1;
      int toret = recordbuffer[recsize + 2];
      done_playback_event (5);
      return toret;
    }
    return NONE;
  }

  int result;

  if (pluginSimulatedClick > NONE) {
    result = pluginSimulatedClick;
    pluginSimulatedClick = NONE;
  }
  else {
    result = mgetbutton();
  }

  if ((result >= 0) && (globalTimerCounter < play.ignore_user_input_until_time))
  {
    // ignoring user input
    result = NONE;
  }

  if (play.recording) {
    if (result >= 0) {
      short buff[3] = {result, mousex, mousey};
      write_record_event (REC_MOUSECLICK, 3, buff);
    }
    if (loopcounter >= replay_last_second + 40) {
      replay_time ++;
      replay_last_second += 40;
    }
  }
  return result;
}

void rec_domouse (int what) {
  
  if (play.recording) {
    int mxwas = mousex, mywas = mousey;
    if (what == DOMOUSE_NOCURSOR)
      mgetgraphpos();
    else
      domouse(what);

    if ((mxwas != mousex) || (mywas != mousey)) {
      // don't divide down the co-ordinates, because we lose
      // the precision, and it might click the wrong thing
      // if eg. hi-res 71 -> 35 in record file -> 70 in playback
      short buff[2] = {mousex, mousey};
      write_record_event (REC_MOUSEMOVE, 2, buff);
    }
    return;
  }
  else if ((play.playback) && (recordbuffer != NULL)) {
    if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_MOUSEMOVE)) {
      filter->SetMousePosition(recordbuffer[recsize+2], recordbuffer[recsize+3]);
      disable_mgetgraphpos = 0;
      if (what == DOMOUSE_NOCURSOR)
        mgetgraphpos();
      else
        domouse(what);
      disable_mgetgraphpos = 1;
      done_playback_event (4);
      return;
    }
  }
  if (what == DOMOUSE_NOCURSOR)
    mgetgraphpos();
  else
    domouse(what);
}
int check_mouse_wheel () {
  if ((play.playback) && (recordbuffer != NULL)) {
    if ((recordbuffer[recsize] == play.gamestep) && (recordbuffer[recsize + 1] == REC_MOUSEWHEEL)) {
      int toret = recordbuffer[recsize+2];
      done_playback_event (3);
      return toret;
    }
    return 0;
  }

  int result = 0;
  if ((mouse_z != mouse_z_was) && (game.options[OPT_MOUSEWHEEL] != 0)) {
    if (mouse_z > mouse_z_was)
      result = 1;
    else
      result = -1;
    mouse_z_was = mouse_z;
  }

  if ((play.recording) && (result)) {
    short buff[1] = {result};
    write_record_event (REC_MOUSEWHEEL, 1, buff);
  }

  return result;
}
#undef kbhit
#define mgetbutton rec_mgetbutton
#define domouse rec_domouse
#define misbuttondown rec_misbuttondown
#define kbhit rec_kbhit
#define getch rec_getch

void quitprintf(char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  vsprintf(displbuf,texx,ap);
  va_end(ap);
  quit(displbuf);
}

void write_log(char*msg) {
  FILE*ooo=fopen("ac.log","at");
  fprintf(ooo,"%s\n",msg);
  fclose(ooo);
}

// this function is only enabled for special builds if a startup
// issue needs to be checked
#define write_log_debug(msg) platform->WriteDebugString(msg)
/*extern "C" {
void write_log_debug(const char*msg) {
  //if (play.debug_mode == 0)
    //return;

  char toWrite[300];
  sprintf(toWrite, "%02d/%02d/%04d %02d:%02d:%02d (EIP=%4d) %s", sc_GetTime(4), sc_GetTime(5),
     sc_GetTime(6), sc_GetTime(1), sc_GetTime(2), sc_GetTime(3), our_eip, msg);

  FILE*ooo=fopen("ac.log","at");
  fprintf(ooo,"%s\n", toWrite);
  fclose(ooo);
}
}*/

/* The idea of this is that non-essential errors such as "sound file not
   found" are logged instead of exiting the program.
*/
void debug_log(char*texx, ...) {
  // if not in debug mode, don't print it so we don't worry the
  // end player
  if (play.debug_mode == 0)
    return;
  static int first_time = 1;
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  vsprintf(displbuf,texx,ap);
  va_end(ap);

  /*if (true) {
    char buffer2[STD_BUFFER_SIZE];
    strcpy(buffer2, "%");
    strcat(buffer2, displbuf);
    quit(buffer2);
  }*/

  char*openmode = "at";
  if (first_time) {
    openmode = "wt";
    first_time = 0;
  }
  FILE*outfil = fopen("warnings.log",openmode);
  if (outfil == NULL)
  {
    debug_write_console("* UNABLE TO WRITE TO WARNINGS.LOG");
    debug_write_console(displbuf);
  }
  else
  {
    fprintf(outfil,"(in room %d): %s\n",displayed_room,displbuf);
    fclose(outfil);
  }
}


void debug_write_console (char *msg, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,msg);
  vsprintf(displbuf,msg,ap);
  va_end(ap);
  displbuf[99] = 0;

  strcpy (debug_line[last_debug_line].text, displbuf);
  ccInstance*curinst = ccGetCurrentInstance();
  if (curinst != NULL) {
    char scriptname[20];
    if (curinst->instanceof == gamescript)
      strcpy(scriptname,"G ");
    else if (curinst->instanceof == thisroom.compiled_script)
      strcpy (scriptname, "R ");
    else if (curinst->instanceof == dialogScriptsScript)
      strcpy(scriptname,"D ");
    else
      strcpy(scriptname,"? ");
    sprintf(debug_line[last_debug_line].script,"%s%d",scriptname,currentline);
  }
  else debug_line[last_debug_line].script[0] = 0;

  platform->WriteDebugString("%s (%s)", displbuf, debug_line[last_debug_line].script);

  last_debug_line = (last_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

  if (last_debug_line == first_debug_line)
    first_debug_line = (first_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

}
#define DEBUG_CONSOLE if (play.debug_mode) debug_write_console


const char *get_cur_script(int numberOfLinesOfCallStack) {
  ccGetCallStack(ccGetCurrentInstance(), pexbuf, numberOfLinesOfCallStack);

  if (pexbuf[0] == 0)
    strcpy(pexbuf, ccErrorCallStack);

  return &pexbuf[0];
}

static const char* BREAK_MESSAGE = "BREAK";

struct Breakpoint
{
  char scriptName[80];
  int lineNumber;
};

DynamicArray<Breakpoint> breakpoints;
int numBreakpoints = 0;

bool send_message_to_editor(const char *msg, const char *errorMsg) 
{
  const char *callStack = get_cur_script(25);
  if (callStack[0] == 0)
    return false;

  char messageToSend[STD_BUFFER_SIZE];
  sprintf(messageToSend, "<?xml version=\"1.0\" encoding=\"Windows-1252\"?><Debugger Command=\"%s\">", msg);
#ifdef WINDOWS_VERSION
  sprintf(&messageToSend[strlen(messageToSend)], "  <EngineWindow>%d</EngineWindow> ", win_get_window());
#endif
  sprintf(&messageToSend[strlen(messageToSend)], "  <ScriptState><![CDATA[%s]]></ScriptState> ", callStack);
  if (errorMsg != NULL)
  {
    sprintf(&messageToSend[strlen(messageToSend)], "  <ErrorMessage><![CDATA[%s]]></ErrorMessage> ", errorMsg);
  }
  strcat(messageToSend, "</Debugger>");

  editor_debugger->SendMessageToEditor(messageToSend);

  return true;
}

bool send_message_to_editor(const char *msg) 
{
  return send_message_to_editor(msg, NULL);
}

bool init_editor_debugging() 
{
#ifdef WINDOWS_VERSION
  editor_debugger = GetEditorDebugger(editor_debugger_instance_token);
#else
  // Editor isn't ported yet
  editor_debugger = NULL;
#endif

  if (editor_debugger == NULL)
    quit("editor_debugger is NULL but debugger enabled");

  if (editor_debugger->Initialize())
  {
    editor_debugging_initialized = 1;

    // Wait for the editor to send the initial breakpoints
    // and then its READY message
    while (check_for_messages_from_editor() != 2) 
    {
      platform->Delay(10);
    }

    send_message_to_editor("START");
    return true;
  }

  return false;
}

int check_for_messages_from_editor()
{
  if (editor_debugger->IsMessageAvailable())
  {
    char *msg = editor_debugger->GetNextMessage();
    if (msg == NULL)
    {
      return 0;
    }

    if (strncmp(msg, "<Engine Command=\"", 17) != 0) 
    {
      //OutputDebugString("Faulty message received from editor:");
      //OutputDebugString(msg);
      free(msg);
      return 0;
    }

    const char *msgPtr = &msg[17];

    if (strncmp(msgPtr, "START", 5) == 0)
    {
      const char *windowHandle = strstr(msgPtr, "EditorWindow") + 14;
      editor_window_handle = (HWND)atoi(windowHandle);
    }
    else if (strncmp(msgPtr, "READY", 5) == 0)
    {
      free(msg);
      return 2;
    }
    else if ((strncmp(msgPtr, "SETBREAK", 8) == 0) ||
             (strncmp(msgPtr, "DELBREAK", 8) == 0))
    {
      bool isDelete = (msgPtr[0] == 'D');
      // Format:  SETBREAK $scriptname$lineNumber$
      msgPtr += 10;
      char scriptNameBuf[100];
      int i = 0;
      while (msgPtr[0] != '$')
      {
        scriptNameBuf[i] = msgPtr[0];
        msgPtr++;
        i++;
      }
      scriptNameBuf[i] = 0;
      msgPtr++;

      int lineNumber = atoi(msgPtr);

      if (isDelete) 
      {
        for (i = 0; i < numBreakpoints; i++)
        {
          if ((breakpoints[i].lineNumber == lineNumber) &&
              (strcmp(breakpoints[i].scriptName, scriptNameBuf) == 0))
          {
            numBreakpoints--;
            for (int j = i; j < numBreakpoints; j++)
            {
              breakpoints[j] = breakpoints[j + 1];
            }
            break;
          }
        }
      }
      else 
      {
        strcpy(breakpoints[numBreakpoints].scriptName, scriptNameBuf);
        breakpoints[numBreakpoints].lineNumber = lineNumber;
        numBreakpoints++;
      }
    }
    else if (strncmp(msgPtr, "RESUME", 6) == 0) 
    {
      game_paused_in_debugger = 0;
    }
    else if (strncmp(msgPtr, "STEP", 4) == 0) 
    {
      game_paused_in_debugger = 0;
      break_on_next_script_step = 1;
    }
    else if (strncmp(msgPtr, "EXIT", 4) == 0) 
    {
      want_exit = 1;
      abort_engine = 1;
      check_dynamic_sprites_at_exit = 0;
    }

    free(msg);
    return 1;
  }

  return 0;
}


void NewInteractionCommand::remove () {
  if (children != NULL) {
    children->reset();
    delete children;
  }
  children = NULL;
  parent = NULL;
  type = 0;
}

void force_audiostream_include() {
  // This should never happen, but the call is here to make it
  // link the audiostream libraries
  stop_audio_stream(NULL);
}


AmbientSound ambient[MAX_SOUND_CHANNELS + 1];  // + 1 just for safety on array iterations

int get_volume_adjusted_for_distance(int volume, int sndX, int sndY, int sndMaxDist)
{
  int distx = playerchar->x - sndX;
  int disty = playerchar->y - sndY;
  // it uses Allegro's "fix" sqrt without the ::
  int dist = (int)::sqrt((double)(distx*distx + disty*disty));

  // if they're quite close, full volume
  int wantvol = volume;

  if (dist >= AMBIENCE_FULL_DIST)
  {
    // get the relative volume
    wantvol = ((dist - AMBIENCE_FULL_DIST) * volume) / sndMaxDist;
    // closer is louder
    wantvol = volume - wantvol;
  }

  return wantvol;
}

void update_directional_sound_vol()
{
  for (int chan = 1; chan < MAX_SOUND_CHANNELS; chan++) 
  {
    if ((channels[chan] != NULL) && (channels[chan]->done == 0) &&
        (channels[chan]->xSource >= 0)) 
    {
      channels[chan]->directionalVolModifier = 
        get_volume_adjusted_for_distance(channels[chan]->vol, 
                channels[chan]->xSource,
                channels[chan]->ySource,
                channels[chan]->maximumPossibleDistanceAway) -
        channels[chan]->vol;

      channels[chan]->set_volume(channels[chan]->vol);
    }
  }
}

void update_ambient_sound_vol () {

  for (int chan = 1; chan < MAX_SOUND_CHANNELS; chan++) {

    AmbientSound *thisSound = &ambient[chan];

    if (thisSound->channel == 0)
      continue;

    int sourceVolume = thisSound->vol;

    if ((channels[SCHAN_SPEECH] != NULL) && (channels[SCHAN_SPEECH]->done == 0)) {
      // Negative value means set exactly; positive means drop that amount
      if (play.speech_music_drop < 0)
        sourceVolume = -play.speech_music_drop;
      else
        sourceVolume -= play.speech_music_drop;

      if (sourceVolume < 0)
        sourceVolume = 0;
      if (sourceVolume > 255)
        sourceVolume = 255;
    }

    // Adjust ambient volume so it maxes out at overall sound volume
    int ambientvol = (sourceVolume * play.sound_volume) / 255;

    int wantvol;

    if ((thisSound->x == 0) && (thisSound->y == 0)) {
      wantvol = ambientvol;
    }
    else {
      wantvol = get_volume_adjusted_for_distance(ambientvol, thisSound->x, thisSound->y, thisSound->maxdist);
    }

    if (channels[thisSound->channel] == NULL)
      quit("Internal error: the ambient sound channel is enabled, but it has been destroyed");

    channels[thisSound->channel]->set_volume(wantvol);
  }
}

void stop_and_destroy_channel_ex(int chid, bool resetLegacyMusicSettings) {
  if ((chid < 0) || (chid > MAX_SOUND_CHANNELS))
    quit("!StopChannel: invalid channel ID");

  if (channels[chid] != NULL) {
    channels[chid]->destroy();
    delete channels[chid];
    channels[chid] = NULL;
  }

  if (play.crossfading_in_channel == chid)
    play.crossfading_in_channel = 0;
  if (play.crossfading_out_channel == chid)
    play.crossfading_out_channel = 0;
  
  // destroyed an ambient sound channel
  if (ambient[chid].channel > 0)
    ambient[chid].channel = 0;

  if ((chid == SCHAN_MUSIC) && (resetLegacyMusicSettings))
  {
    play.cur_music_number = -1;
    current_music_type = 0;
  }
}

void stop_and_destroy_channel (int chid) 
{
	stop_and_destroy_channel_ex(chid, true);
}

void PlayMusicResetQueue(int newmus) {
  play.music_queue_size = 0;
  newmusic(newmus);
}

void StopAmbientSound (int channel) {
  if ((channel < 0) || (channel >= MAX_SOUND_CHANNELS))
    quit("!StopAmbientSound: invalid channel");

  if (ambient[channel].channel == 0)
    return;

  stop_and_destroy_channel(channel);
  ambient[channel].channel = 0;
}

SOUNDCLIP *load_sound_from_path(int soundNumber, int volume, bool repeat) 
{
  SOUNDCLIP *soundfx = load_sound_clip_from_old_style_number(false, soundNumber, repeat);

  if (soundfx != NULL) {
    if (soundfx->play() == 0)
      soundfx = NULL;
  }

  return soundfx;
}

void PlayAmbientSound (int channel, int sndnum, int vol, int x, int y) {
  // the channel parameter is to allow multiple ambient sounds in future
  if ((channel < 1) || (channel == SCHAN_SPEECH) || (channel >= MAX_SOUND_CHANNELS))
    quit("!PlayAmbientSound: invalid channel number");
  if ((vol < 1) || (vol > 255))
    quit("!PlayAmbientSound: volume must be 1 to 255");

  if (usetup.digicard == DIGI_NONE)
    return;

  // only play the sound if it's not already playing
  if ((ambient[channel].channel < 1) || (channels[ambient[channel].channel] == NULL) ||
      (channels[ambient[channel].channel]->done == 1) ||
      (ambient[channel].num != sndnum)) {

    StopAmbientSound(channel);
    // in case a normal non-ambient sound was playing, stop it too
    stop_and_destroy_channel(channel);

    SOUNDCLIP *asound = load_sound_from_path(sndnum, vol, true);

    if (asound == NULL) {
      debug_log ("Cannot load ambient sound %d", sndnum);
      DEBUG_CONSOLE("FAILED to load ambient sound %d", sndnum);
      return;
    }

    DEBUG_CONSOLE("Playing ambient sound %d on channel %d", sndnum, channel);
    ambient[channel].channel = channel;
    channels[channel] = asound;
    channels[channel]->priority = 15;  // ambient sound higher priority than normal sfx
  }
  // calculate the maximum distance away the player can be, using X
  // only (since X centred is still more-or-less total Y)
  ambient[channel].maxdist = ((x > thisroom.width / 2) ? x : (thisroom.width - x)) - AMBIENCE_FULL_DIST;
  ambient[channel].num = sndnum;
  ambient[channel].x = x;
  ambient[channel].y = y;
  ambient[channel].vol = vol;
  update_ambient_sound_vol();
}

/*
#include "almp3_old.h"
ALLEGRO_MP3 *mp3ptr;
int mp3vol=128;

void amp_setvolume(int newvol) { mp3vol=newvol; }
int load_amp(char*namm,int loop) {
  mp3ptr = new ALLEGRO_MP3(namm);
  if (mp3ptr == NULL) return 0;
  if (mp3ptr->get_error_code() != 0) {
    delete mp3ptr;
    return 0;
    }
  mp3ptr->play(mp3vol, 8192);
  return 1;
  }
void install_amp() { }
void unload_amp() {
  mp3ptr->stop();
  delete mp3ptr;
  }
int amp_decode() {
  mp3ptr->poll();
  if (mp3ptr->is_finished()) {
    if (play.music_repeat)
      mp3ptr->play(mp3vol, 8192);
    else return -1;
    }
  return 0;
  }
*/
//#endif


// check and abort game if the script is currently
// inside the rep_exec_always function
void can_run_delayed_command() {
  if (no_blocking_functions)
    quit("!This command cannot be used within non-blocking events such as " REP_EXEC_ALWAYS_NAME);
}

const char *load_game_errors[9] =
  {"No error","File not found","Not an AGS save game",
  "Invalid save game version","Saved with different interpreter",
  "Saved under a different game", "Resolution mismatch",
  "Colour depth mismatch", ""};

void restart_game() {
  can_run_delayed_command();
  if (inside_script) {
    curscript->queue_action(ePSARestartGame, 0, "RestartGame");
    return;
  }
  int errcod;
  if ((errcod = load_game(RESTART_POINT_SAVE_GAME_NUMBER, NULL, NULL))!=0)
    quitprintf("unable to restart game (error:%s)", load_game_errors[-errcod]);

}

void setpal() {
  wsetpalette(0,255,palette);
}

// Check that a supplied buffer from a text script function was not null
#define VALIDATE_STRING(strin) if ((unsigned long)strin <= 4096) quit("!String argument was null: make sure you pass a string, not an int, as a buffer")


// override packfile functions to allow it to load from our
// custom CLIB datafiles
extern "C" {
PACKFILE*_my_temppack;
extern char* clibgetdatafile(char*);
extern PACKFILE *__old_pack_fopen(char *,char *);

#if ALLEGRO_DATE > 19991010
PACKFILE *pack_fopen(const char *filnam1, const char *modd1) {
#else
PACKFILE *pack_fopen(char *filnam1, char *modd1) {
#endif
  char  *filnam = (char *)filnam1;
  char  *modd = (char *)modd1;
  int   needsetback = 0;

  if (filnam[0] == '~') {
    // ~ signals load from specific data file, not the main default one
    char gfname[80];
    int ii = 0;
    
    filnam++;
    while (filnam[0]!='~') {
      gfname[ii] = filnam[0];
      filnam++;
      ii++;
    }
    filnam++;
    // MACPORT FIX 9/6/5: changed from NULL TO '\0'
    gfname[ii] = '\0';
/*    char useloc[250];
#ifdef LINUX_VERSION
    sprintf(useloc,"%s/%s",usetup.data_files_dir,gfname);
#else
    sprintf(useloc,"%s\\%s",usetup.data_files_dir,gfname);
#endif
    csetlib(useloc,"");*/
    
    char *libname = ci_find_file(usetup.data_files_dir, gfname);
    if (csetlib(libname,""))
    {
      // Hack for running in Debugger
      free(libname);
      libname = ci_find_file("Compiled", gfname);
      csetlib(libname,"");
    }
    free(libname);
    
    needsetback = 1;
  }

  // if the file exists, override the internal file
  FILE *testf = fopen(filnam, "rb");
  if (testf != NULL)
    fclose(testf);

  if ((cliboffset(filnam)<1) || (testf != NULL)) {
    if (needsetback) csetlib(game_file_name,"");
    return __old_pack_fopen(filnam, modd);
  } 
  else {
    _my_temppack=__old_pack_fopen(clibgetdatafile(filnam), modd);
    if (_my_temppack == NULL)
      quitprintf("pack_fopen: unable to change datafile: not found: %s", clibgetdatafile(filnam));

    pack_fseek(_my_temppack,cliboffset(filnam));
    
#if ALLEGRO_DATE < 20050101
    _my_temppack->todo=clibfilesize(filnam);
#else
    _my_temppack->normal.todo = clibfilesize(filnam);
#endif

    if (needsetback)
      csetlib(game_file_name,"");
    return _my_temppack;
  }
}

} // end extern "C"

// end packfile functions


// Binary tree structure for holding translations, allows fast
// access
struct TreeMap {
  TreeMap *left, *right;
  char *text;
  char *translation;

  TreeMap() {
    left = NULL;
    right = NULL;
    text = NULL;
    translation = NULL;
  }
  char* findValue (const char* key) {
    if (text == NULL)
      return NULL;

    if (strcmp(key, text) == 0)
      return translation;
    //debug_log("Compare: '%s' with '%s'", key, text);

    if (strcmp (key, text) < 0) {
      if (left == NULL)
        return NULL;
      return left->findValue (key);
    }
    else {
      if (right == NULL)
        return NULL;
      return right->findValue (key);
    }
  }
  void addText (const char* ntx, char *trans) {
    if ((ntx == NULL) || (ntx[0] == 0) ||
        ((text != NULL) && (strcmp(ntx, text) == 0)))
      // don't add if it's an empty string or if it's already here
      return;

    if (text == NULL) {
      text = (char*)malloc(strlen(ntx)+1);
      translation = (char*)malloc(strlen(trans)+1);
      if (translation == NULL)
        quit("load_translation: out of memory");
      strcpy(text, ntx);
      strcpy(translation, trans);
    }
    else if (strcmp(ntx, text) < 0) {
      // Earlier in alphabet, add to left
      if (left == NULL)
        left = new TreeMap();

      left->addText (ntx, trans);
    }
    else if (strcmp(ntx, text) > 0) {
      // Later in alphabet, add to right
      if (right == NULL)
        right = new TreeMap();

      right->addText (ntx, trans);
    }
  }
  void clear() {
    if (left) {
      left->clear();
      delete left;
    }
    if (right) {
      right->clear();
      delete right;
    }
    if (text)
      free(text);
    if (translation)
      free(translation);
    left = NULL;
    right = NULL;
    text = NULL;
    translation = NULL;
  }
  ~TreeMap() {
    clear();
  }
};

TreeMap *transtree = NULL;
long lang_offs_start = 0;
char transFileName[MAX_PATH] = "\0";

void close_translation () {
  if (transtree != NULL) {
    delete transtree;
    transtree = NULL;
  }
}

bool init_translation (const char *lang) {
  char *transFileLoc;

  if (lang == NULL) {
    sprintf(transFileName, "default.tra");
  }
  else {
    sprintf(transFileName, "%s.tra", lang);
  }

  transFileLoc = ci_find_file(usetup.data_files_dir, transFileName);

  FILE *language_file = clibfopen(transFileLoc, "rb");
  free(transFileLoc);

  if (language_file == NULL) 
  {
    if (lang != NULL)
    {
      // Just in case they're running in Debug, try compiled folder
      sprintf(transFileName, "Compiled\\%s.tra", lang);
      language_file = clibfopen(transFileName, "rb");
    }
    if (language_file == NULL)
      return false;
  }
  // in case it's inside a library file, record the offset
  lang_offs_start = ftell(language_file);

  char transsig[16];
  fread(transsig, 15, 1, language_file);
  if (strcmp(transsig, "AGSTranslation") != 0) {
    fclose(language_file);
    return false;
  }

  if (transtree != NULL)
  {
    close_translation();
  }
  transtree = new TreeMap();

  while (!feof (language_file)) {
    int blockType = getw(language_file);
    if (blockType == -1)
      break;
    // MACPORT FIX 9/6/5: remove warning
    /* int blockSize = */ getw(language_file);

    if (blockType == 1) {
      char original[STD_BUFFER_SIZE], translation[STD_BUFFER_SIZE];
      while (1) {
        read_string_decrypt (language_file, original);
        read_string_decrypt (language_file, translation);
        if ((strlen (original) < 1) && (strlen(translation) < 1))
          break;
        if (feof (language_file))
          quit("!Language file is corrupt");
        transtree->addText (original, translation);
      }

    }
    else if (blockType == 2) {
      int uidfrom;
      char wasgamename[100];
      fread (&uidfrom, 4, 1, language_file);
      read_string_decrypt (language_file, wasgamename);
      if ((uidfrom != game.uniqueid) || (strcmp (wasgamename, game.gamename) != 0)) {
        char quitmess[250];
        sprintf(quitmess,
          "!The translation file you have selected is not compatible with this game. "
          "The translation is designed for '%s'. Make sure the translation was compiled by the original game author.",
          wasgamename);
        quit(quitmess);
      }
    }
    else if (blockType == 3) {
      // game settings
      int temp = getw(language_file);
      // normal font
      if (temp >= 0)
        SetNormalFont (temp);
      temp = getw(language_file);
      // speech font
      if (temp >= 0)
        SetSpeechFont (temp);
      temp = getw(language_file);
      // text direction
      if (temp == 1) {
        play.text_align = SCALIGN_LEFT;
        game.options[OPT_RIGHTLEFTWRITE] = 0;
      }
      else if (temp == 2) {
        play.text_align = SCALIGN_RIGHT;
        game.options[OPT_RIGHTLEFTWRITE] = 1;
      }
    }
    else
      quit("Unknown block type in translation file.");
  }

  fclose (language_file);

  if (transtree->text == NULL)
    quit("!The selected translation file was empty. The translation source may have been translated incorrectly or you may have generated a blank file.");

  return true;
}

char *get_translation (const char *text) {
  if (text == NULL)
    quit("!Null string supplied to CheckForTranslations");

  source_text_length = strlen(text);
  if ((text[0] == '&') && (play.unfactor_speech_from_textlength != 0)) {
    // if there's an "&12 text" type line, remove "&12 " from the source
    // length
    int j = 0;
    while ((text[j] != ' ') && (text[j] != 0))
      j++;
    j++;
    source_text_length -= j;
  }

  // check if a plugin wants to translate it - if so, return that
  char *plResult = (char*)platform->RunPluginHooks(AGSE_TRANSLATETEXT, (int)text);
  if (plResult) {
    if (((int)plResult >= -1) && ((int)plResult < 10000))
      quit("!Plugin did not return a string for text translation");
    return plResult;
  }

  if (transtree != NULL) {
    // translate the text using the translation file
    char * transl = transtree->findValue (text);
    if (transl != NULL)
      return transl;
  }
  // return the original text
  return (char*)text;
}

int IsTranslationAvailable () {
  if (transtree != NULL)
    return 1;
  return 0;
}

int GetTranslationName (char* buffer) {
  VALIDATE_STRING (buffer);
  const char *copyFrom = transFileName;

  while (strchr(copyFrom, '\\') != NULL)
  {
    copyFrom = strchr(copyFrom, '\\') + 1;
  }
  while (strchr(copyFrom, '/') != NULL)
  {
    copyFrom = strchr(copyFrom, '/') + 1;
  }

  strcpy (buffer, copyFrom);
  // remove the ".tra" from the end of the filename
  if (strstr (buffer, ".tra") != NULL)
    strstr (buffer, ".tra")[0] = 0;

  return IsTranslationAvailable();
}

const char* Game_GetTranslationFilename() {
  char buffer[STD_BUFFER_SIZE];
  GetTranslationName(buffer);
  return CreateNewScriptString(buffer);
}

int Game_ChangeTranslation(const char *newFilename)
{
  if ((newFilename == NULL) || (newFilename[0] == 0))
  {
    close_translation();
    strcpy(transFileName, "");
    return 1;
  }

  char oldTransFileName[MAX_PATH];
  strcpy(oldTransFileName, transFileName);

  if (!init_translation(newFilename))
  {
    strcpy(transFileName, oldTransFileName);
    return 0;
  }

  return 1;
}

// End translation functions

volatile int timerloop=0;
volatile int mvolcounter = 0;
int update_music_at=0;
int time_between_timers=25;  // in milliseconds
// our timer, used to keep game running at same speed on all systems
#if defined(WINDOWS_VERSION)
void __cdecl dj_timer_handler() {
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" void dj_timer_handler() {
#else
void dj_timer_handler(...) {
#endif
  timerloop++;
  globalTimerCounter++;
  if (mvolcounter > 0) mvolcounter++;
  }
END_OF_FUNCTION(dj_timer_handler);

void set_game_speed(int fps) {
  frames_per_second = fps;
  time_between_timers = 1000 / fps;
  install_int_ex(dj_timer_handler,MSEC_TO_TIMER(time_between_timers));
}

#ifdef USE_15BIT_FIX
extern "C" {
AL_FUNC(GFX_VTABLE *, _get_vtable, (int color_depth));
}

block convert_16_to_15(block iii) {
//  int xx,yy,rpix;
  int iwid = iii->w, ihit = iii->h;
  int x,y;

  if (bitmap_color_depth(iii) > 16) {
    // we want a 32-to-24 conversion
    block tempbl = create_bitmap_ex(final_col_dep,iwid,ihit);

    if (final_col_dep < 24) {
      // 32-to-16
      blit(iii, tempbl, 0, 0, 0, 0, iwid, ihit);
      return tempbl;
    }

	  GFX_VTABLE *vtable = _get_vtable(final_col_dep);
	  if (vtable == NULL) {
      quit("unable to get 24-bit bitmap vtable");
    }

    tempbl->vtable = vtable;

    for (y=0; y < tempbl->h; y++) {
      unsigned char*p32 = (unsigned char *)iii->line[y];
      unsigned char*p24 = (unsigned char *)tempbl->line[y];

      // strip out the alpha channel bit and copy the rest across
      for (x=0; x < tempbl->w; x++) {
        memcpy(&p24[x * 3], &p32[x * 4], 3);
		  }
    }

    return tempbl;
  }

  // we want a 16-to-15 converstion

  unsigned short c,r,g,b;
  // we do this process manually - no allegro color conversion
  // because we store the RGB in a particular order in the data files
  block tempbl = create_bitmap_ex(15,iwid,ihit);

	GFX_VTABLE *vtable = _get_vtable(15);

	if (vtable == NULL) {
    quit("unable to get 15-bit bitmap vtable");
  }

  tempbl->vtable = vtable;

  for (y=0; y < tempbl->h; y++) {
    unsigned short*p16 = (unsigned short *)iii->line[y];
    unsigned short*p15 = (unsigned short *)tempbl->line[y];

    for (x=0; x < tempbl->w; x++) {
			c = p16[x];
			b = _rgb_scale_5[c & 0x1F];
			g = _rgb_scale_6[(c >> 5) & 0x3F];
			r = _rgb_scale_5[(c >> 11) & 0x1F];
			p15[x] = makecol15(r, g, b);
		}
  }
/*
  // the auto color conversion doesn't seem to work
  for (xx=0;xx<iwid;xx++) {
    for (yy=0;yy<ihit;yy++) {
      rpix = _getpixel16(iii,xx,yy);
      rpix = (rpix & 0x001f) | ((rpix >> 1) & 0x7fe0);
      // again putpixel16 because the dest is actually 16bit
      _putpixel15(tempbl,xx,yy,rpix);
    }
  }*/

  return tempbl;
}

int _places_r = 3, _places_g = 2, _places_b = 3;

// convert RGB to BGR for strange graphics cards
block convert_16_to_16bgr(block tempbl) {

  int x,y;
  unsigned short c,r,g,b;

  for (y=0; y < tempbl->h; y++) {
    unsigned short*p16 = (unsigned short *)tempbl->line[y];

    for (x=0; x < tempbl->w; x++) {
			c = p16[x];
      if (c != MASK_COLOR_16) {
        b = _rgb_scale_5[c & 0x1F];
			  g = _rgb_scale_6[(c >> 5) & 0x3F];
			  r = _rgb_scale_5[(c >> 11) & 0x1F];
        // allegro assumes 5-6-5 for 16-bit
        p16[x] = (((r >> _places_r) << _rgb_r_shift_16) |
            ((g >> _places_g) << _rgb_g_shift_16) |
            ((b >> _places_b) << _rgb_b_shift_16));

      }
		}
  }

  return tempbl;
}
#endif


// PSP: convert 32 bit RGB to BGR.
block convert_32_to_32bgr(block tempbl) {

  unsigned char* current = tempbl->line[0];

  int i = 0;
  int j = 0;
  while (i < tempbl->h)
  {
    current = tempbl->line[i];
    while (j < tempbl->w)
    {
      current[0] ^= current[2];
      current[2] ^= current[0];
      current[0] ^= current[2];
      current += 4;
      j++;
    }
    i++;
    j = 0;
  }

  return tempbl;
}




// Begin resolution system functions

// Multiplies up the number of pixels depending on the current 
// resolution, to give a relatively fixed size at any game res
AGS_INLINE int get_fixed_pixel_size(int pixels)
{
  return pixels * current_screen_resolution_multiplier;
}

AGS_INLINE int convert_to_low_res(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord;
  else
    return coord / current_screen_resolution_multiplier;
}

AGS_INLINE int convert_back_to_high_res(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord;
  else
    return coord * current_screen_resolution_multiplier;
}

AGS_INLINE int multiply_up_coordinate(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord * current_screen_resolution_multiplier;
  else
    return coord;
}

AGS_INLINE void multiply_up_coordinates(int *x, int *y)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
  {
    x[0] *= current_screen_resolution_multiplier;
    y[0] *= current_screen_resolution_multiplier;
  }
}

AGS_INLINE void multiply_up_coordinates_round_up(int *x, int *y)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
  {
    x[0] = x[0] * current_screen_resolution_multiplier + (current_screen_resolution_multiplier - 1);
    y[0] = y[0] * current_screen_resolution_multiplier + (current_screen_resolution_multiplier - 1);
  }
}

AGS_INLINE int divide_down_coordinate(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return coord / current_screen_resolution_multiplier;
  else
    return coord;
}

AGS_INLINE int divide_down_coordinate_round_up(int coord)
{
  if (game.options[OPT_NATIVECOORDINATES] == 0)
    return (coord / current_screen_resolution_multiplier) + (current_screen_resolution_multiplier - 1);
  else
    return coord;
}

// End resolution system functions

int wgetfontheight(int font) {
  int htof = wgettextheight(heightTestString, font);

  // automatic outline fonts are 2 pixels taller
  if (game.fontoutline[font] == FONT_OUTLINE_AUTO) {
    // scaled up SCI font, push outline further out
    if ((game.options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
      htof += get_fixed_pixel_size(2);
    // otherwise, just push outline by 1 pixel
    else
      htof += 2;
  }

  return htof;
}

int wgettextwidth_compensate(const char *tex, int font) {
  int wdof = wgettextwidth(tex, font);

  if (game.fontoutline[font] == FONT_OUTLINE_AUTO) {
    // scaled up SCI font, push outline further out
    if ((game.options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
      wdof += get_fixed_pixel_size(2);
    // otherwise, just push outline by 1 pixel
    else
      wdof += get_fixed_pixel_size(1);
  }

  return wdof;
}




// ** dirty rectangle system **

#define MAXDIRTYREGIONS 25
#define WHOLESCREENDIRTY (MAXDIRTYREGIONS + 5)
#define MAX_SPANS_PER_ROW 4
struct InvalidRect {
  int x1, y1, x2, y2;
};
struct IRSpan {
  int x1, x2;
  int mergeSpan(int tx1, int tx2);
};
struct IRRow {
  IRSpan span[MAX_SPANS_PER_ROW];
  int numSpans;
};
IRRow *dirtyRow = NULL;
int _dirtyRowSize;
InvalidRect dirtyRegions[MAXDIRTYREGIONS];
int numDirtyRegions = 0;
int numDirtyBytes = 0;

int IRSpan::mergeSpan(int tx1, int tx2) {
  if ((tx1 > x2) || (tx2 < x1))
    return 0;
  // overlapping, increase the span
  if (tx1 < x1)
    x1 = tx1;
  if (tx2 > x2)
    x2 = tx2;
  return 1;
}

void init_invalid_regions(int scrnHit) {
  numDirtyRegions = WHOLESCREENDIRTY;
  dirtyRow = (IRRow*)malloc(sizeof(IRRow) * scrnHit);

  for (int e = 0; e < scrnHit; e++)
    dirtyRow[e].numSpans = 0;
  _dirtyRowSize = scrnHit;
}

void update_invalid_region(int x, int y, block src, block dest) {
  int i;

  // convert the offsets for the destination into
  // offsets into the source
  x = -x;
  y = -y;

  if (numDirtyRegions == WHOLESCREENDIRTY) {
    blit(src, dest, x, y, 0, 0, dest->w, dest->h);
  }
  else {
    int k, tx1, tx2, srcdepth = bitmap_color_depth(src);
    if ((srcdepth == bitmap_color_depth(dest)) && (is_memory_bitmap(dest))) {
      int bypp = bmp_bpp(src);
      // do the fast copy
      for (i = 0; i < scrnhit; i++) {
        for (k = 0; k < dirtyRow[i].numSpans; k++) {
          tx1 = dirtyRow[i].span[k].x1;
          tx2 = dirtyRow[i].span[k].x2;
          memcpyfast(&dest->line[i][tx1 * bypp], &src->line[i + y][(tx1 + x) * bypp], ((tx2 - tx1) + 1) * bypp);
        }
      }
    }
    else {
      // do the fast copy
      int rowsInOne;
      for (i = 0; i < scrnhit; i++) {
        rowsInOne = 1;
        
        // if there are rows with identical masks, do them all in one go
        while ((i+rowsInOne < scrnhit) && (memcmp(&dirtyRow[i], &dirtyRow[i+rowsInOne], sizeof(IRRow)) == 0))
          rowsInOne++;

        for (k = 0; k < dirtyRow[i].numSpans; k++) {
          tx1 = dirtyRow[i].span[k].x1;
          tx2 = dirtyRow[i].span[k].x2;
          blit(src, dest, tx1 + x, i + y, tx1, i, (tx2 - tx1) + 1, rowsInOne);
        }
        
        i += (rowsInOne - 1);
      }
    }
   /* else {
      // update the dirty regions
      for (i = 0; i < numDirtyRegions; i++) {
        blit(src, dest, x + dirtyRegions[i].x1, y + dirtyRegions[i].y1,
           dirtyRegions[i].x1, dirtyRegions[i].y1,
           (dirtyRegions[i].x2 - dirtyRegions[i].x1) + 1,
           (dirtyRegions[i].y2 - dirtyRegions[i].y1) + 1);
      }
    }*/
  }
}


void update_invalid_region_and_reset(int x, int y, block src, block dest) {

  int i;

  update_invalid_region(x, y, src, dest);

  // screen has been updated, no longer dirty
  numDirtyRegions = 0;
  numDirtyBytes = 0;

  for (i = 0; i < _dirtyRowSize; i++)
    dirtyRow[i].numSpans = 0;

}

int combine_new_rect(InvalidRect *r1, InvalidRect *r2) {

  // check if new rect is within old rect X-wise
  if ((r2->x1 >= r1->x1) && (r2->x2 <= r1->x2)) {
    if ((r2->y1 >= r1->y1) && (r2->y2 <= r1->y2)) {
      // Y is also within the old one - scrap the new rect
      return 1;
    }
  }

  return 0;
}

void invalidate_rect(int x1, int y1, int x2, int y2) {
  if (numDirtyRegions >= MAXDIRTYREGIONS) {
    // too many invalid rectangles, just mark the whole thing dirty
    numDirtyRegions = WHOLESCREENDIRTY;
    return;
  }

  int a;

  if (x1 >= scrnwid) x1 = scrnwid-1;
  if (y1 >= scrnhit) y1 = scrnhit-1;
  if (x2 >= scrnwid) x2 = scrnwid-1;
  if (y2 >= scrnhit) y2 = scrnhit-1;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 < 0) x2 = 0;
  if (y2 < 0) y2 = 0;
/*
  dirtyRegions[numDirtyRegions].x1 = x1;
  dirtyRegions[numDirtyRegions].y1 = y1;
  dirtyRegions[numDirtyRegions].x2 = x2;
  dirtyRegions[numDirtyRegions].y2 = y2;

  for (a = 0; a < numDirtyRegions; a++) {
    // see if we can merge it into any other regions
    if (combine_new_rect(&dirtyRegions[a], &dirtyRegions[numDirtyRegions]))
      return;
  }

  numDirtyBytes += (x2 - x1) * (y2 - y1);

  if (numDirtyBytes > (scrnwid * scrnhit) / 2)
    numDirtyRegions = WHOLESCREENDIRTY;
  else {*/
    numDirtyRegions++;

    // ** Span code
    int s, foundOne;
    // add this rect to the list for this row
    for (a = y1; a <= y2; a++) {
      foundOne = 0;
      for (s = 0; s < dirtyRow[a].numSpans; s++) {
        if (dirtyRow[a].span[s].mergeSpan(x1, x2)) {
          foundOne = 1;
          break;
        }
      }
      if (foundOne) {
        // we were merged into a span, so we're ok
        int t;
        // check whether now two of the spans overlap each other
        // in which case merge them
        for (s = 0; s < dirtyRow[a].numSpans; s++) {
          for (t = s + 1; t < dirtyRow[a].numSpans; t++) {
            if (dirtyRow[a].span[s].mergeSpan(dirtyRow[a].span[t].x1, dirtyRow[a].span[t].x2)) {
              dirtyRow[a].numSpans--;
              for (int u = t; u < dirtyRow[a].numSpans; u++)
                dirtyRow[a].span[u] = dirtyRow[a].span[u + 1];
              break;
            }
          }
        }
      }
      else if (dirtyRow[a].numSpans < MAX_SPANS_PER_ROW) {
        dirtyRow[a].span[dirtyRow[a].numSpans].x1 = x1;
        dirtyRow[a].span[dirtyRow[a].numSpans].x2 = x2;
        dirtyRow[a].numSpans++;
      }
      else {
        // didn't fit in an existing span, and there are none spare
        int nearestDist = 99999, nearestWas = -1, extendLeft;
        int tleft, tright;
        // find the nearest span, and enlarge that to include this rect
        for (s = 0; s < dirtyRow[a].numSpans; s++) {
          tleft = dirtyRow[a].span[s].x1 - x2;
          if ((tleft > 0) && (tleft < nearestDist)) {
            nearestDist = tleft;
            nearestWas = s;
            extendLeft = 1;
          }
          tright = x1 - dirtyRow[a].span[s].x2;
          if ((tright > 0) && (tright < nearestDist)) {
            nearestDist = tright;
            nearestWas = s;
            extendLeft = 0;
          }
        }
        if (extendLeft)
          dirtyRow[a].span[nearestWas].x1 = x1;
        else
          dirtyRow[a].span[nearestWas].x2 = x2;
      }
    }
    // ** End span code
  //}
}

void invalidate_sprite(int x1, int y1, IDriverDependantBitmap *pic) {
  invalidate_rect(x1, y1, x1 + pic->GetWidth(), y1 + pic->GetHeight());
}

void draw_and_invalidate_text(int x1, int y1, int font, const char *text) {
  wouttext_outline(x1, y1, font, (char*)text);
  invalidate_rect(x1, y1, x1 + wgettextwidth_compensate(text, font), y1 + wgetfontheight(font) + get_fixed_pixel_size(1));
}

void invalidate_screen() {
  // mark the whole screen dirty
  numDirtyRegions = WHOLESCREENDIRTY;
}

// ** dirty rectangle system end **

void mark_current_background_dirty()
{
  current_background_is_dirty = true;
}

inline int is_valid_object(int obtest) {
  if ((obtest < 0) || (obtest >= croom->numobj)) return 0;
  return 1;
}

void SetAmbientTint (int red, int green, int blue, int opacity, int luminance) {
  if ((red < 0) || (green < 0) || (blue < 0) ||
      (red > 255) || (green > 255) || (blue > 255) ||
      (opacity < 0) || (opacity > 100) ||
      (luminance < 0) || (luminance > 100))
    quit("!SetTint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-100");

  DEBUG_CONSOLE("Set ambient tint RGB(%d,%d,%d) %d%%", red, green, blue, opacity);

  play.rtint_red = red;
  play.rtint_green = green;
  play.rtint_blue = blue;
  play.rtint_level = opacity;
  play.rtint_light = (luminance * 25) / 10;
}

void SetObjectTint(int obj, int red, int green, int blue, int opacity, int luminance) {
  if ((red < 0) || (green < 0) || (blue < 0) ||
      (red > 255) || (green > 255) || (blue > 255) ||
      (opacity < 0) || (opacity > 100) ||
      (luminance < 0) || (luminance > 100))
    quit("!SetObjectTint: invalid parameter. R,G,B must be 0-255, opacity & luminance 0-100");

  if (!is_valid_object(obj))
    quit("!SetObjectTint: invalid object number specified");

  DEBUG_CONSOLE("Set object %d tint RGB(%d,%d,%d) %d%%", obj, red, green, blue, opacity);

  objs[obj].tint_r = red;
  objs[obj].tint_g = green;
  objs[obj].tint_b = blue;
  objs[obj].tint_level = opacity;
  objs[obj].tint_light = (luminance * 25) / 10;
  objs[obj].flags |= OBJF_HASTINT;
}

void Object_Tint(ScriptObject *objj, int red, int green, int blue, int saturation, int luminance) {
  SetObjectTint(objj->id, red, green, blue, saturation, luminance);
}

void RemoveObjectTint(int obj) {
  if (!is_valid_object(obj))
    quit("!RemoveObjectTint: invalid object");
  
  if (objs[obj].flags & OBJF_HASTINT) {
    DEBUG_CONSOLE("Un-tint object %d", obj);
    objs[obj].flags &= ~OBJF_HASTINT;
  }
  else {
    debug_log("RemoveObjectTint called but object was not tinted");
  }
}

void Object_RemoveTint(ScriptObject *objj) {
  RemoveObjectTint(objj->id);
}

void TintScreen(int red, int grn, int blu) {
  if ((red < 0) || (grn < 0) || (blu < 0) || (red > 100) || (grn > 100) || (blu > 100))
    quit("!TintScreen: RGB values must be 0-100");

  invalidate_screen();

  if ((red == 0) && (grn == 0) && (blu == 0)) {
    play.screen_tint = -1;
    return;
  }
  red = (red * 25) / 10;
  grn = (grn * 25) / 10;
  blu = (blu * 25) / 10;
  play.screen_tint = red + (grn << 8) + (blu << 16);
}

int get_screen_y_adjustment(BITMAP *checkFor) {

  if ((screen == _sub_screen) && (checkFor->h < final_scrn_hit))
    return get_fixed_pixel_size(20);

  return 0;
}

int get_screen_x_adjustment(BITMAP *checkFor) {

  if ((screen == _sub_screen) && (checkFor->w < final_scrn_wid))
    return (final_scrn_wid - checkFor->w) / 2;

  return 0;
}

void render_black_borders(int atx, int aty)
{
  if (!gfxDriver->UsesMemoryBackBuffer())
  {
    if (aty > 0)
    {
      // letterbox borders
      blankImage->SetStretch(scrnwid, aty);
      gfxDriver->DrawSprite(0, -aty, blankImage);
      gfxDriver->DrawSprite(0, scrnhit, blankImage);
    }
    if (atx > 0)
    {
      // sidebar borders for widescreen
      blankSidebarImage->SetStretch(atx, scrnhit);
      gfxDriver->DrawSprite(-atx, 0, blankSidebarImage);
      gfxDriver->DrawSprite(scrnwid, 0, blankSidebarImage);
    }
  }
}

void render_to_screen(BITMAP *toRender, int atx, int aty) {

  atx += get_screen_x_adjustment(toRender);
  aty += get_screen_y_adjustment(toRender);
  gfxDriver->SetRenderOffset(atx, aty);

  render_black_borders(atx, aty);

  gfxDriver->DrawSprite(AGSE_FINALSCREENDRAW, 0, NULL);

  if (play.screen_is_faded_out)
  {
    if (gfxDriver->UsesMemoryBackBuffer())
      gfxDriver->RenderToBackBuffer();
    gfxDriver->ClearDrawList();
    return;
  }

  // only vsync in full screen mode, it makes things worse
  // in a window
  gfxDriver->EnableVsyncBeforeRender((scsystem.vsync > 0) && (usetup.windowed == 0));

  bool succeeded = false;
  while (!succeeded)
  {
    try
    {
      gfxDriver->Render((GlobalFlipType)play.screen_flipped);
      succeeded = true;
    }
    catch (Ali3DFullscreenLostException) 
    { 
      platform->Delay(500);
    }
  }
}

void clear_letterbox_borders() {

  if (multiply_up_coordinate(thisroom.height) < final_scrn_hit) {
    // blank out any traces in borders left by a full-screen room
    gfxDriver->ClearRectangle(0, 0, _old_screen->w - 1, get_fixed_pixel_size(20) - 1, NULL);
    gfxDriver->ClearRectangle(0, final_scrn_hit - get_fixed_pixel_size(20), _old_screen->w - 1, final_scrn_hit - 1, NULL);
  }

}

// writes the virtual screen to the screen, converting colours if
// necessary
void write_screen() {

  if (play.fast_forward)
    return;

  static int wasShakingScreen = 0;
  bool clearScreenBorders = false;
  int at_yp = 0;

  if (play.shakesc_length > 0) {
    wasShakingScreen = 1;
    if ( (loopcounter % play.shakesc_delay) < (play.shakesc_delay / 2) )
      at_yp = multiply_up_coordinate(play.shakesc_amount);
    invalidate_screen();
  }
  else if (wasShakingScreen) {
    wasShakingScreen = 0;

    if (!gfxDriver->RequiresFullRedrawEachFrame())
    {
      clear_letterbox_borders();
    }
  }

  if (play.screen_tint < 1)
    gfxDriver->SetScreenTint(0, 0, 0);
  else
    gfxDriver->SetScreenTint(play.screen_tint & 0xff, (play.screen_tint >> 8) & 0xff, (play.screen_tint >> 16) & 0xff);

  render_to_screen(virtual_screen, 0, at_yp);
}

extern char buffer2[60];
int oldmouse;
void setup_for_dialog() {
  cbuttfont = play.normal_font;
  acdialog_font = play.normal_font;
  wsetscreen(virtual_screen);
  if (!play.mouse_cursor_hidden)
    domouse(1);
  oldmouse=cur_cursor; set_mouse_cursor(CURS_ARROW);
}
void restore_after_dialog() {
  set_mouse_cursor(oldmouse);
  if (!play.mouse_cursor_hidden)
    domouse(2);
  construct_virtual_screen(true);
}

void RestoreGameSlot(int slnum) {
  if (displayed_room < 0)
    quit("!RestoreGameSlot: a game cannot be restored from within game_start");

  can_run_delayed_command();
  if (inside_script) {
    curscript->queue_action(ePSARestoreGame, slnum, "RestoreGameSlot");
    return;
  }
  load_game(slnum, NULL, NULL);
}

void get_save_game_path(int slotNum, char *buffer) {
  strcpy(buffer, saveGameDirectory);
  sprintf(&buffer[strlen(buffer)], sgnametemplate, slotNum);
  strcat(buffer, saveGameSuffix);
}

void DeleteSaveSlot (int slnum) {
  char nametouse[260];
  get_save_game_path(slnum, nametouse);
  unlink (nametouse);
  if ((slnum >= 1) && (slnum <= MAXSAVEGAMES)) {
    char thisname[260];
    for (int i = MAXSAVEGAMES; i > slnum; i--) {
      get_save_game_path(i, thisname);
      FILE *fin = fopen (thisname, "rb");
      if (fin != NULL) {
        fclose (fin);
        // Rename the highest save game to fill in the gap
        rename (thisname, nametouse);
        break;
      }
    }

  }
}

int Game_SetSaveGameDirectory(const char *newFolder) {

  // don't allow them to go to another folder
  if ((newFolder[0] == '/') || (newFolder[0] == '\\') ||
    (newFolder[0] == ' ') ||
    ((newFolder[0] != 0) && (newFolder[1] == ':')))
    return 0;

  char newSaveGameDir[260];
  platform->ReplaceSpecialPaths(newFolder, newSaveGameDir);
  fix_filename_slashes(newSaveGameDir);

#ifdef LINUX_VERSION
  mkdir(newSaveGameDir, 0);
#else
  mkdir(newSaveGameDir);
#endif

  put_backslash(newSaveGameDir);

  char newFolderTempFile[260];
  strcpy(newFolderTempFile, newSaveGameDir);
  strcat(newFolderTempFile, "agstmp.tmp");

  FILE *testTemp = fopen(newFolderTempFile, "wb");
  if (testTemp == NULL) {
    return 0;
  }
  fclose(testTemp);
  unlink(newFolderTempFile);

  // copy the Restart Game file, if applicable
  char restartGamePath[260];
  sprintf(restartGamePath, "%s""agssave.%d%s", saveGameDirectory, RESTART_POINT_SAVE_GAME_NUMBER, saveGameSuffix);
  FILE *restartGameFile = fopen(restartGamePath, "rb");
  if (restartGameFile != NULL) {
    long fileSize = filelength(fileno(restartGameFile));
    char *mbuffer = (char*)malloc(fileSize);
    fread(mbuffer, fileSize, 1, restartGameFile);
    fclose(restartGameFile);

    sprintf(restartGamePath, "%s""agssave.%d%s", newSaveGameDir, RESTART_POINT_SAVE_GAME_NUMBER, saveGameSuffix);
    restartGameFile = fopen(restartGamePath, "wb");
    fwrite(mbuffer, fileSize, 1, restartGameFile);
    fclose(restartGameFile);
    free(mbuffer);
  }

  strcpy(saveGameDirectory, newSaveGameDir);
  return 1;
}

int GetSaveSlotDescription(int slnum,char*desbuf) {
  VALIDATE_STRING(desbuf);
  if (load_game(slnum, desbuf, NULL) == 0)
    return 1;
  sprintf(desbuf,"INVALID SLOT %d", slnum);
  return 0;
}

const char* Game_GetSaveSlotDescription(int slnum) {
  char buffer[STD_BUFFER_SIZE];
  if (load_game(slnum, buffer, NULL) == 0)
    return CreateNewScriptString(buffer);
  return NULL;
}

int LoadSaveSlotScreenshot(int slnum, int width, int height) {
  int gotSlot;
  multiply_up_coordinates(&width, &height);

  if (load_game(slnum, NULL, &gotSlot) != 0)
    return 0;

  if (gotSlot == 0)
    return 0;

  if ((spritewidth[gotSlot] == width) && (spriteheight[gotSlot] == height))
    return gotSlot;

  // resize the sprite to the requested size
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[gotSlot]), width, height);

  stretch_blit(spriteset[gotSlot], newPic,
               0, 0, spritewidth[gotSlot], spriteheight[gotSlot],
               0, 0, width, height);

  update_polled_stuff();

  // replace the bitmap in the sprite set
  free_dynamic_sprite(gotSlot);
  add_dynamic_sprite(gotSlot, newPic);

  return gotSlot;
}

void get_current_dir_path(char* buffer, const char *fileName)
{
  if (use_compiled_folder_as_current_dir)
  {
    sprintf(buffer, "Compiled\\%s", fileName);
  }
  else
  {
    strcpy(buffer, fileName);
  }
}

int LoadImageFile(const char *filename) {
  
  char loadFromPath[MAX_PATH];
  get_current_dir_path(loadFromPath, filename);

  block loadedFile = load_bitmap(loadFromPath, NULL);

  if (loadedFile == NULL)
    return 0;

  int gotSlot = spriteset.findFreeSlot();
  if (gotSlot <= 0)
    return 0;

  add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(loadedFile));

  return gotSlot;
}

int load_game_and_print_error(int toload) {
  int ecret = load_game(toload, NULL, NULL);
  if (ecret < 0) {
    // disable speech in case there are dynamic graphics that
    // have been freed
    int oldalways = game.options[OPT_ALWAYSSPCH];
    game.options[OPT_ALWAYSSPCH] = 0;
    Display("Unable to load game (error: %s).",load_game_errors[-ecret]);
    game.options[OPT_ALWAYSSPCH] = oldalways;
  }
  return ecret;
}

void restore_game_dialog() {
  can_run_delayed_command();
  if (thisroom.options[ST_SAVELOAD] == 1) {
    DisplayMessage (983);
    return;
  }
  if (inside_script) {
    curscript->queue_action(ePSARestoreGameDialog, 0, "RestoreGameDialog");
    return;
  }
  setup_for_dialog();
  int toload=loadgamedialog();
  restore_after_dialog();
  if (toload>=0) {
    load_game_and_print_error(toload);
  }
}

void save_game_dialog() {
  if (thisroom.options[ST_SAVELOAD] == 1) {
    DisplayMessage (983);
    return;
  }
  if (inside_script) {
    curscript->queue_action(ePSASaveGameDialog, 0, "SaveGameDialog");
    return;
  }
  setup_for_dialog();
  int toload=savegamedialog();
  restore_after_dialog();
  if (toload>=0)
    save_game(toload,buffer2);
  }

void update_script_mouse_coords() {
  scmouse.x = divide_down_coordinate(mousex);
  scmouse.y = divide_down_coordinate(mousey);
}

char scfunctionname[30];
int prepare_text_script(ccInstance*sci,char**tsname) {
  ccError=0;
  if (sci==NULL) return -1;
  if (ccGetSymbolAddr(sci,tsname[0]) == NULL) {
    strcpy (ccErrorString, "no such function in script");
    return -2;
  }
  if (sci->pc!=0) {
    strcpy(ccErrorString,"script is already in execution");
    return -3;
  }
  scripts[num_scripts].init();
  scripts[num_scripts].inst = sci;
/*  char tempb[300];
  sprintf(tempb,"Creating script instance for '%s' room %d",tsname[0],displayed_room);
  write_log(tempb);*/
  if (sci->pc != 0) {
//    write_log("Forking instance");
    scripts[num_scripts].inst = ccForkInstance(sci);
    if (scripts[num_scripts].inst == NULL)
      quit("unable to fork instance for secondary script");
    scripts[num_scripts].forked = 1;
    }
  curscript = &scripts[num_scripts];
  num_scripts++;
  if (num_scripts >= MAX_SCRIPT_AT_ONCE)
    quit("too many nested text script instances created");
  // in case script_run_another is the function name, take a backup
  strcpy(scfunctionname,tsname[0]);
  tsname[0]=&scfunctionname[0];
  update_script_mouse_coords();
  inside_script++;
//  aborted_ip=0;
//  abort_executor=0;
  return 0;
  }

void cancel_all_scripts() {
  int aa;

  for (aa = 0; aa < num_scripts; aa++) {
    if (scripts[aa].forked)
      ccAbortAndDestroyInstance(scripts[aa].inst);
    else
      ccAbortInstance(scripts[aa].inst);
    scripts[aa].numanother = 0;
    }
  num_scripts = 0;
/*  if (gameinst!=NULL) ccAbortInstance(gameinst);
  if (roominst!=NULL) ccAbortInstance(roominst);*/
  }

void post_script_cleanup() {
  // should do any post-script stuff here, like go to new room
  if (ccError) quit(ccErrorString);
  ExecutingScript copyof = scripts[num_scripts-1];
//  write_log("Instance finished.");
  if (scripts[num_scripts-1].forked)
    ccFreeInstance(scripts[num_scripts-1].inst);
  num_scripts--;
  inside_script--;

  if (num_scripts > 0)
    curscript = &scripts[num_scripts-1];
  else {
    curscript = NULL;
  }
//  if (abort_executor) user_disabled_data2=aborted_ip;

  int old_room_number = displayed_room;

  // run the queued post-script actions
  for (int ii = 0; ii < copyof.numPostScriptActions; ii++) {
    int thisData = copyof.postScriptActionData[ii];

    switch (copyof.postScriptActions[ii]) {
    case ePSANewRoom:
      // only change rooms when all scripts are done
      if (num_scripts == 0) {
        new_room(thisData, playerchar);
        // don't allow any pending room scripts from the old room
        // in run_another to be executed
        return;
      }
      else
        curscript->queue_action(ePSANewRoom, thisData, "NewRoom");
      break;
    case ePSAInvScreen:
      invscreen();
      break;
    case ePSARestoreGame:
      cancel_all_scripts();
      load_game_and_print_error(thisData);
      return;
    case ePSARestoreGameDialog:
      restore_game_dialog();
      return;
    case ePSARunAGSGame:
      cancel_all_scripts();
      load_new_game = thisData;
      return;
    case ePSARunDialog:
      do_conversation(thisData);
      break;
    case ePSARestartGame:
      cancel_all_scripts();
      restart_game();
      return;
    case ePSASaveGame:
      save_game(thisData, copyof.postScriptSaveSlotDescription[ii]);
      break;
    case ePSASaveGameDialog:
      save_game_dialog();
      break;
    default:
      quitprintf("undefined post script action found: %d", copyof.postScriptActions[ii]);
    }
    // if the room changed in a conversation, for example, abort
    if (old_room_number != displayed_room) {
      return;
    }
  }


  int jj;
  for (jj = 0; jj < copyof.numanother; jj++) {
    old_room_number = displayed_room;
    char runnext[40];
    strcpy(runnext,copyof.script_run_another[jj]);
    copyof.script_run_another[jj][0]=0;
    if (runnext[0]=='#')
      run_text_script_2iparam(gameinst,&runnext[1],copyof.run_another_p1[jj],copyof.run_another_p2[jj]);
    else if (runnext[0]=='!')
      run_text_script_iparam(gameinst,&runnext[1],copyof.run_another_p1[jj]);
    else if (runnext[0]=='|')
      run_text_script(roominst,&runnext[1]);
    else if (runnext[0]=='%')
      run_text_script_2iparam(roominst, &runnext[1], copyof.run_another_p1[jj], copyof.run_another_p2[jj]);
    else if (runnext[0]=='$') {
      run_text_script_iparam(roominst,&runnext[1],copyof.run_another_p1[jj]);
      play.roomscript_finished = 1;
    }
    else
      run_text_script(gameinst,runnext);

    // if they've changed rooms, cancel any further pending scripts
    if ((displayed_room != old_room_number) || (load_new_game))
      break;
  }
  copyof.numanother = 0;

}

void quit_with_script_error(const char *functionName)
{
  quitprintf("%sError running function '%s':\n%s", (ccErrorIsUserError ? "!" : ""), functionName, ccErrorString);
}

void _do_run_script_func_cant_block(ccInstance *forkedinst, NonBlockingScriptFunction* funcToRun, bool *hasTheFunc) {
  if (!hasTheFunc[0])
    return;

  no_blocking_functions++;
  int result;

  if (funcToRun->numParameters == 0)
    result = ccCallInstance(forkedinst, (char*)funcToRun->functionName, 0);
  else if (funcToRun->numParameters == 1)
    result = ccCallInstance(forkedinst, (char*)funcToRun->functionName, 1, funcToRun->param1);
  else if (funcToRun->numParameters == 2)
    result = ccCallInstance(forkedinst, (char*)funcToRun->functionName, 2, funcToRun->param1, funcToRun->param2);
  else
    quit("_do_run_script_func_cant_block called with too many parameters");

  if (result == -2) {
    // the function doens't exist, so don't try and run it again
    hasTheFunc[0] = false;
  }
  else if ((result != 0) && (result != 100)) {
    quit_with_script_error(funcToRun->functionName);
  }
  else
  {
    funcToRun->atLeastOneImplementationExists = true;
  }
  // this might be nested, so don't disrupt blocked scripts
  ccErrorString[0] = 0;
  ccError = 0;
  no_blocking_functions--;
}

void run_function_on_non_blocking_thread(NonBlockingScriptFunction* funcToRun) {

  update_script_mouse_coords();

  int room_changes_was = play.room_changes;
  funcToRun->atLeastOneImplementationExists = false;

  // run modules
  // modules need a forkedinst for this to work
  for (int kk = 0; kk < numScriptModules; kk++) {
    _do_run_script_func_cant_block(moduleInstFork[kk], funcToRun, &funcToRun->moduleHasFunction[kk]);

    if (room_changes_was != play.room_changes)
      return;
  }

  _do_run_script_func_cant_block(gameinstFork, funcToRun, &funcToRun->globalScriptHasFunction);

  if (room_changes_was != play.room_changes)
    return;

  _do_run_script_func_cant_block(roominstFork, funcToRun, &funcToRun->roomHasFunction);
}

int run_script_function_if_exist(ccInstance*sci,char*tsname,int numParam, int iparam, int iparam2, int iparam3) {
  int oldRestoreCount = gameHasBeenRestored;
  // First, save the current ccError state
  // This is necessary because we might be attempting
  // to run Script B, while Script A is still running in the
  // background.
  // If CallInstance here has an error, it would otherwise
  // also abort Script A because ccError is a global variable.
  int cachedCcError = ccError;
  ccError = 0;

  int toret = prepare_text_script(sci,&tsname);
  if (toret) {
    ccError = cachedCcError;
    return -18;
  }

  // Clear the error message
  ccErrorString[0] = 0;

  if (numParam == 0) 
    toret = ccCallInstance(curscript->inst,tsname,numParam);
  else if (numParam == 1)
    toret = ccCallInstance(curscript->inst,tsname,numParam, iparam);
  else if (numParam == 2)
    toret = ccCallInstance(curscript->inst,tsname,numParam,iparam, iparam2);
  else if (numParam == 3)
    toret = ccCallInstance(curscript->inst,tsname,numParam,iparam, iparam2, iparam3);
  else
    quit("Too many parameters to run_script_function_if_exist");

  // 100 is if Aborted (eg. because we are LoadAGSGame'ing)
  if ((toret != 0) && (toret != -2) && (toret != 100)) {
    quit_with_script_error(tsname);
  }

  post_script_cleanup_stack++;

  if (post_script_cleanup_stack > 50)
    quitprintf("!post_script_cleanup call stack exceeded: possible recursive function call? running %s", tsname);

  post_script_cleanup();

  post_script_cleanup_stack--;

  // restore cached error state
  ccError = cachedCcError;

  // if the game has been restored, ensure that any further scripts are not run
  if ((oldRestoreCount != gameHasBeenRestored) && (eventClaimed == EVENT_INPROGRESS))
    eventClaimed = EVENT_CLAIMED;

  return toret;
}

int run_text_script(ccInstance*sci,char*tsname) {
  if (strcmp(tsname, REP_EXEC_NAME) == 0) {
    // run module rep_execs
    int room_changes_was = play.room_changes;
    int restore_game_count_was = gameHasBeenRestored;

    for (int kk = 0; kk < numScriptModules; kk++) {
      if (moduleRepExecAddr[kk] != NULL)
        run_script_function_if_exist(moduleInst[kk], tsname, 0, 0, 0);

      if ((room_changes_was != play.room_changes) ||
          (restore_game_count_was != gameHasBeenRestored))
        return 0;
    }
  }

  int toret = run_script_function_if_exist(sci, tsname, 0, 0, 0);
  if ((toret == -18) && (sci == roominst)) {
    // functions in room script must exist
    quitprintf("prepare_script: error %d (%s) trying to run '%s'   (Room %d)",toret,ccErrorString,tsname, displayed_room);
  }
  return toret;
}

int run_claimable_event(char *tsname, bool includeRoom, int numParams, int param1, int param2, bool *eventWasClaimed) {
  *eventWasClaimed = true;
  // Run the room script function, and if it is not claimed,
  // then run the main one
  // We need to remember the eventClaimed variable's state, in case
  // this is a nested event
  int eventClaimedOldValue = eventClaimed;
  eventClaimed = EVENT_INPROGRESS;
  int toret;

  if (includeRoom) {
    toret = run_script_function_if_exist(roominst, tsname, numParams, param1, param2);

    if (eventClaimed == EVENT_CLAIMED) {
      eventClaimed = eventClaimedOldValue;
      return toret;
    }
  }

  // run script modules
  for (int kk = 0; kk < numScriptModules; kk++) {
    toret = run_script_function_if_exist(moduleInst[kk], tsname, numParams, param1, param2);

    if (eventClaimed == EVENT_CLAIMED) {
      eventClaimed = eventClaimedOldValue;
      return toret;
    }
  }

  eventClaimed = eventClaimedOldValue;
  *eventWasClaimed = false;
  return 0;
}

int run_text_script_iparam(ccInstance*sci,char*tsname,int iparam) {
  if ((strcmp(tsname, "on_key_press") == 0) || (strcmp(tsname, "on_mouse_click") == 0)) {
    bool eventWasClaimed;
    int toret = run_claimable_event(tsname, true, 1, iparam, 0, &eventWasClaimed);

    if (eventWasClaimed)
      return toret;
  }

  return run_script_function_if_exist(sci, tsname, 1, iparam, 0);
}

int run_text_script_2iparam(ccInstance*sci,char*tsname,int iparam,int param2) {
  if (strcmp(tsname, "on_event") == 0) {
    bool eventWasClaimed;
    int toret = run_claimable_event(tsname, true, 2, iparam, param2, &eventWasClaimed);

    if (eventWasClaimed)
      return toret;
  }

  // response to a button click, better update guis
  if (strnicmp(tsname, "interface_click", 15) == 0)
    guis_need_update = 1;

  int toret = run_script_function_if_exist(sci, tsname, 2, iparam, param2);

  // tsname is no longer valid, because run_script_function_if_exist might
  // have restored a save game and freed the memory. Therefore don't 
  // attempt any strcmp's here
  tsname = NULL;

  return toret;
}

void remove_screen_overlay_index(int cc) {
  int dd;
  if (screenover[cc].pic!=NULL)
    wfreeblock(screenover[cc].pic);
  screenover[cc].pic=NULL;

  if (screenover[cc].bmp != NULL)
    gfxDriver->DestroyDDB(screenover[cc].bmp);
  screenover[cc].bmp = NULL;

  if (screenover[cc].type==OVER_COMPLETE) is_complete_overlay--;
  if (screenover[cc].type==OVER_TEXTMSG) is_text_overlay--;

  // if the script didn't actually use the Overlay* return
  // value, dispose of the pointer
  if (screenover[cc].associatedOverlayHandle)
    ccAttemptDisposeObject(screenover[cc].associatedOverlayHandle);

  numscreenover--;
  for (dd = cc; dd < numscreenover; dd++)
    screenover[dd] = screenover[dd+1];

  // if an overlay before the sierra-style speech one is removed,
  // update the index
  if (face_talking > cc)
    face_talking--;
}

void remove_screen_overlay(int type) {
  int cc;
  for (cc=0;cc<numscreenover;cc++) {
    if (screenover[cc].type==type) ;
    else if (type==-1) ;
    else continue;
    remove_screen_overlay_index(cc);
    cc--;
  }
}

int find_overlay_of_type(int typ) {
  int ww;
  for (ww=0;ww<numscreenover;ww++) {
    if (screenover[ww].type == typ) return ww;
    }
  return -1;
  }

int add_screen_overlay(int x,int y,int type,block piccy, bool alphaChannel = false) {
  if (numscreenover>=MAX_SCREEN_OVERLAYS)
    quit("too many screen overlays created");
  if (type==OVER_COMPLETE) is_complete_overlay++;
  if (type==OVER_TEXTMSG) is_text_overlay++;
  if (type==OVER_CUSTOM) {
    int uu;  // find an unused custom ID
    for (uu=OVER_CUSTOM+1;uu<OVER_CUSTOM+100;uu++) {
      if (find_overlay_of_type(uu) == -1) { type=uu; break; }
      }
    }
  screenover[numscreenover].pic=piccy;
  screenover[numscreenover].bmp = gfxDriver->CreateDDBFromBitmap(piccy, alphaChannel);
  screenover[numscreenover].x=x;
  screenover[numscreenover].y=y;
  screenover[numscreenover].type=type;
  screenover[numscreenover].timeout=0;
  screenover[numscreenover].bgSpeechForChar = -1;
  screenover[numscreenover].associatedOverlayHandle = 0;
  screenover[numscreenover].hasAlphaChannel = alphaChannel;
  screenover[numscreenover].positionRelativeToScreen = true;
  numscreenover++;
  return numscreenover-1;
  }

void my_fade_out(int spdd) {
  EndSkippingUntilCharStops();

  if (play.fast_forward)
    return;

  if (play.screen_is_faded_out == 0)
    gfxDriver->FadeOut(spdd, play.fade_to_red, play.fade_to_green, play.fade_to_blue);

  if (game.color_depth > 1)
    play.screen_is_faded_out = 1;
}

void my_fade_in(PALLETE p, int speed) {
  if (game.color_depth > 1) {
    set_palette (p);

    play.screen_is_faded_out = 0;

    if (play.no_hicolor_fadein) {
      return;
    }
  }

  gfxDriver->FadeIn(speed, p, play.fade_to_red, play.fade_to_green, play.fade_to_blue);
}


int GetMaxScreenHeight () {
  int maxhit = BASEHEIGHT;
  if ((maxhit == 200) || (maxhit == 400))
  {
    // uh ... BASEHEIGHT depends on Native Coordinates setting so be careful
    if ((usetup.want_letterbox) && (thisroom.height > maxhit)) 
      maxhit = divide_down_coordinate(multiply_up_coordinate(maxhit) + get_fixed_pixel_size(40));
  }
  return maxhit;
}

block fix_bitmap_size(block todubl) {
  int oldw=todubl->w, oldh=todubl->h;
  int newWidth = multiply_up_coordinate(thisroom.width);
  int newHeight = multiply_up_coordinate(thisroom.height);

  if ((oldw == newWidth) && (oldh == newHeight))
    return todubl;

//  block tempb=create_bitmap(scrnwid,scrnhit);
  block tempb=create_bitmap_ex(bitmap_color_depth(todubl), newWidth, newHeight);
  set_clip(tempb,0,0,tempb->w-1,tempb->h-1);
  set_clip(todubl,0,0,oldw-1,oldh-1);
  clear(tempb);
  stretch_blit(todubl,tempb,0,0,oldw,oldh,0,0,tempb->w,tempb->h);
  destroy_bitmap(todubl); todubl=tempb;
  return todubl;
}


//#define _get_script_data_stack_size() (256*sizeof(int)+((int*)&scrpt[10*4])[0]+((int*)&scrpt[12*4])[0])
//#define _get_script_data_stack_size(instac) ((int*)instac->code)[10]
block temp_virtual = NULL;
color old_palette[256];
void current_fade_out_effect () {
  if (platform->RunPluginHooks(AGSE_TRANSITIONOUT, 0))
    return;

  // get the screen transition type
  int theTransition = play.fade_effect;
  // was a temporary transition selected? if so, use it
  if (play.next_screen_transition >= 0)
    theTransition = play.next_screen_transition;

  if ((theTransition == FADE_INSTANT) || (play.screen_tint >= 0)) {
    if (!play.keep_screen_during_instant_transition)
      wsetpalette(0,255,black_palette);
  }
  else if (theTransition == FADE_NORMAL)
  {
    my_fade_out(5);
  }
  else if (theTransition == FADE_BOXOUT) 
  {
    gfxDriver->BoxOutEffect(true, get_fixed_pixel_size(16), 1000 / GetGameSpeed());
    play.screen_is_faded_out = 1;
  }
  else 
  {
    get_palette(old_palette);
    temp_virtual = create_bitmap_ex(bitmap_color_depth(abuf),virtual_screen->w,virtual_screen->h);
    //blit(abuf,temp_virtual,0,0,0,0,abuf->w,abuf->h);
    gfxDriver->GetCopyOfScreenIntoBitmap(temp_virtual);
  }
}


void save_room_data_segment () {
  if (croom->tsdatasize > 0)
    free(croom->tsdata);
  croom->tsdata = NULL;
  croom->tsdatasize = roominst->globaldatasize;
  if (croom->tsdatasize > 0) {
    croom->tsdata=(char*)malloc(croom->tsdatasize+10);
    ccFlattenGlobalData (roominst);
    memcpy(croom->tsdata,&roominst->globaldata[0],croom->tsdatasize);
    ccUnFlattenGlobalData (roominst);
  }

}

void unload_old_room() {
  int ff;

  // if switching games on restore, don't do this
  if (displayed_room < 0)
    return;

  platform->WriteDebugString("Unloading room %d", displayed_room);

  current_fade_out_effect();

  clear(abuf);
  for (ff=0;ff<croom->numobj;ff++)
    objs[ff].moving = 0;

  if (!play.ambient_sounds_persist) {
    for (ff = 1; ff < MAX_SOUND_CHANNELS; ff++)
      StopAmbientSound(ff);
  }

  cancel_all_scripts();
  numevents = 0;  // cancel any pending room events

  if (roomBackgroundBmp != NULL)
  {
    gfxDriver->DestroyDDB(roomBackgroundBmp);
    roomBackgroundBmp = NULL;
  }

  if (croom==NULL) ;
  else if (roominst!=NULL) {
    save_room_data_segment();
    ccFreeInstance(roominstFork);
    ccFreeInstance(roominst);
    roominstFork = NULL;
    roominst=NULL;
  }
  else croom->tsdatasize=0;
  memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
  play.bg_frame=0;
  play.bg_frame_locked=0;
  play.offsets_locked=0;
  remove_screen_overlay(-1);
  if (raw_saved_screen != NULL) {
    wfreeblock(raw_saved_screen);
    raw_saved_screen = NULL;
  }
  for (ff = 0; ff < MAX_BSCENE; ff++)
    play.raw_modified[ff] = 0;
  for (ff = 0; ff < thisroom.numLocalVars; ff++)
    croom->interactionVariableValues[ff] = thisroom.localvars[ff].value;

  // wipe the character cache when we change rooms
  for (ff = 0; ff < game.numcharacters; ff++) {
    if (charcache[ff].inUse) {
      destroy_bitmap (charcache[ff].image);
      charcache[ff].image = NULL;
      charcache[ff].inUse = 0;
    }
    // ensure that any half-moves (eg. with scaled movement) are stopped
    charextra[ff].xwas = INVALID_X;
  }

  play.swap_portrait_lastchar = -1;

  for (ff = 0; ff < croom->numobj; ff++) {
    // un-export the object's script object
    if (objectScriptObjNames[ff][0] == 0)
      continue;
    
    ccRemoveExternalSymbol(objectScriptObjNames[ff]);
  }

  for (ff = 0; ff < MAX_HOTSPOTS; ff++) {
    if (thisroom.hotspotScriptNames[ff][0] == 0)
      continue;

    ccRemoveExternalSymbol(thisroom.hotspotScriptNames[ff]);
  }

  // clear the object cache
  for (ff = 0; ff < MAX_INIT_SPR; ff++) {
    if (objcache[ff].image != NULL) {
      destroy_bitmap (objcache[ff].image);
      objcache[ff].image = NULL;
    }
  }
  // clear the actsps buffers to save memory, since the
  // objects/characters involved probably aren't on the
  // new screen. this also ensures all cached data is flushed
  for (ff = 0; ff < MAX_INIT_SPR + game.numcharacters; ff++) {
    if (actsps[ff] != NULL)
      destroy_bitmap(actsps[ff]);
    actsps[ff] = NULL;

    if (actspsbmp[ff] != NULL)
      gfxDriver->DestroyDDB(actspsbmp[ff]);
    actspsbmp[ff] = NULL;

    if (actspswb[ff] != NULL)
      destroy_bitmap(actspswb[ff]);
    actspswb[ff] = NULL;

    if (actspswbbmp[ff] != NULL)
      gfxDriver->DestroyDDB(actspswbbmp[ff]);
    actspswbbmp[ff] = NULL;

    actspswbcache[ff].valid = 0;
  }

  // if Hide Player Character was ticked, restore it to visible
  if (play.temporarily_turned_off_character >= 0) {
    game.chars[play.temporarily_turned_off_character].on = 1;
    play.temporarily_turned_off_character = -1;
  }

}

void redo_walkable_areas() {

  // since this is an 8-bit memory bitmap, we can just use direct 
  // memory access
  if ((!is_linear_bitmap(thisroom.walls)) || (bitmap_color_depth(thisroom.walls) != 8))
    quit("Walkable areas bitmap not linear");

  blit(walkareabackup, thisroom.walls, 0, 0, 0, 0, thisroom.walls->w, thisroom.walls->h);

  int hh,ww;
  for (hh=0;hh<walkareabackup->h;hh++) {
    for (ww=0;ww<walkareabackup->w;ww++) {
//      if (play.walkable_areas_on[_getpixel(thisroom.walls,ww,hh)]==0)
      if (play.walkable_areas_on[thisroom.walls->line[hh][ww]]==0)
        _putpixel(thisroom.walls,ww,hh,0);
    }
  }

}

void generate_light_table() {
  int cc;
  if ((game.color_depth == 1) && (color_map == NULL)) {
    // in 256-col mode, check if we need the light table this room
    for (cc=0;cc < MAX_REGIONS;cc++) {
      if (thisroom.regionLightLevel[cc] < 0) {
        create_light_table(&maincoltable,palette,0,0,0,NULL);
        color_map=&maincoltable;
        break;
        }
      }
    }
  }

void SetAreaLightLevel(int area, int brightness) {
  if ((area < 0) || (area > MAX_REGIONS))
    quit("!SetAreaLightLevel: invalid region");
  if (brightness < -100) brightness = -100;
  if (brightness > 100) brightness = 100;
  thisroom.regionLightLevel[area] = brightness;
  // disable RGB tint for this area
  thisroom.regionTintLevel[area] &= ~TINT_IS_ENABLED;
  generate_light_table();
  DEBUG_CONSOLE("Region %d light level set to %d", area, brightness);
}

void Region_SetLightLevel(ScriptRegion *ssr, int brightness) {
  SetAreaLightLevel(ssr->id, brightness);
}

int Region_GetLightLevel(ScriptRegion *ssr) {
  return thisroom.regionLightLevel[ssr->id];
}

void SetRegionTint (int area, int red, int green, int blue, int amount) {
  if ((area < 0) || (area > MAX_REGIONS))
    quit("!SetRegionTint: invalid region");

  if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
      (blue < 0) || (blue > 255)) {
    quit("!SetRegionTint: RGB values must be 0-255");
  }

  // originally the value was passed as 0
  if (amount == 0)
    amount = 100;

  if ((amount < 1) || (amount > 100))
    quit("!SetRegionTint: amount must be 1-100");

  DEBUG_CONSOLE("Region %d tint set to %d,%d,%d", area, red, green, blue);

  /*red -= 100;
  green -= 100;
  blue -= 100;*/

  unsigned char rred = red;
  unsigned char rgreen = green;
  unsigned char rblue = blue;

  thisroom.regionTintLevel[area] = TINT_IS_ENABLED;
  thisroom.regionTintLevel[area] |= rred & 0x000000ff;
  thisroom.regionTintLevel[area] |= (int(rgreen) << 8) & 0x0000ff00;
  thisroom.regionTintLevel[area] |= (int(rblue) << 16) & 0x00ff0000;
  thisroom.regionLightLevel[area] = amount;
}

int Region_GetTintEnabled(ScriptRegion *srr) {
  if (thisroom.regionTintLevel[srr->id] & TINT_IS_ENABLED)
    return 1;
  return 0;
}

int Region_GetTintRed(ScriptRegion *srr) {
  
  return thisroom.regionTintLevel[srr->id] & 0x000000ff;
}

int Region_GetTintGreen(ScriptRegion *srr) {
  
  return (thisroom.regionTintLevel[srr->id] >> 8) & 0x000000ff;
}

int Region_GetTintBlue(ScriptRegion *srr) {
  
  return (thisroom.regionTintLevel[srr->id] >> 16) & 0x000000ff;
}

int Region_GetTintSaturation(ScriptRegion *srr) {
  
  return thisroom.regionLightLevel[srr->id];
}

void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount) {
  SetRegionTint(srr->id, red, green, blue, amount);
}

int is_valid_character(int newchar) {
  if ((newchar < 0) || (newchar >= game.numcharacters)) return 0;
  return 1;
}


// runs the global script on_event fnuction
void run_on_event (int evtype, int wparam) {
  if (inside_script) {
    curscript->run_another("#on_event", evtype, wparam);
  }
  else
    run_text_script_2iparam(gameinst,"on_event", evtype, wparam);
}

int GetBaseWidth () {
  return BASEWIDTH;
}

void HideMouseCursor () {
  play.mouse_cursor_hidden = 1;
}

void ShowMouseCursor () {
  play.mouse_cursor_hidden = 0;
}

// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetVisible(int isOn) {
  if (isOn)
    ShowMouseCursor();
  else
    HideMouseCursor();
}

int Mouse_GetVisible() {
  if (play.mouse_cursor_hidden)
    return 0;
  return 1;
}

#define MOUSE_MAX_Y divide_down_coordinate(vesa_yres)
void SetMouseBounds (int x1, int y1, int x2, int y2) {
  if ((x1 == 0) && (y1 == 0) && (x2 == 0) && (y2 == 0)) {
    x2 = BASEWIDTH-1;
    y2 = MOUSE_MAX_Y - 1;
  }
  if (x2 == BASEWIDTH) x2 = BASEWIDTH-1;
  if (y2 == MOUSE_MAX_Y) y2 = MOUSE_MAX_Y - 1;
  if ((x1 > x2) || (y1 > y2) || (x1 < 0) || (x2 >= BASEWIDTH) ||
      (y1 < 0) || (y2 >= MOUSE_MAX_Y))
    quit("!SetMouseBounds: invalid co-ordinates, must be within (0,0) - (320,200)");
  DEBUG_CONSOLE("Mouse bounds constrained to (%d,%d)-(%d,%d)", x1, y1, x2, y2);
  multiply_up_coordinates(&x1, &y1);
  multiply_up_coordinates_round_up(&x2, &y2);
 
  play.mboundx1 = x1;
  play.mboundx2 = x2;
  play.mboundy1 = y1;
  play.mboundy2 = y2;
  filter->SetMouseLimit(x1,y1,x2,y2);
}

void update_walk_behind_images()
{
  int ee, rr;
  int bpp = (bitmap_color_depth(thisroom.ebscene[play.bg_frame]) + 7) / 8;
  BITMAP *wbbmp;
  for (ee = 1; ee < MAX_OBJ; ee++)
  {
    update_polled_stuff();
    
    if (walkBehindRight[ee] > 0)
    {
      wbbmp = create_bitmap_ex(bitmap_color_depth(thisroom.ebscene[play.bg_frame]), 
                               (walkBehindRight[ee] - walkBehindLeft[ee]) + 1,
                               (walkBehindBottom[ee] - walkBehindTop[ee]) + 1);
      clear_to_color(wbbmp, bitmap_mask_color(wbbmp));
      int yy, startX = walkBehindLeft[ee], startY = walkBehindTop[ee];
      for (rr = startX; rr <= walkBehindRight[ee]; rr++)
      {
        for (yy = startY; yy <= walkBehindBottom[ee]; yy++)
        {
          if (thisroom.object->line[yy][rr] == ee)
          {
            for (int ii = 0; ii < bpp; ii++)
              wbbmp->line[yy - startY][(rr - startX) * bpp + ii] = thisroom.ebscene[play.bg_frame]->line[yy][rr * bpp + ii];
          }
        }
      }

      update_polled_stuff();

      if (walkBehindBitmap[ee] != NULL)
      {
        gfxDriver->DestroyDDB(walkBehindBitmap[ee]);
      }
      walkBehindBitmap[ee] = gfxDriver->CreateDDBFromBitmap(wbbmp, false);
      destroy_bitmap(wbbmp);
    }
  }

  walkBehindsCachedForBgNum = play.bg_frame;
}

void recache_walk_behinds () {
  if (walkBehindExists) {
    free (walkBehindExists);
    free (walkBehindStartY);
    free (walkBehindEndY);
  }

  walkBehindExists = (char*)malloc (thisroom.object->w);
  walkBehindStartY = (int*)malloc (thisroom.object->w * sizeof(int));
  walkBehindEndY = (int*)malloc (thisroom.object->w * sizeof(int));
  noWalkBehindsAtAll = 1;

  int ee,rr,tmm;
  const int NO_WALK_BEHIND = 100000;
  for (ee = 0; ee < MAX_OBJ; ee++)
  {
    walkBehindLeft[ee] = NO_WALK_BEHIND;
    walkBehindTop[ee] = NO_WALK_BEHIND;
    walkBehindRight[ee] = 0;
    walkBehindBottom[ee] = 0;

    if (walkBehindBitmap[ee] != NULL)
    {
      gfxDriver->DestroyDDB(walkBehindBitmap[ee]);
      walkBehindBitmap[ee] = NULL;
    }
  }

  update_polled_stuff();

  // since this is an 8-bit memory bitmap, we can just use direct 
  // memory access
  if ((!is_linear_bitmap(thisroom.object)) || (bitmap_color_depth(thisroom.object) != 8))
    quit("Walk behinds bitmap not linear");

  for (ee=0;ee<thisroom.object->w;ee++) {
    walkBehindExists[ee] = 0;
    for (rr=0;rr<thisroom.object->h;rr++) {
      tmm = thisroom.object->line[rr][ee];
      //tmm = _getpixel(thisroom.object,ee,rr);
      if ((tmm >= 1) && (tmm < MAX_OBJ)) {
        if (!walkBehindExists[ee]) {
          walkBehindStartY[ee] = rr;
          walkBehindExists[ee] = tmm;
          noWalkBehindsAtAll = 0;
        }
        walkBehindEndY[ee] = rr + 1;  // +1 to allow bottom line of screen to work

        if (ee < walkBehindLeft[tmm]) walkBehindLeft[tmm] = ee;
        if (rr < walkBehindTop[tmm]) walkBehindTop[tmm] = rr;
        if (ee > walkBehindRight[tmm]) walkBehindRight[tmm] = ee;
        if (rr > walkBehindBottom[tmm]) walkBehindBottom[tmm] = rr;
      }
    }
  }

  if (walkBehindMethod == DrawAsSeparateSprite)
  {
    update_walk_behind_images();
  }
}

void check_viewport_coords() 
{
  if (offsetx<0) offsetx=0;
  if (offsety<0) offsety=0;

  int roomWidth = multiply_up_coordinate(thisroom.width);
  int roomHeight = multiply_up_coordinate(thisroom.height);
  if (offsetx + scrnwid > roomWidth)
    offsetx = roomWidth - scrnwid;
  if (offsety + scrnhit > roomHeight)
    offsety = roomHeight - scrnhit;
}

void update_viewport()
{
  if ((thisroom.width > BASEWIDTH) || (thisroom.height > BASEHEIGHT)) {
    if (play.offsets_locked == 0) {
      offsetx = multiply_up_coordinate(playerchar->x) - scrnwid/2;
      offsety = multiply_up_coordinate(playerchar->y) - scrnhit/2;
    }
    check_viewport_coords();
  }
  else {
    offsetx=0;
    offsety=0;
  }
}

int get_walkable_area_pixel(int x, int y)
{
  return getpixel(thisroom.walls, convert_to_low_res(x), convert_to_low_res(y));
}

void convert_room_coordinates_to_low_res(roomstruct *rstruc)
{
    int f;
	  for (f = 0; f < rstruc->numsprs; f++)
	  {
		  rstruc->sprs[f].x /= 2;
		  rstruc->sprs[f].y /= 2;
      if (rstruc->objbaseline[f] > 0)
		  {
			  rstruc->objbaseline[f] /= 2;
		  }
	  }

	  for (f = 0; f < rstruc->numhotspots; f++)
	  {
		  rstruc->hswalkto[f].x /= 2;
		  rstruc->hswalkto[f].y /= 2;
	  }

	  for (f = 0; f < rstruc->numobj; f++)
	  {
		  rstruc->objyval[f] /= 2;
	  }

	  rstruc->left /= 2;
	  rstruc->top /= 2;
	  rstruc->bottom /= 2;
	  rstruc->right /= 2;
	  rstruc->width /= 2;
	  rstruc->height /= 2;
}

#define NO_GAME_ID_IN_ROOM_FILE 16325
// forchar = playerchar on NewRoom, or NULL if restore saved game
void load_new_room(int newnum,CharacterInfo*forchar) {

  platform->WriteDebugString("Loading room %d", newnum);

  char rmfile[20];
  int cc;
  done_es_error = 0;
  play.room_changes ++;
  set_color_depth(8);
  displayed_room=newnum;

  sprintf(rmfile,"room%d.crm",newnum);
  if (newnum == 0) {
    // support both room0.crm and intro.crm
    FILE *inpu = clibfopen(rmfile, "rb");
    if (inpu == NULL)
      strcpy(rmfile, "intro.crm");
    else
      fclose(inpu);
  }
  // reset these back, because they might have been changed.
  if (thisroom.object!=NULL)
    destroy_bitmap(thisroom.object);
  thisroom.object=create_bitmap(320,200);

  if (thisroom.ebscene[0]!=NULL)
    destroy_bitmap(thisroom.ebscene[0]);
  thisroom.ebscene[0] = create_bitmap(320,200);

  update_polled_stuff();

  // load the room from disk
  our_eip=200;
  thisroom.gameId = NO_GAME_ID_IN_ROOM_FILE;
  load_room(rmfile, &thisroom, (game.default_resolution > 2));

  if ((thisroom.gameId != NO_GAME_ID_IN_ROOM_FILE) &&
      (thisroom.gameId != game.uniqueid)) {
    quitprintf("!Unable to load '%s'. This room file is assigned to a different game.", rmfile);
  }

  if ((game.default_resolution > 2) && (game.options[OPT_NATIVECOORDINATES] == 0))
  {
    convert_room_coordinates_to_low_res(&thisroom);
  }

  update_polled_stuff();
  our_eip=201;
/*  // apparently, doing this stops volume spiking between tracks
  if (thisroom.options[ST_TUNE]>0) {
    stopmusic();
    delay(100);
  }*/

  play.room_width = thisroom.width;
  play.room_height = thisroom.height;
  play.anim_background_speed = thisroom.bscene_anim_speed;
  play.bg_anim_delay = play.anim_background_speed;

  int dd;
  // do the palette
  for (cc=0;cc<256;cc++) {
    if (game.paluses[cc]==PAL_BACKGROUND)
      palette[cc]=thisroom.pal[cc];
    else {
      // copy the gamewide colours into the room palette
      for (dd = 0; dd < thisroom.num_bscenes; dd++)
        thisroom.bpalettes[dd][cc] = palette[cc];
    }
  }

  if ((bitmap_color_depth(thisroom.ebscene[0]) == 8) &&
      (final_col_dep > 8))
    select_palette(palette);

  for (cc=0;cc<thisroom.num_bscenes;cc++) {
    update_polled_stuff();
  #ifdef USE_15BIT_FIX
    // convert down scenes from 16 to 15-bit if necessary
    if ((final_col_dep != game.color_depth*8) &&
        (bitmap_color_depth(thisroom.ebscene[cc]) == game.color_depth * 8)) {
      block oldblock = thisroom.ebscene[cc];
      thisroom.ebscene[cc] = convert_16_to_15(oldblock);
      wfreeblock(oldblock);
    }
    else if ((bitmap_color_depth (thisroom.ebscene[cc]) == 16) && (convert_16bit_bgr == 1))
      thisroom.ebscene[cc] = convert_16_to_16bgr (thisroom.ebscene[cc]);
  #endif

  // PSP: Convert 32 bit backgrounds.
  if (bitmap_color_depth(thisroom.ebscene[cc]) == 32)
    thisroom.ebscene[cc] = convert_32_to_32bgr(thisroom.ebscene[cc]);

    thisroom.ebscene[cc] = gfxDriver->ConvertBitmapToSupportedColourDepth(thisroom.ebscene[cc]);
  }

  if ((bitmap_color_depth(thisroom.ebscene[0]) == 8) &&
      (final_col_dep > 8))
    unselect_palette();

  update_polled_stuff();

  our_eip=202;
  if (usetup.want_letterbox) {
    int abscreen=0;
    if (abuf==screen) abscreen=1;
    else if (abuf==virtual_screen) abscreen=2;
    // if this is a 640x480 room and we're in letterbox mode, full-screen it
    int newScreenHeight = final_scrn_hit;
    if (multiply_up_coordinate(thisroom.height) < final_scrn_hit) {
      clear_letterbox_borders();
      newScreenHeight = get_fixed_pixel_size(200);
    }

    if (newScreenHeight == _sub_screen->h)
    {
      screen = _sub_screen;
    }
    else if (_sub_screen->w != final_scrn_wid)
    {
      int subBitmapWidth = _sub_screen->w;
      destroy_bitmap(_sub_screen);
      _sub_screen = create_sub_bitmap(_old_screen, _old_screen->w / 2 - subBitmapWidth / 2, _old_screen->h / 2 - newScreenHeight / 2, subBitmapWidth, newScreenHeight);
      screen = _sub_screen;
    }
    else
    {
      screen = _old_screen;
    }

    scrnhit = screen->h;
    vesa_yres = scrnhit;

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
    filter->SetMouseArea(0,0, scrnwid-1, vesa_yres-1);
#endif

    if (virtual_screen->h != scrnhit) {
      int cdepth=bitmap_color_depth(virtual_screen);
      wfreeblock(virtual_screen);
      virtual_screen=create_bitmap_ex(cdepth,scrnwid,scrnhit);
      clear(virtual_screen);
      gfxDriver->SetMemoryBackBuffer(virtual_screen);
//      ignore_mouseoff_bitmap = virtual_screen;
    }

    gfxDriver->SetRenderOffset(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));

    if (abscreen==1) abuf=screen;
    else if (abscreen==2) abuf=virtual_screen;

    update_polled_stuff();
  }
  // update the script viewport height
  scsystem.viewport_height = divide_down_coordinate(scrnhit);

  SetMouseBounds (0,0,0,0);

  our_eip=203;
  in_new_room=1;

  // walkable_areas_temp is used by the pathfinder to generate a
  // copy of the walkable areas - allocate it here to save time later
  if (walkable_areas_temp != NULL)
    wfreeblock (walkable_areas_temp);
  walkable_areas_temp = create_bitmap_ex (8, thisroom.walls->w, thisroom.walls->h);

  // Make a backup copy of the walkable areas prior to
  // any RemoveWalkableArea commands
  if (walkareabackup!=NULL) wfreeblock(walkareabackup);
  walkareabackup=create_bitmap(thisroom.walls->w,thisroom.walls->h);

  our_eip=204;
  // copy the walls screen
  blit(thisroom.walls,walkareabackup,0,0,0,0,thisroom.walls->w,thisroom.walls->h);
  update_polled_stuff();
  redo_walkable_areas();
  // fix walk-behinds to current screen resolution
  thisroom.object = fix_bitmap_size(thisroom.object);
  update_polled_stuff();

  set_color_depth(final_col_dep);
  // convert backgrounds to current res
  if (thisroom.resolution != get_fixed_pixel_size(1)) {
    for (cc=0;cc<thisroom.num_bscenes;cc++)
      thisroom.ebscene[cc] = fix_bitmap_size(thisroom.ebscene[cc]);
  }

  if ((thisroom.ebscene[0]->w < scrnwid) ||
      (thisroom.ebscene[0]->h < scrnhit))
  {
    quitprintf("!The background scene for this room is smaller than the game resolution. If you have recently changed " 
               "the game resolution, you will need to re-import the background for this room. (Room: %d, BG Size: %d x %d)",
               newnum, thisroom.ebscene[0]->w, thisroom.ebscene[0]->h);
  }

  recache_walk_behinds();

  our_eip=205;
  // setup objects
  if (forchar != NULL) {
    // if not restoring a game, always reset this room
    troom.beenhere=0;  
    troom.tsdatasize=0;
    memset(&troom.hotspot_enabled[0],1,MAX_HOTSPOTS);
    memset(&troom.region_enabled[0], 1, MAX_REGIONS);
  }
  if ((newnum>=0) & (newnum<MAX_ROOMS))
    croom=&roomstats[newnum];
  else croom=&troom;

  if (croom->beenhere > 0) {
    // if we've been here before, save the Times Run information
    // since we will overwrite the actual NewInteraction structs
    // (cos they have pointers and this might have been loaded from
    // a save game)
    if (thisroom.roomScripts == NULL)
    {
      thisroom.intrRoom->copy_timesrun_from (&croom->intrRoom);
      for (cc=0;cc < MAX_HOTSPOTS;cc++)
        thisroom.intrHotspot[cc]->copy_timesrun_from (&croom->intrHotspot[cc]);
      for (cc=0;cc < MAX_INIT_SPR;cc++)
        thisroom.intrObject[cc]->copy_timesrun_from (&croom->intrObject[cc]);
      for (cc=0;cc < MAX_REGIONS;cc++)
        thisroom.intrRegion[cc]->copy_timesrun_from (&croom->intrRegion[cc]);
    }
  }
  if (croom->beenhere==0) {
    croom->numobj=thisroom.numsprs;
    croom->tsdatasize=0;
    for (cc=0;cc<croom->numobj;cc++) {
      croom->obj[cc].x=thisroom.sprs[cc].x;
      croom->obj[cc].y=thisroom.sprs[cc].y;

      if (thisroom.wasversion <= 26)
        croom->obj[cc].y += divide_down_coordinate(spriteheight[thisroom.sprs[cc].sprnum]);

      croom->obj[cc].num=thisroom.sprs[cc].sprnum;
      croom->obj[cc].on=thisroom.sprs[cc].on;
      croom->obj[cc].view=-1;
      croom->obj[cc].loop=0;
      croom->obj[cc].frame=0;
      croom->obj[cc].wait=0;
      croom->obj[cc].transparent=0;
      croom->obj[cc].moving=-1;
      croom->obj[cc].flags = thisroom.objectFlags[cc];
      croom->obj[cc].baseline=-1;
      croom->obj[cc].last_zoom = 100;
      croom->obj[cc].last_width = 0;
      croom->obj[cc].last_height = 0;
      croom->obj[cc].blocking_width = 0;
      croom->obj[cc].blocking_height = 0;
      if (thisroom.objbaseline[cc]>=0)
//        croom->obj[cc].baseoffs=thisroom.objbaseline[cc]-thisroom.sprs[cc].y;
        croom->obj[cc].baseline=thisroom.objbaseline[cc];
    }
    memcpy(&croom->walkbehind_base[0],&thisroom.objyval[0],sizeof(short)*MAX_OBJ);
    for (cc=0;cc<MAX_FLAGS;cc++) croom->flagstates[cc]=0;

/*    // we copy these structs for the Score column to work
    croom->misccond=thisroom.misccond;
    for (cc=0;cc<MAX_HOTSPOTS;cc++)
      croom->hscond[cc]=thisroom.hscond[cc];
    for (cc=0;cc<MAX_INIT_SPR;cc++)
      croom->objcond[cc]=thisroom.objcond[cc];*/

    for (cc=0;cc < MAX_HOTSPOTS;cc++) {
      croom->hotspot_enabled[cc] = 1;
    }
    for (cc = 0; cc < MAX_REGIONS; cc++) {
      croom->region_enabled[cc] = 1;
    }
    croom->beenhere=1;
    in_new_room=2;
  }
  else {
    // We have been here before
    for (ff = 0; ff < thisroom.numLocalVars; ff++)
      thisroom.localvars[ff].value = croom->interactionVariableValues[ff];
  }

  update_polled_stuff();

  if (thisroom.roomScripts == NULL)
  {
    // copy interactions from room file into our temporary struct
    croom->intrRoom = thisroom.intrRoom[0];
    for (cc=0;cc<MAX_HOTSPOTS;cc++)
      croom->intrHotspot[cc] = thisroom.intrHotspot[cc][0];
    for (cc=0;cc<MAX_INIT_SPR;cc++)
      croom->intrObject[cc] = thisroom.intrObject[cc][0];
    for (cc=0;cc<MAX_REGIONS;cc++)
      croom->intrRegion[cc] = thisroom.intrRegion[cc][0];
  }

  objs=&croom->obj[0];

  for (cc = 0; cc < MAX_INIT_SPR; cc++) {
    scrObj[cc].obj = &croom->obj[cc];
    objectScriptObjNames[cc][0] = 0;
  }

  for (cc = 0; cc < croom->numobj; cc++) {
    // export the object's script object
    if (thisroom.objectscriptnames[cc][0] == 0)
      continue;
    
    if (thisroom.wasversion >= 26) 
    {
      strcpy(objectScriptObjNames[cc], thisroom.objectscriptnames[cc]);
    }
    else
    {
      sprintf(objectScriptObjNames[cc], "o%s", thisroom.objectscriptnames[cc]);
      strlwr(objectScriptObjNames[cc]);
      if (objectScriptObjNames[cc][1] != 0)
        objectScriptObjNames[cc][1] = toupper(objectScriptObjNames[cc][1]);
    }

    ccAddExternalSymbol(objectScriptObjNames[cc], &scrObj[cc]);
  }

  for (cc = 0; cc < MAX_HOTSPOTS; cc++) {
    if (thisroom.hotspotScriptNames[cc][0] == 0)
      continue;

    ccAddExternalSymbol(thisroom.hotspotScriptNames[cc], &scrHotspot[cc]);
  }

  our_eip=206;
/*  THIS IS DONE IN THE EDITOR NOW
  thisroom.ebpalShared[0] = 1;
  for (dd = 1; dd < thisroom.num_bscenes; dd++) {
    if (memcmp (&thisroom.bpalettes[dd][0], &palette[0], sizeof(color) * 256) == 0)
      thisroom.ebpalShared[dd] = 1;
    else
      thisroom.ebpalShared[dd] = 0;
  }
  // only make the first frame shared if the last is
  if (thisroom.ebpalShared[thisroom.num_bscenes - 1] == 0)
    thisroom.ebpalShared[0] = 0;*/

  update_polled_stuff();

  our_eip = 210;
  if (IS_ANTIALIAS_SPRITES) {
    // sometimes the palette has corrupt entries, which crash
    // the create_rgb_table call
    // so, fix them
    for (ff = 0; ff < 256; ff++) {
      if (palette[ff].r > 63)
        palette[ff].r = 63;
      if (palette[ff].g > 63)
        palette[ff].g = 63;
      if (palette[ff].b > 63)
        palette[ff].b = 63;
    }
    create_rgb_table (&rgb_table, palette, NULL);
    rgb_map = &rgb_table;
  }
  our_eip = 211;
  if (forchar!=NULL) {
    // if it's not a Restore Game

    // if a following character is still waiting to come into the
    // previous room, force it out so that the timer resets
    for (ff = 0; ff < game.numcharacters; ff++) {
      if ((game.chars[ff].following >= 0) && (game.chars[ff].room < 0)) {
        if ((game.chars[ff].following == game.playercharacter) &&
            (forchar->prevroom == newnum))
          // the player went back to the previous room, so make sure
          // the following character is still there
          game.chars[ff].room = newnum;
        else
          game.chars[ff].room = game.chars[game.chars[ff].following].room;
      }
    }

    offsetx=0;
    offsety=0;
    forchar->prevroom=forchar->room;
    forchar->room=newnum;
    // only stop moving if it's a new room, not a restore game
    for (cc=0;cc<game.numcharacters;cc++)
      StopMoving(cc);

  }

  update_polled_stuff();

  roominst=NULL;
  if (debug_flags & DBG_NOSCRIPT) ;
  else if (thisroom.compiled_script!=NULL) {
    compile_room_script();
    if (croom->tsdatasize>0) {
      if (croom->tsdatasize != roominst->globaldatasize)
        quit("room script data segment size has changed");
      memcpy(&roominst->globaldata[0],croom->tsdata,croom->tsdatasize);
      ccUnFlattenGlobalData (roominst);
      }
    }
  our_eip=207;
  play.entered_edge = -1;

  if ((new_room_x != SCR_NO_VALUE) && (forchar != NULL))
  {
    forchar->x = new_room_x;
    forchar->y = new_room_y;
  }
  new_room_x = SCR_NO_VALUE;

  if ((new_room_pos>0) & (forchar!=NULL)) {
    if (new_room_pos>=4000) {
      play.entered_edge = 3;
      forchar->y = thisroom.top + get_fixed_pixel_size(1);
      forchar->x=new_room_pos%1000;
      if (forchar->x==0) forchar->x=thisroom.width/2;
      if (forchar->x <= thisroom.left)
        forchar->x = thisroom.left + 3;
      if (forchar->x >= thisroom.right)
        forchar->x = thisroom.right - 3;
      forchar->loop=0;
      }
    else if (new_room_pos>=3000) {
      play.entered_edge = 2;
      forchar->y = thisroom.bottom - get_fixed_pixel_size(1);
      forchar->x=new_room_pos%1000;
      if (forchar->x==0) forchar->x=thisroom.width/2;
      if (forchar->x <= thisroom.left)
        forchar->x = thisroom.left + 3;
      if (forchar->x >= thisroom.right)
        forchar->x = thisroom.right - 3;
      forchar->loop=3;
      }
    else if (new_room_pos>=2000) {
      play.entered_edge = 1;
      forchar->x = thisroom.right - get_fixed_pixel_size(1);
      forchar->y=new_room_pos%1000;
      if (forchar->y==0) forchar->y=thisroom.height/2;
      if (forchar->y <= thisroom.top)
        forchar->y = thisroom.top + 3;
      if (forchar->y >= thisroom.bottom)
        forchar->y = thisroom.bottom - 3;
      forchar->loop=1;
      }
    else if (new_room_pos>=1000) {
      play.entered_edge = 0;
      forchar->x = thisroom.left + get_fixed_pixel_size(1);
      forchar->y=new_room_pos%1000;
      if (forchar->y==0) forchar->y=thisroom.height/2;
      if (forchar->y <= thisroom.top)
        forchar->y = thisroom.top + 3;
      if (forchar->y >= thisroom.bottom)
        forchar->y = thisroom.bottom - 3;
      forchar->loop=2;
      }
    // if starts on un-walkable area
    if (get_walkable_area_pixel(forchar->x, forchar->y) == 0) {
      if (new_room_pos>=3000) { // bottom or top of screen
        int tryleft=forchar->x - 1,tryright=forchar->x + 1;
        while (1) {
          if (get_walkable_area_pixel(tryleft, forchar->y) > 0) {
            forchar->x=tryleft; break; }
          if (get_walkable_area_pixel(tryright, forchar->y) > 0) {
            forchar->x=tryright; break; }
          int nowhere=0;
          if (tryleft>thisroom.left) { tryleft--; nowhere++; }
          if (tryright<thisroom.right) { tryright++; nowhere++; }
          if (nowhere==0) break;  // no place to go, so leave him
          }
        }
      else if (new_room_pos>=1000) { // left or right
        int tryleft=forchar->y - 1,tryright=forchar->y + 1;
        while (1) {
          if (get_walkable_area_pixel(forchar->x, tryleft) > 0) {
            forchar->y=tryleft; break; }
          if (get_walkable_area_pixel(forchar->x, tryright) > 0) {
            forchar->y=tryright; break; }
          int nowhere=0;
          if (tryleft>thisroom.top) { tryleft--; nowhere++; }
          if (tryright<thisroom.bottom) { tryright++; nowhere++; }
          if (nowhere==0) break;  // no place to go, so leave him
          }
        }
      }
    new_room_pos=0;
    }
  if (forchar!=NULL) {
    play.entered_at_x=forchar->x;
    play.entered_at_y=forchar->y;
    if (forchar->x >= thisroom.right)
      play.entered_edge = 1;
    else if (forchar->x <= thisroom.left)
      play.entered_edge = 0;
    else if (forchar->y >= thisroom.bottom)
      play.entered_edge = 2;
    else if (forchar->y <= thisroom.top)
      play.entered_edge = 3;
  }
/*  if ((playerchar->x > thisroom.width) | (playerchar->y > thisroom.height))
    quit("!NewRoomEx: x/y co-ordinates are invalid");*/
  if (thisroom.options[ST_TUNE]>0)
    PlayMusicResetQueue(thisroom.options[ST_TUNE]);

  our_eip=208;
  if (forchar!=NULL) {
    if (thisroom.options[ST_MANDISABLED]==0) { forchar->on=1;
      enable_cursor_mode(0); }
    else {
      forchar->on=0;
      disable_cursor_mode(0);
      // remember which character we turned off, in case they
      // use SetPlyaerChracter within this room (so we re-enable
      // the correct character when leaving the room)
      play.temporarily_turned_off_character = game.playercharacter;
    }
    if (forchar->flags & CHF_FIXVIEW) ;
    else if (thisroom.options[ST_MANVIEW]==0) forchar->view=forchar->defview;
    else forchar->view=thisroom.options[ST_MANVIEW]-1;
    forchar->frame=0;   // make him standing
    }
  color_map = NULL;

  our_eip = 209;
  update_polled_stuff();
  generate_light_table();
  update_music_volume();
  update_viewport();
  our_eip = 212;
  invalidate_screen();
  for (cc=0;cc<croom->numobj;cc++) {
    if (objs[cc].on == 2)
      MergeObject(cc);
    }
  new_room_flags=0;
  play.gscript_timer=-1;  // avoid screw-ups with changing screens
  play.player_on_region = 0;
  // trash any input which they might have done while it was loading
  while (kbhit()) { if (getch()==0) getch(); }
  while (mgetbutton()!=NONE) ;
  // no fade in, so set the palette immediately in case of 256-col sprites
  if (game.color_depth > 1)
    setpal();
  our_eip=220;
  update_polled_stuff();
  DEBUG_CONSOLE("Now in room %d", displayed_room);
  guis_need_update = 1;
  platform->RunPluginHooks(AGSE_ENTERROOM, displayed_room);
//  MoveToWalkableArea(game.playercharacter);
//  MSS_CHECK_ALL_BLOCKS;
  }

char bname[40],bne[40];
char* make_ts_func_name(char*base,int iii,int subd) {
  sprintf(bname,base,iii);
  sprintf(bne,"%s_%c",bname,subd+'a');
  return &bne[0];
}


void run_room_event(int id) {
  evblockbasename="room";
  
  if (thisroom.roomScripts != NULL)
  {
    run_interaction_script(thisroom.roomScripts, id);
  }
  else
  {
    run_interaction_event (&croom->intrRoom, id);
  }
}

// new_room: changes the current room number, and loads the new room from disk
void new_room(int newnum,CharacterInfo*forchar) {
  EndSkippingUntilCharStops();
  
  platform->WriteDebugString("Room change requested to room %d", newnum);

  update_polled_stuff();

  // we are currently running Leaves Screen scripts
  in_leaves_screen = newnum;

  // player leaves screen event
  run_room_event(8);
  // Run the global OnRoomLeave event
  run_on_event (GE_LEAVE_ROOM, displayed_room);

  platform->RunPluginHooks(AGSE_LEAVEROOM, displayed_room);

  // update the new room number if it has been altered by OnLeave scripts
  newnum = in_leaves_screen;
  in_leaves_screen = -1;

  if ((playerchar->following >= 0) &&
      (game.chars[playerchar->following].room != newnum)) {
    // the player character is following another character,
    // who is not in the new room. therefore, abort the follow
    playerchar->following = -1;
  }
  update_polled_stuff();

  // change rooms
  unload_old_room();

  update_polled_stuff();

  load_new_room(newnum,forchar);
}

// animation player start

void main_loop_until(int untilwhat,int udata,int mousestuff) {
  play.disabled_user_interface++;
  guis_need_update = 1;
  // Only change the mouse cursor if it hasn't been specifically changed first
  // (or if it's speech, always change it)
  if (((cur_cursor == cur_mode) || (untilwhat == UNTIL_NOOVERLAY)) &&
      (cur_mode != CURS_WAIT))
    set_mouse_cursor(CURS_WAIT);

  restrict_until=untilwhat;
  user_disabled_data=udata;
  return;
}

// event list functions
void setevent(int evtyp,int ev1,int ev2,int ev3) {
  event[numevents].type=evtyp;
  event[numevents].data1=ev1;
  event[numevents].data2=ev2;
  event[numevents].data3=ev3;
  event[numevents].player=game.playercharacter;
  numevents++;
  if (numevents>=MAXEVENTS) quit("too many events posted");
}

void draw_screen_callback()
{
  construct_virtual_screen(false);

  render_black_borders(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));
}

IDriverDependantBitmap* prepare_screen_for_transition_in()
{
  if (temp_virtual == NULL)
    quit("Crossfade: buffer is null attempting transition");

  temp_virtual = gfxDriver->ConvertBitmapToSupportedColourDepth(temp_virtual);
  if (temp_virtual->h < scrnhit)
  {
    block enlargedBuffer = create_bitmap_ex(bitmap_color_depth(temp_virtual), temp_virtual->w, scrnhit);
    blit(temp_virtual, enlargedBuffer, 0, 0, 0, (scrnhit - temp_virtual->h) / 2, temp_virtual->w, temp_virtual->h);
    destroy_bitmap(temp_virtual);
    temp_virtual = enlargedBuffer;
  }
  else if (temp_virtual->h > scrnhit)
  {
    block clippedBuffer = create_bitmap_ex(bitmap_color_depth(temp_virtual), temp_virtual->w, scrnhit);
    blit(temp_virtual, clippedBuffer, 0, (temp_virtual->h - scrnhit) / 2, 0, 0, temp_virtual->w, temp_virtual->h);
    destroy_bitmap(temp_virtual);
    temp_virtual = clippedBuffer;
  }
  acquire_bitmap(temp_virtual);
  IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(temp_virtual, false);
  return ddb;
}

void process_event(EventHappened*evp) {
  if (evp->type==EV_TEXTSCRIPT) {
    int resl=0; ccError=0;
    if (evp->data2 > -1000) {
      if (inside_script) {
        char nameToExec[50];
        sprintf (nameToExec, "!%s", tsnames[evp->data1]);
        curscript->run_another(nameToExec, evp->data2, 0);
      }
      else
        resl=run_text_script_iparam(gameinst,tsnames[evp->data1],evp->data2);
    }
    else {
      if (inside_script)
        curscript->run_another (tsnames[evp->data1], 0, 0);
      else
        resl=run_text_script(gameinst,tsnames[evp->data1]);
    }
//    Display("relt: %d err:%d",resl,scErrorNo);
  }
  else if (evp->type==EV_NEWROOM) {
    NewRoom(evp->data1);
  }
  else if (evp->type==EV_RUNEVBLOCK) {
    NewInteraction*evpt=NULL;
    InteractionScripts *scriptPtr = NULL;
    char *oldbasename = evblockbasename;
    int   oldblocknum = evblocknum;

    if (evp->data1==EVB_HOTSPOT) {

      if (thisroom.hotspotScripts != NULL)
        scriptPtr = thisroom.hotspotScripts[evp->data2];
      else
        evpt=&croom->intrHotspot[evp->data2];

      evblockbasename="hotspot%d";
      evblocknum=evp->data2;
      //platform->WriteDebugString("Running hotspot interaction for hotspot %d, event %d", evp->data2, evp->data3);
    }
    else if (evp->data1==EVB_ROOM) {

      if (thisroom.roomScripts != NULL)
        scriptPtr = thisroom.roomScripts;
      else
        evpt=&croom->intrRoom;
      
      evblockbasename="room";
      if (evp->data3 == 5) {
        in_enters_screen ++;
        run_on_event (GE_ENTER_ROOM, displayed_room);
        
      }
      //platform->WriteDebugString("Running room interaction, event %d", evp->data3);
    }

    if (scriptPtr != NULL)
    {
      run_interaction_script(scriptPtr, evp->data3);
    }
    else if (evpt != NULL)
    {
      run_interaction_event(evpt,evp->data3);
    }
    else
      quit("process_event: RunEvBlock: unknown evb type");

    evblockbasename = oldbasename;
    evblocknum = oldblocknum;

    if ((evp->data3 == 5) && (evp->data1 == EVB_ROOM))
      in_enters_screen --;
    }
  else if (evp->type==EV_FADEIN) {
    // if they change the transition type before the fadein, make
    // sure the screen doesn't freeze up
    play.screen_is_faded_out = 0;

    // determine the transition style
    int theTransition = play.fade_effect;

    if (play.next_screen_transition >= 0) {
      // a one-off transition was selected, so use it
      theTransition = play.next_screen_transition;
      play.next_screen_transition = -1;
    }

    if (platform->RunPluginHooks(AGSE_TRANSITIONIN, 0))
      return;

    if (play.fast_forward)
      return;
    
    if (((theTransition == FADE_CROSSFADE) || (theTransition == FADE_DISSOLVE)) &&
      (temp_virtual == NULL)) 
    {
      // transition type was not crossfade/dissolve when the screen faded out,
      // but it is now when the screen fades in (Eg. a save game was restored
      // with a different setting). Therefore just fade normally.
      my_fade_out(5);
      theTransition = FADE_NORMAL;
    }

    if ((theTransition == FADE_INSTANT) || (play.screen_tint >= 0))
      wsetpalette(0,255,palette);
    else if (theTransition == FADE_NORMAL)
    {
      if (gfxDriver->UsesMemoryBackBuffer())
        gfxDriver->RenderToBackBuffer();

      my_fade_in(palette,5);
    }
    else if (theTransition == FADE_BOXOUT) 
    {
      if (!gfxDriver->UsesMemoryBackBuffer())
      {
        gfxDriver->BoxOutEffect(false, get_fixed_pixel_size(16), 1000 / GetGameSpeed());
      }
      else
      {
        wsetpalette(0,255,palette);
        gfxDriver->RenderToBackBuffer();
        gfxDriver->SetMemoryBackBuffer(screen);
        clear(screen);
        render_to_screen(screen, 0, 0);

        int boxwid = get_fixed_pixel_size(16);
        int boxhit = multiply_up_coordinate(GetMaxScreenHeight() / 20);
        while (boxwid < screen->w) {
          timerloop = 0;
          boxwid += get_fixed_pixel_size(16);
          boxhit += multiply_up_coordinate(GetMaxScreenHeight() / 20);
          int lxp = scrnwid / 2 - boxwid / 2, lyp = scrnhit / 2 - boxhit / 2;
          gfxDriver->Vsync();
          blit(virtual_screen, screen, lxp, lyp, lxp, lyp,
            boxwid, boxhit);
          render_to_screen(screen, 0, 0);
          UPDATE_MP3
          while (timerloop == 0) ;
        }
        gfxDriver->SetMemoryBackBuffer(virtual_screen);
      }
      play.screen_is_faded_out = 0;
    }
    else if (theTransition == FADE_CROSSFADE) 
    {
      if (game.color_depth == 1)
        quit("!Cannot use crossfade screen transition in 256-colour games");

      IDriverDependantBitmap *ddb = prepare_screen_for_transition_in();
      
      int transparency = 254;

      while (transparency > 0) {
        timerloop=0;
        // do the crossfade
        ddb->SetTransparency(transparency);
        invalidate_screen();
        draw_screen_callback();

        if (transparency > 16)
        {
          // on last frame of fade (where transparency < 16), don't
          // draw the old screen on top
          gfxDriver->DrawSprite(0, -(temp_virtual->h - virtual_screen->h), ddb);
        }
        render_to_screen(screen, 0, 0);
        update_polled_stuff();
        while (timerloop == 0) ;
        transparency -= 16;
      }
      release_bitmap(temp_virtual);
      
      wfreeblock(temp_virtual);
      temp_virtual = NULL;
      wsetpalette(0,255,palette);
      gfxDriver->DestroyDDB(ddb);
    }
    else if (theTransition == FADE_DISSOLVE) {
      int pattern[16]={0,4,14,9,5,11,2,8,10,3,12,7,15,6,13,1};
      int aa,bb,cc,thcol=0;
      color interpal[256];

      IDriverDependantBitmap *ddb = prepare_screen_for_transition_in();
      
      for (aa=0;aa<16;aa++) {
        timerloop=0;
        // merge the palette while dithering
        if (game.color_depth == 1) 
        {
          fade_interpolate(old_palette,palette,interpal,aa*4,0,255);
          wsetpalette(0,255,interpal);
        }
        // do the dissolving
        int maskCol = bitmap_mask_color(temp_virtual);
        for (bb=0;bb<scrnwid;bb+=4) {
          for (cc=0;cc<scrnhit;cc+=4) {
            putpixel(temp_virtual, bb+pattern[aa]/4, cc+pattern[aa]%4, maskCol);
          }
        }
        gfxDriver->UpdateDDBFromBitmap(ddb, temp_virtual, false);
        invalidate_screen();
        draw_screen_callback();
        gfxDriver->DrawSprite(0, -(temp_virtual->h - virtual_screen->h), ddb);
        render_to_screen(screen, 0, 0);
        update_polled_stuff();
        while (timerloop == 0) ;
      }
      release_bitmap(temp_virtual);
      
      wfreeblock(temp_virtual);
      temp_virtual = NULL;
      wsetpalette(0,255,palette);
      gfxDriver->DestroyDDB(ddb);
    }
    
  }
  else if (evp->type==EV_IFACECLICK)
    process_interface_click(evp->data1, evp->data2, evp->data3);
  else quit("process_event: unknown event to process");
}

void runevent_now (int evtyp, int ev1, int ev2, int ev3) {
   EventHappened evh;
   evh.type = evtyp;
   evh.data1 = ev1;
   evh.data2 = ev2;
   evh.data3 = ev3;
   evh.player = game.playercharacter;
   process_event(&evh);
}

int inside_processevent=0;
void processallevents(int numev,EventHappened*evlist) {
  int dd;

  if (inside_processevent)
    return;

  // make a copy of the events - if processing an event includes
  // a blocking function it will continue to the next game loop
  // and wipe out the event pointer we were passed
  EventHappened copyOfList[MAXEVENTS];
  memcpy(&copyOfList[0], &evlist[0], sizeof(EventHappened) * numev);

  int room_was = play.room_changes;

  inside_processevent++;

  for (dd=0;dd<numev;dd++) {

    process_event(&copyOfList[dd]);

    if (room_was != play.room_changes)
      break;  // changed room, so discard other events
  }

  inside_processevent--;
}

void update_events() {
  processallevents(numevents,&event[0]);
  numevents=0;
  }
// end event list functions


void PauseGame() {
  game_paused++;
  DEBUG_CONSOLE("Game paused");
}
void UnPauseGame() {
  if (game_paused > 0)
    game_paused--;
  DEBUG_CONSOLE("Game UnPaused, pause level now %d", game_paused);
}

void update_inv_cursor(int invnum) {

  if ((game.options[OPT_FIXEDINVCURSOR]==0) && (invnum > 0)) {
    int cursorSprite = game.invinfo[invnum].cursorPic;
    game.mcurs[MODE_USE].pic = cursorSprite;
    // all cursor images must be pre-cached
    spriteset.precache(cursorSprite);

    if ((game.invinfo[invnum].hotx > 0) || (game.invinfo[invnum].hoty > 0)) {
      // if the hotspot was set (unfortunately 0,0 isn't a valid co-ord)
      game.mcurs[MODE_USE].hotx=game.invinfo[invnum].hotx;
      game.mcurs[MODE_USE].hoty=game.invinfo[invnum].hoty;
      }
    else {
      game.mcurs[MODE_USE].hotx = spritewidth[cursorSprite] / 2;
      game.mcurs[MODE_USE].hoty = spriteheight[cursorSprite] / 2;
      }
    }
  }

void putpixel_compensate (block onto, int xx,int yy, int col) {
  if ((bitmap_color_depth(onto) == 32) && (col != 0)) {
    // ensure the alpha channel is preserved if it has one
    int alphaval = geta32(getpixel(onto, xx, yy));
    col = makeacol32(getr32(col), getg32(col), getb32(col), alphaval);
  }
  rectfill(onto, xx, yy, xx + get_fixed_pixel_size(1) - 1, yy + get_fixed_pixel_size(1) - 1, col);
}

void update_cached_mouse_cursor() 
{
  if (mouseCursor != NULL)
    gfxDriver->DestroyDDB(mouseCursor);
  mouseCursor = gfxDriver->CreateDDBFromBitmap(mousecurs[0], alpha_blend_cursor != 0);
}

void set_new_cursor_graphic (int spriteslot) {
  mousecurs[0] = spriteset[spriteslot];

  if ((spriteslot < 1) || (mousecurs[0] == NULL))
  {
    if (blank_mouse_cursor == NULL)
    {
      blank_mouse_cursor = create_bitmap_ex(final_col_dep, 1, 1);
      clear_to_color(blank_mouse_cursor, bitmap_mask_color(blank_mouse_cursor));
    }
    mousecurs[0] = blank_mouse_cursor;
  }

  if (game.spriteflags[spriteslot] & SPF_ALPHACHANNEL)
    alpha_blend_cursor = 1;
  else
    alpha_blend_cursor = 0;

  update_cached_mouse_cursor();
}

void draw_sprite_support_alpha(int xpos, int ypos, block image, int slot) {

  if ((game.spriteflags[slot] & SPF_ALPHACHANNEL) && (trans_mode == 0)) 
  {
    set_alpha_blender();
    draw_trans_sprite(abuf, image, xpos, ypos);
  }
  else {
    put_sprite_256(xpos, ypos, image);
  }

}

// mouse cursor functions:
// set_mouse_cursor: changes visual appearance to specified cursor
void set_mouse_cursor(int newcurs) {
  int hotspotx = game.mcurs[newcurs].hotx, hotspoty = game.mcurs[newcurs].hoty;

  set_new_cursor_graphic(game.mcurs[newcurs].pic);
  if (dotted_mouse_cursor) {
    wfreeblock (dotted_mouse_cursor);
    dotted_mouse_cursor = NULL;
  }

  if ((newcurs == MODE_USE) && (game.mcurs[newcurs].pic > 0) &&
      ((game.hotdot > 0) || (game.invhotdotsprite > 0)) ) {
    // If necessary, create a copy of the cursor and put the hotspot
    // dot onto it
    dotted_mouse_cursor = create_bitmap_ex (bitmap_color_depth(mousecurs[0]), mousecurs[0]->w,mousecurs[0]->h);
    blit (mousecurs[0], dotted_mouse_cursor, 0, 0, 0, 0, mousecurs[0]->w, mousecurs[0]->h);

    if (game.invhotdotsprite > 0) {
      block abufWas = abuf;
      abuf = dotted_mouse_cursor;

      draw_sprite_support_alpha(
        hotspotx - spritewidth[game.invhotdotsprite] / 2,
        hotspoty - spriteheight[game.invhotdotsprite] / 2,
        spriteset[game.invhotdotsprite],
        game.invhotdotsprite);

      abuf = abufWas;
    }
    else {
      putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty,
        (bitmap_color_depth(dotted_mouse_cursor) > 8) ? get_col8_lookup (game.hotdot) : game.hotdot);

      if (game.hotdotouter > 0) {
        int outercol = game.hotdotouter;
        if (bitmap_color_depth (dotted_mouse_cursor) > 8)
          outercol = get_col8_lookup(game.hotdotouter);

        putpixel_compensate (dotted_mouse_cursor, hotspotx + get_fixed_pixel_size(1), hotspoty, outercol);
        putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty + get_fixed_pixel_size(1), outercol);
        putpixel_compensate (dotted_mouse_cursor, hotspotx - get_fixed_pixel_size(1), hotspoty, outercol);
        putpixel_compensate (dotted_mouse_cursor, hotspotx, hotspoty - get_fixed_pixel_size(1), outercol);
      }
    }
    mousecurs[0] = dotted_mouse_cursor;
    update_cached_mouse_cursor();
  }
  msethotspot(hotspotx, hotspoty);
  if (newcurs != cur_cursor)
  {
    cur_cursor = newcurs;
    mouse_frame=0;
    mouse_delay=0;
  }
}

void precache_view(int view) 
{
  if (view < 0) 
    return;

  for (int i = 0; i < views[view].numLoops; i++) {
    for (int j = 0; j < views[view].loops[i].numFrames; j++)
      spriteset.precache (views[view].loops[i].frames[j].pic);
  }
}

// set_default_cursor: resets visual appearance to current mode (walk, look, etc)
void set_default_cursor() {
  set_mouse_cursor(cur_mode);
  }

// permanently change cursor graphic
void ChangeCursorGraphic (int curs, int newslot) {
  if ((curs < 0) || (curs >= game.numcursors))
    quit("!ChangeCursorGraphic: invalid mouse cursor");

  if ((curs == MODE_USE) && (game.options[OPT_FIXEDINVCURSOR] == 0))
    debug_log("Mouse.ChangeModeGraphic should not be used on the Inventory cursor when the cursor is linked to the active inventory item");

  game.mcurs[curs].pic = newslot;
  spriteset.precache (newslot);
  if (curs == cur_mode)
    set_mouse_cursor (curs);
}

int Mouse_GetModeGraphic(int curs) {
  if ((curs < 0) || (curs >= game.numcursors))
    quit("!Mouse.GetModeGraphic: invalid mouse cursor");

  return game.mcurs[curs].pic;
}

void ChangeCursorHotspot (int curs, int x, int y) {
  if ((curs < 0) || (curs >= game.numcursors))
    quit("!ChangeCursorHotspot: invalid mouse cursor");
  game.mcurs[curs].hotx = multiply_up_coordinate(x);
  game.mcurs[curs].hoty = multiply_up_coordinate(y);
  if (curs == cur_cursor)
    set_mouse_cursor (cur_cursor);
}

void Mouse_ChangeModeView(int curs, int newview) {
  if ((curs < 0) || (curs >= game.numcursors))
    quit("!Mouse.ChangeModeView: invalid mouse cursor");

  newview--;

  game.mcurs[curs].view = newview;

  if (newview >= 0)
  {
    precache_view(newview);
  }

  if (curs == cur_cursor)
    mouse_delay = 0;  // force update
}

int find_next_enabled_cursor(int startwith) {
  if (startwith >= game.numcursors)
    startwith = 0;
  int testing=startwith;
  do {
    if ((game.mcurs[testing].flags & MCF_DISABLED)==0) {
      // inventory cursor, and they have an active item
      if (testing == MODE_USE) 
      {
        if (playerchar->activeinv > 0)
          break;
      }
      // standard cursor that's not disabled, go with it
      else if (game.mcurs[testing].flags & MCF_STANDARD)
        break;
    }

    testing++;
    if (testing >= game.numcursors) testing=0;
  } while (testing!=startwith);

  if (testing!=startwith)
    set_cursor_mode(testing);

  return testing;
}

void SetNextCursor () {
  set_cursor_mode (find_next_enabled_cursor(cur_mode + 1));
}

// set_cursor_mode: changes mode and appearance
void set_cursor_mode(int newmode) {
  if ((newmode < 0) || (newmode >= game.numcursors))
    quit("!SetCursorMode: invalid cursor mode specified");

  guis_need_update = 1;
  if (game.mcurs[newmode].flags & MCF_DISABLED) {
    find_next_enabled_cursor(newmode);
    return; }
  if (newmode == MODE_USE) {
    if (playerchar->activeinv == -1) {
      find_next_enabled_cursor(0);
      return;
      }
    update_inv_cursor(playerchar->activeinv);
    }
  cur_mode=newmode;
  set_default_cursor();

  DEBUG_CONSOLE("Cursor mode set to %d", newmode);
}

void set_inv_item_cursorpic(int invItemId, int piccy) 
{
  game.invinfo[invItemId].cursorPic = piccy;

  if ((cur_cursor == MODE_USE) && (playerchar->activeinv == invItemId)) 
  {
    update_inv_cursor(invItemId);
    set_mouse_cursor(cur_cursor);
  }
}

void InventoryItem_SetCursorGraphic(ScriptInvItem *iitem, int newSprite) 
{
  set_inv_item_cursorpic(iitem->id, newSprite);
}

int InventoryItem_GetCursorGraphic(ScriptInvItem *iitem) 
{
  return game.invinfo[iitem->id].cursorPic;
}

void set_inv_item_pic(int invi, int piccy) {
  if ((invi < 1) || (invi > game.numinvitems))
    quit("!SetInvItemPic: invalid inventory item specified");

  if (game.invinfo[invi].pic == piccy)
    return;

  if (game.invinfo[invi].pic == game.invinfo[invi].cursorPic)
  {
    // Backwards compatibility -- there didn't used to be a cursorPic,
    // so if they're the same update both.
    set_inv_item_cursorpic(invi, piccy);
  }

  game.invinfo[invi].pic = piccy;
  guis_need_update = 1;
}

void InventoryItem_SetGraphic(ScriptInvItem *iitem, int piccy) {
  set_inv_item_pic(iitem->id, piccy);
}

void SetInvItemName(int invi, const char *newName) {
  if ((invi < 1) || (invi > game.numinvitems))
    quit("!SetInvName: invalid inventory item specified");

  // set the new name, making sure it doesn't overflow the buffer
  strncpy(game.invinfo[invi].name, newName, 25);
  game.invinfo[invi].name[24] = 0;

  // might need to redraw the GUI if it has the inv item name on it
  guis_need_update = 1;
}

void InventoryItem_SetName(ScriptInvItem *scii, const char *newname) {
  SetInvItemName(scii->id, newname);
}

int InventoryItem_GetID(ScriptInvItem *scii) {
  return scii->id;
}

void enable_cursor_mode(int modd) {
  game.mcurs[modd].flags&=~MCF_DISABLED;
  // now search the interfaces for related buttons to re-enable
  int uu,ww;

  for (uu=0;uu<game.numgui;uu++) {
    for (ww=0;ww<guis[uu].numobjs;ww++) {
      if ((guis[uu].objrefptr[ww] >> 16)!=GOBJ_BUTTON) continue;
      GUIButton*gbpt=(GUIButton*)guis[uu].objs[ww];
      if (gbpt->leftclick!=IBACT_SETMODE) continue;
      if (gbpt->lclickdata!=modd) continue;
      gbpt->Enable();
      }
    }
  guis_need_update = 1;
  }

void disable_cursor_mode(int modd) {
  game.mcurs[modd].flags|=MCF_DISABLED;
  // now search the interfaces for related buttons to kill
  int uu,ww;

  for (uu=0;uu<game.numgui;uu++) {
    for (ww=0;ww<guis[uu].numobjs;ww++) {
      if ((guis[uu].objrefptr[ww] >> 16)!=GOBJ_BUTTON) continue;
      GUIButton*gbpt=(GUIButton*)guis[uu].objs[ww];
      if (gbpt->leftclick!=IBACT_SETMODE) continue;
      if (gbpt->lclickdata!=modd) continue;
      gbpt->Disable();
      }
    }
  if (cur_mode==modd) find_next_enabled_cursor(modd);
  guis_need_update = 1;
  }

void remove_popup_interface(int ifacenum) {
  if (ifacepopped != ifacenum) return;
  ifacepopped=-1; UnPauseGame();
  guis[ifacenum].on=0;
  if (mousey<=guis[ifacenum].popupyp)
    filter->SetMousePosition(mousex, guis[ifacenum].popupyp+2);
  if ((!IsInterfaceEnabled()) && (cur_cursor == cur_mode))
    // Only change the mouse cursor if it hasn't been specifically changed first
    set_mouse_cursor(CURS_WAIT);
  else if (IsInterfaceEnabled())
    set_default_cursor();

  if (ifacenum==mouse_on_iface) mouse_on_iface=-1;
  guis_need_update = 1;
  }

void process_interface_click(int ifce, int btn, int mbut) {
  if (btn < 0) {
    // click on GUI background
    run_text_script_2iparam(gameinst, guis[ifce].clickEventHandler, (int)&scrGui[ifce], mbut);
    return;
  }

  int btype=(guis[ifce].objrefptr[btn] >> 16) & 0x000ffff;
  int rtype=0,rdata;
  if (btype==GOBJ_BUTTON) {
    GUIButton*gbuto=(GUIButton*)guis[ifce].objs[btn];
    rtype=gbuto->leftclick;
    rdata=gbuto->lclickdata;
    }
  else if ((btype==GOBJ_SLIDER) || (btype == GOBJ_TEXTBOX) || (btype == GOBJ_LISTBOX))
    rtype = IBACT_SCRIPT;
  else quit("unknown GUI object triggered process_interface");

  if (rtype==0) ;
  else if (rtype==IBACT_SETMODE)
    set_cursor_mode(rdata);
  else if (rtype==IBACT_SCRIPT) {
    GUIObject *theObj = guis[ifce].objs[btn];
    // if the object has a special handler script then run it;
    // otherwise, run interface_click
    if ((theObj->GetNumEvents() > 0) &&
        (theObj->eventHandlers[0][0] != 0) &&
        (ccGetSymbolAddr(gameinst, theObj->eventHandlers[0]) != NULL)) {
      // control-specific event handler
      if (strchr(theObj->GetEventArgs(0), ',') != NULL)
        run_text_script_2iparam(gameinst, theObj->eventHandlers[0], (int)theObj, mbut);
      else
        run_text_script_iparam(gameinst, theObj->eventHandlers[0], (int)theObj);
    }
    else
      run_text_script_2iparam(gameinst,"interface_click",ifce,btn);
  }
}

int offset_over_inv(GUIInv *inv) {

  int mover = mouse_ifacebut_xoffs / multiply_up_coordinate(inv->itemWidth);
  // if it's off the edge of the visible items, ignore
  if (mover >= inv->itemsPerLine)
    return -1;
  mover += (mouse_ifacebut_yoffs / multiply_up_coordinate(inv->itemHeight)) * inv->itemsPerLine;
  if (mover >= inv->itemsPerLine * inv->numLines)
    return -1;

  mover += inv->topIndex;
  if ((mover < 0) || (mover >= charextra[inv->CharToDisplay()].invorder_count))
    return -1;

  return charextra[inv->CharToDisplay()].invorder[mover];
}

void run_event_block_inv(int invNum, int aaa) {
  evblockbasename="inventory%d";
  if (game.invScripts != NULL)
  {
    run_interaction_script(game.invScripts[invNum], aaa);
  }
  else 
  {
    run_interaction_event(game.intrInv[invNum], aaa);
  }

}

void SetActiveInventory(int iit) {

  ScriptInvItem *tosend = NULL;
  if ((iit > 0) && (iit < game.numinvitems))
    tosend = &scrInv[iit];
  else if (iit != -1)
    quitprintf("!SetActiveInventory: invalid inventory number %d", iit);

  Character_SetActiveInventory(playerchar, tosend);
}

int IsGamePaused() {
  if (game_paused>0) return 1;
  return 0;
  }

int IsButtonDown(int which) {
  if ((which < 1) || (which > 3))
    quit("!IsButtonDown: only works with eMouseLeft, eMouseRight, eMouseMiddle");
  if (misbuttondown(which-1))
    return 1;
  return 0;
}

void SetCharacterIdle(int who, int iview, int itime) {
  if (!is_valid_character(who))
    quit("!SetCharacterIdle: Invalid character specified");

  Character_SetIdleView(&game.chars[who], iview, itime);
}

int IsKeyPressed (int keycode) {
#ifdef ALLEGRO_KEYBOARD_HANDLER
  if (keyboard_needs_poll())
    poll_keyboard();
  if (keycode >= 300) {
    // function keys are 12 lower in allegro 4
    if ((keycode>=359) & (keycode<=368)) keycode-=12;
    // F11-F12
    else if ((keycode==433) || (keycode==434)) keycode-=76;
    // left arrow
    else if (keycode==375) keycode=382;
    // right arrow
    else if (keycode==377) keycode=383;
    // up arrow
    else if (keycode==372) keycode=384;
    // down arrow
    else if (keycode==380) keycode=385;
    // numeric keypad
    else if (keycode==379) keycode=338;
    else if (keycode==380) keycode=339;
    else if (keycode==381) keycode=340;
    else if (keycode==375) keycode=341;
    else if (keycode==376) keycode=342;
    else if (keycode==377) keycode=343;
    else if (keycode==371) keycode=344;
    else if (keycode==372) keycode=345;
    else if (keycode==373) keycode=346;
    // insert
    else if (keycode == AGS_KEYCODE_INSERT) keycode = KEY_INSERT + 300;
    // delete
    else if (keycode == AGS_KEYCODE_DELETE) keycode = KEY_DEL + 300;

    // deal with shift/ctrl/alt
    if (keycode == 403) keycode = KEY_LSHIFT;
    else if (keycode == 404) keycode = KEY_RSHIFT;
    else if (keycode == 405) keycode = KEY_LCONTROL;
    else if (keycode == 406) keycode = KEY_RCONTROL;
    else if (keycode == 407) keycode = KEY_ALT;
    else keycode -= 300;

    if (rec_iskeypressed(keycode))
      return 1;
    // deal with numeric pad keys having different codes to arrow keys
    if ((keycode == KEY_LEFT) && (rec_iskeypressed(KEY_4_PAD) != 0))
      return 1;
    if ((keycode == KEY_RIGHT) && (rec_iskeypressed(KEY_6_PAD) != 0))
      return 1;
    if ((keycode == KEY_UP) && (rec_iskeypressed(KEY_8_PAD) != 0))
      return 1;
    if ((keycode == KEY_DOWN) && (rec_iskeypressed(KEY_2_PAD) != 0))
      return 1;
    // PgDn/PgUp are equivalent to 3 and 9 on numeric pad
    if ((keycode == KEY_9_PAD) && (rec_iskeypressed(KEY_PGUP) != 0))
      return 1;
    if ((keycode == KEY_3_PAD) && (rec_iskeypressed(KEY_PGDN) != 0))
      return 1;
    // Home/End are equivalent to 7 and 1
    if ((keycode == KEY_7_PAD) && (rec_iskeypressed(KEY_HOME) != 0))
      return 1;
    if ((keycode == KEY_1_PAD) && (rec_iskeypressed(KEY_END) != 0))
      return 1;
    // insert/delete have numpad equivalents
    if ((keycode == KEY_INSERT) && (rec_iskeypressed(KEY_0_PAD) != 0))
      return 1;
    if ((keycode == KEY_DEL) && (rec_iskeypressed(KEY_DEL_PAD) != 0))
      return 1;

    return 0;
  }
  // convert ascii to scancode
  else if ((keycode >= 'A') && (keycode <= 'Z'))
  {
    keycode = platform->ConvertKeycodeToScanCode(keycode);
  }
  else if ((keycode >= '0') && (keycode <= '9'))
    keycode -= ('0' - KEY_0);
  else if (keycode == 8)
    keycode = KEY_BACKSPACE;
  else if (keycode == 9)
    keycode = KEY_TAB;
  else if (keycode == 13) {
    // check both the main return key and the numeric pad enter
    if (rec_iskeypressed(KEY_ENTER))
      return 1;
    keycode = KEY_ENTER_PAD;
  }
  else if (keycode == ' ')
    keycode = KEY_SPACE;
  else if (keycode == 27)
    keycode = KEY_ESC;
  else if (keycode == '-') {
    // check both the main - key and the numeric pad
    if (rec_iskeypressed(KEY_MINUS))
      return 1;
    keycode = KEY_MINUS_PAD;
  }
  else if (keycode == '+') {
    // check both the main + key and the numeric pad
    if (rec_iskeypressed(KEY_EQUALS))
      return 1;
    keycode = KEY_PLUS_PAD;
  }
  else if (keycode == '/') {
    // check both the main / key and the numeric pad
    if (rec_iskeypressed(KEY_SLASH))
      return 1;
    keycode = KEY_SLASH_PAD;
  }
  else if (keycode == '=')
    keycode = KEY_EQUALS;
  else if (keycode == '[')
    keycode = KEY_OPENBRACE;
  else if (keycode == ']')
    keycode = KEY_CLOSEBRACE;
  else if (keycode == '\\')
    keycode = KEY_BACKSLASH;
  else if (keycode == ';')
    keycode = KEY_SEMICOLON;
  else if (keycode == '\'')
    keycode = KEY_QUOTE;
  else if (keycode == ',')
    keycode = KEY_COMMA;
  else if (keycode == '.')
    keycode = KEY_STOP;
  else {
    DEBUG_CONSOLE("IsKeyPressed: unsupported keycode %d", keycode);
    return 0;
  }

  if (rec_iskeypressed(keycode))
    return 1;
  return 0;
#else
  // old allegro version
  quit("allegro keyboard handler not in use??");
#endif
}

void start_skipping_cutscene () {
  play.fast_forward = 1;
  // if a drop-down icon bar is up, remove it as it will pause the game
  if (ifacepopped>=0)
    remove_popup_interface(ifacepopped);

  // if a text message is currently displayed, remove it
  if (is_text_overlay > 0)
    remove_screen_overlay(OVER_TEXTMSG);

}

void check_skip_cutscene_keypress (int kgn) {

  if ((play.in_cutscene > 0) && (play.in_cutscene != 3)) {
    if ((kgn != 27) && ((play.in_cutscene == 1) || (play.in_cutscene == 5)))
      ;
    else
      start_skipping_cutscene();
  }

}

void RunInventoryInteraction (int iit, int modd) {
  if ((iit < 0) || (iit >= game.numinvitems))
    quit("!RunInventoryInteraction: invalid inventory number");

  evblocknum = iit;
  if (modd == MODE_LOOK)
    run_event_block_inv(iit, 0);
  else if (modd == MODE_HAND)
    run_event_block_inv(iit, 1);
  else if (modd == MODE_USE) {
    play.usedinv = playerchar->activeinv;
    run_event_block_inv(iit, 3);
  }
  else if (modd == MODE_TALK)
    run_event_block_inv(iit, 2);
  else // other click on invnetory
    run_event_block_inv(iit, 4);
}

void InventoryItem_RunInteraction(ScriptInvItem *iitem, int mood) {
  RunInventoryInteraction(iitem->id, mood);
}

// check_controls: checks mouse & keyboard interface
void check_controls() {
  int numevents_was = numevents;
  our_eip = 1007;
  NEXT_ITERATION();

  int aa,mongu=-1;
  // If all GUIs are off, skip the loop
  if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0)) ;
  else {
    // Scan for mouse-y-pos GUIs, and pop one up if appropriate
    // Also work out the mouse-over GUI while we're at it
    int ll;
    for (ll = 0; ll < game.numgui;ll++) {
      aa = play.gui_draw_order[ll];
      if (guis[aa].is_mouse_on_gui()) mongu=aa;

      if (guis[aa].popup!=POPUP_MOUSEY) continue;
      if (is_complete_overlay>0) break;  // interfaces disabled
  //    if (play.disabled_user_interface>0) break;
      if (ifacepopped==aa) continue;
      if (guis[aa].on==-1) continue;
      // Don't allow it to be popped up while skipping cutscene
      if (play.fast_forward) continue;
      
      if (mousey < guis[aa].popupyp) {
        set_mouse_cursor(CURS_ARROW);
        guis[aa].on=1; guis_need_update = 1;
        ifacepopped=aa; PauseGame();
        break;
      }
    }
  }

  mouse_on_iface=mongu;
  if ((ifacepopped>=0) && (mousey>=guis[ifacepopped].y+guis[ifacepopped].hit))
    remove_popup_interface(ifacepopped);

  // check mouse clicks on GUIs
  static int wasbutdown=0,wasongui=0;

  if ((wasbutdown>0) && (misbuttondown(wasbutdown-1))) {
    for (aa=0;aa<guis[wasongui].numobjs;aa++) {
      if (guis[wasongui].objs[aa]->activated<1) continue;
      if (guis[wasongui].get_control_type(aa)!=GOBJ_SLIDER) continue;
      // GUI Slider repeatedly activates while being dragged
      guis[wasongui].objs[aa]->activated=0;
      setevent(EV_IFACECLICK, wasongui, aa, wasbutdown);
      break;
      }
    }
  else if ((wasbutdown>0) && (!misbuttondown(wasbutdown-1))) {
    guis[wasongui].mouse_but_up();
    int whichbut=wasbutdown;
    wasbutdown=0;

    for (aa=0;aa<guis[wasongui].numobjs;aa++) {
      if (guis[wasongui].objs[aa]->activated<1) continue;
      guis[wasongui].objs[aa]->activated=0;
      if (!IsInterfaceEnabled()) break;

      int cttype=guis[wasongui].get_control_type(aa);
      if ((cttype == GOBJ_BUTTON) || (cttype == GOBJ_SLIDER) || (cttype == GOBJ_LISTBOX)) {
        setevent(EV_IFACECLICK, wasongui, aa, whichbut);
      }
      else if (cttype == GOBJ_INVENTORY) {
        mouse_ifacebut_xoffs=mousex-(guis[wasongui].objs[aa]->x)-guis[wasongui].x;
        mouse_ifacebut_yoffs=mousey-(guis[wasongui].objs[aa]->y)-guis[wasongui].y;
        int iit=offset_over_inv((GUIInv*)guis[wasongui].objs[aa]);
        if (iit>=0) {
          evblocknum=iit;
          play.used_inv_on = iit;
          if (game.options[OPT_HANDLEINVCLICKS]) {
            // Let the script handle the click
            // LEFTINV is 5, RIGHTINV is 6
            setevent(EV_TEXTSCRIPT,TS_MCLICK, whichbut + 4);
          }
          else if (whichbut==2)  // right-click is always Look
            run_event_block_inv(iit, 0);
          else if (cur_mode == MODE_HAND)
            SetActiveInventory(iit);
          else
            RunInventoryInteraction (iit, cur_mode);
          evblocknum=-1;
        }
      }
      else quit("clicked on unknown control type");
      if (guis[wasongui].popup==POPUP_MOUSEY)
        remove_popup_interface(wasongui);
      break;
    }

    run_on_event(GE_GUI_MOUSEUP, wasongui);
  }

  aa=mgetbutton();
  if (aa>NONE) {
    if ((play.in_cutscene == 3) || (play.in_cutscene == 4))
      start_skipping_cutscene();
    if ((play.in_cutscene == 5) && (aa == RIGHT))
      start_skipping_cutscene();

    if (play.fast_forward) { }
    else if ((play.wait_counter > 0) && (play.key_skip_wait > 1))
      play.wait_counter = -1;
    else if (is_text_overlay > 0) {
      if (play.cant_skip_speech & SKIP_MOUSECLICK)
        remove_screen_overlay(OVER_TEXTMSG);
    }
    else if (!IsInterfaceEnabled()) ;  // blocking cutscene, ignore mouse
    else if (platform->RunPluginHooks(AGSE_MOUSECLICK, aa+1)) {
      // plugin took the click
      DEBUG_CONSOLE("Plugin handled mouse button %d", aa+1);
    }
    else if (mongu>=0) {
      if (wasbutdown==0) {
        DEBUG_CONSOLE("Mouse click over GUI %d", mongu);
        guis[mongu].mouse_but_down();
        // run GUI click handler if not on any control
        if ((guis[mongu].mousedownon < 0) && (guis[mongu].clickEventHandler[0] != 0))
          setevent(EV_IFACECLICK, mongu, -1, aa + 1);

        run_on_event(GE_GUI_MOUSEDOWN, mongu);
      }
      wasongui=mongu;
      wasbutdown=aa+1;
    }
    else setevent(EV_TEXTSCRIPT,TS_MCLICK,aa+1);
//    else run_text_script_iparam(gameinst,"on_mouse_click",aa+1);
  }
  aa = check_mouse_wheel();
  if (aa < 0)
    setevent (EV_TEXTSCRIPT, TS_MCLICK, 9);
  else if (aa > 0)
    setevent (EV_TEXTSCRIPT, TS_MCLICK, 8);

  // check keypresses
  if (kbhit()) {
    // in case they press the finish-recording button, make sure we know
    int was_playing = play.playback;

    int kgn = getch();
    if (kgn==0) kgn=getch()+300;
//    if (kgn==367) restart_game();
//    if (kgn==2) Display("numover: %d character movesped: %d, animspd: %d",numscreenover,playerchar->walkspeed,playerchar->animspeed);
//    if (kgn==2) CreateTextOverlay(50,60,170,FONT_SPEECH,14,"This is a test screen overlay which shouldn't disappear");
//    if (kgn==2) { Display("Crashing"); strcpy(NULL, NULL); }
//    if (kgn == 2) FaceLocation (game.playercharacter, playerchar->x + 1, playerchar->y);
    //if (kgn == 2) SetCharacterIdle (game.playercharacter, 5, 0);
    //if (kgn == 2) Display("Some for?ign text");
    //if (kgn == 2) do_conversation(5);

    if (kgn == play.replay_hotkey) {
      // start/stop recording
      if (play.recording)
        stop_recording();
      else if ((play.playback) || (was_playing))
        ;  // do nothing (we got the replay of the stop key)
      else
        replay_start_this_time = 1;
    }

    check_skip_cutscene_keypress (kgn);

    if (play.fast_forward) { }
    else if (platform->RunPluginHooks(AGSE_KEYPRESS, kgn)) {
      // plugin took the keypress
      DEBUG_CONSOLE("Keypress code %d taken by plugin", kgn);
    }
    else if ((kgn == '`') && (play.debug_mode > 0)) {
      // debug console
      display_console = !display_console;
    }
    else if ((is_text_overlay > 0) &&
             (play.cant_skip_speech & SKIP_KEYPRESS) &&
             (kgn != 434)) {
      // 434 = F12, allow through for screenshot of text
      // (though atm with one script at a time that won't work)
      // only allow a key to remove the overlay if the icon bar isn't up
      if (IsGamePaused() == 0) {
        // check if it requires a specific keypress
        if ((play.skip_speech_specific_key > 0) &&
          (kgn != play.skip_speech_specific_key)) { }
        else
          remove_screen_overlay(OVER_TEXTMSG);
      }
    }
    else if ((play.wait_counter > 0) && (play.key_skip_wait > 0)) {
      play.wait_counter = -1;
      DEBUG_CONSOLE("Keypress code %d ignored - in Wait", kgn);
    }
    else if ((kgn == 5) && (display_fps == 2)) {
      // if --fps paramter is used, Ctrl+E will max out frame rate
      SetGameSpeed(1000);
      display_fps = 2;
    }
    else if ((kgn == 4) && (play.debug_mode > 0)) {
      // ctrl+D - show info
      char infobuf[900];
      int ff;
	  // MACPORT FIX 9/6/5: added last %s
      sprintf(infobuf,"In room %d %s[Player at %d, %d (view %d, loop %d, frame %d)%s%s%s",
        displayed_room, (noWalkBehindsAtAll ? "(has no walk-behinds)" : ""), playerchar->x,playerchar->y,
        playerchar->view + 1, playerchar->loop,playerchar->frame,
        (IsGamePaused() == 0) ? "" : "[Game paused.",
        (play.ground_level_areas_disabled == 0) ? "" : "[Ground areas disabled.",
        (IsInterfaceEnabled() == 0) ? "[Game in Wait state" : "");
      for (ff=0;ff<croom->numobj;ff++) {
        if (ff >= 8) break; // buffer not big enough for more than 7
        sprintf(&infobuf[strlen(infobuf)],
          "[Object %d: (%d,%d) size (%d x %d) on:%d moving:%s animating:%d slot:%d trnsp:%d clkble:%d",
          ff, objs[ff].x, objs[ff].y,
          (spriteset[objs[ff].num] != NULL) ? spritewidth[objs[ff].num] : 0,
          (spriteset[objs[ff].num] != NULL) ? spriteheight[objs[ff].num] : 0,
          objs[ff].on,
          (objs[ff].moving > 0) ? "yes" : "no", objs[ff].cycling,
          objs[ff].num, objs[ff].transparent,
          ((objs[ff].flags & OBJF_NOINTERACT) != 0) ? 0 : 1 );
      }
      Display(infobuf);
      int chd = game.playercharacter;
      char bigbuffer[STD_BUFFER_SIZE] = "CHARACTERS IN THIS ROOM:[";
      for (ff = 0; ff < game.numcharacters; ff++) {
        if (game.chars[ff].room != displayed_room) continue;
        if (strlen(bigbuffer) > 430) {
          strcat(bigbuffer, "and more...");
          Display(bigbuffer);
          strcpy(bigbuffer, "CHARACTERS IN THIS ROOM (cont'd):[");
        }
        chd = ff;
        sprintf(&bigbuffer[strlen(bigbuffer)], 
          "%s (view/loop/frm:%d,%d,%d  x/y/z:%d,%d,%d  idleview:%d,time:%d,left:%d walk:%d anim:%d follow:%d flags:%X wait:%d zoom:%d)[",
          game.chars[chd].scrname, game.chars[chd].view+1, game.chars[chd].loop, game.chars[chd].frame,
          game.chars[chd].x, game.chars[chd].y, game.chars[chd].z,
          game.chars[chd].idleview, game.chars[chd].idletime, game.chars[chd].idleleft,
          game.chars[chd].walking, game.chars[chd].animating, game.chars[chd].following,
          game.chars[chd].flags, game.chars[chd].wait, charextra[chd].zoom);
      }
      Display(bigbuffer);

    }
/*    else if (kgn == 21) {
      play.debug_mode++;
      script_debug(5,0);
      play.debug_mode--;
    }*/
    else if ((kgn == 22) && (play.wait_counter < 1) && (is_text_overlay == 0) && (restrict_until == 0)) {
      // make sure we can't interrupt a Wait()
      // and desync the music to cutscene
      play.debug_mode++;
      script_debug (1,0);
      play.debug_mode--;
    }
    else if (inside_script) {
      // Don't queue up another keypress if it can't be run instantly
      DEBUG_CONSOLE("Keypress %d ignored (game blocked)", kgn);
    }
    else {
      int keywasprocessed = 0;
      // determine if a GUI Text Box should steal the click
      // it should do if a displayable character (32-255) is
      // pressed, but exclude control characters (<32) and
      // extended keys (eg. up/down arrow; 256+)
      if ( ((kgn >= 32) && (kgn != '[') && (kgn < 256)) || (kgn == 13) || (kgn == 8) ) {
        int uu,ww;
        for (uu=0;uu<game.numgui;uu++) {
          if (guis[uu].on < 1) continue;
          for (ww=0;ww<guis[uu].numobjs;ww++) {
            // not a text box, ignore it
            if ((guis[uu].objrefptr[ww] >> 16)!=GOBJ_TEXTBOX)
              continue;
            GUITextBox*guitex=(GUITextBox*)guis[uu].objs[ww];
            // if the text box is disabled, it cannot except keypresses
            if ((guitex->IsDisabled()) || (!guitex->IsVisible()))
              continue;
            guitex->KeyPress(kgn);
            if (guitex->activated) {
              guitex->activated = 0;
              setevent(EV_IFACECLICK, uu, ww, 1);
            }
            keywasprocessed = 1;
          }
        }
      }
      if (!keywasprocessed) {
        if ((kgn>='a') & (kgn<='z')) kgn-=32;
        DEBUG_CONSOLE("Running on_key_press keycode %d", kgn);
        setevent(EV_TEXTSCRIPT,TS_KEYPRESS,kgn);
      }
    }
//    run_text_script_iparam(gameinst,"on_key_press",kgn);
  }

  if ((IsInterfaceEnabled()) && (IsGamePaused() == 0) &&
      (in_new_room == 0) && (new_room_was == 0)) {
    // Only allow walking off edges if not in wait mode, and
    // if not in Player Enters Screen (allow walking in from off-screen)
    int edgesActivated[4] = {0, 0, 0, 0};
    // Only do it if nothing else has happened (eg. mouseclick)
    if ((numevents == numevents_was) &&
        ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0)) {

      if (playerchar->x <= thisroom.left)
        edgesActivated[0] = 1;
      else if (playerchar->x >= thisroom.right)
        edgesActivated[1] = 1;
      if (playerchar->y >= thisroom.bottom)
        edgesActivated[2] = 1;
      else if (playerchar->y <= thisroom.top)
        edgesActivated[3] = 1;

      if ((play.entered_edge >= 0) && (play.entered_edge <= 3)) {
        // once the player is no longer outside the edge, forget the stored edge
        if (edgesActivated[play.entered_edge] == 0)
          play.entered_edge = -10;
        // if we are walking in from off-screen, don't activate edges
        else
          edgesActivated[play.entered_edge] = 0;
      }

      for (int ii = 0; ii < 4; ii++) {
        if (edgesActivated[ii])
          setevent(EV_RUNEVBLOCK, EVB_ROOM, 0, ii);
      }
    }
  }
  our_eip = 1008;

}  // end check_controls


int IsChannelPlaying(int chan) {
  if (play.fast_forward)
    return 0;

  if ((chan < 0) || (chan >= MAX_SOUND_CHANNELS))
    quit("!IsChannelPlaying: invalid sound channel");

  if ((channels[chan] != NULL) && (channels[chan]->done == 0))
    return 1;

  return 0;
}

int IsSoundPlaying() {
  if (play.fast_forward)
    return 0;

  // find if there's a sound playing
  for (int i = SCHAN_NORMAL; i < numSoundChannels; i++) {
    if ((channels[i] != NULL) && (channels[i]->done == 0))
      return 1;
  }

  return 0;
}

void SetFrameSound (int vii, int loop, int frame, int sound) {
  if ((vii < 1) || (vii > game.numviews))
    quit("!SetFrameSound: invalid view number");
  vii--;

  if (loop >= views[vii].numLoops)
    quit("!SetFrameSound: invalid loop number");

  if (frame >= views[vii].loops[loop].numFrames)
    quit("!SetFrameSound: invalid frame number");

  if (sound < 1)
  {
    views[vii].loops[loop].frames[frame].sound = -1;
  }
  else
  {
    ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, sound);
    if (clip == NULL)
      quitprintf("!SetFrameSound: audio clip aSound%d not found", sound);

    views[vii].loops[loop].frames[frame].sound = clip->id;
  }
}

// the specified frame has just appeared, see if we need
// to play a sound or whatever
void CheckViewFrame (int view, int loop, int frame) {
  if (views[view].loops[loop].frames[frame].sound >= 0) {
    // play this sound (eg. footstep)
    play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound);
  }
}

void CheckViewFrameForCharacter(CharacterInfo *chi) {

  int soundVolumeWas = play.sound_volume;

  if (chi->flags & CHF_SCALEVOLUME) {
    // adjust the sound volume using the character's zoom level
    int zoom_level = charextra[chi->index_id].zoom;
    if (zoom_level == 0)
      zoom_level = 100;

    play.sound_volume = (play.sound_volume * zoom_level) / 100;

    if (play.sound_volume < 0)
      play.sound_volume = 0;
    if (play.sound_volume > 255)
      play.sound_volume = 255;
  }

  CheckViewFrame(chi->view, chi->loop, chi->frame);

  play.sound_volume = soundVolumeWas;
}

// return the walkable area at the character's feet, taking into account
// that he might just be off the edge of one
int get_walkable_area_at_location(int xx, int yy) {

  int onarea = get_walkable_area_pixel(xx, yy);

  if (onarea < 0) {
    // the character has walked off the edge of the screen, so stop them
    // jumping up to full size when leaving
    if (xx >= thisroom.width)
      onarea = get_walkable_area_pixel(thisroom.width-1, yy);
    else if (xx < 0)
      onarea = get_walkable_area_pixel(0, yy);
    else if (yy >= thisroom.height)
      onarea = get_walkable_area_pixel(xx, thisroom.height - 1);
    else if (yy < 0)
      onarea = get_walkable_area_pixel(xx, 1);
  }
  if (onarea==0) {
    // the path finder sometimes slightly goes into non-walkable areas;
    // so check for scaling in adjacent pixels
    const int TRYGAP=2;
    onarea = get_walkable_area_pixel(xx + TRYGAP, yy);
    if (onarea<=0)
      onarea = get_walkable_area_pixel(xx - TRYGAP, yy);
    if (onarea<=0)
      onarea = get_walkable_area_pixel(xx, yy + TRYGAP);
    if (onarea<=0)
      onarea = get_walkable_area_pixel(xx, yy - TRYGAP);
    if (onarea < 0)
      onarea = 0;
  }

  return onarea;
}

int get_walkable_area_at_character (int charnum) {
  CharacterInfo *chin = &game.chars[charnum];
  return get_walkable_area_at_location(chin->x, chin->y);
}

// Calculate which frame of the loop to use for this character of
// speech
int GetLipSyncFrame (char *curtex, int *stroffs) {
  /*char *frameletters[MAXLIPSYNCFRAMES] =
    {"./,/ ", "A", "O", "F/V", "D/N/G/L/R", "B/P/M",
     "Y/H/K/Q/C", "I/T/E/X/th", "U/W", "S/Z/J/ch", NULL,
     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};*/

  int bestfit_len = 0, bestfit = game.default_lipsync_frame;
  for (int aa = 0; aa < MAXLIPSYNCFRAMES; aa++) {
    char *tptr = game.lipSyncFrameLetters[aa];
    while (tptr[0] != 0) {
      int lenthisbit = strlen(tptr);
      if (strchr(tptr, '/'))
        lenthisbit = strchr(tptr, '/') - tptr;
      
      if ((strnicmp (curtex, tptr, lenthisbit) == 0) && (lenthisbit > bestfit_len)) {
        bestfit = aa;
        bestfit_len = lenthisbit;
      }
      tptr += lenthisbit;
      while (tptr[0] == '/')
        tptr++;
    }
  }
  // If it's an unknown character, use the default frame
  if (bestfit_len == 0)
    bestfit_len = 1;
  *stroffs += bestfit_len;
  return bestfit;
}


int update_lip_sync(int talkview, int talkloop, int *talkframeptr) {
  int talkframe = talkframeptr[0];
  int talkwait = 0;

  // lip-sync speech
  char *nowsaying = &text_lips_text[text_lips_offset];
  // if it's an apostraphe, skip it (we'll, I'll, etc)
  if (nowsaying[0] == '\'') {
    text_lips_offset++;
    nowsaying++;
  }

  if (text_lips_offset >= (int)strlen(text_lips_text))
    talkframe = 0;
  else {
    talkframe = GetLipSyncFrame (nowsaying, &text_lips_offset);
    if (talkframe >= views[talkview].loops[talkloop].numFrames)
      talkframe = 0;
  }

  talkwait = loops_per_character + views[talkview].loops[talkloop].frames[talkframe].speed;

  talkframeptr[0] = talkframe;
  return talkwait;
}


// ** start animating buttons code

// returns 1 if animation finished
int UpdateAnimatingButton(int bu) {
  if (animbuts[bu].wait > 0) {
    animbuts[bu].wait--;
    return 0;
  }
  ViewStruct *tview = &views[animbuts[bu].view];

  animbuts[bu].frame++;

  if (animbuts[bu].frame >= tview->loops[animbuts[bu].loop].numFrames) 
  {
    if (tview->loops[animbuts[bu].loop].RunNextLoop()) {
      // go to next loop
      animbuts[bu].loop++;
      animbuts[bu].frame = 0;
    }
    else if (animbuts[bu].repeat) {
      animbuts[bu].frame = 0;
      // multi-loop anim, go back
      while ((animbuts[bu].loop > 0) && 
             (tview->loops[animbuts[bu].loop - 1].RunNextLoop()))
        animbuts[bu].loop--;
    }
    else
      return 1;
  }

  CheckViewFrame(animbuts[bu].view, animbuts[bu].loop, animbuts[bu].frame);

  // update the button's image
  guibuts[animbuts[bu].buttonid].pic = tview->loops[animbuts[bu].loop].frames[animbuts[bu].frame].pic;
  guibuts[animbuts[bu].buttonid].usepic = guibuts[animbuts[bu].buttonid].pic;
  guibuts[animbuts[bu].buttonid].pushedpic = 0;
  guibuts[animbuts[bu].buttonid].overpic = 0;
  guis_need_update = 1;

  animbuts[bu].wait = animbuts[bu].speed + tview->loops[animbuts[bu].loop].frames[animbuts[bu].frame].speed;
  return 0;
}

void StopButtonAnimation(int idxn) {
  numAnimButs--;
  for (int aa = idxn; aa < numAnimButs; aa++) {
    animbuts[aa] = animbuts[aa + 1];
  }
}

void FindAndRemoveButtonAnimation(int guin, int objn) {

  for (int ii = 0; ii < numAnimButs; ii++) {
    if ((animbuts[ii].ongui == guin) && (animbuts[ii].onguibut == objn)) {
      StopButtonAnimation(ii);
      ii--;
    }

  }
}
// ** end animating buttons code

int GetCharacterWidth(int ww) {
  CharacterInfo *char1 = &game.chars[ww];
  
  if (charextra[ww].width < 1)
  {
    if ((char1->view < 0) ||
        (char1->loop >= views[char1->view].numLoops) ||
        (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
    {
      debug_log("GetCharacterWidth: Character %s has invalid frame: view %d, loop %d, frame %d", char1->scrname, char1->view + 1, char1->loop, char1->frame);
      return multiply_up_coordinate(4);
    }

    return spritewidth[views[char1->view].loops[char1->loop].frames[char1->frame].pic];
  }
  else 
    return charextra[ww].width;
}

int GetCharacterHeight(int charid) {
  CharacterInfo *char1 = &game.chars[charid];

  if (charextra[charid].height < 1)
  {
    if ((char1->view < 0) ||
        (char1->loop >= views[char1->view].numLoops) ||
        (char1->frame >= views[char1->view].loops[char1->loop].numFrames))
    {
      debug_log("GetCharacterHeight: Character %s has invalid frame: view %d, loop %d, frame %d", char1->scrname, char1->view + 1, char1->loop, char1->frame);
      return multiply_up_coordinate(2);
    }

    return spriteheight[views[char1->view].loops[char1->loop].frames[char1->frame].pic];
  }
  else
    return charextra[charid].height;
}

void get_char_blocking_rect(int charid, int *x1, int *y1, int *width, int *y2) {
  CharacterInfo *char1 = &game.chars[charid];
  int cwidth, fromx;

  if (char1->blocking_width < 1)
    cwidth = divide_down_coordinate(GetCharacterWidth(charid)) - 4;
  else
    cwidth = char1->blocking_width;

  fromx = char1->x - cwidth/2;
  if (fromx < 0) {
    cwidth += fromx;
    fromx = 0;
  }
  if (fromx + cwidth >= convert_back_to_high_res(walkable_areas_temp->w))
    cwidth = convert_back_to_high_res(walkable_areas_temp->w) - fromx;

  if (x1)
    *x1 = fromx;
  if (width)
    *width = cwidth;
  if (y1)
    *y1 = char1->get_blocking_top();
  if (y2)
    *y2 = char1->get_blocking_bottom();
}

// Check whether the source char has walked onto character ww
int is_char_on_another (int sourceChar, int ww, int*fromxptr, int*cwidptr) {

  int fromx, cwidth;
  int y1, y2;
  get_char_blocking_rect(ww, &fromx, &y1, &cwidth, &y2);

  if (fromxptr)
    fromxptr[0] = fromx;
  if (cwidptr)
    cwidptr[0] = cwidth;

  // if the character trying to move is already on top of
  // this char somehow, allow them through
  if ((sourceChar >= 0) &&
      // x/width are left and width co-ords, so they need >= and <
      (game.chars[sourceChar].x >= fromx) &&
      (game.chars[sourceChar].x < fromx + cwidth) &&
      // y1/y2 are the top/bottom co-ords, so they need >= / <=
      (game.chars[sourceChar].y >= y1 ) &&
      (game.chars[sourceChar].y <= y2 ))
    return 1;
 
  return 0;
}

void get_object_blocking_rect(int objid, int *x1, int *y1, int *width, int *y2) {
  RoomObject *tehobj = &objs[objid];
  int cwidth, fromx;

  if (tehobj->blocking_width < 1)
    cwidth = divide_down_coordinate(tehobj->last_width) - 4;
  else
    cwidth = tehobj->blocking_width;

  fromx = tehobj->x + (divide_down_coordinate(tehobj->last_width) / 2) - cwidth / 2;
  if (fromx < 0) {
    cwidth += fromx;
    fromx = 0;
  }
  if (fromx + cwidth >= convert_back_to_high_res(walkable_areas_temp->w))
    cwidth = convert_back_to_high_res(walkable_areas_temp->w) - fromx;

  if (x1)
    *x1 = fromx;
  if (width)
    *width = cwidth;
  if (y1) {
    if (tehobj->blocking_height > 0)
      *y1 = tehobj->y - tehobj->blocking_height / 2;
    else
      *y1 = tehobj->y - 2;
  }
  if (y2) {
    if (tehobj->blocking_height > 0)
      *y2 = tehobj->y + tehobj->blocking_height / 2;
    else
      *y2 = tehobj->y + 3;
  }
}

int is_point_in_rect(int x, int y, int left, int top, int right, int bottom) {
  if ((x >= left) && (x < right) && (y >= top ) && (y <= bottom))
    return 1;
  return 0;
}


int wantMoveNow (int chnum, CharacterInfo *chi) {
  // check most likely case first
  if ((charextra[chnum].zoom == 100) || ((chi->flags & CHF_SCALEMOVESPEED) == 0))
    return 1;

  // the % checks don't work when the counter is negative, so once
  // it wraps round, correct it
  while (chi->walkwaitcounter < 0) {
    chi->walkwaitcounter += 12000;
  }

  // scaling 170-200%, move 175% speed
  if (charextra[chnum].zoom >= 170) {
    if ((chi->walkwaitcounter % 4) >= 1)
      return 2;
    else
      return 1;
  }
  // scaling 140-170%, move 150% speed
  else if (charextra[chnum].zoom >= 140) {
    if ((chi->walkwaitcounter % 2) == 1)
      return 2;
    else
      return 1;
  }
  // scaling 115-140%, move 125% speed
  else if (charextra[chnum].zoom >= 115) {
    if ((chi->walkwaitcounter % 4) >= 3)
      return 2;
    else
      return 1;
  }
  // scaling 80-120%, normal speed
  else if (charextra[chnum].zoom >= 80)
    return 1;
  // scaling 60-80%, move 75% speed
  if (charextra[chnum].zoom >= 60) {
    if ((chi->walkwaitcounter % 4) >= 1)
      return 1;
  }
  // scaling 30-60%, move 50% speed
  else if (charextra[chnum].zoom >= 30) {
    if ((chi->walkwaitcounter % 2) == 1)
      return -1;
    else if (charextra[chnum].xwas != INVALID_X) {
      // move the second half of the movement to make it smoother
      chi->x = charextra[chnum].xwas;
      chi->y = charextra[chnum].ywas;
      charextra[chnum].xwas = INVALID_X;
    }
  }
  // scaling 0-30%, move 25% speed
  else {
    if ((chi->walkwaitcounter % 4) >= 3)
      return -1;
    if (((chi->walkwaitcounter % 4) == 1) && (charextra[chnum].xwas != INVALID_X)) {
      // move the second half of the movement to make it smoother
      chi->x = charextra[chnum].xwas;
      chi->y = charextra[chnum].ywas;
      charextra[chnum].xwas = INVALID_X;
    }

  }

  return 0;
}

// draws a view frame, flipped if appropriate
void DrawViewFrame(block target, ViewFrame *vframe, int x, int y) {
  if (vframe->flags & VFLG_FLIPSPRITE)
    draw_sprite_h_flip(target, spriteset[vframe->pic], x, y);
  else
    draw_sprite(target, spriteset[vframe->pic], x, y);
}

// update_stuff: moves and animates objects, executes repeat scripts, and
// the like.
void update_stuff() {
  int aa;
  our_eip = 20;

  if (play.gscript_timer > 0) play.gscript_timer--;
  for (aa=0;aa<MAX_TIMERS;aa++) {
    if (play.script_timers[aa] > 1) play.script_timers[aa]--;
    }
  // update graphics for object if cycling view
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on != 1) continue;
    if (objs[aa].moving>0) {
      do_movelist_move(&objs[aa].moving,&objs[aa].x,&objs[aa].y);
      }
    if (objs[aa].cycling==0) continue;
    if (objs[aa].view<0) continue;
    if (objs[aa].wait>0) { objs[aa].wait--; continue; }

    if (objs[aa].cycling >= ANIM_BACKWARDS) {
      // animate backwards
      objs[aa].frame--;
      if (objs[aa].frame < 0) {
        if ((objs[aa].loop > 0) && 
           (views[objs[aa].view].loops[objs[aa].loop - 1].RunNextLoop())) 
        {
          // If it's a Go-to-next-loop on the previous one, then go back
          objs[aa].loop --;
          objs[aa].frame = views[objs[aa].view].loops[objs[aa].loop].numFrames - 1;
        }
        else if (objs[aa].cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it on the first frame
          objs[aa].cycling = 0;
          objs[aa].frame = 0;
        }
        else { // repeating animation
          objs[aa].frame = views[objs[aa].view].loops[objs[aa].loop].numFrames - 1;
        }
      }
    }
    else {  // Animate forwards
      objs[aa].frame++;
      if (objs[aa].frame >= views[objs[aa].view].loops[objs[aa].loop].numFrames) {
        // go to next loop thing
        if (views[objs[aa].view].loops[objs[aa].loop].RunNextLoop()) {
          if (objs[aa].loop+1 >= views[objs[aa].view].numLoops)
            quit("!Last loop in a view requested to move to next loop");
          objs[aa].loop++;
          objs[aa].frame=0;
        }
        else if (objs[aa].cycling % ANIM_BACKWARDS == ANIM_ONCE) {
          // leave it on the last frame
          objs[aa].cycling=0;
          objs[aa].frame--;
          }
        else {
          if (play.no_multiloop_repeat == 0) {
            // multi-loop anims, go back to start of it
            while ((objs[aa].loop > 0) && 
              (views[objs[aa].view].loops[objs[aa].loop - 1].RunNextLoop()))
              objs[aa].loop --;
          }
          if (objs[aa].cycling % ANIM_BACKWARDS == ANIM_ONCERESET)
            objs[aa].cycling=0;
          objs[aa].frame=0;
        }
      }
    }  // end if forwards

    ViewFrame*vfptr=&views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame];
    objs[aa].num = vfptr->pic;

    if (objs[aa].cycling == 0)
      continue;

    objs[aa].wait=vfptr->speed+objs[aa].overall_speed;
    CheckViewFrame (objs[aa].view, objs[aa].loop, objs[aa].frame);
  }
  our_eip = 21;

  // shadow areas
  int onwalkarea = get_walkable_area_at_character (game.playercharacter);
  if (onwalkarea<0) ;
  else if (playerchar->flags & CHF_FIXVIEW) ;
  else { onwalkarea=thisroom.shadinginfo[onwalkarea];
    if (onwalkarea>0) playerchar->view=onwalkarea-1;
    else if (thisroom.options[ST_MANVIEW]==0) playerchar->view=playerchar->defview;
    else playerchar->view=thisroom.options[ST_MANVIEW]-1;
  }
  our_eip = 22;

  #define MAX_SHEEP 30
  int numSheep = 0;
  int followingAsSheep[MAX_SHEEP];

  // move & animate characters
  for (aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on != 1) continue;
    CharacterInfo*chi=&game.chars[aa];
    
    // walking
    if (chi->walking >= TURNING_AROUND) {
      // Currently rotating to correct direction
      if (chi->walkwait > 0) chi->walkwait--;
      else {
        // Work out which direction is next
        int wantloop = find_looporder_index(chi->loop) + 1;
        // going anti-clockwise, take one before instead
        if (chi->walking >= TURNING_BACKWARDS)
          wantloop -= 2;
        while (1) {
          if (wantloop >= 8)
            wantloop = 0;
          if (wantloop < 0)
            wantloop = 7;
          if ((turnlooporder[wantloop] >= views[chi->view].numLoops) ||
              (views[chi->view].loops[turnlooporder[wantloop]].numFrames < 1) ||
              ((turnlooporder[wantloop] >= 4) && ((chi->flags & CHF_NODIAGONAL)!=0))) {
            if (chi->walking >= TURNING_BACKWARDS)
              wantloop--;
            else
              wantloop++;
          }
          else break;
        }
        chi->loop = turnlooporder[wantloop];
        chi->walking -= TURNING_AROUND;
        // if still turning, wait for next frame
        if (chi->walking % TURNING_BACKWARDS >= TURNING_AROUND)
          chi->walkwait = chi->animspeed;
        else
          chi->walking = chi->walking % TURNING_BACKWARDS;
        charextra[aa].animwait = 0;
      }
      continue;
    }
    // Make sure it doesn't flash up a blue cup
    if (chi->view < 0) ;
    else if (chi->loop >= views[chi->view].numLoops)
      chi->loop = 0;

    int doing_nothing = 1;

    if ((chi->walking > 0) && (chi->room == displayed_room))
    {
      if (chi->walkwait > 0) chi->walkwait--;
      else 
      {
        chi->flags &= ~CHF_AWAITINGMOVE;

        // Move the character
        int numSteps = wantMoveNow(aa, chi);

        if ((numSteps) && (charextra[aa].xwas != INVALID_X)) {
          // if the zoom level changed mid-move, the walkcounter
          // might not have come round properly - so sort it out
          chi->x = charextra[aa].xwas;
          chi->y = charextra[aa].ywas;
          charextra[aa].xwas = INVALID_X;
        }

        int oldxp = chi->x, oldyp = chi->y;

        for (int ff = 0; ff < abs(numSteps); ff++) {
          if (doNextCharMoveStep (aa, chi))
            break;
          if ((chi->walking == 0) || (chi->walking >= TURNING_AROUND))
            break;
        }

        if (numSteps < 0) {
          // very small scaling, intersperse the movement
          // to stop it being jumpy
          charextra[aa].xwas = chi->x;
          charextra[aa].ywas = chi->y;
          chi->x = ((chi->x) - oldxp) / 2 + oldxp;
          chi->y = ((chi->y) - oldyp) / 2 + oldyp;
        }
        else if (numSteps > 0)
          charextra[aa].xwas = INVALID_X;

        if ((chi->flags & CHF_ANTIGLIDE) == 0)
          chi->walkwaitcounter++;
      }

      if (chi->loop >= views[chi->view].numLoops)
        quitprintf("Unable to render character %d (%s) because loop %d does not exist in view %d", chi->index_id, chi->name, chi->loop, chi->view + 1);

      // check don't overflow loop
      int framesInLoop = views[chi->view].loops[chi->loop].numFrames;
      if (chi->frame > framesInLoop)
      {
        chi->frame = 1;

        if (framesInLoop < 2)
          chi->frame = 0;

        if (framesInLoop < 1)
          quitprintf("Unable to render character %d (%s) because there are no frames in loop %d", chi->index_id, chi->name, chi->loop);
      }

      if (chi->walking<1) {
        charextra[aa].process_idle_this_time = 1;
        doing_nothing=1;
        chi->walkwait=0;
        charextra[aa].animwait = 0;
        // use standing pic
        StopMoving(aa);
        chi->frame = 0;
        CheckViewFrameForCharacter(chi);
      }
      else if (charextra[aa].animwait > 0) charextra[aa].animwait--;
      else {
        if (chi->flags & CHF_ANTIGLIDE)
          chi->walkwaitcounter++;

        if ((chi->flags & CHF_MOVENOTWALK) == 0)
        {
          chi->frame++;
          if (chi->frame >= views[chi->view].loops[chi->loop].numFrames)
          {
            // end of loop, so loop back round skipping the standing frame
            chi->frame = 1;

            if (views[chi->view].loops[chi->loop].numFrames < 2)
              chi->frame = 0;
          }

          charextra[aa].animwait = views[chi->view].loops[chi->loop].frames[chi->frame].speed + chi->animspeed;

          if (chi->flags & CHF_ANTIGLIDE)
            chi->walkwait = charextra[aa].animwait;
          else
            chi->walkwait = 0;

          CheckViewFrameForCharacter(chi);
        }
      }
      doing_nothing = 0;
    }
    // not moving, but animating
    // idleleft is <0 while idle view is playing (.animating is 0)
    if (((chi->animating != 0) || (chi->idleleft < 0)) &&
        ((chi->walking == 0) || ((chi->flags & CHF_MOVENOTWALK) != 0)) &&
        (chi->room == displayed_room)) 
    {
      doing_nothing = 0;
      // idle anim doesn't count as doing something
      if (chi->idleleft < 0)
        doing_nothing = 1;

      if (chi->wait>0) chi->wait--;
      else if ((char_speaking == aa) && (game.options[OPT_LIPSYNCTEXT] != 0)) {
        // currently talking with lip-sync speech
        int fraa = chi->frame;
        chi->wait = update_lip_sync (chi->view, chi->loop, &fraa) - 1;
        // closed mouth at end of sentence
        if ((play.messagetime >= 0) && (play.messagetime < play.close_mouth_speech_time))
          chi->frame = 0;

        if (chi->frame != fraa) {
          chi->frame = fraa;
          CheckViewFrameForCharacter(chi);
        }
        
        continue;
      }
      else {
        int oldframe = chi->frame;
        if (chi->animating & CHANIM_BACKWARDS) {
          chi->frame--;
          if (chi->frame < 0) {
            // if the previous loop is a Run Next Loop one, go back to it
            if ((chi->loop > 0) && 
              (views[chi->view].loops[chi->loop - 1].RunNextLoop())) {

              chi->loop --;
              chi->frame = views[chi->view].loops[chi->loop].numFrames - 1;
            }
            else if (chi->animating & CHANIM_REPEAT) {

              chi->frame = views[chi->view].loops[chi->loop].numFrames - 1;

              while (views[chi->view].loops[chi->loop].RunNextLoop()) {
                chi->loop++;
                chi->frame = views[chi->view].loops[chi->loop].numFrames - 1;
              }
            }
            else {
              chi->frame++;
              chi->animating = 0;
            }
          }
        }
        else
          chi->frame++;

        if ((aa == char_speaking) &&
            (channels[SCHAN_SPEECH] == NULL) &&
            (play.close_mouth_speech_time > 0) &&
            (play.messagetime < play.close_mouth_speech_time)) {
          // finished talking - stop animation
          chi->animating = 0;
          chi->frame = 0;
        }

        if (chi->frame >= views[chi->view].loops[chi->loop].numFrames) {
          
          if (views[chi->view].loops[chi->loop].RunNextLoop()) 
          {
            if (chi->loop+1 >= views[chi->view].numLoops)
              quit("!Animating character tried to overrun last loop in view");
            chi->loop++;
            chi->frame=0;
          }
          else if ((chi->animating & CHANIM_REPEAT)==0) {
            chi->animating=0;
            chi->frame--;
            // end of idle anim
            if (chi->idleleft < 0) {
              // constant anim, reset (need this cos animating==0)
              if (chi->idletime == 0)
                chi->frame = 0;
              // one-off anim, stop
              else {
                ReleaseCharacterView(aa);
                chi->idleleft=chi->idletime;
              }
            }
          }
          else {
            chi->frame=0;
            // if it's a multi-loop animation, go back to start
            if (play.no_multiloop_repeat == 0) {
              while ((chi->loop > 0) && 
                  (views[chi->view].loops[chi->loop - 1].RunNextLoop()))
                chi->loop--;
            }
          }
        }
        chi->wait = views[chi->view].loops[chi->loop].frames[chi->frame].speed;
        // idle anim doesn't have speed stored cos animating==0
        if (chi->idleleft < 0)
          chi->wait += chi->animspeed+5;
        else 
          chi->wait += (chi->animating >> 8) & 0x00ff;

        if (chi->frame != oldframe)
          CheckViewFrameForCharacter(chi);
      }
    }

    if ((chi->following >= 0) && (chi->followinfo == FOLLOW_ALWAYSONTOP)) {
      // an always-on-top follow
      if (numSheep >= MAX_SHEEP)
        quit("too many sheep");
      followingAsSheep[numSheep] = aa;
      numSheep++;
    }
    // not moving, but should be following another character
    else if ((chi->following >= 0) && (doing_nothing == 1)) {
      short distaway=(chi->followinfo >> 8) & 0x00ff;
      // no character in this room
      if ((game.chars[chi->following].on == 0) || (chi->on == 0)) ;
      else if (chi->room < 0) {
        chi->room ++;
        if (chi->room == 0) {
          // appear in the new room
          chi->room = game.chars[chi->following].room;
          chi->x = play.entered_at_x;
          chi->y = play.entered_at_y;
        }
      }
      // wait a bit, so we're not constantly walking
      else if (Random(100) < (chi->followinfo & 0x00ff)) ;
      // the followed character has changed room
      else if ((chi->room != game.chars[chi->following].room)
            && (game.chars[chi->following].on == 0))
        ;  // do nothing if the player isn't visible
      else if (chi->room != game.chars[chi->following].room) {
        chi->prevroom = chi->room;
        chi->room = game.chars[chi->following].room;

        if (chi->room == displayed_room) {
          // only move to the room-entered position if coming into
          // the current room
          if (play.entered_at_x > (thisroom.width - 8)) {
            chi->x = thisroom.width+8;
            chi->y = play.entered_at_y;
            }
          else if (play.entered_at_x < 8) {
            chi->x = -8;
            chi->y = play.entered_at_y;
            }
          else if (play.entered_at_y > (thisroom.height - 8)) {
            chi->y = thisroom.height+8;
            chi->x = play.entered_at_x;
            }
          else if (play.entered_at_y < thisroom.top+8) {
            chi->y = thisroom.top+1;
            chi->x = play.entered_at_x;
            }
          else {
            // not at one of the edges
            // delay for a few seconds to let the player move
            chi->room = -play.follow_change_room_timer;
          }
          if (chi->room >= 0) {
            walk_character(aa,play.entered_at_x,play.entered_at_y,1, true);
            doing_nothing = 0;
          }
        }
      }
      else if (chi->room != displayed_room) {
        // if the characetr is following another character and
        // neither is in the current room, don't try to move
      }
      else if ((abs(game.chars[chi->following].x - chi->x) > distaway+30) |
        (abs(game.chars[chi->following].y - chi->y) > distaway+30) |
        ((chi->followinfo & 0x00ff) == 0)) {
        // in same room
        int goxoffs=(Random(50)-25);
        // make sure he's not standing on top of the other man
        if (goxoffs < 0) goxoffs-=distaway;
        else goxoffs+=distaway;
        walk_character(aa,game.chars[chi->following].x + goxoffs,
          game.chars[chi->following].y + (Random(50)-25),0, true);
        doing_nothing = 0;
      }
    }

    // no idle animation, so skip this bit
    if (chi->idleview < 1) ;
    // currently playing idle anim
    else if (chi->idleleft < 0) ;
    // not in the current room
    else if (chi->room != displayed_room) ;
    // they are moving or animating (or the view is locked), so 
    // reset idle timeout
    else if ((doing_nothing == 0) || ((chi->flags & CHF_FIXVIEW) != 0))
      chi->idleleft = chi->idletime;
    // count idle time
    else if ((loopcounter%40==0) || (charextra[aa].process_idle_this_time == 1)) {
      chi->idleleft--;
      if (chi->idleleft == -1) {
        int useloop=chi->loop;
        DEBUG_CONSOLE("%s: Now idle (view %d)", chi->scrname, chi->idleview+1);
        SetCharacterView(aa,chi->idleview+1);
        // SetCharView resets it to 0
        chi->idleleft = -2;
        int maxLoops = views[chi->idleview].numLoops;
        // if the char is set to "no diagonal loops", don't try
        // to use diagonal idle loops either
        if ((maxLoops > 4) && (useDiagonal(chi)))
          maxLoops = 4;
        // If it's not a "swimming"-type idleanim, choose a random loop
        // if there arent enough loops to do the current one.
        if ((chi->idletime > 0) && (useloop >= maxLoops)) {
          do {
            useloop = rand() % maxLoops;
          // don't select a loop which is a continuation of a previous one
          } while ((useloop > 0) && (views[chi->idleview].loops[useloop-1].RunNextLoop()));
        }
        // Normal idle anim - just reset to loop 0 if not enough to
        // use the current one
        else if (useloop >= maxLoops)
          useloop = 0;

        animate_character(chi,useloop,
          chi->animspeed+5,(chi->idletime == 0) ? 1 : 0, 1);

        // don't set Animating while the idle anim plays
        chi->animating = 0;
      }
    }  // end do idle animation

    charextra[aa].process_idle_this_time = 0;
  }

  // update location of all following_exactly characters
  for (aa = 0; aa < numSheep; aa++) {
    CharacterInfo *chi = &game.chars[followingAsSheep[aa]];

    chi->x = game.chars[chi->following].x;
    chi->y = game.chars[chi->following].y;
    chi->z = game.chars[chi->following].z;
    chi->room = game.chars[chi->following].room;
    chi->prevroom = game.chars[chi->following].prevroom;

    int usebase = game.chars[chi->following].get_baseline();

    if (chi->flags & CHF_BEHINDSHEPHERD)
      chi->baseline = usebase - 1;
    else
      chi->baseline = usebase + 1;
  }

  our_eip = 23;

  // update overlay timers
  for (aa=0;aa<numscreenover;aa++) {
    if (screenover[aa].timeout > 0) {
      screenover[aa].timeout--;
      if (screenover[aa].timeout == 0)
        remove_screen_overlay(screenover[aa].type);
    }
  }

  // determine if speech text should be removed
  if (play.messagetime>=0) {
    play.messagetime--;
    // extend life of text if the voice hasn't finished yet
    if (channels[SCHAN_SPEECH] != NULL) {
      if ((!rec_isSpeechFinished()) && (play.fast_forward == 0)) {
      //if ((!channels[SCHAN_SPEECH]->done) && (play.fast_forward == 0)) {
        if (play.messagetime <= 1)
          play.messagetime = 1;
      }
      else  // if the voice has finished, remove the speech
        play.messagetime = 0;
    }

    if (play.messagetime < 1) 
    {
      if (play.fast_forward > 0)
      {
        remove_screen_overlay(OVER_TEXTMSG);
      }
      else if (play.cant_skip_speech & SKIP_AUTOTIMER)
      {
        remove_screen_overlay(OVER_TEXTMSG);
        play.ignore_user_input_until_time = globalTimerCounter + (play.ignore_user_input_after_text_timeout_ms / time_between_timers);
      }
    }
  }
  our_eip = 24;

  // update sierra-style speech
  if ((face_talking >= 0) && (play.fast_forward == 0)) 
{
    int updatedFrame = 0;

    if ((facetalkchar->blinkview > 0) && (facetalkAllowBlink)) {
      if (facetalkchar->blinktimer > 0) {
        // countdown to playing blink anim
        facetalkchar->blinktimer--;
        if (facetalkchar->blinktimer == 0) {
          facetalkchar->blinkframe = 0;
          facetalkchar->blinktimer = -1;
          updatedFrame = 2;
        }
      }
      else if (facetalkchar->blinktimer < 0) {
        // currently playing blink anim
        if (facetalkchar->blinktimer < ( (0 - 6) - views[facetalkchar->blinkview].loops[facetalkBlinkLoop].frames[facetalkchar->blinkframe].speed)) {
          // time to advance to next frame
          facetalkchar->blinktimer = -1;
          facetalkchar->blinkframe++;
          updatedFrame = 2;
          if (facetalkchar->blinkframe >= views[facetalkchar->blinkview].loops[facetalkBlinkLoop].numFrames) 
          {
            facetalkchar->blinkframe = 0;
            facetalkchar->blinktimer = facetalkchar->blinkinterval;
          }
        }
        else
          facetalkchar->blinktimer--;
      }

    }

    if (curLipLine >= 0) {
      // check voice lip sync
      int spchOffs = channels[SCHAN_SPEECH]->get_pos_ms ();
      if (curLipLinePhenome >= splipsync[curLipLine].numPhenomes) {
        // the lip-sync has finished, so just stay idle
      }
      else 
      {
        while ((curLipLinePhenome < splipsync[curLipLine].numPhenomes) &&
          ((curLipLinePhenome < 0) || (spchOffs >= splipsync[curLipLine].endtimeoffs[curLipLinePhenome])))
        {
          curLipLinePhenome ++;
          if (curLipLinePhenome >= splipsync[curLipLine].numPhenomes)
            facetalkframe = game.default_lipsync_frame;
          else
            facetalkframe = splipsync[curLipLine].frame[curLipLinePhenome];

          if (facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames)
            facetalkframe = 0;

          updatedFrame |= 1;
        }
      }
    }
    else if (facetalkwait>0) facetalkwait--;
    // don't animate if the speech has finished
    else if ((play.messagetime < 1) && (facetalkframe == 0) && (play.close_mouth_speech_time > 0))
      ;
    else {
      // Close mouth at end of sentence
      if ((play.messagetime < play.close_mouth_speech_time) &&
          (play.close_mouth_speech_time > 0)) {
        facetalkframe = 0;
        facetalkwait = play.messagetime;
      }
      else if ((game.options[OPT_LIPSYNCTEXT]) && (facetalkrepeat > 0)) {
        // lip-sync speech (and not a thought)
        facetalkwait = update_lip_sync (facetalkview, facetalkloop, &facetalkframe);
        // It is actually displayed for facetalkwait+1 loops
        // (because when it's 1, it gets --'d then wait for next time)
        facetalkwait --;
      }
      else {
        // normal non-lip-sync
        facetalkframe++;
        if ((facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames) ||
            ((play.messagetime < 1) && (play.close_mouth_speech_time > 0))) {

          if ((facetalkframe >= views[facetalkview].loops[facetalkloop].numFrames) &&
              (views[facetalkview].loops[facetalkloop].RunNextLoop())) 
          {
            facetalkloop++;
          }
          else 
          {
            facetalkloop = 0;
          }
          facetalkframe = 0;
          if (!facetalkrepeat)
            facetalkwait = 999999;
        }
        if ((facetalkframe != 0) || (facetalkrepeat == 1))
          facetalkwait = views[facetalkview].loops[facetalkloop].frames[facetalkframe].speed + GetCharacterSpeechAnimationDelay(facetalkchar);
      }
      updatedFrame |= 1;
    }

    // is_text_overlay might be 0 if it was only just destroyed this loop
    if ((updatedFrame) && (is_text_overlay > 0)) {

      if (updatedFrame & 1)
        CheckViewFrame (facetalkview, facetalkloop, facetalkframe);
      if (updatedFrame & 2)
        CheckViewFrame (facetalkchar->blinkview, facetalkBlinkLoop, facetalkchar->blinkframe);

      int yPos = 0;
      int thisPic = views[facetalkview].loops[facetalkloop].frames[facetalkframe].pic;
      
      if (game.options[OPT_SPEECHTYPE] == 3) {
        // QFG4-style fullscreen dialog
        yPos = (screenover[face_talking].pic->h / 2) - (spriteheight[thisPic] / 2);
        clear_to_color(screenover[face_talking].pic, 0);
      }
      else {
        clear_to_color(screenover[face_talking].pic, bitmap_mask_color(screenover[face_talking].pic));
      }

      DrawViewFrame(screenover[face_talking].pic, &views[facetalkview].loops[facetalkloop].frames[facetalkframe], 0, yPos);
//      draw_sprite(screenover[face_talking].pic, spriteset[thisPic], 0, yPos);

      if ((facetalkchar->blinkview > 0) && (facetalkchar->blinktimer < 0)) {
        // draw the blinking sprite on top
        DrawViewFrame(screenover[face_talking].pic,
            &views[facetalkchar->blinkview].loops[facetalkBlinkLoop].frames[facetalkchar->blinkframe],
            0, yPos);

/*        draw_sprite(screenover[face_talking].pic,
          spriteset[views[facetalkchar->blinkview].frames[facetalkloop][facetalkchar->blinkframe].pic],
          0, yPos);*/
      }

      gfxDriver->UpdateDDBFromBitmap(screenover[face_talking].bmp, screenover[face_talking].pic, false);
    }  // end if updatedFrame
  }

  our_eip = 25;
}

void replace_macro_tokens(char*statusbarformat,char*cur_stb_text) {
  char*curptr=&statusbarformat[0];
  char tmpm[3];
  char*endat = curptr + strlen(statusbarformat);
  cur_stb_text[0]=0;
  char tempo[STD_BUFFER_SIZE];

  while (1) {
    if (curptr[0]==0) break;
    if (curptr>=endat) break;
    if (curptr[0]=='@') {
      char *curptrWasAt = curptr;
      char macroname[21]; int idd=0; curptr++;
      for (idd=0;idd<20;idd++) {
        if (curptr[0]=='@') {
          macroname[idd]=0;
          curptr++;
          break;
        }
        // unterminated macro (eg. "@SCORETEXT"), so abort
        if (curptr[0] == 0)
          break;
        macroname[idd]=curptr[0];
        curptr++;
      }
      macroname[idd]=0; 
      tempo[0]=0;
      if (stricmp(macroname,"score")==0)
        sprintf(tempo,"%d",play.score);
      else if (stricmp(macroname,"totalscore")==0)
        sprintf(tempo,"%d",MAXSCORE);
      else if (stricmp(macroname,"scoretext")==0)
        sprintf(tempo,"%d of %d",play.score,MAXSCORE);
      else if (stricmp(macroname,"gamename")==0)
        strcpy(tempo, play.game_name);
      else if (stricmp(macroname,"overhotspot")==0) {
        // While game is in Wait mode, no overhotspot text
        if (!IsInterfaceEnabled())
          tempo[0] = 0;
        else
          GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);
      }
      else { // not a macro, there's just a @ in the message
        curptr = curptrWasAt + 1;
        strcpy(tempo, "@");
      }
        
      strcat(cur_stb_text,tempo);
    }
    else {
      tmpm[0]=curptr[0]; tmpm[1]=0;
      strcat(cur_stb_text,tmpm);
      curptr++;
    }
  }
}

void update_invorder() {
  for (int cc = 0; cc < game.numcharacters; cc++) {
    charextra[cc].invorder_count = 0;
    int ff, howmany;
    // Iterate through all inv items, adding them once (or multiple
    // times if requested) to the list.
    for (ff=0;ff < game.numinvitems;ff++) {
      howmany = game.chars[cc].inv[ff];
      if ((game.options[OPT_DUPLICATEINV] == 0) && (howmany > 1))
        howmany = 1;

      for (int ts = 0; ts < howmany; ts++) {
        if (charextra[cc].invorder_count >= MAX_INVORDER)
          quit("!Too many inventory items to display: 500 max");

        charextra[cc].invorder[charextra[cc].invorder_count] = ff;
        charextra[cc].invorder_count++;
      }
    }
  }
  // backwards compatibility
  play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;

  guis_need_update = 1;
}

int GUIInv::CharToDisplay() {
  if (this->charId < 0)
    return game.playercharacter;

  return this->charId;
}

void GUIInv::Draw() {
  if ((IsDisabled()) && (gui_disabled_style == GUIDIS_BLACKOUT))
    return;

  // backwards compatibility
  play.inv_numinline = this->itemsPerLine;
  play.inv_numdisp = this->numLines * this->itemsPerLine;
  play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
  // if the user changes top_inv_item, switch into backwards
  // compatibiltiy mode
  if (play.inv_top) {
    play.inv_backwards_compatibility = 1;
  }

  if (play.inv_backwards_compatibility) {
    this->topIndex = play.inv_top;
  }

  // draw the items
  int xxx = x;
  int uu, cxp = x, cyp = y;
  int lastItem = this->topIndex + (this->itemsPerLine * this->numLines);
  if (lastItem > charextra[this->CharToDisplay()].invorder_count)
    lastItem = charextra[this->CharToDisplay()].invorder_count;

  for (uu = this->topIndex; uu < lastItem; uu++) {
    // draw inv graphic
    wputblock(cxp, cyp, spriteset[game.invinfo[charextra[this->CharToDisplay()].invorder[uu]].pic], 1);
    cxp += multiply_up_coordinate(this->itemWidth);

    // go to next row when appropriate
    if ((uu - this->topIndex) % this->itemsPerLine == (this->itemsPerLine - 1)) {
      cxp = xxx;
      cyp += multiply_up_coordinate(this->itemHeight);
    }
  }

  if ((IsDisabled()) &&
      (gui_disabled_style == GUIDIS_GREYOUT) && 
      (play.inventory_greys_out == 1)) {
    int col8 = get_col8_lookup(8);
    int jj, kk;   // darken the inventory when disabled
    for (jj = 0; jj < wid; jj++) {
      for (kk = jj % 2; kk < hit; kk += 2)
        putpixel(abuf, x + jj, y + kk, col8);
    }
  }

}

// Avoid freeing and reallocating the memory if possible
IDriverDependantBitmap* recycle_ddb_bitmap(IDriverDependantBitmap *bimp, BITMAP *source, bool hasAlpha) {
  if (bimp != NULL) {
    // same colour depth, width and height -> reuse
    if (((bimp->GetColorDepth() + 1) / 8 == bmp_bpp(source)) && 
        (bimp->GetWidth() == source->w) && (bimp->GetHeight() == source->h))
    {
      gfxDriver->UpdateDDBFromBitmap(bimp, source, hasAlpha);
      return bimp;
    }

    gfxDriver->DestroyDDB(bimp);
  }
  bimp = gfxDriver->CreateDDBFromBitmap(source, hasAlpha, false);
  return bimp;
}

// sort_out_walk_behinds: modifies the supplied sprite by overwriting parts
// of it with transparent pixels where there are walk-behind areas
// Returns whether any pixels were updated
int sort_out_walk_behinds(block sprit,int xx,int yy,int basel, block copyPixelsFrom = NULL, block checkPixelsFrom = NULL, int zoom=100) {
  if (noWalkBehindsAtAll)
    return 0;

  if ((!is_memory_bitmap(thisroom.object)) ||
      (!is_memory_bitmap(sprit)))
    quit("!sort_out_walk_behinds: wb bitmap not linear");

  int rr,tmm, toheight;//,tcol;
  // precalculate this to try and shave some time off
  int maskcol = bitmap_mask_color(sprit);
  int spcoldep = bitmap_color_depth(sprit);
  int screenhit = thisroom.object->h;
  short *shptr, *shptr2;
  long *loptr, *loptr2;
  int pixelsChanged = 0;
  int ee = 0;
  if (xx < 0)
    ee = 0 - xx;

  if ((checkPixelsFrom != NULL) && (bitmap_color_depth(checkPixelsFrom) != spcoldep))
    quit("sprite colour depth does not match background colour depth");

  for ( ; ee < sprit->w; ee++) {
    if (ee + xx >= thisroom.object->w)
      break;

    if ((!walkBehindExists[ee+xx]) ||
        (walkBehindEndY[ee+xx] <= yy) ||
        (walkBehindStartY[ee+xx] > yy+sprit->h))
      continue;

    toheight = sprit->h;

    if (walkBehindStartY[ee+xx] < yy)
      rr = 0;
    else
      rr = (walkBehindStartY[ee+xx] - yy);

    // Since we will use _getpixel, ensure we only check within the screen
    if (rr + yy < 0)
      rr = 0 - yy;
    if (toheight + yy > screenhit)
      toheight = screenhit - yy;
    if (toheight + yy > walkBehindEndY[ee+xx])
      toheight = walkBehindEndY[ee+xx] - yy;
    if (rr < 0)
      rr = 0;

    for ( ; rr < toheight;rr++) {
      
      // we're ok with _getpixel because we've checked the screen edges
      //tmm = _getpixel(thisroom.object,ee+xx,rr+yy);
      // actually, _getpixel is well inefficient, do it ourselves
      // since we know it's 8-bit bitmap
      tmm = thisroom.object->line[rr+yy][ee+xx];
      if (tmm<1) continue;
      if (croom->walkbehind_base[tmm] <= basel) continue;

      if (copyPixelsFrom != NULL)
      {
        if (spcoldep <= 8)
        {
          if (checkPixelsFrom->line[(rr * 100) / zoom][(ee * 100) / zoom] != maskcol) {
            sprit->line[rr][ee] = copyPixelsFrom->line[rr + yy][ee + xx];
            pixelsChanged = 1;
          }
        }
        else if (spcoldep <= 16) {
          shptr = (short*)&sprit->line[rr][0];
          shptr2 = (short*)&checkPixelsFrom->line[(rr * 100) / zoom][0];
          if (shptr2[(ee * 100) / zoom] != maskcol) {
            shptr[ee] = ((short*)(&copyPixelsFrom->line[rr + yy][0]))[ee + xx];
            pixelsChanged = 1;
          }
        }
        else if (spcoldep == 24) {
          char *chptr = (char*)&sprit->line[rr][0];
          char *chptr2 = (char*)&checkPixelsFrom->line[(rr * 100) / zoom][0];
          if (memcmp(&chptr2[((ee * 100) / zoom) * 3], &maskcol, 3) != 0) {
            memcpy(&chptr[ee * 3], &copyPixelsFrom->line[rr + yy][(ee + xx) * 3], 3);
            pixelsChanged = 1;
          }
        }
        else if (spcoldep <= 32) {
          loptr = (long*)&sprit->line[rr][0];
          loptr2 = (long*)&checkPixelsFrom->line[(rr * 100) / zoom][0];
          if (loptr2[(ee * 100) / zoom] != maskcol) {
            loptr[ee] = ((long*)(&copyPixelsFrom->line[rr + yy][0]))[ee + xx];
            pixelsChanged = 1;
          }
        }
      }
      else
      {
        pixelsChanged = 1;
        if (spcoldep <= 8)
          sprit->line[rr][ee] = maskcol;
        else if (spcoldep <= 16) {
          shptr = (short*)&sprit->line[rr][0];
          shptr[ee] = maskcol;
        }
        else if (spcoldep == 24) {
          char *chptr = (char*)&sprit->line[rr][0];
          memcpy(&chptr[ee * 3], &maskcol, 3);
        }
        else if (spcoldep <= 32) {
          loptr = (long*)&sprit->line[rr][0];
          loptr[ee] = maskcol;
        }
        else
          quit("!Sprite colour depth >32 ??");
      }
    }
  }
  return pixelsChanged;
}

void invalidate_cached_walkbehinds() 
{
  memset(&actspswbcache[0], 0, sizeof(CachedActSpsData) * actSpsCount);
}

void sort_out_char_sprite_walk_behind(int actspsIndex, int xx, int yy, int basel, int zoom, int width, int height)
{
  if (noWalkBehindsAtAll)
    return;

  if ((!actspswbcache[actspsIndex].valid) ||
    (actspswbcache[actspsIndex].xWas != xx) ||
    (actspswbcache[actspsIndex].yWas != yy) ||
    (actspswbcache[actspsIndex].baselineWas != basel))
  {
    actspswb[actspsIndex] = recycle_bitmap(actspswb[actspsIndex], bitmap_color_depth(thisroom.ebscene[play.bg_frame]), width, height);

    block wbSprite = actspswb[actspsIndex];
    clear_to_color(wbSprite, bitmap_mask_color(wbSprite));

    actspswbcache[actspsIndex].isWalkBehindHere = sort_out_walk_behinds(wbSprite, xx, yy, basel, thisroom.ebscene[play.bg_frame], actsps[actspsIndex], zoom);
    actspswbcache[actspsIndex].xWas = xx;
    actspswbcache[actspsIndex].yWas = yy;
    actspswbcache[actspsIndex].baselineWas = basel;
    actspswbcache[actspsIndex].valid = 1;

    if (actspswbcache[actspsIndex].isWalkBehindHere)
    {
      actspswbbmp[actspsIndex] = recycle_ddb_bitmap(actspswbbmp[actspsIndex], actspswb[actspsIndex], false);
    }
  }

  if (actspswbcache[actspsIndex].isWalkBehindHere)
  {
    add_to_sprite_list(actspswbbmp[actspsIndex], xx - offsetx, yy - offsety, basel, 0, -1, true);
  }
}

void clear_draw_list() {
  thingsToDrawSize = 0;
}
void add_thing_to_draw(IDriverDependantBitmap* bmp, int x, int y, int trans, bool alphaChannel) {
  thingsToDrawList[thingsToDrawSize].pic = NULL;
  thingsToDrawList[thingsToDrawSize].bmp = bmp;
  thingsToDrawList[thingsToDrawSize].x = x;
  thingsToDrawList[thingsToDrawSize].y = y;
  thingsToDrawList[thingsToDrawSize].transparent = trans;
  thingsToDrawList[thingsToDrawSize].hasAlphaChannel = alphaChannel;
  thingsToDrawSize++;
  if (thingsToDrawSize >= MAX_THINGS_TO_DRAW - 1)
    quit("add_thing_to_draw: too many things added");
}

// the sprite list is an intermediate list used to order 
// objects and characters by their baselines before everything
// is added to the Thing To Draw List
void clear_sprite_list() {
  sprlistsize=0;
}
void add_to_sprite_list(IDriverDependantBitmap* spp, int xx, int yy, int baseline, int trans, int sprNum, bool isWalkBehind) {

  // completely invisible, so don't draw it at all
  if (trans == 255)
    return;

  if ((sprNum >= 0) && ((game.spriteflags[sprNum] & SPF_ALPHACHANNEL) != 0))
    sprlist[sprlistsize].hasAlphaChannel = true;
  else
    sprlist[sprlistsize].hasAlphaChannel = false;

  sprlist[sprlistsize].bmp = spp;
  sprlist[sprlistsize].baseline = baseline;
  sprlist[sprlistsize].x=xx;
  sprlist[sprlistsize].y=yy;
  sprlist[sprlistsize].transparent=trans;

  if (walkBehindMethod == DrawAsSeparateSprite)
    sprlist[sprlistsize].takesPriorityIfEqual = !isWalkBehind;
  else
    sprlist[sprlistsize].takesPriorityIfEqual = isWalkBehind;

  sprlistsize++;

  if (sprlistsize >= MAX_SPRITES_ON_SCREEN)
    quit("Too many sprites have been added to the sprite list. There is a limit of 75 objects and characters being visible at the same time. You may want to reconsider your design since you have over 75 objects/characters visible at once.");

  if (spp == NULL)
    quit("add_to_sprite_list: attempted to draw NULL sprite");
}

void put_sprite_256(int xxx,int yyy,block piccy) {

  if (trans_mode >= 255) {
    // fully transparent, don't draw it at all
    trans_mode = 0;
    return;
  }

  int screen_depth = bitmap_color_depth(abuf);

#ifdef USE_15BIT_FIX
  if (bitmap_color_depth(piccy) < screen_depth) {

    if ((bitmap_color_depth(piccy) == 8) && (screen_depth >= 24)) {
      // 256-col sprite -> truecolor background
      // this is automatically supported by allegro, no twiddling needed
      draw_sprite(abuf, piccy, xxx, yyy);
      return;
    }
    // 256-col spirte -> hi-color background, or
    // 16-bit sprite -> 32-bit background
    block hctemp=create_bitmap_ex(screen_depth, piccy->w, piccy->h);
    blit(piccy,hctemp,0,0,0,0,hctemp->w,hctemp->h);
    int bb,cc,mask_col = bitmap_mask_color(abuf);
    if (bitmap_color_depth(piccy) == 8) {
      // only do this for 256-col, cos the blit call converts
      // transparency for 16->32 bit
      for (bb=0;bb<hctemp->w;bb++) {
        for (cc=0;cc<hctemp->h;cc++)
          if (_getpixel(piccy,bb,cc)==0) putpixel(hctemp,bb,cc,mask_col);
      }
    }
    wputblock(xxx,yyy,hctemp,1);
    wfreeblock(hctemp);
  }
  else
#endif
  {
    if ((trans_mode!=0) && (game.color_depth > 1) && (bmp_bpp(piccy) > 1) && (bmp_bpp(abuf) > 1)) {
      set_trans_blender(0,0,0,trans_mode);
      draw_trans_sprite(abuf,piccy,xxx,yyy);
      }
/*    else if ((lit_mode < 0) && (game.color_depth == 1) && (bmp_bpp(piccy) == 1)) {
      draw_lit_sprite(abuf,piccy,xxx,yyy,250 - ((-lit_mode) * 5)/2);
      }*/
    else
      wputblock(xxx,yyy,piccy,1);
  }
  trans_mode=0;
}

void repair_alpha_channel(block dest, block bgpic)
{
  // Repair the alpha channel, because sprites may have been drawn
  // over it by the buttons, etc
  int theWid = (dest->w < bgpic->w) ? dest->w : bgpic->w;
  int theHit = (dest->h < bgpic->h) ? dest->h : bgpic->h;
  for (int y = 0; y < theHit; y++) 
  {
    unsigned long *destination = ((unsigned long*)dest->line[y]);
    unsigned long *source = ((unsigned long*)bgpic->line[y]);
    for (int x = 0; x < theWid; x++) 
    {
      destination[x] |= (source[x] & 0xff000000);
    }
  }
}


// used by GUI renderer to draw images
void draw_sprite_compensate(int picc,int xx,int yy,int useAlpha) 
{
  if ((useAlpha) && 
    (game.options[OPT_NEWGUIALPHA] > 0) &&
    (bitmap_color_depth(abuf) == 32))
  {
    if (game.spriteflags[picc] & SPF_ALPHACHANNEL)
      set_additive_alpha_blender();
    else
      set_opaque_alpha_blender();

    draw_trans_sprite(abuf, spriteset[picc], xx, yy);
  }
  else
  {
    put_sprite_256(xx, yy, spriteset[picc]);
  }
}

// function to sort the sprites into baseline order
extern "C" int compare_listentries(const void *elem1, const void *elem2) {
  SpriteListEntry *e1, *e2;
  e1 = (SpriteListEntry*)elem1;
  e2 = (SpriteListEntry*)elem2;

  if (e1->baseline == e2->baseline) 
  { 
    if (e1->takesPriorityIfEqual)
      return 1;
    if (e2->takesPriorityIfEqual)
      return -1;
  }

  // returns >0 if e1 is lower down, <0 if higher, =0 if the same
  return e1->baseline - e2->baseline;
}

void draw_sprite_list() {

  if (walkBehindMethod == DrawAsSeparateSprite)
  {
    for (int ee = 1; ee < MAX_OBJ; ee++)
    {
      if (walkBehindBitmap[ee] != NULL)
      {
        add_to_sprite_list(walkBehindBitmap[ee], walkBehindLeft[ee] - offsetx, walkBehindTop[ee] - offsety, 
                           croom->walkbehind_base[ee], 0, -1, true);
      }
    }
  }

  // 2.60.672 - convert horrid bubble sort to use qsort instead
  qsort(sprlist, sprlistsize, sizeof(SpriteListEntry), compare_listentries);

  clear_draw_list();

  add_thing_to_draw(NULL, AGSE_PRESCREENDRAW, 0, TRANS_RUN_PLUGIN, false);

  // copy the sorted sprites into the Things To Draw list
  thingsToDrawSize += sprlistsize;
  memcpy(&thingsToDrawList[1], sprlist, sizeof(SpriteListEntry) * sprlistsize);
}

// Avoid freeing and reallocating the memory if possible
block recycle_bitmap(block bimp, int coldep, int wid, int hit) {
  if (bimp != NULL) {
    // same colour depth, width and height -> reuse
    if ((bitmap_color_depth(bimp) == coldep) && (bimp->w == wid)
       && (bimp->h == hit))
      return bimp;

    destroy_bitmap(bimp);
  }
  bimp = create_bitmap_ex(coldep, wid, hit);
  return bimp;
}

int GetRegionAt (int xxx, int yyy) {
  // if the co-ordinates are off the edge of the screen,
  // correct them to be just within
  // this fixes walk-off-screen problems
  xxx = convert_to_low_res(xxx);
  yyy = convert_to_low_res(yyy);

  if (xxx >= thisroom.regions->w)
	  xxx = thisroom.regions->w - 1;
  if (yyy >= thisroom.regions->h)
	  yyy = thisroom.regions->h - 1;
  if (xxx < 0)
	  xxx = 0;
  if (yyy < 0)
	  yyy = 0;

  int hsthere = getpixel (thisroom.regions, xxx, yyy);
  if (hsthere < 0)
    hsthere = 0;

  if (hsthere >= MAX_REGIONS) {
    char tempmsg[300];
    sprintf(tempmsg, "!An invalid pixel was found on the room region mask (colour %d, location: %d, %d)", hsthere, xxx, yyy);
    quit(tempmsg);
  }

  if (croom->region_enabled[hsthere] == 0)
    return 0;
  return hsthere;
}

ScriptRegion *GetRegionAtLocation(int xx, int yy) {
  int hsnum = GetRegionAt(xx, yy);
  if (hsnum <= 0)
    return &scrRegion[0];
  return &scrRegion[hsnum];
}

// Get the local tint at the specified X & Y co-ordinates, based on
// room regions and SetAmbientTint
// tint_amnt will be set to 0 if there is no tint enabled
// if this is the case, then light_lev holds the light level (0=none)
void get_local_tint(int xpp, int ypp, int nolight,
                    int *tint_amnt, int *tint_r, int *tint_g,
                    int *tint_b, int *tint_lit,
                    int *light_lev) {

  int tint_level = 0, light_level = 0;
  int tint_amount = 0;
  int tint_red = 0;
  int tint_green = 0;
  int tint_blue = 0;
  int tint_light = 255;

  if (nolight == 0) {

    int onRegion = 0;

    if ((play.ground_level_areas_disabled & GLED_EFFECTS) == 0) {
      // check if the player is on a region, to find its
      // light/tint level
      onRegion = GetRegionAt (xpp, ypp);
      if (onRegion == 0) {
        // when walking, he might just be off a walkable area
        onRegion = GetRegionAt (xpp - 3, ypp);
        if (onRegion == 0)
          onRegion = GetRegionAt (xpp + 3, ypp);
        if (onRegion == 0)
          onRegion = GetRegionAt (xpp, ypp - 3);
        if (onRegion == 0)
          onRegion = GetRegionAt (xpp, ypp + 3);
      }
    }

    if ((onRegion > 0) && (onRegion <= MAX_REGIONS)) {
      light_level = thisroom.regionLightLevel[onRegion];
      tint_level = thisroom.regionTintLevel[onRegion];
    }
    else if (onRegion <= 0) {
      light_level = thisroom.regionLightLevel[0];
      tint_level = thisroom.regionTintLevel[0];
    }
    if ((game.color_depth == 1) || ((tint_level & 0x00ffffff) == 0) ||
        ((tint_level & TINT_IS_ENABLED) == 0))
      tint_level = 0;

    if (tint_level) {
      tint_red = (unsigned char)(tint_level & 0x000ff);
      tint_green = (unsigned char)((tint_level >> 8) & 0x000ff);
      tint_blue = (unsigned char)((tint_level >> 16) & 0x000ff);
      tint_amount = light_level;
      // older versions of the editor had a bug - work around it
      if (tint_amount < 0)
        tint_amount = 50;
      /*red = ((red + 100) * 25) / 20;
      grn = ((grn + 100) * 25) / 20;
      blu = ((blu + 100) * 25) / 20;*/
    }

    if (play.rtint_level > 0) {
      // override with room tint
      tint_level = 1;
      tint_red = play.rtint_red;
      tint_green = play.rtint_green;
      tint_blue = play.rtint_blue;
      tint_amount = play.rtint_level;
      tint_light = play.rtint_light;
    }
  }

  // copy to output parameters
  *tint_amnt = tint_amount;
  *tint_r = tint_red;
  *tint_g = tint_green;
  *tint_b = tint_blue;
  *tint_lit = tint_light;
  if (light_lev)
    *light_lev = light_level;
}

// Applies the specified RGB Tint or Light Level to the actsps
// sprite indexed with actspsindex
void apply_tint_or_light(int actspsindex, int light_level,
                         int tint_amount, int tint_red, int tint_green,
                         int tint_blue, int tint_light, int coldept,
                         block blitFrom) {

  // In a 256-colour game, we cannot do tinting or lightening
  // (but we can do darkening, if light_level < 0)
  if (game.color_depth == 1) {
    if ((light_level > 0) || (tint_amount != 0))
      return;
  }

  // we can only do tint/light if the colour depths match
  if (final_col_dep == bitmap_color_depth(actsps[actspsindex])) {
    block oldwas;
    // if the caller supplied a source bitmap, blit from it
    // (used as a speed optimisation where possible)
    if (blitFrom) 
      oldwas = blitFrom;
    // otherwise, make a new target bmp
    else {
      oldwas = actsps[actspsindex];
      actsps[actspsindex] = create_bitmap_ex(coldept, oldwas->w, oldwas->h);
    }

    if (tint_amount) {
      // It is an RGB tint

      tint_image (oldwas, actsps[actspsindex], tint_red, tint_green, tint_blue, tint_amount, tint_light);
    }
    else {
      // the RGB values passed to set_trans_blender decide whether it will darken
      // or lighten sprites ( <128=darken, >128=lighten). The parameter passed
      // to draw_lit_sprite defines how much it will be darkened/lightened by.
      int lit_amnt;
      clear_to_color(actsps[actspsindex], bitmap_mask_color(actsps[actspsindex]));
      // It's a light level, not a tint
      if (game.color_depth == 1) {
        // 256-col
        lit_amnt = (250 - ((-light_level) * 5)/2);
      }
      else {
        // hi-color
        if (light_level < 0)
          set_my_trans_blender(8,8,8,0);
        else
          set_my_trans_blender(248,248,248,0);
        lit_amnt = abs(light_level) * 2;
      }

      draw_lit_sprite(actsps[actspsindex], oldwas, 0, 0, lit_amnt);
    }

    if (oldwas != blitFrom)
      wfreeblock(oldwas);

  }
  else if (blitFrom) {
    // sprite colour depth != game colour depth, so don't try and tint
    // but we do need to do something, so copy the source
    blit(blitFrom, actsps[actspsindex], 0, 0, 0, 0, actsps[actspsindex]->w, actsps[actspsindex]->h);
  }

}

// Draws the specified 'sppic' sprite onto actsps[useindx] at the
// specified width and height, and flips the sprite if necessary.
// Returns 1 if something was drawn to actsps; returns 0 if no
// scaling or stretching was required, in which case nothing was done
int scale_and_flip_sprite(int useindx, int coldept, int zoom_level,
                          int sppic, int newwidth, int newheight,
                          int isMirrored) {

  int actsps_used = 1;

  // create and blank out the new sprite
  actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, newwidth, newheight);
  clear_to_color(actsps[useindx],bitmap_mask_color(actsps[useindx]));

  if (zoom_level != 100) {
    // Scaled character

    our_eip = 334;

    // Ensure that anti-aliasing routines have a palette to
    // use for mapping while faded out
    if (in_new_room)
      select_palette (palette);

    
    if (isMirrored) {
      block tempspr = create_bitmap_ex (coldept, newwidth, newheight);
      clear_to_color (tempspr, bitmap_mask_color(actsps[useindx]));
      if ((IS_ANTIALIAS_SPRITES) && ((game.spriteflags[sppic] & SPF_ALPHACHANNEL) == 0))
        aa_stretch_sprite (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      else
        stretch_sprite (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      draw_sprite_h_flip (actsps[useindx], tempspr, 0, 0);
      wfreeblock (tempspr);
    }
    else if ((IS_ANTIALIAS_SPRITES) && ((game.spriteflags[sppic] & SPF_ALPHACHANNEL) == 0))
      aa_stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
    else
      stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);

/*  AASTR2 version of code (doesn't work properly, gives black borders)
    if (IS_ANTIALIAS_SPRITES) {
      int aa_mode = AA_MASKED; 
      if (game.spriteflags[sppic] & SPF_ALPHACHANNEL)
        aa_mode |= AA_ALPHA | AA_RAW_ALPHA;
      if (isMirrored)
        aa_mode |= AA_HFLIP;

      aa_set_mode(aa_mode);
      aa_stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
    }
    else if (isMirrored) {
      block tempspr = create_bitmap_ex (coldept, newwidth, newheight);
      clear_to_color (tempspr, bitmap_mask_color(actsps[useindx]));
      stretch_sprite (tempspr, spriteset[sppic], 0, 0, newwidth, newheight);
      draw_sprite_h_flip (actsps[useindx], tempspr, 0, 0);
      wfreeblock (tempspr);
    }
    else
      stretch_sprite(actsps[useindx],spriteset[sppic],0,0,newwidth,newheight);
*/
    if (in_new_room)
      unselect_palette();

  } 
  else {
    // Not a scaled character, draw at normal size

    our_eip = 339;

    if (isMirrored)
      draw_sprite_h_flip (actsps[useindx], spriteset[sppic], 0, 0);
    else
      actsps_used = 0;
      //blit (spriteset[sppic], actsps[useindx], 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);
  }

  return actsps_used;
}


int get_area_scaling (int onarea, int xx, int yy) {

  int zoom_level = 100;
  xx = convert_to_low_res(xx);
  yy = convert_to_low_res(yy);

  if ((onarea >= 0) && (onarea <= MAX_WALK_AREAS) &&
      (thisroom.walk_area_zoom2[onarea] != NOT_VECTOR_SCALED)) {
    // We have vector scaling!
    // In case the character is off the screen, limit the Y co-ordinate
    // to within the area range (otherwise we get silly zoom levels
    // that cause Out Of Memory crashes)
    if (yy > thisroom.walk_area_bottom[onarea])
      yy = thisroom.walk_area_bottom[onarea];
    if (yy < thisroom.walk_area_top[onarea])
      yy = thisroom.walk_area_top[onarea];
    // Work it all out without having to use floats
    // Percent = ((y - top) * 100) / (areabottom - areatop)
    // Zoom level = ((max - min) * Percent) / 100
    int percent = ((yy - thisroom.walk_area_top[onarea]) * 100)
          / (thisroom.walk_area_bottom[onarea] - thisroom.walk_area_top[onarea]);

    zoom_level = ((thisroom.walk_area_zoom2[onarea] - thisroom.walk_area_zoom[onarea]) * (percent)) / 100 + thisroom.walk_area_zoom[onarea];
    zoom_level += 100;
  }
  else if ((onarea >= 0) & (onarea <= MAX_WALK_AREAS))
    zoom_level = thisroom.walk_area_zoom[onarea] + 100;

  if (zoom_level == 0)
    zoom_level = 100;

  return zoom_level;
}

void scale_sprite_size(int sppic, int zoom_level, int *newwidth, int *newheight) {
  newwidth[0] = (spritewidth[sppic] * zoom_level) / 100;
  newheight[0] = (spriteheight[sppic] * zoom_level) / 100;
  if (newwidth[0] < 1)
    newwidth[0] = 1;
  if (newheight[0] < 1)
    newheight[0] = 1;
}

// create the actsps[aa] image with the object drawn correctly
// returns 1 if nothing at all has changed and actsps is still
// intact from last time; 0 otherwise
int construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware) {
  int useindx = aa;
  bool hardwareAccelerated = gfxDriver->HasAcceleratedStretchAndFlip();

  if (alwaysUseSoftware)
    hardwareAccelerated = false;

  if (spriteset[objs[aa].num] == NULL)
    quitprintf("There was an error drawing object %d. Its current sprite, %d, is invalid.", aa, objs[aa].num);

  int coldept = bitmap_color_depth(spriteset[objs[aa].num]);
  int sprwidth = spritewidth[objs[aa].num];
  int sprheight = spriteheight[objs[aa].num];

  int tint_red, tint_green, tint_blue;
  int tint_level, tint_light, light_level;
  int zoom_level = 100;

  // calculate the zoom level
  if (objs[aa].flags & OBJF_USEROOMSCALING) {
    int onarea = get_walkable_area_at_location(objs[aa].x, objs[aa].y);

    if ((onarea <= 0) && (thisroom.walk_area_zoom[0] == 0)) {
      // just off the edge of an area -- use the scaling we had
      // while on the area
      zoom_level = objs[aa].last_zoom;
    }
    else
      zoom_level = get_area_scaling(onarea, objs[aa].x, objs[aa].y);

    if (zoom_level != 100)
      scale_sprite_size(objs[aa].num, zoom_level, &sprwidth, &sprheight);
    
  }
  // save the zoom level for next time
  objs[aa].last_zoom = zoom_level;

  // save width/height into parameters if requested
  if (drawnWidth)
    *drawnWidth = sprwidth;
  if (drawnHeight)
    *drawnHeight = sprheight;

  objs[aa].last_width = sprwidth;
  objs[aa].last_height = sprheight;

  if (objs[aa].flags & OBJF_HASTINT) {
    // object specific tint, use it
    tint_red = objs[aa].tint_r;
    tint_green = objs[aa].tint_g;
    tint_blue = objs[aa].tint_b;
    tint_level = objs[aa].tint_level;
    tint_light = objs[aa].tint_light;
    light_level = 0;
  }
  else {
    // get the ambient or region tint
    int ignoreRegionTints = 1;
    if (objs[aa].flags & OBJF_USEREGIONTINTS)
      ignoreRegionTints = 0;

    get_local_tint(objs[aa].x, objs[aa].y, ignoreRegionTints,
      &tint_level, &tint_red, &tint_green, &tint_blue,
      &tint_light, &light_level);
  }

  // check whether the image should be flipped
  int isMirrored = 0;
  if ( (objs[aa].view >= 0) &&
       (views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].pic == objs[aa].num) &&
      ((views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].flags & VFLG_FLIPSPRITE) != 0)) {
    isMirrored = 1;
  }

  if ((objcache[aa].image != NULL) &&
      (objcache[aa].sppic == objs[aa].num) &&
      (walkBehindMethod != DrawOverCharSprite) &&
      (actsps[useindx] != NULL) &&
      (hardwareAccelerated))
  {
    // HW acceleration
    objcache[aa].tintamntwas = tint_level;
    objcache[aa].tintredwas = tint_red;
    objcache[aa].tintgrnwas = tint_green;
    objcache[aa].tintbluwas = tint_blue;
    objcache[aa].tintlightwas = tint_light;
    objcache[aa].lightlevwas = light_level;
    objcache[aa].zoomWas = zoom_level;
    objcache[aa].mirroredWas = isMirrored;

    return 1;
  }

  if ((!hardwareAccelerated) && (gfxDriver->HasAcceleratedStretchAndFlip()))
  {
    // They want to draw it in software mode with the D3D driver,
    // so force a redraw
    objcache[aa].sppic = -389538;
  }

  // If we have the image cached, use it
  if ((objcache[aa].image != NULL) &&
      (objcache[aa].sppic == objs[aa].num) &&
      (objcache[aa].tintamntwas == tint_level) &&
      (objcache[aa].tintlightwas == tint_light) &&
      (objcache[aa].tintredwas == tint_red) &&
      (objcache[aa].tintgrnwas == tint_green) &&
      (objcache[aa].tintbluwas == tint_blue) &&
      (objcache[aa].lightlevwas == light_level) &&
      (objcache[aa].zoomWas == zoom_level) &&
      (objcache[aa].mirroredWas == isMirrored)) {
    // the image is the same, we can use it cached!
    if ((walkBehindMethod != DrawOverCharSprite) &&
        (actsps[useindx] != NULL))
      return 1;
    // Check if the X & Y co-ords are the same, too -- if so, there
    // is scope for further optimisations
    if ((objcache[aa].xwas == objs[aa].x) &&
        (objcache[aa].ywas == objs[aa].y) &&
        (actsps[useindx] != NULL) &&
        (walk_behind_baselines_changed == 0))
      return 1;
    actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, sprwidth, sprheight);
    blit(objcache[aa].image, actsps[useindx], 0, 0, 0, 0, objcache[aa].image->w, objcache[aa].image->h);
    return 0;
  }

  // Not cached, so draw the image

  int actspsUsed = 0;
  if (!hardwareAccelerated)
  {
    // draw the base sprite, scaled and flipped as appropriate
    actspsUsed = scale_and_flip_sprite(useindx, coldept, zoom_level,
                         objs[aa].num, sprwidth, sprheight, isMirrored);
  }
  else
  {
    // ensure actsps exists
    actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, spritewidth[objs[aa].num], spriteheight[objs[aa].num]);
  }

  // direct read from source bitmap, where possible
  block comeFrom = NULL;
  if (!actspsUsed)
    comeFrom = spriteset[objs[aa].num];

  // apply tints or lightenings where appropriate, else just copy
  // the source bitmap
  if (((tint_level > 0) || (light_level != 0)) &&
      (!hardwareAccelerated))
  {
    apply_tint_or_light(useindx, light_level, tint_level, tint_red,
                        tint_green, tint_blue, tint_light, coldept,
                        comeFrom);
  }
  else if (!actspsUsed)
    blit(spriteset[objs[aa].num],actsps[useindx],0,0,0,0,spritewidth[objs[aa].num],spriteheight[objs[aa].num]);

  // Re-use the bitmap if it's the same size
  objcache[aa].image = recycle_bitmap(objcache[aa].image, coldept, sprwidth, sprheight);

  // Create the cached image and store it
  blit(actsps[useindx], objcache[aa].image, 0, 0, 0, 0, sprwidth, sprheight);

  objcache[aa].sppic = objs[aa].num;
  objcache[aa].tintamntwas = tint_level;
  objcache[aa].tintredwas = tint_red;
  objcache[aa].tintgrnwas = tint_green;
  objcache[aa].tintbluwas = tint_blue;
  objcache[aa].tintlightwas = tint_light;
  objcache[aa].lightlevwas = light_level;
  objcache[aa].zoomWas = zoom_level;
  objcache[aa].mirroredWas = isMirrored;
  return 0;
}

int GetScalingAt (int x, int y) {
  int onarea = get_walkable_area_pixel(x, y);
  if (onarea < 0)
    return 100;

  return get_area_scaling (onarea, x, y);
}

void SetAreaScaling(int area, int min, int max) {
  if ((area < 0) || (area > MAX_WALK_AREAS))
    quit("!SetAreaScaling: invalid walkalbe area");

  if (min > max)
    quit("!SetAreaScaling: min > max");

  if ((min < 5) || (max < 5) || (min > 200) || (max > 200))
    quit("!SetAreaScaling: min and max must be in range 5-200");

  // the values are stored differently
  min -= 100;
  max -= 100;
  
  if (min == max) {
    thisroom.walk_area_zoom[area] = min;
    thisroom.walk_area_zoom2[area] = NOT_VECTOR_SCALED;
  }
  else {
    thisroom.walk_area_zoom[area] = min;
    thisroom.walk_area_zoom2[area] = max;
  }
}

// This is only called from draw_screen_background, but it's seperated
// to help with profiling the program
void prepare_objects_for_drawing() {
  int aa,atxp,atyp,useindx;
  our_eip=32;

  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on != 1) continue;
    // offscreen, don't draw
    if ((objs[aa].x >= thisroom.width) || (objs[aa].y < 1))
      continue;

    useindx = aa;
    int tehHeight;

    int actspsIntact = construct_object_gfx(aa, NULL, &tehHeight, false);

    // update the cache for next time
    objcache[aa].xwas = objs[aa].x;
    objcache[aa].ywas = objs[aa].y;

    atxp = multiply_up_coordinate(objs[aa].x) - offsetx;
    atyp = (multiply_up_coordinate(objs[aa].y) - tehHeight) - offsety;

    int usebasel = objs[aa].get_baseline();

    if (objs[aa].flags & OBJF_NOWALKBEHINDS) {
      // ignore walk-behinds, do nothing
      if (walkBehindMethod == DrawAsSeparateSprite)
      {
        usebasel += thisroom.height;
      }
    }
    else if (walkBehindMethod == DrawAsSeparateCharSprite) 
    {
      sort_out_char_sprite_walk_behind(useindx, atxp+offsetx, atyp+offsety, usebasel, objs[aa].last_zoom, objs[aa].last_width, objs[aa].last_height);
    }
    else if ((!actspsIntact) && (walkBehindMethod == DrawOverCharSprite))
    {
      sort_out_walk_behinds(actsps[useindx],atxp+offsetx,atyp+offsety,usebasel);
    }

    if ((!actspsIntact) || (actspsbmp[useindx] == NULL))
    {
      bool hasAlpha = (game.spriteflags[objs[aa].num] & SPF_ALPHACHANNEL) != 0;

      if (actspsbmp[useindx] != NULL)
        gfxDriver->DestroyDDB(actspsbmp[useindx]);
      actspsbmp[useindx] = gfxDriver->CreateDDBFromBitmap(actsps[useindx], hasAlpha);
    }

    if (gfxDriver->HasAcceleratedStretchAndFlip())
    {
      actspsbmp[useindx]->SetFlippedLeftRight(objcache[aa].mirroredWas != 0);
      actspsbmp[useindx]->SetStretch(objs[aa].last_width, objs[aa].last_height);
      actspsbmp[useindx]->SetTint(objcache[aa].tintredwas, objcache[aa].tintgrnwas, objcache[aa].tintbluwas, (objcache[aa].tintamntwas * 256) / 100);

      if (objcache[aa].tintamntwas > 0)
      {
        if (objcache[aa].tintlightwas == 0)  // luminance of 0 -- pass 1 to enable
          actspsbmp[useindx]->SetLightLevel(1);
        else if (objcache[aa].tintlightwas < 250)
          actspsbmp[useindx]->SetLightLevel(objcache[aa].tintlightwas);
        else
          actspsbmp[useindx]->SetLightLevel(0);
      }
      else if (objcache[aa].lightlevwas != 0)
        actspsbmp[useindx]->SetLightLevel((objcache[aa].lightlevwas * 25) / 10 + 256);
      else
        actspsbmp[useindx]->SetLightLevel(0);
    }

    add_to_sprite_list(actspsbmp[useindx],atxp,atyp,usebasel,objs[aa].transparent,objs[aa].num);
  }

}




// Draws srcimg onto destimg, tinting to the specified level
// Totally overwrites the contents of the destination image
void tint_image (block srcimg, block destimg, int red, int grn, int blu, int light_level, int luminance) {

  if ((bitmap_color_depth(srcimg) != bitmap_color_depth(destimg)) ||
      (bitmap_color_depth(srcimg) <= 8)) {
    debug_log("Image tint failed - images must both be hi-color");
    // the caller expects something to have been copied
    blit(srcimg, destimg, 0, 0, 0, 0, srcimg->w, srcimg->h);
    return;
  }

  // For performance reasons, we have a seperate blender for
  // when light is being adjusted and when it is not.
  // If luminance >= 250, then normal brightness, otherwise darken
  if (luminance >= 250)
    set_blender_mode (_myblender_color15, _myblender_color16, _myblender_color32, red, grn, blu, 0);
  else
    set_blender_mode (_myblender_color15_light, _myblender_color16_light, _myblender_color32_light, red, grn, blu, 0);

  if (light_level >= 100) {
    // fully colourised
    clear_to_color(destimg, bitmap_mask_color(destimg));
    draw_lit_sprite(destimg, srcimg, 0, 0, luminance);
  }
  else {
    // light_level is between -100 and 100 normally; 0-100 in
    // this case when it's a RGB tint
    light_level = (light_level * 25) / 10;

    // Copy the image to the new bitmap
    blit(srcimg, destimg, 0, 0, 0, 0, srcimg->w, srcimg->h);
    // Render the colourised image to a temporary bitmap,
    // then transparently draw it over the original image
    block finaltarget = create_bitmap_ex(bitmap_color_depth(srcimg), srcimg->w, srcimg->h);
    clear_to_color(finaltarget, bitmap_mask_color(finaltarget));
    draw_lit_sprite(finaltarget, srcimg, 0, 0, luminance);

    // customized trans blender to preserve alpha channel
    set_my_trans_blender (0, 0, 0, light_level);
    draw_trans_sprite (destimg, finaltarget, 0, 0);
    destroy_bitmap (finaltarget);
  }
}

void prepare_characters_for_drawing() {
  int zoom_level,newwidth,newheight,onarea,sppic,atxp,atyp,useindx;
  int light_level,coldept,aa;
  int tint_red, tint_green, tint_blue, tint_amount, tint_light = 255;

  our_eip=33;
  // draw characters
  for (aa=0;aa<game.numcharacters;aa++) {
    if (game.chars[aa].on==0) continue;
    if (game.chars[aa].room!=displayed_room) continue;
    eip_guinum = aa;
    useindx = aa + MAX_INIT_SPR;

    CharacterInfo*chin=&game.chars[aa];
    our_eip = 330;
    // if it's on but set to view -1, they're being silly
    if (chin->view < 0) {
      quitprintf("!The character '%s' was turned on in the current room (room %d) but has not been assigned a view number.",
        chin->name, displayed_room);
    }

    if (chin->frame >= views[chin->view].loops[chin->loop].numFrames)
      chin->frame = 0;

    if ((chin->loop >= views[chin->view].numLoops) ||
        (views[chin->view].loops[chin->loop].numFrames < 1)) {
      quitprintf("!The character '%s' could not be displayed because there were no frames in loop %d of view %d.",
        chin->name, chin->loop, chin->view + 1);
    }

    sppic=views[chin->view].loops[chin->loop].frames[chin->frame].pic;
    if ((sppic < 0) || (sppic >= MAX_SPRITES))
      sppic = 0;  // in case it's screwed up somehow
    our_eip = 331;
    // sort out the stretching if required
    onarea = get_walkable_area_at_character (aa);
    our_eip = 332;
    light_level = 0;
    tint_amount = 0;
     
    if (chin->flags & CHF_MANUALSCALING)  // character ignores scaling
      zoom_level = charextra[aa].zoom;
    else if ((onarea <= 0) && (thisroom.walk_area_zoom[0] == 0)) {
      zoom_level = charextra[aa].zoom;
      if (zoom_level == 0)
        zoom_level = 100;
    }
    else
      zoom_level = get_area_scaling (onarea, chin->x, chin->y);

    charextra[aa].zoom = zoom_level;

    if (chin->flags & CHF_HASTINT) {
      // object specific tint, use it
      tint_red = charextra[aa].tint_r;
      tint_green = charextra[aa].tint_g;
      tint_blue = charextra[aa].tint_b;
      tint_amount = charextra[aa].tint_level;
      tint_light = charextra[aa].tint_light;
      light_level = 0;
    }
    else {
      get_local_tint(chin->x, chin->y, chin->flags & CHF_NOLIGHTING,
        &tint_amount, &tint_red, &tint_green, &tint_blue,
        &tint_light, &light_level);
    }

    /*if (actsps[useindx]!=NULL) {
      wfreeblock(actsps[useindx]);
      actsps[useindx] = NULL;
    }*/

    our_eip = 3330;
    int isMirrored = 0, specialpic = sppic;
    bool usingCachedImage = false;

    coldept = bitmap_color_depth(spriteset[sppic]);

    // adjust the sppic if mirrored, so it doesn't accidentally
    // cache the mirrored frame as the real one
    if (views[chin->view].loops[chin->loop].frames[chin->frame].flags & VFLG_FLIPSPRITE) {
      isMirrored = 1;
      specialpic = -sppic;
    }

    our_eip = 3331;

    // if the character was the same sprite and scaling last time,
    // just use the cached image
    if ((charcache[aa].inUse) &&
        (charcache[aa].sppic == specialpic) &&
        (charcache[aa].scaling == zoom_level) &&
        (charcache[aa].tintredwas == tint_red) &&
        (charcache[aa].tintgrnwas == tint_green) &&
        (charcache[aa].tintbluwas == tint_blue) &&
        (charcache[aa].tintamntwas == tint_amount) &&
        (charcache[aa].tintlightwas == tint_light) &&
        (charcache[aa].lightlevwas == light_level)) 
    {
      if (walkBehindMethod == DrawOverCharSprite)
      {
        actsps[useindx] = recycle_bitmap(actsps[useindx], bitmap_color_depth(charcache[aa].image), charcache[aa].image->w, charcache[aa].image->h);
        blit (charcache[aa].image, actsps[useindx], 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);
      }
      else 
      {
        usingCachedImage = true;
      }
    }
    else if ((charcache[aa].inUse) && 
             (charcache[aa].sppic == specialpic) &&
             (gfxDriver->HasAcceleratedStretchAndFlip()))
    {
      usingCachedImage = true;
    }
    else if (charcache[aa].inUse) {
      //destroy_bitmap (charcache[aa].image);
      charcache[aa].inUse = 0;
    }

    our_eip = 3332;
    
    if (zoom_level != 100) {
      // it needs to be stretched, so calculate the new dimensions

      scale_sprite_size(sppic, zoom_level, &newwidth, &newheight);
      charextra[aa].width=newwidth;
      charextra[aa].height=newheight;
    }
    else {
      // draw at original size, so just use the sprite width and height
      charextra[aa].width=0;
      charextra[aa].height=0;
      newwidth = spritewidth[sppic];
      newheight = spriteheight[sppic];
    }

    our_eip = 3336;

    // Calculate the X & Y co-ordinates of where the sprite will be
    atxp=(multiply_up_coordinate(chin->x) - offsetx) - newwidth/2;
    atyp=(multiply_up_coordinate(chin->y) - newheight) - offsety;

    charcache[aa].scaling = zoom_level;
    charcache[aa].sppic = specialpic;
    charcache[aa].tintredwas = tint_red;
    charcache[aa].tintgrnwas = tint_green;
    charcache[aa].tintbluwas = tint_blue;
    charcache[aa].tintamntwas = tint_amount;
    charcache[aa].tintlightwas = tint_light;
    charcache[aa].lightlevwas = light_level;

    // If cache needs to be re-drawn
    if (!charcache[aa].inUse) {

      // create the base sprite in actsps[useindx], which will
      // be scaled and/or flipped, as appropriate
      int actspsUsed = 0;
      if (!gfxDriver->HasAcceleratedStretchAndFlip())
      {
        actspsUsed = scale_and_flip_sprite(
                            useindx, coldept, zoom_level, sppic,
                            newwidth, newheight, isMirrored);
      }
      else 
      {
        // ensure actsps exists
        actsps[useindx] = recycle_bitmap(actsps[useindx], coldept, spritewidth[sppic], spriteheight[sppic]);
      }

      our_eip = 335;

      if (((light_level != 0) || (tint_amount != 0)) &&
          (!gfxDriver->HasAcceleratedStretchAndFlip())) {
        // apply the lightening or tinting
        block comeFrom = NULL;
        // if possible, direct read from the source image
        if (!actspsUsed)
          comeFrom = spriteset[sppic];

        apply_tint_or_light(useindx, light_level, tint_amount, tint_red,
                            tint_green, tint_blue, tint_light, coldept,
                            comeFrom);
      }
      else if (!actspsUsed)
        // no scaling, flipping or tinting was done, so just blit it normally
        blit (spriteset[sppic], actsps[useindx], 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);

      // update the character cache with the new image
      charcache[aa].inUse = 1;
      //charcache[aa].image = create_bitmap_ex (coldept, actsps[useindx]->w, actsps[useindx]->h);
      charcache[aa].image = recycle_bitmap(charcache[aa].image, coldept, actsps[useindx]->w, actsps[useindx]->h);
      blit (actsps[useindx], charcache[aa].image, 0, 0, 0, 0, actsps[useindx]->w, actsps[useindx]->h);

    } // end if !cache.inUse

    int usebasel = chin->get_baseline();

    // adjust the Y positioning for the character's Z co-ord
    atyp -= multiply_up_coordinate(chin->z);

    our_eip = 336;

    int bgX = atxp + offsetx + chin->pic_xoffs;
    int bgY = atyp + offsety + chin->pic_yoffs;

    if (chin->flags & CHF_NOWALKBEHINDS) {
      // ignore walk-behinds, do nothing
      if (walkBehindMethod == DrawAsSeparateSprite)
      {
        usebasel += thisroom.height;
      }
    }
    else if (walkBehindMethod == DrawAsSeparateCharSprite) 
    {
      sort_out_char_sprite_walk_behind(useindx, bgX, bgY, usebasel, charextra[aa].zoom, newwidth, newheight);
    }
    else if (walkBehindMethod == DrawOverCharSprite)
    {
      sort_out_walk_behinds(actsps[useindx], bgX, bgY, usebasel);
    }

    if ((!usingCachedImage) || (actspsbmp[useindx] == NULL))
    {
      bool hasAlpha = (game.spriteflags[sppic] & SPF_ALPHACHANNEL) != 0;

      actspsbmp[useindx] = recycle_ddb_bitmap(actspsbmp[useindx], actsps[useindx], hasAlpha);
    }

    if (gfxDriver->HasAcceleratedStretchAndFlip()) 
    {
      actspsbmp[useindx]->SetStretch(newwidth, newheight);
      actspsbmp[useindx]->SetFlippedLeftRight(isMirrored != 0);
      actspsbmp[useindx]->SetTint(tint_red, tint_green, tint_blue, (tint_amount * 256) / 100);

      if (tint_amount != 0)
      {
        if (tint_light == 0) // tint with 0 luminance, pass as 1 instead
          actspsbmp[useindx]->SetLightLevel(1);
        else if (tint_light < 250)
          actspsbmp[useindx]->SetLightLevel(tint_light);
        else
          actspsbmp[useindx]->SetLightLevel(0);
      }
      else if (light_level != 0)
        actspsbmp[useindx]->SetLightLevel((light_level * 25) / 10 + 256);
      else
        actspsbmp[useindx]->SetLightLevel(0);

    }

    our_eip = 337;
    // disable alpha blending with tinted sprites (because the
    // alpha channel was lost in the tinting process)
    //if (((tint_level) && (tint_amount < 100)) || (light_level))
      //sppic = -1;
    add_to_sprite_list(actspsbmp[useindx], atxp + chin->pic_xoffs, atyp + chin->pic_yoffs, usebasel, chin->transparency, sppic);

    chin->actx=atxp+offsetx;
    chin->acty=atyp+offsety;
  }
}

// draw_screen_background: draws the background scene, all the interfaces
// and objects; basically, the entire screen
void draw_screen_background() {

  static int offsetxWas = -100, offsetyWas = -100;

  screen_reset = 1;

  if (is_complete_overlay) {
    // this is normally called as part of drawing sprites - clear it
    // here instead
    clear_draw_list();
    return;
  }

  // don't draw it before the room fades in
/*  if ((in_new_room > 0) & (game.color_depth > 1)) {
    clear(abuf);
    return;
    }*/
  our_eip=30;
  update_viewport();
  
  our_eip=31;

  if ((offsetx != offsetxWas) || (offsety != offsetyWas)) {
    invalidate_screen();

    offsetxWas = offsetx;
    offsetyWas = offsety;
  }

  if (play.screen_tint >= 0)
    invalidate_screen();

  if (gfxDriver->RequiresFullRedrawEachFrame())
  {
    if (roomBackgroundBmp == NULL) 
    {
      update_polled_stuff();
      roomBackgroundBmp = gfxDriver->CreateDDBFromBitmap(thisroom.ebscene[play.bg_frame], false, true);

      if ((walkBehindMethod == DrawAsSeparateSprite) && (walkBehindsCachedForBgNum != play.bg_frame))
      {
        update_walk_behind_images();
      }
    }
    else if (current_background_is_dirty)
    {
      update_polled_stuff();
      gfxDriver->UpdateDDBFromBitmap(roomBackgroundBmp, thisroom.ebscene[play.bg_frame], false);
      current_background_is_dirty = false;
      if (walkBehindMethod == DrawAsSeparateSprite)
      {
        update_walk_behind_images();
      }
    }
    gfxDriver->DrawSprite(-offsetx, -offsety, roomBackgroundBmp);
  }
  else
  {
    // the following line takes up to 50% of the game CPU time at
    // high resolutions and colour depths - if we can optimise it
    // somehow, significant performance gains to be had
    update_invalid_region_and_reset(-offsetx, -offsety, thisroom.ebscene[play.bg_frame], abuf);
  }

  clear_sprite_list();

  if ((debug_flags & DBG_NOOBJECTS)==0) {

    prepare_objects_for_drawing();

    prepare_characters_for_drawing ();

    if ((debug_flags & DBG_NODRAWSPRITES)==0) {
      our_eip=34;
      draw_sprite_list();
    }
  }
  our_eip=36;
}

void get_script_name(ccInstance *rinst, char *curScrName) {
  if (rinst == NULL)
    strcpy (curScrName, "Not in a script");
  else if (rinst->instanceof == gamescript)
    strcpy (curScrName, "Global script");
  else if (rinst->instanceof == thisroom.compiled_script)
    sprintf (curScrName, "Room %d script", displayed_room);
  else
    strcpy (curScrName, "Unknown script");
}

void update_gui_zorder() {
  int numdone = 0, b;
  
  // for each GUI
  for (int a = 0; a < game.numgui; a++) {
    // find the right place in the draw order array
    int insertAt = numdone;
    for (b = 0; b < numdone; b++) {
      if (guis[a].zorder < guis[play.gui_draw_order[b]].zorder) {
        insertAt = b;
        break;
      }
    }
    // insert the new item
    for (b = numdone - 1; b >= insertAt; b--)
      play.gui_draw_order[b + 1] = play.gui_draw_order[b];
    play.gui_draw_order[insertAt] = a;
    numdone++;
  }

}

void get_overlay_position(int overlayidx, int *x, int *y) {
  int tdxp, tdyp;

  if (screenover[overlayidx].x == OVR_AUTOPLACE) {
    // auto place on character
    int charid = screenover[overlayidx].y;
    int charpic = views[game.chars[charid].view].loops[game.chars[charid].loop].frames[0].pic;
    
    tdyp = multiply_up_coordinate(game.chars[charid].get_effective_y()) - offsety - 5;
    if (charextra[charid].height<1)
      tdyp -= spriteheight[charpic];
    else
      tdyp -= charextra[charid].height;

    tdyp -= screenover[overlayidx].pic->h;
    if (tdyp < 5) tdyp=5;
    tdxp = (multiply_up_coordinate(game.chars[charid].x) - screenover[overlayidx].pic->w/2) - offsetx;
    if (tdxp < 0) tdxp=0;

    if ((tdxp + screenover[overlayidx].pic->w) >= scrnwid)
      tdxp = (scrnwid - screenover[overlayidx].pic->w) - 1;
    if (game.chars[charid].room != displayed_room) {
      tdxp = scrnwid/2 - screenover[overlayidx].pic->w/2;
      tdyp = scrnhit/2 - screenover[overlayidx].pic->h/2;
    }
  }
  else {
    tdxp = screenover[overlayidx].x;
    tdyp = screenover[overlayidx].y;

    if (!screenover[overlayidx].positionRelativeToScreen)
    {
      tdxp -= offsetx;
      tdyp -= offsety;
    }
  }
  *x = tdxp;
  *y = tdyp;
}

void draw_fps()
{
  static IDriverDependantBitmap* ddb = NULL;
  static block fpsDisplay = NULL;

  if (fpsDisplay == NULL)
  {
    fpsDisplay = create_bitmap_ex(final_col_dep, get_fixed_pixel_size(100), (wgetfontheight(FONT_SPEECH) + get_fixed_pixel_size(5)));
    fpsDisplay = gfxDriver->ConvertBitmapToSupportedColourDepth(fpsDisplay);
  }
  clear_to_color(fpsDisplay, bitmap_mask_color(fpsDisplay));
  block oldAbuf = abuf;
  abuf = fpsDisplay;
  char tbuffer[60];
  sprintf(tbuffer,"FPS: %d",fps);
  wtextcolor(14);
  wouttext_outline(1, 1, FONT_SPEECH, tbuffer);
  abuf = oldAbuf;

  if (ddb == NULL)
    ddb = gfxDriver->CreateDDBFromBitmap(fpsDisplay, false);
  else
    gfxDriver->UpdateDDBFromBitmap(ddb, fpsDisplay, false);

  int yp = scrnhit - fpsDisplay->h;

  gfxDriver->DrawSprite(1, yp, ddb);
  invalidate_sprite(1, yp, ddb);

  sprintf(tbuffer,"Loop %ld", loopcounter);
  draw_and_invalidate_text(get_fixed_pixel_size(250), yp, FONT_SPEECH,tbuffer);
}

// draw_screen_overlay: draws any stuff currently on top of the background,
// like a message box or popup interface
void draw_screen_overlay() {
  int gg;

  add_thing_to_draw(NULL, AGSE_PREGUIDRAW, 0, TRANS_RUN_PLUGIN, false);

  // draw overlays, except text boxes
  for (gg=0;gg<numscreenover;gg++) {
    // complete overlay draw in non-transparent mode
    if (screenover[gg].type == OVER_COMPLETE)
      add_thing_to_draw(screenover[gg].bmp, screenover[gg].x, screenover[gg].y, TRANS_OPAQUE, false);
    else if (screenover[gg].type != OVER_TEXTMSG) {
      int tdxp, tdyp;
      get_overlay_position(gg, &tdxp, &tdyp);
      add_thing_to_draw(screenover[gg].bmp, tdxp, tdyp, 0, screenover[gg].hasAlphaChannel);
    }
  }

  // Draw GUIs - they should always be on top of overlays like
  // speech background text
  our_eip=35;
  mouse_on_iface_button=-1;
  if (((debug_flags & DBG_NOIFACE)==0) && (displayed_room >= 0)) {
    int aa;

    if (playerchar->activeinv >= MAX_INV) {
      quit("!The player.activeinv variable has been corrupted, probably as a result\n"
       "of an incorrect assignment in the game script.");
    }
    if (playerchar->activeinv < 1) gui_inv_pic=-1;
    else gui_inv_pic=game.invinfo[playerchar->activeinv].pic;
/*    for (aa=0;aa<game.numgui;aa++) {
      if (guis[aa].on<1) continue;
      guis[aa].draw();
      guis[aa].poll();
      }*/
    our_eip = 37;
    if (guis_need_update) {
      block abufwas = abuf;
      guis_need_update = 0;
      for (aa=0;aa<game.numgui;aa++) {
        if (guis[aa].on<1) continue;
        eip_guinum = aa;
        our_eip = 370;
        clear_to_color (guibg[aa], bitmap_mask_color(guibg[aa]));
        abuf = guibg[aa];
        our_eip = 372;
        guis[aa].draw_at(0,0);
        our_eip = 373;

        bool isAlpha = false;
        if (guis[aa].is_alpha()) 
        {
          isAlpha = true;

          if ((game.options[OPT_NEWGUIALPHA] == 0) && (guis[aa].bgpic > 0))
          {
            // old-style (pre-3.0.2) GUI alpha rendering
            repair_alpha_channel(guibg[aa], spriteset[guis[aa].bgpic]);
          }
        }

        if (guibgbmp[aa] != NULL) 
        {
          gfxDriver->UpdateDDBFromBitmap(guibgbmp[aa], guibg[aa], isAlpha);
        }
        else
        {
          guibgbmp[aa] = gfxDriver->CreateDDBFromBitmap(guibg[aa], isAlpha);
        }
        our_eip = 374;
      }
      abuf = abufwas;
    }
    our_eip = 38;
    // Draw the GUIs
    for (gg = 0; gg < game.numgui; gg++) {
      aa = play.gui_draw_order[gg];
      if (guis[aa].on < 1) continue;

      // Don't draw GUI if "GUIs Turn Off When Disabled"
      if ((game.options[OPT_DISABLEOFF] == 3) &&
          (all_buttons_disabled > 0) &&
          (guis[aa].popup != POPUP_NOAUTOREM))
        continue;

      add_thing_to_draw(guibgbmp[aa], guis[aa].x, guis[aa].y, guis[aa].transparency, guis[aa].is_alpha());
      
      // only poll if the interface is enabled (mouseovers should not
      // work while in Wait state)
      if (IsInterfaceEnabled())
        guis[aa].poll();
    }
  }

  // draw text boxes (so that they appear over GUIs)
  for (gg=0;gg<numscreenover;gg++) 
  {
    if (screenover[gg].type == OVER_TEXTMSG) 
    {
      int tdxp, tdyp;
      get_overlay_position(gg, &tdxp, &tdyp);
      add_thing_to_draw(screenover[gg].bmp, tdxp, tdyp, 0, false);
    }
  }

  our_eip = 1099;

  // *** Draw the Things To Draw List ***

  SpriteListEntry *thisThing;

  for (gg = 0; gg < thingsToDrawSize; gg++) {
    thisThing = &thingsToDrawList[gg];

    if (thisThing->bmp != NULL) {
      // mark the image's region as dirty
      invalidate_sprite(thisThing->x, thisThing->y, thisThing->bmp);
    }
    else if ((thisThing->transparent != TRANS_RUN_PLUGIN) &&
      (thisThing->bmp == NULL)) 
    {
      quit("Null pointer added to draw list");
    }
    
    if (thisThing->bmp != NULL)
    {
      if (thisThing->transparent <= 255)
      {
        thisThing->bmp->SetTransparency(thisThing->transparent);
      }

      gfxDriver->DrawSprite(thisThing->x, thisThing->y, thisThing->bmp);
    }
    else if (thisThing->transparent == TRANS_RUN_PLUGIN) 
    {
      // meta entry to run the plugin hook
      gfxDriver->DrawSprite(thisThing->x, thisThing->y, NULL);
    }
    else
      quit("Unknown entry in draw list");
  }

  clear_draw_list();

  our_eip = 1100;


  if (display_fps) 
  {
    draw_fps();
  }
/*
  if (channels[SCHAN_SPEECH] != NULL) {
    
    char tbuffer[60];
    sprintf(tbuffer,"mpos: %d", channels[SCHAN_SPEECH]->get_pos_ms());
    write_log(tbuffer);
    int yp = scrnhit - (wgetfontheight(FONT_SPEECH) + 25 * symult);
    wtextcolor(14);
    draw_and_invalidate_text(1, yp, FONT_SPEECH,tbuffer);
  }*/

  if (play.recording) {
    // Flash "REC" while recording
    wtextcolor (12);
    //if ((loopcounter % (frames_per_second * 2)) > frames_per_second/2) {
      char tformat[30];
      sprintf (tformat, "REC %02d:%02d:%02d", replay_time / 3600, (replay_time % 3600) / 60, replay_time % 60);
      draw_and_invalidate_text(get_fixed_pixel_size(5), get_fixed_pixel_size(10), FONT_SPEECH, tformat);
    //}
  }
  else if (play.playback) {
    wtextcolor (10);
    char tformat[30];
    sprintf (tformat, "PLAY %02d:%02d:%02d", replay_time / 3600, (replay_time % 3600) / 60, replay_time % 60);

    draw_and_invalidate_text(get_fixed_pixel_size(5), get_fixed_pixel_size(10), FONT_SPEECH, tformat);
  }

  our_eip = 1101;
}

bool GfxDriverNullSpriteCallback(int x, int y)
{
  if (displayed_room < 0)
  {
    // if no room loaded, various stuff won't be initialized yet
    return 1;
  }
  return (platform->RunPluginHooks(x, y) != 0);
}

void GfxDriverOnInitCallback(void *data)
{
  platform->RunPluginInitGfxHooks(gfxDriver->GetDriverID(), data);
}

void SeekMIDIPosition (int position) {
  if (play.silent_midi)
    midi_seek (position);
  if (current_music_type == MUS_MIDI) {
    midi_seek(position);
    DEBUG_CONSOLE("Seek MIDI position to %d", position);
  }
}

int GetMIDIPosition () {
  if (play.silent_midi)
    return midi_pos;
  if (current_music_type != MUS_MIDI)
    return -1;
  if (play.fast_forward)
    return 99999;

  return midi_pos;
}


int get_hotspot_at(int xpp,int ypp) {
  int onhs=getpixel(thisroom.lookat, convert_to_low_res(xpp), convert_to_low_res(ypp));
  if (onhs<0) return 0;
  if (croom->hotspot_enabled[onhs]==0) return 0;
  return onhs;
}

int numOnStack = 0;
block screenstack[10];
void push_screen () {
  if (numOnStack >= 10)
    quit("!Too many push screen calls");

  screenstack[numOnStack] = abuf;
  numOnStack++;
}
void pop_screen() {
  if (numOnStack <= 0)
    quit("!Too many pop screen calls");
  numOnStack--;
  wsetscreen(screenstack[numOnStack]);
}


// update_screen: copies the contents of the virtual screen to the actual
// screen, and draws the mouse cursor on.
void update_screen() {
  // cos hi-color doesn't fade in, don't draw it the first time
  if ((in_new_room > 0) & (game.color_depth > 1))
    return;
  gfxDriver->DrawSprite(AGSE_POSTSCREENDRAW, 0, NULL);

  // update animating mouse cursor
  if (game.mcurs[cur_cursor].view>=0) {
    domouse (DOMOUSE_NOCURSOR);
    // only on mousemove, and it's not moving
    if (((game.mcurs[cur_cursor].flags & MCF_ANIMMOVE)!=0) &&
      (mousex==lastmx) && (mousey==lastmy)) ;
    // only on hotspot, and it's not on one
    else if (((game.mcurs[cur_cursor].flags & MCF_HOTSPOT)!=0) &&
        (GetLocationType(divide_down_coordinate(mousex), divide_down_coordinate(mousey)) == 0))
      set_new_cursor_graphic(game.mcurs[cur_cursor].pic);
    else if (mouse_delay>0) mouse_delay--;
    else {
      int viewnum=game.mcurs[cur_cursor].view;
      int loopnum=0;
      if (loopnum >= views[viewnum].numLoops)
        quitprintf("An animating mouse cursor is using view %d which has no loops", viewnum + 1);
      if (views[viewnum].loops[loopnum].numFrames < 1)
        quitprintf("An animating mouse cursor is using view %d which has no frames in loop %d", viewnum + 1, loopnum);

      mouse_frame++;
      if (mouse_frame >= views[viewnum].loops[loopnum].numFrames)
        mouse_frame=0;
      set_new_cursor_graphic(views[viewnum].loops[loopnum].frames[mouse_frame].pic);
      mouse_delay = views[viewnum].loops[loopnum].frames[mouse_frame].speed + 5;
      CheckViewFrame (viewnum, loopnum, mouse_frame);
    }
    lastmx=mousex; lastmy=mousey;
  }

  // draw the debug console, if appropriate
  if ((play.debug_mode > 0) && (display_console != 0)) 
  {
    int otextc = textcol, ypp = 1;
    int txtheight = wgetfontheight(0);
    int barheight = (DEBUG_CONSOLE_NUMLINES - 1) * txtheight + 4;

    if (debugConsoleBuffer == NULL)
      debugConsoleBuffer = create_bitmap_ex(final_col_dep, scrnwid, barheight);

    push_screen();
    abuf = debugConsoleBuffer;
    wsetcolor(15);
    wbar (0, 0, scrnwid - 1, barheight);
    wtextcolor(16);
    for (int jj = first_debug_line; jj != last_debug_line; jj = (jj + 1) % DEBUG_CONSOLE_NUMLINES) {
      wouttextxy(1, ypp, 0, debug_line[jj].text);
      wouttextxy(scrnwid - get_fixed_pixel_size(40), ypp, 0, debug_line[jj].script);
      ypp += txtheight;
    }
    textcol = otextc;
    pop_screen();

    if (debugConsole == NULL)
      debugConsole = gfxDriver->CreateDDBFromBitmap(debugConsoleBuffer, false, true);
    else
      gfxDriver->UpdateDDBFromBitmap(debugConsole, debugConsoleBuffer, false);

    gfxDriver->DrawSprite(0, 0, debugConsole);
    invalidate_sprite(0, 0, debugConsole);
  }

  domouse(DOMOUSE_NOCURSOR);

  if (!play.mouse_cursor_hidden)
  {
    gfxDriver->DrawSprite(mousex - hotx, mousey - hoty, mouseCursor);
    invalidate_sprite(mousex - hotx, mousey - hoty, mouseCursor);
  }

  /*
  domouse(1);
  // if the cursor is hidden, remove it again. However, it needs
  // to go on-off in order to update the stored mouse coordinates
  if (play.mouse_cursor_hidden)
    domouse(2);*/

  write_screen();

  wsetscreen(virtual_screen);

  if (!play.screen_is_faded_out) {
    // always update the palette, regardless of whether the plugin
    // vetos the screen update
    if (bg_just_changed) {
      setpal ();
      bg_just_changed = 0;
    }
  }

  //if (!play.mouse_cursor_hidden)
//    domouse(2);

  screen_is_dirty = 0;
}

const char *get_engine_version() {
  return ACI_VERSION_TEXT;
}

void atexit_handler() {
  if (proper_exit==0) {
    sprintf(pexbuf,"\nError: the program has exited without requesting it.\n"
      "Program pointer: %+03d  (write this number down), ACI version " ACI_VERSION_TEXT "\n"
      "If you see a list of numbers above, please write them down and contact\n"
      "Chris Jones. Otherwise, note down any other information displayed.\n",
      our_eip);
    platform->DisplayAlert(pexbuf);
  }

  if (!(music_file == NULL))
    free(music_file);

  if (!(speech_file == NULL))
    free(speech_file);

  // Deliberately commented out, because chances are game_file_name
  // was not allocated on the heap, it points to argv[0] or
  // the gamefilenamebuf memory
  // It will get freed by the system anyway, leaving it in can
  // cause a crash on exit
  /*if (!(game_file_name == NULL))
    free(game_file_name);*/
}

void start_recording() {
  if (play.playback) {
    play.recording = 0;  // stop quit() crashing
    play.playback = 0;
    quit("!playback and recording of replay selected simultaneously");
  }

  srand (play.randseed);
  play.gamestep = 0;

  recbuffersize = 10000;
  recordbuffer = (short*)malloc (recbuffersize * sizeof(short));
  recsize = 0;
  memset (playback_keystate, -1, KEY_MAX);
  replay_last_second = loopcounter;
  replay_time = 0;
  strcpy (replayfile, "New.agr");
}

void start_replay_record () {
  FILE *ott = fopen(replayTempFile, "wb");
  save_game_data (ott, NULL);
  fclose (ott);
  start_recording();
  play.recording = 1;
}

void scStartRecording (int keyToStop) {
  quit("!StartRecording: not et suppotreD");
}

void stop_recording() {
  if (!play.recording)
    return;

  write_record_event (REC_ENDOFFILE, 0, NULL);

  play.recording = 0;
  char replaydesc[100] = "";
  sc_inputbox ("Enter replay description:", replaydesc);
  sc_inputbox ("Enter replay filename:", replayfile);
  if (replayfile[0] == 0)
    strcpy (replayfile, "Untitled");
  if (strchr (replayfile, '.') != NULL)
    strchr (replayfile, '.')[0] = 0;
  strcat (replayfile, ".agr");

  FILE *ooo = fopen(replayfile, "wb");
  fwrite ("AGSRecording", 12, 1, ooo);
  fputstring (ACI_VERSION_TEXT, ooo);
  int write_version = 2;
  FILE *fsr = fopen(replayTempFile, "rb");
  if (fsr != NULL) {
    // There was a save file created
    write_version = 3;
  }
  putw (write_version, ooo);

  fputstring (game.gamename, ooo);
  putw (game.uniqueid, ooo);
  putw (replay_time, ooo);
  fputstring (replaydesc, ooo);  // replay description, maybe we'll use this later
  putw (play.randseed, ooo);
  if (write_version >= 3)
    putw (recsize, ooo);
  fwrite (recordbuffer, recsize, sizeof(short), ooo);
  if (fsr != NULL) {
    putw (1, ooo);  // yes there is a save present
    int lenno = filelength(fileno(fsr));
    char *tbufr = (char*)malloc (lenno);
    fread (tbufr, lenno, 1, fsr);
    fwrite (tbufr, lenno, 1, ooo);
    free (tbufr);
    fclose (fsr);
    unlink (replayTempFile);
  }
  else if (write_version >= 3) {
    putw (0, ooo);
  }
  fclose (ooo);

  free (recordbuffer);
  recordbuffer = NULL;
}

bool send_exception_to_editor(char *qmsg)
{
#ifdef WINDOWS_VERSION
  want_exit = 0;
  // allow the editor to break with the error message
  const char *errorMsgToSend = qmsg;
  if (errorMsgToSend[0] == '?')
    errorMsgToSend++;

  if (editor_window_handle != NULL)
    SetForegroundWindow(editor_window_handle);

  if (!send_message_to_editor("ERROR", errorMsgToSend))
    return false;

  while ((check_for_messages_from_editor() == 0) && (want_exit == 0))
  {
    UPDATE_MP3
    platform->Delay(10);
  }
#endif
  return true;
}

char return_to_roomedit[30] = "\0";
char return_to_room[150] = "\0";
// quit - exits the engine, shutting down everything gracefully
// The parameter is the message to print. If this message begins with
// an '!' character, then it is printed as a "contact game author" error.
// If it begins with a '|' then it is treated as a "thanks for playing" type
// message. If it begins with anything else, it is treated as an internal
// error.
// "!|" is a special code used to mean that the player has aborted (Alt+X)
void quit(char*quitmsg) {
  int i;
  // Need to copy it in case it's from a plugin (since we're
  // about to free plugins)
  char qmsgbufr[STD_BUFFER_SIZE];
  strncpy(qmsgbufr, quitmsg, STD_BUFFER_SIZE);
  qmsgbufr[STD_BUFFER_SIZE - 1] = 0;
  char *qmsg = &qmsgbufr[0];

  bool handledErrorInEditor = false;

  if (editor_debugging_initialized)
  {
    if ((qmsg[0] == '!') && (qmsg[1] != '|'))
    {
      handledErrorInEditor = send_exception_to_editor(&qmsg[1]);
    }
    send_message_to_editor("EXIT");
    editor_debugger->Shutdown();
  }

  our_eip = 9900;
  stop_recording();

  if (need_to_stop_cd)
    cd_manager(3,0);

  our_eip = 9020;
  ccUnregisterAllObjects();

  our_eip = 9019;
  platform->AboutToQuitGame();

  our_eip = 9016;
  platform->ShutdownPlugins();

  if ((qmsg[0] == '|') && (check_dynamic_sprites_at_exit) && 
      (game.options[OPT_DEBUGMODE] != 0)) {
    // game exiting normally -- make sure the dynamic sprites
    // have been deleted
    for (i = 1; i < spriteset.elements; i++) {
      if (game.spriteflags[i] & SPF_DYNAMICALLOC)
        debug_log("Dynamic sprite %d was never deleted", i);
    }
  }

  // allegro_exit assumes screen is correct
  if (_old_screen)
    screen = _old_screen;

  platform->FinishedUsingGraphicsMode();

  if (use_cdplayer)
    platform->ShutdownCDPlayer();

  our_eip = 9917;
  game.options[OPT_CROSSFADEMUSIC] = 0;
  stopmusic();
  if (opts.mod_player)
    remove_mod_player();
  remove_sound();
  our_eip = 9901;

  char alertis[1500]="\0";
  if (qmsg[0]=='|') ; //qmsg++;
  else if (qmsg[0]=='!') { 
    qmsg++;

    if (qmsg[0] == '|')
      strcpy (alertis, "Abort key pressed.\n\n");
    else if (qmsg[0] == '?') {
      strcpy(alertis, "A fatal error has been generated by the script using the AbortGame function. Please contact the game author for support.\n\n");
      qmsg++;
    }
    else
      strcpy(alertis,"An error has occurred. Please contact the game author for support, as this "
        "is likely to be a scripting error and not a bug in AGS.\n"
        "(ACI version " ACI_VERSION_TEXT ")\n\n");

    strcat (alertis, get_cur_script(5) );

    if (qmsg[0] != '|')
      strcat(alertis,"\nError: ");
    else
      qmsg = "";
    }
  else if (qmsg[0] == '%') {
    qmsg++;

    sprintf(alertis, "A warning has been generated. This is not normally fatal, but you have selected "
      "to treat warnings as errors.\n"
      "(ACI version " ACI_VERSION_TEXT ")\n\n%s\n", get_cur_script(5));
  }
  else strcpy(alertis,"An internal error has occurred. Please note down the following information.\n"
   "If the problem persists, post the details on the AGS Technical Forum.\n"
   "(ACI version " ACI_VERSION_TEXT ")\n"
   "\nError: ");

  shutdown_font_renderer();
  our_eip = 9902;

  // close graphics mode (Win) or return to text mode (DOS)
  if (_sub_screen) {
    destroy_bitmap(_sub_screen);
    _sub_screen = NULL;
  }
  
  our_eip = 9907;

  close_translation();

  our_eip = 9908;

  // Release the display mode (and anything dependant on the window)
  if (gfxDriver != NULL)
    gfxDriver->UnInit();

  // Tell Allegro that we are no longer in graphics mode
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

  // successful exit displays no messages (because Windoze closes the dos-box
  // if it is empty).
  if (qmsg[0]=='|') ;
  else if (!handledErrorInEditor)
  {
    // Display the message (at this point the window still exists)
    sprintf(pexbuf,"%s\n",qmsg);
    strcat(alertis,pexbuf);
    platform->DisplayAlert(alertis);
  }

  // remove the game window
  allegro_exit();

  if (gfxDriver != NULL)
  {
    delete gfxDriver;
    gfxDriver = NULL;
  }
  
  platform->PostAllegroExit();

  our_eip = 9903;

  // wipe all the interaction structs so they don't de-alloc the children twice
  memset (&roomstats[0], 0, sizeof(RoomStatus) * MAX_ROOMS);
  memset (&troom, 0, sizeof(RoomStatus));

/*  _CrtMemState memstart;
  _CrtMemCheckpoint(&memstart);
  _CrtMemDumpStatistics( &memstart );*/

/*  // print the FPS if there wasn't an error
  if ((play.debug_mode!=0) && (qmsg[0]=='|'))
    printf("Last cycle fps: %d\n",fps);*/
  al_ffblk	dfb;
  int	dun = al_findfirst("~ac*.tmp",&dfb,FA_SEARCH);
  while (!dun) {
    unlink(dfb.name);
    dun = al_findnext(&dfb);
  }
  al_findclose (&dfb);

  proper_exit=1;

  write_log_debug("***** ENGINE HAS SHUTDOWN");

  our_eip = 9904;
  exit(EXIT_NORMAL);
}

extern "C" {
	void quit_c(char*msg) {
		quit(msg);
		  }
}

void setup_sierra_interface() {
  int rr;
  game.numgui =0;
  for (rr=0;rr<42;rr++) game.paluses[rr]=PAL_GAMEWIDE;
  for (rr=42;rr<256;rr++) game.paluses[rr]=PAL_BACKGROUND;
}

void set_default_glmsg (int msgnum, const char* val) {
  if (game.messages[msgnum-500] == NULL) {
    game.messages[msgnum-500] = (char*)malloc (strlen(val)+5);
    strcpy (game.messages[msgnum-500], val);
  }
}

void split_lines_rightleft (char *todis, int wii, int fonnt) {
  // start on the last character
  char *thisline = todis + strlen(todis) - 1;
  char prevlwas, *prevline = NULL;
  // work backwards
  while (thisline >= todis) {

    int needBreak = 0;
    if (thisline <= todis) 
      needBreak = 1;
    // ignore \[ sequence
    else if ((thisline > todis) && (thisline[-1] == '\\')) { }
    else if (thisline[0] == '[') {
      needBreak = 1;
      thisline++;
    }
    else if (wgettextwidth_compensate(thisline, fonnt) >= wii) {
      // go 'back' to the nearest word
      while ((thisline[0] != ' ') && (thisline[0] != 0))
        thisline++;

      if (thisline[0] == 0)
        quit("!Single word too wide for window");

      thisline++;
      needBreak = 1;
    }

    if (needBreak) {
      strcpy(lines[numlines], thisline);
      removeBackslashBracket(lines[numlines]);
      numlines++;
      if (prevline) {
        prevline[0] = prevlwas;
      }
      thisline--;
      prevline = thisline;
      prevlwas = prevline[0];
      prevline[0] = 0;
    }

    thisline--;
  }
  if (prevline)
    prevline[0] = prevlwas;
}



char *reverse_text(char *text) {
  int stlen = strlen(text), rr;
  char *backwards = (char*)malloc(stlen + 1);
  for (rr = 0; rr < stlen; rr++)
    backwards[rr] = text[(stlen - rr) - 1];
  backwards[stlen] = 0;
  return backwards;
}

void wouttext_reverseifnecessary(int x, int y, int font, char *text) {
  char *backwards = NULL;
  char *otext = text;
  if (game.options[OPT_RIGHTLEFTWRITE]) {
    backwards = reverse_text(text);
    otext = backwards;
  }

  wouttext_outline(x, y, font, otext);

  if (backwards)
    free(backwards);
}

void break_up_text_into_lines(int wii,int fonnt,char*todis) {
  if (fonnt == -1)
    fonnt = play.normal_font;

//  char sofar[100];
  if (todis[0]=='&') {
    while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
    if (todis[0]==' ') todis++;
    }
  numlines=0;
  longestline=0;

  // Don't attempt to display anything if the width is tiny
  if (wii < 3)
    return;

  int rr;

  if (game.options[OPT_RIGHTLEFTWRITE] == 0)
  {
    split_lines_leftright(todis, wii, fonnt);
  }
  else {
    // Right-to-left just means reverse the text then
    // write it as normal
    char *backwards = reverse_text(todis);
    split_lines_rightleft (backwards, wii, fonnt);
    free(backwards);
  }

  for (rr=0;rr<numlines;rr++) {
    if (wgettextwidth_compensate(lines[rr],fonnt) > longestline)
      longestline = wgettextwidth_compensate(lines[rr],fonnt);
  }
}

void stop_all_sound_and_music() 
{
  int a;
  stopmusic();
  // make sure it doesn't start crossfading when it comes back
  crossFading = 0;
  // any ambient sound will be aborted
  for (a = 0; a <= MAX_SOUND_CHANNELS; a++)
    stop_and_destroy_channel(a);
}

void shutdown_sound() 
{
  stop_all_sound_and_music();

  if (opts.mod_player)
    remove_mod_player();
  remove_sound();
}

void setup_player_character(int charid) {
  game.playercharacter = charid;
  playerchar = &game.chars[charid];
  _sc_PlayerCharPtr = ccGetObjectHandleFromAddress((char*)playerchar);
}


// *** The script serialization routines for built-in types

int AGSCCDynamicObject::Dispose(const char *address, bool force) {
  // cannot be removed from memory
  return 0;
}

void AGSCCDynamicObject::StartSerialize(char *sbuffer) {
  bytesSoFar = 0;
  serbuffer = sbuffer;
}

void AGSCCDynamicObject::SerializeInt(int val) {
  char *chptr = &serbuffer[bytesSoFar];
  int *iptr = (int*)chptr;
  *iptr = val;
  bytesSoFar += 4;
}

int AGSCCDynamicObject::EndSerialize() {
  return bytesSoFar;
}

void AGSCCDynamicObject::StartUnserialize(const char *sbuffer, int pTotalBytes) {
  bytesSoFar = 0;
  totalBytes = pTotalBytes;
  serbuffer = (char*)sbuffer;
}

int AGSCCDynamicObject::UnserializeInt() {
  if (bytesSoFar >= totalBytes)
    quit("Unserialise: internal error: read past EOF");

  char *chptr = &serbuffer[bytesSoFar];
  bytesSoFar += 4;
  int *iptr = (int*)chptr;
  return *iptr;
}

struct ScriptDialogOptionsRendering : AGSCCDynamicObject {
  int x, y, width, height;
  int parserTextboxX, parserTextboxY;
  int parserTextboxWidth;
  int dialogID;
  int activeOptionID;
  ScriptDrawingSurface *surfaceToRenderTo;
  bool surfaceAccessed;

  // return the type name of the object
  virtual const char *GetType() {
    return "DialogOptionsRendering";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    return 0;
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    ccRegisterUnserializedObject(index, this, this);
  }

  void Reset()
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    parserTextboxX = 0;
    parserTextboxY = 0;
    parserTextboxWidth = 0;
    dialogID = 0;
    surfaceToRenderTo = NULL;
    surfaceAccessed = false;
    activeOptionID = -1;
  }

  ScriptDialogOptionsRendering()
  {
    Reset();
  }
};

struct CCGUIObject : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "GUIObject";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    GUIObject *guio = (GUIObject*)address;
    StartSerialize(buffer);
    SerializeInt(guio->guin);
    SerializeInt(guio->objn);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int guinum = UnserializeInt();
    int objnum = UnserializeInt();
    ccRegisterUnserializedObject(index, guis[guinum].objs[objnum], this);
  }

};

struct CCCharacter : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "Character";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    CharacterInfo *chaa = (CharacterInfo*)address;
    StartSerialize(buffer);
    SerializeInt(chaa->index_id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &game.chars[num], this);
  }

};

struct CCHotspot : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "Hotspot";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptHotspot *shh = (ScriptHotspot*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrHotspot[num], this);
  }

};

struct CCRegion : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "Region";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptRegion *shh = (ScriptRegion*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrRegion[num], this);
  }

};

struct CCInventory : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "Inventory";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptInvItem *shh = (ScriptInvItem*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrInv[num], this);
  }

};

struct CCDialog : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "Dialog";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptDialog *shh = (ScriptDialog*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrDialog[num], this);
  }

};

struct CCGUI : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "GUI";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptGUI *shh = (ScriptGUI*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrGui[num], this);
  }
};

struct CCObject : AGSCCDynamicObject {

  // return the type name of the object
  virtual const char *GetType() {
    return "Object";
  }

  // serialize the object into BUFFER (which is BUFSIZE bytes)
  // return number of bytes used
  virtual int Serialize(const char *address, char *buffer, int bufsize) {
    ScriptObject *shh = (ScriptObject*)address;
    StartSerialize(buffer);
    SerializeInt(shh->id);
    return EndSerialize();
  }

  virtual void Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    int num = UnserializeInt();
    ccRegisterUnserializedObject(index, &scrObj[num], this);
  }

};

// ** SCRIPT DRAWINGSURFACE OBJECT

void DrawingSurface_Release(ScriptDrawingSurface* sds)
{
  if (sds->roomBackgroundNumber >= 0)
  {
    if (sds->modified)
    {
      if (sds->roomBackgroundNumber == play.bg_frame)
      {
        invalidate_screen();
        mark_current_background_dirty();
      }
      play.raw_modified[sds->roomBackgroundNumber] = 1;
    }

    sds->roomBackgroundNumber = -1;
  }
  if (sds->dynamicSpriteNumber >= 0)
  {
    if (sds->modified)
    {
      int tt;
      // force a refresh of any cached object or character images
      if (croom != NULL) 
      {
        for (tt = 0; tt < croom->numobj; tt++) 
        {
          if (objs[tt].num == sds->dynamicSpriteNumber)
            objcache[tt].sppic = -31999;
        }
      }
      for (tt = 0; tt < game.numcharacters; tt++) 
      {
        if (charcache[tt].sppic == sds->dynamicSpriteNumber)
          charcache[tt].sppic = -31999;
      }
      for (tt = 0; tt < game.numgui; tt++) 
      {
        if ((guis[tt].bgpic == sds->dynamicSpriteNumber) &&
            (guis[tt].on == 1))
        {
          guis_need_update = 1;
          break;
        }
      }
    }

    sds->dynamicSpriteNumber = -1;
  }
  if (sds->dynamicSurfaceNumber >= 0)
  {
    destroy_bitmap(dynamicallyCreatedSurfaces[sds->dynamicSurfaceNumber]);
    dynamicallyCreatedSurfaces[sds->dynamicSurfaceNumber] = NULL;
    sds->dynamicSurfaceNumber = -1;
  }
  sds->modified = 0;
}

void ScriptDrawingSurface::MultiplyCoordinates(int *xcoord, int *ycoord)
{
  if (this->highResCoordinates)
  {
    if (current_screen_resolution_multiplier == 1) 
    {
      // using high-res co-ordinates but game running at low-res
      xcoord[0] /= 2;
      ycoord[0] /= 2;
    }
  }
  else
  {
    if (current_screen_resolution_multiplier > 1) 
    {
      // using low-res co-ordinates but game running at high-res
      xcoord[0] *= 2;
      ycoord[0] *= 2;
    }
  }
}

void ScriptDrawingSurface::MultiplyThickness(int *valueToAdjust)
{
  if (this->highResCoordinates)
  {
    if (current_screen_resolution_multiplier == 1) 
    {
      valueToAdjust[0] /= 2;
      if (valueToAdjust[0] < 1)
        valueToAdjust[0] = 1;
    }
  }
  else
  {
    if (current_screen_resolution_multiplier > 1) 
    {
      valueToAdjust[0] *= 2;
    }
  }
}

// convert actual co-ordinate back to what the script is expecting
void ScriptDrawingSurface::UnMultiplyThickness(int *valueToAdjust)
{
  if (this->highResCoordinates)
  {
    if (current_screen_resolution_multiplier == 1) 
    {
      valueToAdjust[0] *= 2;
    }
  }
  else
  {
    if (current_screen_resolution_multiplier > 1) 
    {
      valueToAdjust[0] /= 2;
      if (valueToAdjust[0] < 1)
        valueToAdjust[0] = 1;
    }
  }
}

ScriptDrawingSurface* DrawingSurface_CreateCopy(ScriptDrawingSurface *sds)
{
  BITMAP *sourceBitmap = sds->GetBitmapSurface();

  for (int i = 0; i < MAX_DYNAMIC_SURFACES; i++)
  {
    if (dynamicallyCreatedSurfaces[i] == NULL)
    {
      dynamicallyCreatedSurfaces[i] = create_bitmap_ex(bitmap_color_depth(sourceBitmap), sourceBitmap->w, sourceBitmap->h);
      blit(sourceBitmap, dynamicallyCreatedSurfaces[i], 0, 0, 0, 0, sourceBitmap->w, sourceBitmap->h);
      ScriptDrawingSurface *newSurface = new ScriptDrawingSurface();
      newSurface->dynamicSurfaceNumber = i;
      newSurface->hasAlphaChannel = sds->hasAlphaChannel;
      ccRegisterManagedObject(newSurface, newSurface);
      return newSurface;
    }
  }

  quit("!DrawingSurface.CreateCopy: too many copied surfaces created");
  return NULL;
}

void DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int translev) {
  if ((translev < 0) || (translev > 99))
    quit("!DrawingSurface.DrawSurface: invalid parameter (transparency must be 0-99)");

  target->StartDrawing();
  BITMAP *surfaceToDraw = source->GetBitmapSurface();

  if (surfaceToDraw == abuf)
    quit("!DrawingSurface.DrawSurface: cannot draw surface onto itself");

  if (translev == 0) {
    // just draw it over the top, no transparency
    blit(surfaceToDraw, abuf, 0, 0, 0, 0, surfaceToDraw->w, surfaceToDraw->h);
    target->FinishedDrawing();
    return;
  }

  if (bitmap_color_depth(surfaceToDraw) <= 8)
    quit("!DrawingSurface.DrawSurface: 256-colour surfaces cannot be drawn transparently");

  // Draw it transparently
  trans_mode = ((100-translev) * 25) / 10;
  put_sprite_256(0, 0, surfaceToDraw);
  target->FinishedDrawing();
}

void DrawingSurface_DrawImage(ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height)
{
  if ((slot < 0) || (slot >= MAX_SPRITES) || (spriteset[slot] == NULL))
    quit("!DrawingSurface.DrawImage: invalid sprite slot number specified");

  if ((trans < 0) || (trans > 100))
    quit("!DrawingSurface.DrawImage: invalid transparency setting");

  // 100% transparency, don't draw anything
  if (trans == 100)
    return;

  BITMAP *sourcePic = spriteset[slot];
  bool needToFreeBitmap = false;

  if (width != SCR_NO_VALUE)
  {
    // Resize specified

    if ((width < 1) || (height < 1))
      return;

    sds->MultiplyCoordinates(&width, &height);

    // resize the sprite to the requested size
    block newPic = create_bitmap_ex(bitmap_color_depth(sourcePic), width, height);

    stretch_blit(sourcePic, newPic,
                 0, 0, spritewidth[slot], spriteheight[slot],
                 0, 0, width, height);

    sourcePic = newPic;
    needToFreeBitmap = true;
    update_polled_stuff();
  }

  sds->StartDrawing();
  sds->MultiplyCoordinates(&xx, &yy);

  if (bitmap_color_depth(sourcePic) != bitmap_color_depth(abuf)) {
    debug_log("RawDrawImage: Sprite %d colour depth %d-bit not same as background depth %d-bit", slot, bitmap_color_depth(spriteset[slot]), bitmap_color_depth(abuf));
  }

  if (trans > 0)
  {
    trans_mode = ((100 - trans) * 255) / 100;
  }

  draw_sprite_support_alpha(xx, yy, sourcePic, slot);

  sds->FinishedDrawing();

  if (needToFreeBitmap)
    destroy_bitmap(sourcePic);
}

int Game_GetColorFromRGB(int red, int grn, int blu) {
  if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
      (blu < 0) || (blu > 255))
    quit("!GetColorFromRGB: colour values must be 0-255");

  if (game.color_depth == 1)
  {
    return makecol8(red, grn, blu);
  }

  int agscolor = ((blu >> 3) & 0x1f);
  agscolor += ((grn >> 2) & 0x3f) << 5;
  agscolor += ((red >> 3) & 0x1f) << 11;
  return agscolor;
}

void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *sds, int newColour) 
{
  sds->currentColourScript = newColour;
  // StartDrawing to set up abuf to set the colour at the appropriate
  // depth for the background
  sds->StartDrawing();
  if (newColour == SCR_COLOR_TRANSPARENT)
  {
    sds->currentColour = bitmap_mask_color(abuf);
  }
  else
  {
    sds->currentColour = get_col8_lookup(newColour);
  }
  sds->FinishedDrawingReadOnly();
}

int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *sds) 
{
  return sds->currentColourScript;
}

void DrawingSurface_SetUseHighResCoordinates(ScriptDrawingSurface *sds, int highRes) 
{
  sds->highResCoordinates = (highRes) ? 1 : 0;
}

int DrawingSurface_GetUseHighResCoordinates(ScriptDrawingSurface *sds) 
{
  return sds->highResCoordinates;
}

int DrawingSurface_GetHeight(ScriptDrawingSurface *sds) 
{
  sds->StartDrawing();
  int height = abuf->h;
  sds->FinishedDrawingReadOnly();
  sds->UnMultiplyThickness(&height);
  return height;
}

int DrawingSurface_GetWidth(ScriptDrawingSurface *sds) 
{
  sds->StartDrawing();
  int width = abuf->w;
  sds->FinishedDrawingReadOnly();
  sds->UnMultiplyThickness(&width);
  return width;
}

void DrawingSurface_Clear(ScriptDrawingSurface *sds, int colour) 
{
  sds->StartDrawing();
  int allegroColor;
  if ((colour == -SCR_NO_VALUE) || (colour == SCR_COLOR_TRANSPARENT))
  {
    allegroColor = bitmap_mask_color(abuf);
  }
  else
  {
    allegroColor = get_col8_lookup(colour);
  }
  clear_to_color(abuf, allegroColor);
  sds->FinishedDrawing();
}

void DrawingSurface_DrawCircle(ScriptDrawingSurface *sds, int x, int y, int radius) 
{
  sds->MultiplyCoordinates(&x, &y);
  sds->MultiplyThickness(&radius);

  sds->StartDrawing();
  circlefill(abuf, x, y, radius, sds->currentColour);
  sds->FinishedDrawing();
}

void DrawingSurface_DrawRectangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2) 
{
  sds->MultiplyCoordinates(&x1, &y1);
  sds->MultiplyCoordinates(&x2, &y2);

  sds->StartDrawing();
  rectfill(abuf, x1,y1,x2,y2, sds->currentColour);
  sds->FinishedDrawing();
}

void DrawingSurface_DrawTriangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3) 
{
  sds->MultiplyCoordinates(&x1, &y1);
  sds->MultiplyCoordinates(&x2, &y2);
  sds->MultiplyCoordinates(&x3, &y3);

  sds->StartDrawing();
  triangle(abuf, x1,y1,x2,y2,x3,y3, sds->currentColour);
  sds->FinishedDrawing();
}

void DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...) 
{
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf,get_translation(texx),ap);
  va_end(ap);
  // don't use wtextcolor because it will do a 16->32 conversion
  textcol = sds->currentColour;

  sds->MultiplyCoordinates(&xx, &yy);
  sds->StartDrawing();
  wtexttransparent(TEXTFG);
  if ((bitmap_color_depth(abuf) <= 8) && (play.raw_color > 255)) {
    wtextcolor(1);
    debug_log ("RawPrint: Attempted to use hi-color on 256-col background");
  }
  wouttext_outline(xx, yy, font, displbuf);
  sds->FinishedDrawing();
}

void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg) {
  int texthit = wgetfontheight(font);
  sds->MultiplyCoordinates(&xx, &yy);
  sds->MultiplyThickness(&wid);

  break_up_text_into_lines(wid, font, (char*)msg);

  textcol = sds->currentColour;
  sds->StartDrawing();

  wtexttransparent(TEXTFG);
  for (int i = 0; i < numlines; i++)
  {
    int drawAtX = xx;

    if (alignment == SCALIGN_CENTRE)
    {
      drawAtX = xx + ((wid / 2) - wgettextwidth(lines[i], font) / 2);
    }
    else if (alignment == SCALIGN_RIGHT)
    {
      drawAtX = (xx + wid) - wgettextwidth(lines[i], font);
    }

    wouttext_outline(drawAtX, yy + texthit*i, font, lines[i]);
  }

  sds->FinishedDrawing();
}

void DrawingSurface_DrawMessageWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm)
{
  char displbuf[3000];
  get_message_text(msgm, displbuf);
  // it's probably too late but check anyway
  if (strlen(displbuf) > 2899)
    quit("!RawPrintMessageWrapped: message too long");

  DrawingSurface_DrawStringWrapped(sds, xx, yy, wid, font, SCALIGN_LEFT, displbuf);
}

void DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness) {
  sds->MultiplyCoordinates(&fromx, &fromy);
  sds->MultiplyCoordinates(&tox, &toy);
  sds->MultiplyThickness(&thickness);
  int ii,jj,xx,yy;
  sds->StartDrawing();
  // draw several lines to simulate the thickness
  for (ii = 0; ii < thickness; ii++) 
  {
    xx = (ii - (thickness / 2));
    for (jj = 0; jj < thickness; jj++)
    {
      yy = (jj - (thickness / 2));
      line (abuf, fromx + xx, fromy + yy, tox + xx, toy + yy, sds->currentColour);
    }
  }
  sds->FinishedDrawing();
}

void DrawingSurface_DrawPixel(ScriptDrawingSurface *sds, int x, int y) {
  sds->MultiplyCoordinates(&x, &y);
  int thickness = 1;
  sds->MultiplyThickness(&thickness);
  int ii,jj;
  sds->StartDrawing();
  // draw several pixels to simulate the thickness
  for (ii = 0; ii < thickness; ii++) 
  {
    for (jj = 0; jj < thickness; jj++)
    {
      putpixel(abuf, x + ii, y + jj, sds->currentColour);
    }
  }
  sds->FinishedDrawing();
}

int DrawingSurface_GetPixel(ScriptDrawingSurface *sds, int x, int y) {
  sds->MultiplyCoordinates(&x, &y);
  sds->StartDrawing();
  unsigned int rawPixel = getpixel(abuf, x, y);
  unsigned int maskColor = bitmap_mask_color(abuf);
  int colDepth = bitmap_color_depth(abuf);

  if (rawPixel == maskColor)
  {
    rawPixel = SCR_COLOR_TRANSPARENT;
  }
  else if (colDepth > 8)
  {
    int r = getr_depth(colDepth, rawPixel);
    int g = getg_depth(colDepth, rawPixel);
    int b = getb_depth(colDepth, rawPixel);

    rawPixel = Game_GetColorFromRGB(r, g, b);
  }

  sds->FinishedDrawingReadOnly();

  return rawPixel;
}

BITMAP* ScriptDrawingSurface::GetBitmapSurface()
{
  if (roomBackgroundNumber >= 0)
    return thisroom.ebscene[roomBackgroundNumber];
  else if (dynamicSpriteNumber >= 0)
    return spriteset[dynamicSpriteNumber];
  else if (dynamicSurfaceNumber >= 0)
    return dynamicallyCreatedSurfaces[dynamicSurfaceNumber];
  else if (linkedBitmapOnly != NULL)
    return linkedBitmapOnly;
  else
    quit("!DrawingSurface: attempted to use surface after Release was called");

  return NULL;
}

void ScriptDrawingSurface::StartDrawing()
{
  abufBackup = abuf;
  abuf = this->GetBitmapSurface();
}

void ScriptDrawingSurface::FinishedDrawingReadOnly()
{
  abuf = abufBackup;
}

void ScriptDrawingSurface::FinishedDrawing()
{
  FinishedDrawingReadOnly();
  modified = 1;
}

int ScriptDrawingSurface::Dispose(const char *address, bool force) {

  // dispose the drawing surface
  DrawingSurface_Release(this);
  delete this;
  return 1;
}

const char *ScriptDrawingSurface::GetType() {
  return "DrawingSurface";
}

int ScriptDrawingSurface::Serialize(const char *address, char *buffer, int bufsize) {
  StartSerialize(buffer);
  SerializeInt(roomBackgroundNumber);
  SerializeInt(dynamicSpriteNumber);
  SerializeInt(dynamicSurfaceNumber);
  SerializeInt(currentColour);
  SerializeInt(currentColourScript);
  SerializeInt(highResCoordinates);
  SerializeInt(modified);
  SerializeInt(hasAlphaChannel);
  SerializeInt(isLinkedBitmapOnly ? 1 : 0);
  return EndSerialize();
}

void ScriptDrawingSurface::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  roomBackgroundNumber = UnserializeInt();
  dynamicSpriteNumber = UnserializeInt();
  dynamicSurfaceNumber = UnserializeInt();
  currentColour = UnserializeInt();
  currentColourScript = UnserializeInt();
  highResCoordinates = UnserializeInt();
  modified = UnserializeInt();
  hasAlphaChannel = UnserializeInt();
  isLinkedBitmapOnly = (UnserializeInt() != 0);
  ccRegisterUnserializedObject(index, this, this);
}

ScriptDrawingSurface::ScriptDrawingSurface() 
{
  roomBackgroundNumber = -1;
  dynamicSpriteNumber = -1;
  dynamicSurfaceNumber = -1;
  isLinkedBitmapOnly = false;
  linkedBitmapOnly = NULL;
  currentColour = play.raw_color;
  currentColourScript = 0;
  modified = 0;
  hasAlphaChannel = 0;
  highResCoordinates = 0;

  if ((game.options[OPT_NATIVECOORDINATES] != 0) &&
      (game.default_resolution > 2))
  {
    highResCoordinates = 1;
  }
}


// ** SCRIPT DIALOGOPTIONSRENDERING OBJECT

int DialogOptionsRendering_GetX(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->x;
}

void DialogOptionsRendering_SetX(ScriptDialogOptionsRendering *dlgOptRender, int newX)
{
  dlgOptRender->x = newX;
}

int DialogOptionsRendering_GetY(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->y;
}

void DialogOptionsRendering_SetY(ScriptDialogOptionsRendering *dlgOptRender, int newY)
{
  dlgOptRender->y = newY;
}

int DialogOptionsRendering_GetWidth(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->width;
}

void DialogOptionsRendering_SetWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
{
  dlgOptRender->width = newWidth;
}

int DialogOptionsRendering_GetHeight(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->height;
}

void DialogOptionsRendering_SetHeight(ScriptDialogOptionsRendering *dlgOptRender, int newHeight)
{
  dlgOptRender->height = newHeight;
}

int DialogOptionsRendering_GetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->parserTextboxX;
}

void DialogOptionsRendering_SetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender, int newX)
{
  dlgOptRender->parserTextboxX = newX;
}

int DialogOptionsRendering_GetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->parserTextboxY;
}

void DialogOptionsRendering_SetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender, int newY)
{
  dlgOptRender->parserTextboxY = newY;
}

int DialogOptionsRendering_GetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->parserTextboxWidth;
}

void DialogOptionsRendering_SetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
{
  dlgOptRender->parserTextboxWidth = newWidth;
}

ScriptDialog* DialogOptionsRendering_GetDialogToRender(ScriptDialogOptionsRendering *dlgOptRender)
{
  return &scrDialog[dlgOptRender->dialogID];
}

ScriptDrawingSurface* DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender)
{
  dlgOptRender->surfaceAccessed = true;
  return dlgOptRender->surfaceToRenderTo;
}

int DialogOptionsRendering_GetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender)
{
  return dlgOptRender->activeOptionID + 1;
}

void DialogOptionsRendering_SetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID)
{
  int optionCount = dialog[scrDialog[dlgOptRender->dialogID].id].numoptions;
  if ((activeOptionID < 0) || (activeOptionID > optionCount))
    quitprintf("DialogOptionsRenderingInfo.ActiveOptionID: invalid ID specified for this dialog (specified %d, valid range: 1..%d)", activeOptionID, optionCount);

  dlgOptRender->activeOptionID = activeOptionID - 1;
}


// ** SCRIPT DATETIME OBJECT

int ScriptDateTime::Dispose(const char *address, bool force) {
  // always dispose a DateTime
  delete this;
  return 1;
}

const char *ScriptDateTime::GetType() {
  return "DateTime";
}

int ScriptDateTime::Serialize(const char *address, char *buffer, int bufsize) {
  StartSerialize(buffer);
  SerializeInt(year);
  SerializeInt(month);
  SerializeInt(day);
  SerializeInt(hour);
  SerializeInt(minute);
  SerializeInt(second);
  SerializeInt(rawUnixTime);
  return EndSerialize();
}

void ScriptDateTime::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  year = UnserializeInt();
  month = UnserializeInt();
  day = UnserializeInt();
  hour = UnserializeInt();
  minute = UnserializeInt();
  second = UnserializeInt();
  rawUnixTime = UnserializeInt();
  ccRegisterUnserializedObject(index, this, this);
}

ScriptDateTime::ScriptDateTime() {
  year = month = day = 0;
  hour = minute = second = 0;
  rawUnixTime = 0;
}


// ** SCRIPT VIEW FRAME OBJECT

int ScriptViewFrame::Dispose(const char *address, bool force) {
  // always dispose a ViewFrame
  delete this;
  return 1;
}

const char *ScriptViewFrame::GetType() {
  return "ViewFrame";
}

int ScriptViewFrame::Serialize(const char *address, char *buffer, int bufsize) {
  StartSerialize(buffer);
  SerializeInt(view);
  SerializeInt(loop);
  SerializeInt(frame);
  return EndSerialize();
}

void ScriptViewFrame::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  view = UnserializeInt();
  loop = UnserializeInt();
  frame = UnserializeInt();
  ccRegisterUnserializedObject(index, this, this);
}

ScriptViewFrame::ScriptViewFrame(int p_view, int p_loop, int p_frame) {
  view = p_view;
  loop = p_loop;
  frame = p_frame;
}

ScriptViewFrame::ScriptViewFrame() {
  view = -1;
  loop = -1;
  frame = -1;
}


// ** SCRIPT STRING

const char *CreateNewScriptString(const char *fromText, bool reAllocate) {
  ScriptString *str;
  if (reAllocate) {
    str = new ScriptString(fromText);
  }
  else {
    str = new ScriptString();
    str->text = (char*)fromText;
  }

  ccRegisterManagedObject(str->text, str);

  /*long handle = ccRegisterManagedObject(str->text, str);
  char buffer[1000];
  sprintf(buffer, "String %p (handle %d) allocated: '%s'", str->text, handle, str->text);
  write_log(buffer);*/

  return str->text;
}

void* ScriptString::CreateString(const char *fromText) {
  return (void*)CreateNewScriptString(fromText);
}

int ScriptString::Dispose(const char *address, bool force) {
  // always dispose
  if (text) {
/*    char buffer[1000];
    sprintf(buffer, "String %p deleted: '%s'", text, text);
    write_log(buffer);*/
    free(text);
  }
  delete this;
  return 1;
}

const char *ScriptString::GetType() {
  return "String";
}

int ScriptString::Serialize(const char *address, char *buffer, int bufsize) {
  if (text == NULL)
    text = "";
  StartSerialize(buffer);
  SerializeInt(strlen(text));
  strcpy(&serbuffer[bytesSoFar], text);
  bytesSoFar += strlen(text) + 1;
  return EndSerialize();
}

void ScriptString::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  int textsize = UnserializeInt();
  text = (char*)malloc(textsize + 1);
  strcpy(text, &serializedData[bytesSoFar]);
  ccRegisterUnserializedObject(index, text, this);
}

ScriptString::ScriptString() {
  text = NULL;
}

ScriptString::ScriptString(const char *fromText) {
  text = (char*)malloc(strlen(fromText) + 1);
  strcpy(text, fromText);
}

int String_IsNullOrEmpty(const char *thisString) 
{
  if ((thisString == NULL) || (thisString[0] == 0))
    return 1;

  return 0;
}

const char* String_Copy(const char *srcString) {
  return CreateNewScriptString(srcString);
}

const char* String_Append(const char *thisString, const char *extrabit) {
  char *buffer = (char*)malloc(strlen(thisString) + strlen(extrabit) + 1);
  strcpy(buffer, thisString);
  strcat(buffer, extrabit);
  return CreateNewScriptString(buffer, false);
}

const char* String_AppendChar(const char *thisString, char extraOne) {
  char *buffer = (char*)malloc(strlen(thisString) + 2);
  sprintf(buffer, "%s%c", thisString, extraOne);
  return CreateNewScriptString(buffer, false);
}

const char* String_ReplaceCharAt(const char *thisString, int index, char newChar) {
  if ((index < 0) || (index >= (int)strlen(thisString)))
    quit("!String.ReplaceCharAt: index outside range of string");

  char *buffer = (char*)malloc(strlen(thisString) + 1);
  strcpy(buffer, thisString);
  buffer[index] = newChar;
  return CreateNewScriptString(buffer, false);
}

const char* String_Truncate(const char *thisString, int length) {
  if (length < 0)
    quit("!String.Truncate: invalid length");

  if (length >= (int)strlen(thisString))
    return thisString;

  char *buffer = (char*)malloc(length + 1);
  strncpy(buffer, thisString, length);
  buffer[length] = 0;
  return CreateNewScriptString(buffer, false);
}

const char* String_Substring(const char *thisString, int index, int length) {
  if (length < 0)
    quit("!String.Substring: invalid length");
  if ((index < 0) || (index > (int)strlen(thisString)))
    quit("!String.Substring: invalid index");

  char *buffer = (char*)malloc(length + 1);
  strncpy(buffer, &thisString[index], length);
  buffer[length] = 0;
  return CreateNewScriptString(buffer, false);
}

int String_CompareTo(const char *thisString, const char *otherString, bool caseSensitive) {

  if (caseSensitive) {
    return strcmp(thisString, otherString);
  }
  else {
    return stricmp(thisString, otherString);
  }
}

int String_StartsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

  if (caseSensitive) {
    return (strncmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
  }
  else {
    return (strnicmp(thisString, checkForString, strlen(checkForString)) == 0) ? 1 : 0;
  }
}

int String_EndsWith(const char *thisString, const char *checkForString, bool caseSensitive) {

  int checkAtOffset = strlen(thisString) - strlen(checkForString);

  if (checkAtOffset < 0)
  {
    return 0;
  }

  if (caseSensitive) 
  {
    return (strcmp(&thisString[checkAtOffset], checkForString) == 0) ? 1 : 0;
  }
  else 
  {
    return (stricmp(&thisString[checkAtOffset], checkForString) == 0) ? 1 : 0;
  }
}

const char* String_Replace(const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
{
  char resultBuffer[STD_BUFFER_SIZE] = "";
  int thisStringLen = (int)strlen(thisString);
  int outputSize = 0;
  for (int i = 0; i < thisStringLen; i++)
  {
    bool matchHere = false;
    if (caseSensitive)
    {
      matchHere = (strncmp(&thisString[i], lookForText, strlen(lookForText)) == 0);
    }
    else
    {
      matchHere = (strnicmp(&thisString[i], lookForText, strlen(lookForText)) == 0);
    }

    if (matchHere)
    {
      strcpy(&resultBuffer[outputSize], replaceWithText);
      outputSize += strlen(replaceWithText);
      i += strlen(lookForText) - 1;
    }
    else
    {
      resultBuffer[outputSize] = thisString[i];
      outputSize++;
    }
  }

  resultBuffer[outputSize] = 0;

  return CreateNewScriptString(resultBuffer, true);
}

const char* String_LowerCase(const char *thisString) {
  char *buffer = (char*)malloc(strlen(thisString) + 1);
  strcpy(buffer, thisString);
  strlwr(buffer);
  return CreateNewScriptString(buffer, false);
}

const char* String_UpperCase(const char *thisString) {
  char *buffer = (char*)malloc(strlen(thisString) + 1);
  strcpy(buffer, thisString);
  strupr(buffer);
  return CreateNewScriptString(buffer, false);
}

const char* String_Format(const char *texx, ...) {
  char displbuf[STD_BUFFER_SIZE];

  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf, get_translation(texx), ap);
  va_end(ap);

  return CreateNewScriptString(displbuf);
}

int String_GetChars(const char *texx, int index) {
  if ((index < 0) || (index >= (int)strlen(texx)))
    return 0;
  return texx[index];
}



// ** SCRIPT DYNAMIC SPRITE

int ScriptDynamicSprite::Dispose(const char *address, bool force) {
  // always dispose
  if ((slot) && (!force))
    free_dynamic_sprite(slot);

  delete this;
  return 1;
}

const char *ScriptDynamicSprite::GetType() {
  return "DynamicSprite";
}

int ScriptDynamicSprite::Serialize(const char *address, char *buffer, int bufsize) {
  StartSerialize(buffer);
  SerializeInt(slot);
  return EndSerialize();
}

void ScriptDynamicSprite::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  slot = UnserializeInt();
  ccRegisterUnserializedObject(index, this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite(int theSlot) {
  slot = theSlot;
  ccRegisterManagedObject(this, this);
}

ScriptDynamicSprite::ScriptDynamicSprite() {
  slot = 0;
}

void DynamicSprite_Delete(ScriptDynamicSprite *sds) {
  if (sds->slot) {
    free_dynamic_sprite(sds->slot);
    sds->slot = 0;
  }
}

ScriptDrawingSurface* DynamicSprite_GetDrawingSurface(ScriptDynamicSprite *dss)
{
  ScriptDrawingSurface *surface = new ScriptDrawingSurface();
  surface->dynamicSpriteNumber = dss->slot;

  if ((game.spriteflags[dss->slot] & SPF_ALPHACHANNEL) != 0)
    surface->hasAlphaChannel = true;

  ccRegisterManagedObject(surface, surface);
  return surface;
}

int DynamicSprite_GetGraphic(ScriptDynamicSprite *sds) {
  if (sds->slot == 0)
    quit("!DynamicSprite.Graphic: Cannot get graphic, sprite has been deleted");
  return sds->slot;
}

int DynamicSprite_GetWidth(ScriptDynamicSprite *sds) {
  return divide_down_coordinate(spritewidth[sds->slot]);
}

int DynamicSprite_GetHeight(ScriptDynamicSprite *sds) {
  return divide_down_coordinate(spriteheight[sds->slot]);
}

int DynamicSprite_GetColorDepth(ScriptDynamicSprite *sds) {
  int depth = bitmap_color_depth(spriteset[sds->slot]);
  if (depth == 15)
    depth = 16;
  if (depth == 24)
    depth = 32;
  return depth;
}

void DynamicSprite_Resize(ScriptDynamicSprite *sds, int width, int height) {
  if ((width < 1) || (height < 1))
    quit("!DynamicSprite.Resize: width and height must be greater than zero");
  if (sds->slot == 0)
    quit("!DynamicSprite.Resize: sprite has been deleted");

  multiply_up_coordinates(&width, &height);

  if (width * height >= 25000000)
    quitprintf("!DynamicSprite.Resize: new size is too large: %d x %d", width, height);

  // resize the sprite to the requested size
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);

  stretch_blit(spriteset[sds->slot], newPic,
               0, 0, spritewidth[sds->slot], spriteheight[sds->slot],
               0, 0, width, height);

  wfreeblock(spriteset[sds->slot]);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Flip(ScriptDynamicSprite *sds, int direction) {
  if ((direction < 1) || (direction > 3))
    quit("!DynamicSprite.Flip: invalid direction");
  if (sds->slot == 0)
    quit("!DynamicSprite.Flip: sprite has been deleted");

  // resize the sprite to the requested size
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), spritewidth[sds->slot], spriteheight[sds->slot]);
  clear_to_color(newPic, bitmap_mask_color(newPic));

  if (direction == 1)
    draw_sprite_h_flip(newPic, spriteset[sds->slot], 0, 0);
  else if (direction == 2)
    draw_sprite_v_flip(newPic, spriteset[sds->slot], 0, 0);
  else if (direction == 3)
    draw_sprite_vh_flip(newPic, spriteset[sds->slot], 0, 0);

  wfreeblock(spriteset[sds->slot]);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_CopyTransparencyMask(ScriptDynamicSprite *sds, int sourceSprite) {
  if (sds->slot == 0)
    quit("!DynamicSprite.CopyTransparencyMask: sprite has been deleted");

  if ((spritewidth[sds->slot] != spritewidth[sourceSprite]) ||
      (spriteheight[sds->slot] != spriteheight[sourceSprite]))
  {
    quit("!DynamicSprite.CopyTransparencyMask: sprites are not the same size");
  }

  block target = spriteset[sds->slot];
  block source = spriteset[sourceSprite];

  if (bitmap_color_depth(target) != bitmap_color_depth(source))
  {
    quit("!DynamicSprite.CopyTransparencyMask: sprites are not the same colour depth");
  }

  // set the target's alpha channel depending on the source
  bool sourceHasAlpha = (game.spriteflags[sourceSprite] & SPF_ALPHACHANNEL) != 0;
  game.spriteflags[sds->slot] &= ~SPF_ALPHACHANNEL;
  if (sourceHasAlpha)
  {
    game.spriteflags[sds->slot] |= SPF_ALPHACHANNEL;
  }

  unsigned int maskColor = bitmap_mask_color(source);
  int colDep = bitmap_color_depth(source);
  int bytesPerPixel = (colDep + 1) / 8;

  unsigned short *shortPtr;
  unsigned long *longPtr;
  for (int y = 0; y < target->h; y++)
  {
    unsigned char * sourcePixel = source->line[y];
    unsigned char * targetPixel = target->line[y];
    for (int x = 0; x < target->w; x++)
    {
      shortPtr = (unsigned short*)sourcePixel;
      longPtr = (unsigned long*)sourcePixel;

      if ((colDep == 8) && (sourcePixel[0] == maskColor))
      {
        targetPixel[0] = maskColor;
      }
      else if ((bytesPerPixel == 2) && (shortPtr[0] == maskColor))
      {
        ((unsigned short*)targetPixel)[0] = maskColor;
      }
      else if ((bytesPerPixel == 3) && (memcmp(sourcePixel, &maskColor, 3) == 0))
      {
        memcpy(targetPixel, sourcePixel, 3);
      }
      else if ((bytesPerPixel == 4) && (longPtr[0] == maskColor))
      {
        ((unsigned long*)targetPixel)[0] = maskColor;
      }
      else if ((bytesPerPixel == 4) && (sourceHasAlpha))
      {
        // the fourth byte is the alpha channel, copy it
        targetPixel[3] = sourcePixel[3];
      }
      else if (bytesPerPixel == 4)
      {
        // set the alpha channel byte to opaque
        targetPixel[3] = 0xff;
      }
        
      sourcePixel += bytesPerPixel;
      targetPixel += bytesPerPixel;
    }
  }
}

void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *sds, int width, int height, int x, int y) 
{
  if (sds->slot == 0)
    quit("!DynamicSprite.ChangeCanvasSize: sprite has been deleted");
  if ((width < 1) || (height < 1))
    quit("!DynamicSprite.ChangeCanvasSize: new size is too small");

  multiply_up_coordinates(&x, &y);
  multiply_up_coordinates(&width, &height);

  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);
  clear_to_color(newPic, bitmap_mask_color(newPic));
  // blit it into the enlarged image
  blit(spriteset[sds->slot], newPic, 0, 0, x, y, spritewidth[sds->slot], spriteheight[sds->slot]);

  wfreeblock(spriteset[sds->slot]);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Crop(ScriptDynamicSprite *sds, int x1, int y1, int width, int height) {
  if ((width < 1) || (height < 1))
    quit("!DynamicSprite.Crop: co-ordinates do not make sense");
  if (sds->slot == 0)
    quit("!DynamicSprite.Crop: sprite has been deleted");

  multiply_up_coordinates(&x1, &y1);
  multiply_up_coordinates(&width, &height);

  if ((width > spritewidth[sds->slot]) || (height > spriteheight[sds->slot]))
    quit("!DynamicSprite.Crop: requested to crop an area larger than the source");

  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);
  // blit it cropped
  blit(spriteset[sds->slot], newPic, x1, y1, 0, 0, newPic->w, newPic->h);

  wfreeblock(spriteset[sds->slot]);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Rotate(ScriptDynamicSprite *sds, int angle, int width, int height) {
  if ((angle < 1) || (angle > 359))
    quit("!DynamicSprite.Rotate: invalid angle (must be 1-359)");
  if (sds->slot == 0)
    quit("!DynamicSprite.Rotate: sprite has been deleted");

  if ((width == SCR_NO_VALUE) || (height == SCR_NO_VALUE)) {
    // calculate the new image size automatically
    // 1 degree = 181 degrees in terms of x/y size, so % 180
    int useAngle = angle % 180;
    // and 0..90 is the same as 180..90
    if (useAngle > 90)
      useAngle = 180 - useAngle;
    // useAngle is now between 0 and 90 (otherwise the sin/cos stuff doesn't work)
    double angleInRadians = (double)useAngle * (M_PI / 180.0);
    double sinVal = sin(angleInRadians);
    double cosVal = cos(angleInRadians);

    width = (cosVal * (double)spritewidth[sds->slot] + sinVal * (double)spriteheight[sds->slot]);
    height = (sinVal * (double)spritewidth[sds->slot] + cosVal * (double)spriteheight[sds->slot]);
  }
  else {
    multiply_up_coordinates(&width, &height);
  }

  // convert to allegro angle
  angle = (angle * 256) / 360;

  // resize the sprite to the requested size
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[sds->slot]), width, height);
  clear_to_color(newPic, bitmap_mask_color(newPic));

  // rotate the sprite about its centre
  // (+ width%2 fixes one pixel offset problem)
  pivot_sprite(newPic, spriteset[sds->slot], width / 2 + width % 2, height / 2,
               spritewidth[sds->slot] / 2, spriteheight[sds->slot] / 2, itofix(angle));

  wfreeblock(spriteset[sds->slot]);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

void DynamicSprite_Tint(ScriptDynamicSprite *sds, int red, int green, int blue, int saturation, int luminance) 
{
  block source = spriteset[sds->slot];
  block newPic = create_bitmap_ex(bitmap_color_depth(source), source->w, source->h);

  tint_image(source, newPic, red, green, blue, saturation, (luminance * 25) / 10);

  destroy_bitmap(source);
  // replace the bitmap in the sprite set
  add_dynamic_sprite(sds->slot, newPic, (game.spriteflags[sds->slot] & SPF_ALPHACHANNEL) != 0);
}

int DynamicSprite_SaveToFile(ScriptDynamicSprite *sds, const char* namm) {
  if (sds->slot == 0)
    quit("!DynamicSprite.SaveToFile: sprite has been deleted");

  char fileName[MAX_PATH];
  get_current_dir_path(fileName, namm);

  if (strchr(namm,'.') == NULL)
    strcat(fileName, ".bmp");

  if (save_bitmap(fileName, spriteset[sds->slot], palette))
    return 0; // failed

  return 1;  // successful
}

ScriptDynamicSprite* DynamicSprite_CreateFromSaveGame(int sgslot, int width, int height) {
  int slotnum = LoadSaveSlotScreenshot(sgslot, width, height);
  if (slotnum) {
    return new ScriptDynamicSprite(slotnum);
  }
  return NULL;
}

ScriptDynamicSprite* DynamicSprite_CreateFromFile(const char *filename) {
  int slotnum = LoadImageFile(filename);
  if (slotnum) {
    return new ScriptDynamicSprite(slotnum);
  }
  return NULL;
}

ScriptDynamicSprite* DynamicSprite_CreateFromScreenShot(int width, int height) {
  
  int gotSlot = spriteset.findFreeSlot();
  if (gotSlot <= 0)
    return NULL;

  if (width <= 0)
    width = virtual_screen->w;
  else
    width = multiply_up_coordinate(width);

  if (height <= 0)
    height = virtual_screen->h;
  else
    height = multiply_up_coordinate(height);

  BITMAP *newPic;
  if (!gfxDriver->UsesMemoryBackBuffer()) 
  {
    // D3D driver
    BITMAP *scrndump = create_bitmap_ex(final_col_dep, scrnwid, scrnhit);
    gfxDriver->GetCopyOfScreenIntoBitmap(scrndump);

    update_polled_stuff();

    if ((scrnwid != width) || (scrnhit != height))
    {
      newPic = create_bitmap_ex(final_col_dep, width, height);
      stretch_blit(scrndump, newPic,
                   0, 0, scrndump->w, scrndump->h,
                   0, 0, width, height);
      destroy_bitmap(scrndump);
    }
    else
    {
      newPic = scrndump;
    }
  }
  else
  {
    // resize the sprite to the requested size
    newPic = create_bitmap_ex(bitmap_color_depth(virtual_screen), width, height);

    stretch_blit(virtual_screen, newPic,
               0, 0, virtual_screen->w, virtual_screen->h,
               0, 0, width, height);
  }

  // replace the bitmap in the sprite set
  add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(newPic));
  return new ScriptDynamicSprite(gotSlot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite(int slot, int preserveAlphaChannel) {
  
  int gotSlot = spriteset.findFreeSlot();
  if (gotSlot <= 0)
    return NULL;

  if (!spriteset.doesSpriteExist(slot))
    quitprintf("DynamicSprite.CreateFromExistingSprite: sprite %d does not exist", slot);

  // create a new sprite as a copy of the existing one
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[slot]), spritewidth[slot], spriteheight[slot]);
  if (newPic == NULL)
    return NULL;

  blit(spriteset[slot], newPic, 0, 0, 0, 0, spritewidth[slot], spriteheight[slot]);

  bool hasAlpha = (preserveAlphaChannel) && ((game.spriteflags[slot] & SPF_ALPHACHANNEL) != 0);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(gotSlot, newPic, hasAlpha);
  return new ScriptDynamicSprite(gotSlot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *sds, int x, int y, int width, int height) 
{
  int gotSlot = spriteset.findFreeSlot();
  if (gotSlot <= 0)
    return NULL;

  // use DrawingSurface resolution
  sds->MultiplyCoordinates(&x, &y);
  sds->MultiplyCoordinates(&width, &height);

  sds->StartDrawing();

  if ((x < 0) || (y < 0) || (x + width > abuf->w) || (y + height > abuf->h))
    quit("!DynamicSprite.CreateFromDrawingSurface: requested area is outside the surface");

  int colDepth = bitmap_color_depth(abuf);

  block newPic = create_bitmap_ex(colDepth, width, height);
  if (newPic == NULL)
    return NULL;

  blit(abuf, newPic, x, y, 0, 0, width, height);

  sds->FinishedDrawingReadOnly();

  add_dynamic_sprite(gotSlot, newPic, (sds->hasAlphaChannel != 0));
  return new ScriptDynamicSprite(gotSlot);
}

ScriptDynamicSprite* DynamicSprite_Create(int width, int height, int alphaChannel) 
{
  multiply_up_coordinates(&width, &height);

  int gotSlot = spriteset.findFreeSlot();
  if (gotSlot <= 0)
    return NULL;

  block newPic = create_bitmap_ex(final_col_dep, width, height);
  if (newPic == NULL)
    return NULL;
  clear_to_color(newPic, bitmap_mask_color(newPic));

  if ((alphaChannel) && (final_col_dep < 32))
    alphaChannel = false;

  add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(newPic), alphaChannel != 0);
  return new ScriptDynamicSprite(gotSlot);
}

ScriptDynamicSprite* DynamicSprite_CreateFromExistingSprite_Old(int slot) 
{
  return DynamicSprite_CreateFromExistingSprite(slot, 0);
}

ScriptDynamicSprite* DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height) {

  if (frame == SCR_NO_VALUE) {
    frame = play.bg_frame;
  }
  else if ((frame < 0) || (frame >= thisroom.num_bscenes))
    quit("!DynamicSprite.CreateFromBackground: invalid frame specified");

  if (x1 == SCR_NO_VALUE) {
    x1 = 0;
    y1 = 0;
    width = play.room_width;
    height = play.room_height;
  }
  else if ((x1 < 0) || (y1 < 0) || (width < 1) || (height < 1) ||
      (x1 + width > play.room_width) || (y1 + height > play.room_height))
    quit("!DynamicSprite.CreateFromBackground: invalid co-ordinates specified");

  multiply_up_coordinates(&x1, &y1);
  multiply_up_coordinates(&width, &height);
  
  int gotSlot = spriteset.findFreeSlot();
  if (gotSlot <= 0)
    return NULL;

  // create a new sprite as a copy of the existing one
  block newPic = create_bitmap_ex(bitmap_color_depth(thisroom.ebscene[frame]), width, height);
  if (newPic == NULL)
    return NULL;

  blit(thisroom.ebscene[frame], newPic, x1, y1, 0, 0, width, height);

  // replace the bitmap in the sprite set
  add_dynamic_sprite(gotSlot, newPic);
  return new ScriptDynamicSprite(gotSlot);
}


// ** SCRIPT OVERLAY OBJECT

int ScriptOverlay::Dispose(const char *address, bool force) 
{
  // since the managed object is being deleted, remove the
  // reference so it doesn't try and dispose something else
  // with that handle later
  int overlayIndex = find_overlay_of_type(overlayId);
  if (overlayIndex >= 0)
  {
    screenover[overlayIndex].associatedOverlayHandle = 0;
  }

  // if this is being removed voluntarily (ie. pointer out of
  // scope) then remove the associateed overlay
  // Otherwise, it's a Restre Game or something so don't
  if ((!force) && (!isBackgroundSpeech) && (Overlay_GetValid(this)))
  {
    Remove();
  }

  delete this;
  return 1;
}

const char *ScriptOverlay::GetType() {
  return "Overlay";
}

int ScriptOverlay::Serialize(const char *address, char *buffer, int bufsize) {
  StartSerialize(buffer);
  SerializeInt(overlayId);
  SerializeInt(borderWidth);
  SerializeInt(borderHeight);
  SerializeInt(isBackgroundSpeech);
  return EndSerialize();
}

void ScriptOverlay::Unserialize(int index, const char *serializedData, int dataSize) {
  StartUnserialize(serializedData, dataSize);
  overlayId = UnserializeInt();
  borderWidth = UnserializeInt();
  borderHeight = UnserializeInt();
  isBackgroundSpeech = UnserializeInt();
  ccRegisterUnserializedObject(index, this, this);
}

void ScriptOverlay::Remove() 
{
  int overlayIndex = find_overlay_of_type(overlayId);
  if (overlayIndex < 0)
  {
    quit("ScriptOverlay::Remove: overlay is not there!");
  }
  remove_screen_overlay_index(overlayIndex);
  overlayId = -1;
}


ScriptOverlay::ScriptOverlay() {
  overlayId = -1;
}


CCGUIObject ccDynamicGUIObject;
CCCharacter ccDynamicCharacter;
CCHotspot   ccDynamicHotspot;
CCRegion    ccDynamicRegion;
CCInventory ccDynamicInv;
CCGUI       ccDynamicGUI;
CCObject    ccDynamicObject;
CCDialog    ccDynamicDialog;
ScriptString myScriptStringImpl;
ScriptDialogOptionsRendering ccDialogOptionsRendering;
ScriptDrawingSurface* dialogOptionsRenderingSurface;

// *** De-serialization of script objects


struct AGSDeSerializer : ICCObjectReader {

  virtual void Unserialize(int index, const char *objectType, const char *serializedData, int dataSize) {
    if (strcmp(objectType, "GUIObject") == 0) {
      ccDynamicGUIObject.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "Character") == 0) {
      ccDynamicCharacter.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "Hotspot") == 0) {
      ccDynamicHotspot.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "Region") == 0) {
      ccDynamicRegion.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "Inventory") == 0) {
      ccDynamicInv.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "Dialog") == 0) {
      ccDynamicDialog.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "GUI") == 0) {
      ccDynamicGUI.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "Object") == 0) {
      ccDynamicObject.Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "String") == 0) {
      ScriptString *scf = new ScriptString();
      scf->Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "File") == 0) {
      // files cannot be restored properly -- so just recreate
      // the object; attempting any operations on it will fail
      sc_File *scf = new sc_File();
      ccRegisterUnserializedObject(index, scf, scf);
    }
    else if (strcmp(objectType, "Overlay") == 0) {
      ScriptOverlay *scf = new ScriptOverlay();
      scf->Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "DateTime") == 0) {
      ScriptDateTime *scf = new ScriptDateTime();
      scf->Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "ViewFrame") == 0) {
      ScriptViewFrame *scf = new ScriptViewFrame();
      scf->Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "DynamicSprite") == 0) {
      ScriptDynamicSprite *scf = new ScriptDynamicSprite();
      scf->Unserialize(index, serializedData, dataSize);
    }
    else if (strcmp(objectType, "DrawingSurface") == 0) {
      ScriptDrawingSurface *sds = new ScriptDrawingSurface();
      sds->Unserialize(index, serializedData, dataSize);

      if (sds->isLinkedBitmapOnly)
      {
        dialogOptionsRenderingSurface = sds;
      }
    }
    else if (strcmp(objectType, "DialogOptionsRendering") == 0)
    {
      ccDialogOptionsRendering.Unserialize(index, serializedData, dataSize);
    }
    else if (!unserialize_audio_script_object(index, objectType, serializedData, dataSize)) 
    {
      // check if the type is read by a plugin
      for (int ii = 0; ii < numPluginReaders; ii++) {
        if (strcmp(objectType, pluginReaders[ii].type) == 0) {
          pluginReaders[ii].reader->Unserialize(index, serializedData, dataSize);
          return;
        }
      }
      quitprintf("Unserialise: unknown object type: '%s'", objectType);
    }
  }

};

AGSDeSerializer ccUnserializer;

void export_gui_controls(int ee) {

  for (int ff = 0; ff < guis[ee].numobjs; ff++) {
    if (guis[ee].objs[ff]->scriptName[0] != 0)
      ccAddExternalSymbol(guis[ee].objs[ff]->scriptName, guis[ee].objs[ff]);

    ccRegisterManagedObject(guis[ee].objs[ff], &ccDynamicGUIObject);
  }
}

void unexport_gui_controls(int ee) {

  for (int ff = 0; ff < guis[ee].numobjs; ff++) {
    if (guis[ee].objs[ff]->scriptName[0] != 0)
      ccRemoveExternalSymbol(guis[ee].objs[ff]->scriptName);

    if (!ccUnRegisterManagedObject(guis[ee].objs[ff]))
      quit("unable to unregister guicontrol object");
  }
}

int create_global_script() {
  ccSetOption(SCOPT_AUTOIMPORT, 1);
  for (int kk = 0; kk < numScriptModules; kk++) {
    moduleInst[kk] = ccCreateInstance(scriptModules[kk]);
    if (moduleInst[kk] == NULL)
      return -3;
    // create a forked instance for rep_exec_always
    moduleInstFork[kk] = ccForkInstance(moduleInst[kk]);
    if (moduleInstFork[kk] == NULL)
      return -3;

    moduleRepExecAddr[kk] = ccGetSymbolAddr(moduleInst[kk], REP_EXEC_NAME);
  }
  gameinst = ccCreateInstance(gamescript);
  if (gameinst == NULL)
    return -3;
  // create a forked instance for rep_exec_always
  gameinstFork = ccForkInstance(gameinst);
  if (gameinstFork == NULL)
    return -3;

  if (dialogScriptsScript != NULL)
  {
    dialogScriptsInst = ccCreateInstance(dialogScriptsScript);
    if (dialogScriptsInst == NULL)
      return -3;
  }

  ccSetOption(SCOPT_AUTOIMPORT, 0);
  return 0;
}

void allocate_memory_for_views(int viewCount)
{
  views = (ViewStruct*)calloc(sizeof(ViewStruct) * viewCount, 1);
  game.viewNames = (char**)malloc(sizeof(char*) * viewCount);
  game.viewNames[0] = (char*)malloc(MAXVIEWNAMELENGTH * viewCount);

  for (int i = 1; i < viewCount; i++)
  {
    game.viewNames[i] = game.viewNames[0] + (MAXVIEWNAMELENGTH * i);
  }
}

int adjust_pixel_size_for_loaded_data(int size, int filever)
{
  if (filever < 37)
  {
    return multiply_up_coordinate(size);
  }
  return size;
}

void adjust_pixel_sizes_for_loaded_data(int *x, int *y, int filever)
{
  x[0] = adjust_pixel_size_for_loaded_data(x[0], filever);
  y[0] = adjust_pixel_size_for_loaded_data(y[0], filever);
}

void adjust_sizes_for_resolution(int filever)
{
  int ee;
  for (ee = 0; ee < game.numcursors; ee++) 
  {
    game.mcurs[ee].hotx = adjust_pixel_size_for_loaded_data(game.mcurs[ee].hotx, filever);
    game.mcurs[ee].hoty = adjust_pixel_size_for_loaded_data(game.mcurs[ee].hoty, filever);
  }

  for (ee = 0; ee < game.numinvitems; ee++) 
  {
    adjust_pixel_sizes_for_loaded_data(&game.invinfo[ee].hotx, &game.invinfo[ee].hoty, filever);
  }

  for (ee = 0; ee < game.numgui; ee++) 
  {
    GUIMain*cgp=&guis[ee];
    adjust_pixel_sizes_for_loaded_data(&cgp->x, &cgp->y, filever);
    if (cgp->wid < 1)
      cgp->wid = 1;
    if (cgp->hit < 1)
      cgp->hit = 1;
    // Temp fix for older games
    if (cgp->wid == BASEWIDTH - 1)
      cgp->wid = BASEWIDTH;

    adjust_pixel_sizes_for_loaded_data(&cgp->wid, &cgp->hit, filever);
    
    cgp->popupyp = adjust_pixel_size_for_loaded_data(cgp->popupyp, filever);

    for (ff = 0; ff < cgp->numobjs; ff++) 
    {
      adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->x, &cgp->objs[ff]->y, filever);
      adjust_pixel_sizes_for_loaded_data(&cgp->objs[ff]->wid, &cgp->objs[ff]->hit, filever);
      cgp->objs[ff]->activated=0;
    }
  }

  if ((filever >= 37) && (game.options[OPT_NATIVECOORDINATES] == 0) &&
      (game.default_resolution > 2))
  {
    // New 3.1 format game file, but with Use Native Coordinates off

    for (ee = 0; ee < game.numcharacters; ee++) 
    {
      game.chars[ee].x /= 2;
      game.chars[ee].y /= 2;
    }

    for (ee = 0; ee < numguiinv; ee++)
    {
      guiinv[ee].itemWidth /= 2;
      guiinv[ee].itemHeight /= 2;
    }
  }

}

int load_game_file() {
  int ee, bb;
  char teststr[31];

  game_paused = 0;  // reset the game paused flag
  ifacepopped = -1;

  FILE*iii = clibfopen("game28.dta","rb");
  if (iii==NULL) return -1;

  our_eip=-18;
  setup_script_exports();

  our_eip=-16;

  teststr[30]=0;
  fread(&teststr[0],30,1,iii);
  int filever=getw(iii);
  if (filever < 42) {
    fclose(iii);
    return -2; 
  }
  int engineverlen = getw(iii);
  char engineneeds[20];
  // MACPORT FIX 13/6/5: switch 'size' and 'nmemb' so it doesn't treat the string as an int
  fread(&engineneeds[0], sizeof(char), engineverlen, iii);
  engineneeds[engineverlen] = 0;

  if (filever > GAME_FILE_VERSION) {
    platform->DisplayAlert("This game requires a newer version of AGS (%s). It cannot be run.", engineneeds);
    fclose(iii);
    return -2;
  }

  if (strcmp (engineneeds, ACI_VERSION_TEXT) > 0)
    platform->DisplayAlert("This game requires a newer version of AGS (%s). It may not run correctly.", engineneeds);
  
  {
    int major, minor;
    sscanf(engineneeds, "%d.%d", &major, &minor);
    engineNeedsAsInt = 100*major + minor;
  }

  loaded_game_file_version = filever;

  game.charScripts = NULL;
  game.invScripts = NULL;
  memset(&game.spriteflags[0], 0, MAX_SPRITES);

#ifndef ALLEGRO_BIG_ENDIAN
  fread(&game, sizeof (GameSetupStructBase), 1, iii);
#else
  GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
  gameBase->ReadFromFile(iii);
#endif
    
  if (game.numfonts > MAX_FONTS)
    quit("!This game requires a newer version of AGS. Too many fonts for this version to handle.");

  fread(&game.guid[0], 1, MAX_GUID_LENGTH, iii);
  fread(&game.saveGameFileExtension[0], 1, MAX_SG_EXT_LENGTH, iii);
  fread(&game.saveGameFolderName[0], 1, MAX_SG_FOLDER_LEN, iii);

  if (game.saveGameFileExtension[0] != 0)
    sprintf(saveGameSuffix, ".%s", game.saveGameFileExtension);
  else
    saveGameSuffix[0] = 0;

  fread(&game.fontflags[0], 1, game.numfonts, iii);
  fread(&game.fontoutline[0], 1, game.numfonts, iii);

  int numToRead = getw(iii);
  if (numToRead > MAX_SPRITES) {
    quit("Too many sprites; need newer AGS version");
  }
  fread(&game.spriteflags[0], 1, numToRead, iii);
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems, iii);
#else
  for (int iteratorCount = 0; iteratorCount < game.numinvitems; ++iteratorCount)
  {
    game.invinfo[iteratorCount].ReadFromFile(iii);
  }
#endif

  if (game.numcursors > MAX_CURSOR)
    quit("Too many cursors: need newer AGS version");
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&game.mcurs[0], sizeof(MouseCursor), game.numcursors, iii);
#else
  for (int iteratorCount = 0; iteratorCount < game.numcursors; ++iteratorCount)
  {
    game.mcurs[iteratorCount].ReadFromFile(iii);
  }
#endif

  numGlobalVars = 0;
  game.charScripts = new InteractionScripts*[game.numcharacters];
  game.invScripts = new InteractionScripts*[game.numinvitems];
  for (bb = 0; bb < game.numcharacters; bb++) {
    game.charScripts[bb] = new InteractionScripts();
    deserialize_interaction_scripts(iii, game.charScripts[bb]);
  }
  for (bb = 1; bb < game.numinvitems; bb++) {
    game.invScripts[bb] = new InteractionScripts();
    deserialize_interaction_scripts(iii, game.invScripts[bb]);
  }

  if (game.dict != NULL) {
    game.dict = (WordsDictionary*)malloc(sizeof(WordsDictionary));
    read_dictionary (game.dict, iii);
  }

  if (game.compiled_script == NULL)
    quit("No global script in game; data load error");

  gamescript = fread_script(iii);
  if (gamescript == NULL)
    quit("Global script load failed; need newer version?");

  dialogScriptsScript = fread_script(iii);
  if (dialogScriptsScript == NULL)
    quit("Dialog scripts load failed; need newer version?");

  numScriptModules = getw(iii);
  if (numScriptModules > MAX_SCRIPT_MODULES)
    quit("too many script modules; need newer version?");

  for (bb = 0; bb < numScriptModules; bb++) {
    scriptModules[bb] = fread_script(iii);
    if (scriptModules[bb] == NULL)
      quit("Script module load failure; need newer version?");
    moduleInst[bb] = NULL;
    moduleInstFork[bb] = NULL;
    moduleRepExecAddr[bb] = NULL;
  }
  
  our_eip=-15;

  charextra = (CharacterExtras*)calloc(game.numcharacters, sizeof(CharacterExtras));
  mls = (MoveList*)calloc(game.numcharacters + MAX_INIT_SPR + 1, sizeof(MoveList));
  actSpsCount = game.numcharacters + MAX_INIT_SPR + 2;
  actsps = (block*)calloc(actSpsCount, sizeof(block));
  actspsbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
  actspswb = (block*)calloc(actSpsCount, sizeof(block));
  actspswbbmp = (IDriverDependantBitmap**)calloc(actSpsCount, sizeof(IDriverDependantBitmap*));
  actspswbcache = (CachedActSpsData*)calloc(actSpsCount, sizeof(CachedActSpsData));
  game.charProps = (CustomProperties*)calloc(game.numcharacters, sizeof(CustomProperties));

  allocate_memory_for_views(game.numviews);

  for (int iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
  {
    views[iteratorCount].ReadFromFile(iii);
  }

  our_eip=-14;

  game.chars=(CharacterInfo*)calloc(1,sizeof(CharacterInfo)*game.numcharacters+5);
#ifndef ALLEGRO_BIG_ENDIAN
  fread(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,iii);
#else
  for (int iteratorCount = 0; iteratorCount < game.numcharacters; ++iteratorCount)
  {
    game.chars[iteratorCount].ReadFromFile(iii);
  }
#endif
  charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*game.numcharacters+5);

  fread(&game.lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50, iii);

  for (ee=0;ee<MAXGLOBALMES;ee++) {
    if (game.messages[ee]==NULL) continue;
    game.messages[ee]=(char*)malloc(500);
    read_string_decrypt(iii, game.messages[ee]);
  }
  set_default_glmsg (983, "Sorry, not now.");
  set_default_glmsg (984, "Restore");
  set_default_glmsg (985, "Cancel");
  set_default_glmsg (986, "Select a game to restore:");
  set_default_glmsg (987, "Save");
  set_default_glmsg (988, "Type a name to save as:");
  set_default_glmsg (989, "Replace");
  set_default_glmsg (990, "The save directory is full. You must replace an existing game:");
  set_default_glmsg (991, "Replace:");
  set_default_glmsg (992, "With:");
  set_default_glmsg (993, "Quit");
  set_default_glmsg (994, "Play");
  set_default_glmsg (995, "Are you sure you want to quit?");
  our_eip=-13;

  dialog=(DialogTopic*)malloc(sizeof(DialogTopic)*game.numdialog+5);

#ifndef ALLEGRO_BIG_ENDIAN
  fread(&dialog[0],sizeof(DialogTopic),game.numdialog,iii);
#else
  for (int iteratorCount = 0; iteratorCount < game.numdialog; ++iteratorCount)
  {
    dialog[iteratorCount].ReadFromFile(iii);
  }
#endif

  read_gui(iii,guis,&game, &guis);

  for (bb = 0; bb < numguilabels; bb++) {
    // labels are not clickable by default
    guilabels[bb].SetClickable(false);
  }

  play.gui_draw_order = (int*)calloc(game.numgui * sizeof(int), 1);

  platform->ReadPluginsFromDisk(iii);

  if (game.propSchema.UnSerialize(iii))
    quit("load room: unable to deserialize prop schema");

  int errors = 0;

  for (bb = 0; bb < game.numcharacters; bb++)
    errors += game.charProps[bb].UnSerialize (iii);
  for (bb = 0; bb < game.numinvitems; bb++)
    errors += game.invProps[bb].UnSerialize (iii);

  if (errors > 0)
    quit("LoadGame: errors encountered reading custom props");

  for (bb = 0; bb < game.numviews; bb++)
    fgetstring_limit(game.viewNames[bb], iii, MAXVIEWNAMELENGTH);

  for (bb = 0; bb < game.numinvitems; bb++)
    fgetstring_limit(game.invScriptNames[bb], iii, MAX_SCRIPT_NAME_LEN);

  for (bb = 0; bb < game.numdialog; bb++)
    fgetstring_limit(game.dialogScriptNames[bb], iii, MAX_SCRIPT_NAME_LEN);

  if (filever >= 41)
  {
    game.audioClipTypeCount = getw(iii);

    if (game.audioClipTypeCount > MAX_AUDIO_TYPES)
      quit("LoadGame: too many audio types");

    game.audioClipTypes = (AudioClipType*)malloc(game.audioClipTypeCount * sizeof(AudioClipType));
    fread(&game.audioClipTypes[0], sizeof(AudioClipType), game.audioClipTypeCount, iii);
    game.audioClipCount = getw(iii);
    game.audioClips = (ScriptAudioClip*)malloc(game.audioClipCount * sizeof(ScriptAudioClip));
    fread(&game.audioClips[0], sizeof(ScriptAudioClip), game.audioClipCount, iii);
    play.score_sound = getw(iii);
  }
  else
  {
    game.audioClipCount = 0;
    play.score_sound = -1;
  }

  if ((filever >= 36) && (game.options[OPT_DEBUGMODE] != 0))
  {
    game.roomCount = getw(iii);
    game.roomNumbers = (int*)malloc(game.roomCount * sizeof(int));
    game.roomNames = (char**)malloc(game.roomCount * sizeof(char*));
    for (bb = 0; bb < game.roomCount; bb++)
    {
      game.roomNumbers[bb] = getw(iii);
      fgetstring_limit(pexbuf, iii, sizeof(pexbuf));
      game.roomNames[bb] = (char*)malloc(strlen(pexbuf) + 1);
      strcpy(game.roomNames[bb], pexbuf);
    }
  }
  else
  {
    game.roomCount = 0;
  }

  fclose(iii);

  update_gui_zorder();

  if (game.numfonts == 0)
    return -2;  // old v2.00 version

  our_eip=-11;
  characterScriptObjNames = (char**)malloc(sizeof(char*) * game.numcharacters);

  for (ee=0;ee<game.numcharacters;ee++) {
    game.chars[ee].walking = 0;
    game.chars[ee].animating = 0;
    game.chars[ee].pic_xoffs = 0;
    game.chars[ee].pic_yoffs = 0;
    game.chars[ee].blinkinterval = 140;
    game.chars[ee].blinktimer = game.chars[ee].blinkinterval;
    game.chars[ee].index_id = ee;
    game.chars[ee].blocking_width = 0;
    game.chars[ee].blocking_height = 0;
    game.chars[ee].prevroom = -1;
    game.chars[ee].loop = 0;
    game.chars[ee].frame = 0;
    game.chars[ee].walkwait = -1;
    ccRegisterManagedObject(&game.chars[ee], &ccDynamicCharacter);

    // export the character's script object
    characterScriptObjNames[ee] = (char*)malloc(strlen(game.chars[ee].scrname) + 5);
    strcpy(characterScriptObjNames[ee], game.chars[ee].scrname);

    ccAddExternalSymbol(characterScriptObjNames[ee], &game.chars[ee]);
  }

  for (ee = 0; ee < MAX_HOTSPOTS; ee++) {
    scrHotspot[ee].id = ee;
    scrHotspot[ee].reserved = 0;

    ccRegisterManagedObject(&scrHotspot[ee], &ccDynamicHotspot);
  }

  for (ee = 0; ee < MAX_REGIONS; ee++) {
    scrRegion[ee].id = ee;
    scrRegion[ee].reserved = 0;

    ccRegisterManagedObject(&scrRegion[ee], &ccDynamicRegion);
  }

  for (ee = 0; ee < MAX_INV; ee++) {
    scrInv[ee].id = ee;
    scrInv[ee].reserved = 0;

    ccRegisterManagedObject(&scrInv[ee], &ccDynamicInv);

    if (game.invScriptNames[ee][0] != 0)
      ccAddExternalSymbol(game.invScriptNames[ee], &scrInv[ee]);
  }

  for (ee = 0; ee < game.numdialog; ee++) {
    scrDialog[ee].id = ee;
    scrDialog[ee].reserved = 0;

    ccRegisterManagedObject(&scrDialog[ee], &ccDynamicDialog);

    if (game.dialogScriptNames[ee][0] != 0)
      ccAddExternalSymbol(game.dialogScriptNames[ee], &scrDialog[ee]);
  }

  scrGui = (ScriptGUI*)malloc(sizeof(ScriptGUI) * game.numgui);
  for (ee = 0; ee < game.numgui; ee++) {
    scrGui[ee].gui = NULL;
    scrGui[ee].id = -1;
  }

  guiScriptObjNames = (char**)malloc(sizeof(char*) * game.numgui);

  for (ee=0;ee<game.numgui;ee++) {
    guis[ee].rebuild_array();
    if ((guis[ee].popup == POPUP_NONE) || (guis[ee].popup == POPUP_NOAUTOREM))
      guis[ee].on = 1;
    else
      guis[ee].on = 0;

    // export all the GUI's controls
    export_gui_controls(ee);

    // copy the script name to its own memory location
    // because ccAddExtSymbol only keeps a reference
    guiScriptObjNames[ee] = (char*)malloc(21);
    strcpy(guiScriptObjNames[ee], guis[ee].name);

    scrGui[ee].gui = &guis[ee];
    scrGui[ee].id = ee;

    ccAddExternalSymbol(guiScriptObjNames[ee], &scrGui[ee]);
    ccRegisterManagedObject(&scrGui[ee], &ccDynamicGUI);
  }

  //ccRegisterManagedObject(&dummygui, NULL);
  //ccRegisterManagedObject(&dummyguicontrol, NULL);

  our_eip=-22;
  for (ee=0;ee<game.numfonts;ee++) 
  {
    int fontsize = game.fontflags[ee] & FFLG_SIZEMASK;
    if (fontsize == 0)
      fontsize = 8;

    if ((game.options[OPT_NOSCALEFNT] == 0) && (game.default_resolution > 2))
      fontsize *= 2;

    if (!wloadfont_size(ee, fontsize))
      quitprintf("Unable to load font %d, no renderer could load a matching file", ee);
  }

  wtexttransparent(TEXTFG);
  play.fade_effect=game.options[OPT_FADETYPE];

  our_eip=-21;

  for (ee = 0; ee < MAX_INIT_SPR; ee++) {
    ccRegisterManagedObject(&scrObj[ee], &ccDynamicObject);
  }

  register_audio_script_objects();

  ccRegisterManagedObject(&ccDialogOptionsRendering, &ccDialogOptionsRendering);
  
  dialogOptionsRenderingSurface = new ScriptDrawingSurface();
  dialogOptionsRenderingSurface->isLinkedBitmapOnly = true;
  long dorsHandle = ccRegisterManagedObject(dialogOptionsRenderingSurface, dialogOptionsRenderingSurface);
  ccAddObjectReference(dorsHandle);

  ccAddExternalSymbol("character",&game.chars[0]);
  setup_player_character(game.playercharacter);
  ccAddExternalSymbol("player", &_sc_PlayerCharPtr);
  ccAddExternalSymbol("object",&scrObj[0]);
  ccAddExternalSymbol("gui",&scrGui[0]);
  ccAddExternalSymbol("hotspot",&scrHotspot[0]);
  ccAddExternalSymbol("region",&scrRegion[0]);
  ccAddExternalSymbol("inventory",&scrInv[0]);
  ccAddExternalSymbol("dialog", &scrDialog[0]);

  our_eip = -23;
  platform->StartPlugins();

  our_eip = -24;
  ccSetScriptAliveTimer(150000);
  ccSetStringClassImpl(&myScriptStringImpl);
  if (create_global_script())
    return -3;

  return 0;
}

void free_do_once_tokens() 
{
  for (int i = 0; i < play.num_do_once_tokens; i++)
  {
    free(play.do_once_tokens[i]);
  }
  if (play.do_once_tokens != NULL) 
  {
    free(play.do_once_tokens);
    play.do_once_tokens = NULL;
  }
  play.num_do_once_tokens = 0;
}


// Free all the memory associated with the game
void unload_game_file() {
  int bb, ee;

  for (bb = 0; bb < game.numcharacters; bb++) {
    if (game.charScripts != NULL)
      delete game.charScripts[bb];

    if (game.intrChar != NULL)
    {
      if (game.intrChar[bb] != NULL)
        delete game.intrChar[bb];
      game.intrChar[bb] = NULL;
    }
    free(characterScriptObjNames[bb]);
    game.charProps[bb].reset();
  }
  if (game.intrChar != NULL)
  {
    free(game.intrChar);
    game.intrChar = NULL;
  }
  free(characterScriptObjNames);
  free(charextra);
  free(mls);
  free(actsps);
  free(actspsbmp);
  free(actspswb);
  free(actspswbbmp);
  free(actspswbcache);
  free(game.charProps);

  for (bb = 1; bb < game.numinvitems; bb++) {
    if (game.invScripts != NULL)
      delete game.invScripts[bb];
    if (game.intrInv[bb] != NULL)
      delete game.intrInv[bb];
    game.intrInv[bb] = NULL;
  }

  if (game.charScripts != NULL)
  {
    delete game.charScripts;
    delete game.invScripts;
    game.charScripts = NULL;
    game.invScripts = NULL;
  }

  if (game.dict != NULL) {
    game.dict->free_memory();
    free (game.dict);
    game.dict = NULL;
  }

  if ((gameinst != NULL) && (gameinst->pc != 0))
    quit("Error: unload_game called while script still running");
    //ccAbortAndDestroyInstance (gameinst);
  else {
    ccFreeInstance(gameinstFork);
    ccFreeInstance(gameinst);
    gameinstFork = NULL;
    gameinst = NULL;
  }

  ccFreeScript (gamescript);
  gamescript = NULL;

  if ((dialogScriptsInst != NULL) && (dialogScriptsInst->pc != 0))
    quit("Error: unload_game called while dialog script still running");
  else if (dialogScriptsInst != NULL)
  {
    ccFreeInstance(dialogScriptsInst);
    dialogScriptsInst = NULL;
  }

  if (dialogScriptsScript != NULL)
  {
    ccFreeScript(dialogScriptsScript);
    dialogScriptsScript = NULL;
  }

  for (ee = 0; ee < numScriptModules; ee++) {
    ccFreeInstance(moduleInstFork[ee]);
    ccFreeInstance(moduleInst[ee]);
    ccFreeScript(scriptModules[ee]);
  }
  numScriptModules = 0;

  if (game.audioClipCount > 0)
  {
    free(game.audioClips);
    game.audioClipCount = 0;
    free(game.audioClipTypes);
    game.audioClipTypeCount = 0;
  }

  free(game.viewNames[0]);
  free(game.viewNames);
  free (views);
  views = NULL;

  free (game.chars);
  game.chars = NULL;

  free (charcache);
  charcache = NULL;

  if (splipsync != NULL)
  {
    for (ee = 0; ee < numLipLines; ee++)
    {
      free(splipsync[ee].endtimeoffs);
      free(splipsync[ee].frame);
    }
    free(splipsync);
    splipsync = NULL;
    numLipLines = 0;
    curLipLine = -1;
  }

  for (ee=0;ee < MAXGLOBALMES;ee++) {
    if (game.messages[ee]==NULL) continue;
    free (game.messages[ee]);
    game.messages[ee] = NULL;
  }

  for (ee = 0; ee < game.roomCount; ee++)
  {
    free(game.roomNames[ee]);
  }
  if (game.roomCount > 0)
  {
    free(game.roomNames);
    free(game.roomNumbers);
    game.roomCount = 0;
  }

  for (ee=0;ee<game.numdialog;ee++) {
    if (dialog[ee].optionscripts!=NULL)
      free (dialog[ee].optionscripts);
    dialog[ee].optionscripts = NULL;
  }
  free (dialog);
  dialog = NULL;

  for (ee = 0; ee < game.numgui; ee++) {
    free (guibg[ee]);
    guibg[ee] = NULL;
    free(guiScriptObjNames[ee]);
  }

  free(guiScriptObjNames);
  free(guibg);
  free (guis);
  guis = NULL;
  free(scrGui);

  platform->ShutdownPlugins();
  ccRemoveAllSymbols();
  ccUnregisterAllObjects();

  for (ee=0;ee<game.numfonts;ee++)
    wfreefont(ee);

  free_do_once_tokens();
  free(play.gui_draw_order);

  free (roomstats);
  roomstats=(RoomStatus*)calloc(sizeof(RoomStatus),MAX_ROOMS);
  for (ee=0;ee<MAX_ROOMS;ee++) {
    roomstats[ee].beenhere=0;
    roomstats[ee].numobj=0;
    roomstats[ee].tsdatasize=0;
    roomstats[ee].tsdata=NULL;
  }
  
}

// **** text script exported functions

int IsMusicPlaying() {
  // in case they have a "while (IsMusicPlaying())" loop
  if ((play.fast_forward) && (play.skip_until_char_stops < 0))
    return 0;

  if (usetup.midicard == MIDI_NONE)
    return 0;

  if (current_music_type != 0) {
    if (channels[SCHAN_MUSIC] == NULL)
      current_music_type = 0;
    else if (channels[SCHAN_MUSIC]->done == 0)
      return 1;
    else if ((crossFading > 0) && (channels[crossFading] != NULL))
      return 1;
    return 0;
  }

  return 0;
}

void clear_music_cache() {

  if (cachedQueuedMusic != NULL) {
    cachedQueuedMusic->destroy();
    delete cachedQueuedMusic;
    cachedQueuedMusic = NULL;
  }

}

int PlayMusicQueued(int musnum) {

  // Just get the queue size
  if (musnum < 0)
    return play.music_queue_size;

  if ((IsMusicPlaying() == 0) && (play.music_queue_size == 0)) {
    newmusic(musnum);
    return 0;
  }

  if (play.music_queue_size >= MAX_QUEUED_MUSIC) {
    DEBUG_CONSOLE("Too many queued music, cannot add %d", musnum);
    return 0;
  }

  if ((play.music_queue_size > 0) && 
      (play.music_queue[play.music_queue_size - 1] >= QUEUED_MUSIC_REPEAT)) {
    quit("!PlayMusicQueued: cannot queue music after a repeating tune has been queued");
  }

  if (play.music_repeat) {
    DEBUG_CONSOLE("Queuing music %d to loop", musnum);
    musnum += QUEUED_MUSIC_REPEAT;
  }
  else {
    DEBUG_CONSOLE("Queuing music %d", musnum);
  }

  play.music_queue[play.music_queue_size] = musnum;
  play.music_queue_size++;

  if (play.music_queue_size == 1) {

    clear_music_cache();

    cachedQueuedMusic = load_music_from_disk(musnum, (play.music_repeat > 0));
  }

  return play.music_queue_size;
}

void play_next_queued() {
  // check if there's a queued one to play
  if (play.music_queue_size > 0) {

    int tuneToPlay = play.music_queue[0];

    if (tuneToPlay >= QUEUED_MUSIC_REPEAT) {
      // Loop it!
      play.music_repeat++;
      play_new_music(tuneToPlay - QUEUED_MUSIC_REPEAT, cachedQueuedMusic);
      play.music_repeat--;
    }
    else {
      // Don't loop it!
      int repeatWas = play.music_repeat;
      play.music_repeat = 0;
      play_new_music(tuneToPlay, cachedQueuedMusic);
      play.music_repeat = repeatWas;
    }

    // don't free the memory, as it has been transferred onto the
    // main music channel
    cachedQueuedMusic = NULL;

    play.music_queue_size--;
    for (int i = 0; i < play.music_queue_size; i++)
      play.music_queue[i] = play.music_queue[i + 1];

    if (play.music_queue_size > 0)
      cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
  }

}

int calculate_max_volume() {
  // quieter so that sounds can be heard better
  int newvol=play.music_master_volume + ((int)thisroom.options[ST_VOLUME]) * 30;
  if (newvol>255) newvol=255;
  if (newvol<0) newvol=0;

  if (play.fast_forward)
    newvol = 0;

  return newvol;
}

void update_polled_stuff()
{
  update_polled_stuff(true);
}

// add/remove the volume drop to the audio channels while speech is playing
void apply_volume_drop_modifier(bool applyModifier)
{
  for (int i = 0; i < MAX_SOUND_CHANNELS; i++) 
  {
    if ((channels[i] != NULL) && (channels[i]->done == 0) && (channels[i]->sourceClip != NULL))
    {
      if (applyModifier)
      {
        int audioType = ((ScriptAudioClip*)channels[i]->sourceClip)->type;
        channels[i]->volModifier = -(game.audioClipTypes[audioType].volume_reduction_while_speech_playing * 255 / 100);
      }
      else
        channels[i]->volModifier = 0;

      channels[i]->set_volume(channels[i]->vol);
    }
  }
}

void update_polled_stuff(bool checkForDebugMessages) {
  UPDATE_MP3

  if (want_exit) {
    want_exit = 0;
    quit("||exit!");
  }
  if (mvolcounter > update_music_at) {
    update_music_volume();
    apply_volume_drop_modifier(false);
    update_music_at = 0;
    mvolcounter = 0;
    update_ambient_sound_vol();
  }

  if ((editor_debugging_initialized) && (checkForDebugMessages))
    check_for_messages_from_editor();
}

// Update the music, and advance the crossfade on a step
// (this should only be called once per game loop)
void update_polled_stuff_and_crossfade () {
  update_polled_stuff ();

  audio_update_polled_stuff();

  if (crossFading) {
    crossFadeStep++;
    update_music_volume();
  }

  // Check if the current music has finished playing
  if ((play.cur_music_number >= 0) && (play.fast_forward == 0)) {
    if (IsMusicPlaying() == 0) {
      // The current music has finished
      play.cur_music_number = -1;
      play_next_queued();
    }
    else if ((game.options[OPT_CROSSFADEMUSIC] > 0) &&
             (play.music_queue_size > 0) && (!crossFading)) {
      // want to crossfade, and new tune in the queue
      int curpos = channels[SCHAN_MUSIC]->get_pos_ms();
      int muslen = channels[SCHAN_MUSIC]->get_length_ms();
      if ((curpos > 0) && (muslen > 0)) {
        // we want to crossfade, and we know how far through
        // the tune we are
        int takesSteps = calculate_max_volume() / game.options[OPT_CROSSFADEMUSIC];
        int takesMs = (takesSteps * 1000) / frames_per_second;
        if (curpos >= muslen - takesMs)
          play_next_queued();
      }
    }
  }

}

void do_corner(int sprn,int xx1,int yy1,int typx,int typy) {
  if (sprn<0) return;
  block thisone = spriteset[sprn];
  if (thisone == NULL)
    thisone = spriteset[0];

  put_sprite_256(xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],thisone);
//  wputblock(xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],thisone,1);
}

int get_but_pic(GUIMain*guo,int indx) {
  return guibuts[guo->objrefptr[indx] & 0x000ffff].pic;
  }

void ClaimEvent() {
  if (eventClaimed == EVENT_NONE)
    quit("!ClaimEvent: no event to claim");

  eventClaimed = EVENT_CLAIMED;
}

void draw_button_background(int xx1,int yy1,int xx2,int yy2,GUIMain*iep) {
  if (iep==NULL) {  // standard window
    rectfill(abuf,xx1,yy1,xx2,yy2,get_col8_lookup(15));
    rect(abuf,xx1,yy1,xx2,yy2,get_col8_lookup(16));
/*    wsetcolor(opts.tws.backcol); wbar(xx1,yy1,xx2,yy2);
    wsetcolor(opts.tws.textcol); wrectangle(xx1+1,yy1+1,xx2-1,yy2-1);*/
    }
  else {
    if (iep->bgcol >= 0) wsetcolor(iep->bgcol);
    else wsetcolor(0); // black backrgnd behind picture

    if (iep->bgcol > 0)
      wbar(xx1,yy1,xx2,yy2);

    int leftRightWidth = spritewidth[get_but_pic(iep,4)];
    int topBottomHeight = spriteheight[get_but_pic(iep,6)];
    if (iep->bgpic>0) {
      // offset the background image and clip it so that it is drawn
      // such that the border graphics can have a transparent outside
      // edge
      int bgoffsx = xx1 - leftRightWidth / 2;
      int bgoffsy = yy1 - topBottomHeight / 2;
      set_clip(abuf, bgoffsx, bgoffsy, xx2 + leftRightWidth / 2, yy2 + topBottomHeight / 2);
      int bgfinishx = xx2;
      int bgfinishy = yy2;
      int bgoffsyStart = bgoffsy;
      while (bgoffsx <= bgfinishx)
      {
        bgoffsy = bgoffsyStart;
        while (bgoffsy <= bgfinishy)
        {
          wputblock(bgoffsx, bgoffsy, spriteset[iep->bgpic],0);
          bgoffsy += spriteheight[iep->bgpic];
        }
        bgoffsx += spritewidth[iep->bgpic];
      }
      // return to normal clipping rectangle
      set_clip(abuf, 0, 0, abuf->w - 1, abuf->h - 1);
    }
    int uu;
    for (uu=yy1;uu <= yy2;uu+=spriteheight[get_but_pic(iep,4)]) {
      do_corner(get_but_pic(iep,4),xx1,uu,-1,0);   // left side
      do_corner(get_but_pic(iep,5),xx2+1,uu,0,0);  // right side
      }
    for (uu=xx1;uu <= xx2;uu+=spritewidth[get_but_pic(iep,6)]) {
      do_corner(get_but_pic(iep,6),uu,yy1,0,-1);  // top side
      do_corner(get_but_pic(iep,7),uu,yy2+1,0,0); // bottom side
      }
    do_corner(get_but_pic(iep,0),xx1,yy1,-1,-1);  // top left
    do_corner(get_but_pic(iep,1),xx1,yy2+1,-1,0);  // bottom left
    do_corner(get_but_pic(iep,2),xx2+1,yy1,0,-1);  //  top right
    do_corner(get_but_pic(iep,3),xx2+1,yy2+1,0,0);  // bottom right
    }
  }

// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width (int twgui) {
  if (twgui < 0)
    return 0;

  if (!guis[twgui].is_textwindow())
    quit("!GUI set as text window but is not actually a text window GUI");

  int borwid = spritewidth[get_but_pic(&guis[twgui], 4)] + 
               spritewidth[get_but_pic(&guis[twgui], 5)];

  return borwid;
}

// get the hegiht of the text window's top border
int get_textwindow_top_border_height (int twgui) {
  if (twgui < 0)
    return 0;

  if (!guis[twgui].is_textwindow())
    quit("!GUI set as text window but is not actually a text window GUI");

  return spriteheight[get_but_pic(&guis[twgui], 6)];
}


struct TopBarSettings {
  int wantIt;
  int height;
  int font;
  char text[200];

  TopBarSettings() {
    wantIt = 0;
    height = 0;
    font = 0;
    text[0] = 0;
  }
};
TopBarSettings topBar;
// draw_text_window: draws the normal or custom text window
// create a new bitmap the size of the window before calling, and
//   point abuf to it
// returns text start x & y pos in parameters
block screenop = NULL;
int wantFreeScreenop = 0;
int texthit;
void draw_text_window(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight=0, int ifnum=-1) {
  if (ifnum < 0)
    ifnum = game.options[OPT_TWCUSTOM];

  if (ifnum <= 0) {
    if (ovrheight)
      quit("!Cannot use QFG4 style options without custom text window");
    draw_button_background(0,0,abuf->w - 1,abuf->h - 1,NULL);
    wtextcolor(16);
    xins[0]=3;
    yins[0]=3;
    }
  else {
    if (ifnum >= game.numgui)
      quitprintf("!Invalid GUI %d specified as text window (total GUIs: %d)", ifnum, game.numgui);
    if (!guis[ifnum].is_textwindow())
      quit("!GUI set as text window but is not actually a text window GUI");

    int tbnum = get_but_pic(&guis[ifnum], 0);

    wii[0] += get_textwindow_border_width (ifnum);
    xx[0]-=spritewidth[tbnum];
    yy[0]-=spriteheight[tbnum];
    if (ovrheight == 0)
      ovrheight = numlines*texthit;

    if ((wantFreeScreenop > 0) && (screenop != NULL))
      destroy_bitmap(screenop);
    screenop = create_bitmap_ex(final_col_dep,wii[0],ovrheight+6+spriteheight[tbnum]*2);
    clear_to_color(screenop, bitmap_mask_color(screenop));
    wsetscreen(screenop);
    int xoffs=spritewidth[tbnum],yoffs=spriteheight[tbnum];
    draw_button_background(xoffs,yoffs,(abuf->w - xoffs) - 1,(abuf->h - yoffs) - 1,&guis[ifnum]);
    wtextcolor(guis[ifnum].fgcol);
    xins[0]=xoffs+3;
    yins[0]=yoffs+3;
  }

}

void draw_text_window_and_bar(int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight=0, int ifnum=-1) {

  draw_text_window(xins, yins, xx, yy, wii, ovrheight, ifnum);

  if ((topBar.wantIt) && (screenop != NULL)) {
    // top bar on the dialog window with character's name
    // create an enlarged window, then free the old one
    block newScreenop = create_bitmap_ex(final_col_dep, screenop->w, screenop->h + topBar.height);
    blit(screenop, newScreenop, 0, 0, 0, topBar.height, screenop->w, screenop->h);
    wfreeblock(screenop);
    screenop = newScreenop;
    wsetscreen(screenop);

    // draw the top bar
    rectfill(screenop, 0, 0, screenop->w - 1, topBar.height - 1, get_col8_lookup(play.top_bar_backcolor));
    if (play.top_bar_backcolor != play.top_bar_bordercolor) {
      // draw the border
      for (int j = 0; j < multiply_up_coordinate(play.top_bar_borderwidth); j++)
        rect(screenop, j, j, screenop->w - (j + 1), topBar.height - (j + 1), get_col8_lookup(play.top_bar_bordercolor));
    }
    
    int textcolwas = textcol;
    // draw the text
    int textx = (screenop->w / 2) - wgettextwidth_compensate(topBar.text, topBar.font) / 2;
    wtextcolor(play.top_bar_textcolor);
    wouttext_outline(textx, play.top_bar_borderwidth + get_fixed_pixel_size(1), topBar.font, topBar.text);
    // restore the current text colour
    textcol = textcolwas;

    // don't draw it next time
    topBar.wantIt = 0;
    // adjust the text Y position
    yins[0] += topBar.height;
  }
  else if (topBar.wantIt)
    topBar.wantIt = 0;
}


void wouttext_outline(int xxp, int yyp, int usingfont, char*texx) {
  int otextc=textcol;

  if (game.fontoutline[usingfont] >= 0) {
    wtextcolor(play.speech_text_shadow);
    // MACPORT FIX 9/6/5: cast
    wouttextxy(xxp, yyp, (int)game.fontoutline[usingfont], texx);
  }
  else if (game.fontoutline[usingfont] == FONT_OUTLINE_AUTO) {
    wtextcolor(play.speech_text_shadow);

    int outlineDist = 1;

    if ((game.options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[usingfont]->SupportsExtendedCharacters(usingfont))) {
      // if it's a scaled up SCI font, move the outline out more
      outlineDist = get_fixed_pixel_size(1);
    }

    // move the text over so that it's still within the bounding rect
    xxp += outlineDist;
    yyp += outlineDist;

    wouttextxy(xxp - outlineDist, yyp, usingfont, texx);
    wouttextxy(xxp + outlineDist, yyp, usingfont, texx);
    wouttextxy(xxp, yyp + outlineDist, usingfont, texx);
    wouttextxy(xxp, yyp - outlineDist, usingfont, texx);
    wouttextxy(xxp - outlineDist, yyp - outlineDist, usingfont, texx);
    wouttextxy(xxp - outlineDist, yyp + outlineDist, usingfont, texx);
    wouttextxy(xxp + outlineDist, yyp + outlineDist, usingfont, texx);
    wouttextxy(xxp + outlineDist, yyp - outlineDist, usingfont, texx);
  }

  textcol = otextc;
  wouttextxy(xxp, yyp, usingfont, texx);
}

int GetTextDisplayTime (char *text, int canberel=0) {
  int uselen = strlen(text);

  int fpstimer = frames_per_second;

  // if it's background speech, make it stay relative to game speed
  if ((canberel == 1) && (play.bgspeech_game_speed == 1))
    fpstimer = 40;

  if (source_text_length >= 0) {
    // sync to length of original text, to make sure any animations
    // and music sync up correctly
    uselen = source_text_length;
    source_text_length = -1;
  }
  else {
    if ((text[0] == '&') && (play.unfactor_speech_from_textlength != 0)) {
      // if there's an "&12 text" type line, remove "&12 " from the source
      // length
      int j = 0;
      while ((text[j] != ' ') && (text[j] != 0))
        j++;
      j++;
      uselen -= j;
    }

  }

  if (uselen <= 0)
    return 0;

  if (play.text_speed + play.text_speed_modifier <= 0)
    quit("!Text speed is zero; unable to display text. Check your game.text_speed settings.");
  
  // Store how many game loops per character of text
  // This is calculated using a hard-coded 15 for the text speed,
  // so that it's always the same no matter how fast the user
  // can read.
  loops_per_character = (((uselen/play.lipsync_speed)+1) * fpstimer) / uselen;

  int textDisplayTimeInMS = ((uselen / (play.text_speed + play.text_speed_modifier)) + 1) * 1000;
  if (textDisplayTimeInMS < play.text_min_display_time_ms)
    textDisplayTimeInMS = play.text_min_display_time_ms;

  return (textDisplayTimeInMS * fpstimer) / 1000;
}

int convert_gui_disabled_style(int oldStyle) {
  int toret = GUIDIS_GREYOUT;

  // if GUIs Turn Off is selected, don't grey out buttons for
  // any Persistent GUIs which remain
  // set to 0x80 so that it is still non-zero, but has no effect
  if (oldStyle == 3)
    toret = GUIDIS_GUIOFF;
  // GUIs Go Black
  else if (oldStyle == 1)
    toret = GUIDIS_BLACKOUT;
  // GUIs unchanged
  else if (oldStyle == 2)
    toret = GUIDIS_UNCHANGED;

  return toret;
}

void update_gui_disabled_status() {
  // update GUI display status (perhaps we've gone into
  // an interface disabled state)
  int all_buttons_was = all_buttons_disabled;
  all_buttons_disabled = 0;

  if (!IsInterfaceEnabled()) {
    all_buttons_disabled = gui_disabled_style;
  }

  if (all_buttons_was != all_buttons_disabled) {
    // GUIs might have been removed/added
    for (int aa = 0; aa < game.numgui; aa++) {
      guis[aa].control_positions_changed();
    }
    guis_need_update = 1;
    invalidate_screen();
  }
}


int adjust_x_for_guis (int xx, int yy) {
  if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
    return xx;
  // If it's covered by a GUI, move it right a bit
  for (int aa=0;aa < game.numgui; aa++) {
    if (guis[aa].on < 1)
      continue;
    if ((guis[aa].x > xx) || (guis[aa].y > yy) || (guis[aa].y + guis[aa].hit < yy))
      continue;
    // totally transparent GUI, ignore
    if ((guis[aa].bgcol == 0) && (guis[aa].bgpic < 1))
      continue;

    // try to deal with full-width GUIs across the top
    if (guis[aa].x + guis[aa].wid >= get_fixed_pixel_size(280))
      continue;

    if (xx < guis[aa].x + guis[aa].wid) 
      xx = guis[aa].x + guis[aa].wid + 2;        
  }
  return xx;
}

int adjust_y_for_guis ( int yy) {
  if ((game.options[OPT_DISABLEOFF]==3) && (all_buttons_disabled > 0))
    return yy;
  // If it's covered by a GUI, move it down a bit
  for (int aa=0;aa < game.numgui; aa++) {
    if (guis[aa].on < 1)
      continue;
    if (guis[aa].y > yy)
      continue;
    // totally transparent GUI, ignore
    if ((guis[aa].bgcol == 0) && (guis[aa].bgpic < 1))
      continue;

    // try to deal with full-height GUIs down the left or right
    if (guis[aa].hit > get_fixed_pixel_size(50))
      continue;

    if (yy < guis[aa].y + guis[aa].hit) 
      yy = guis[aa].y + guis[aa].hit + 2;        
  }
  return yy;
}

void wouttext_aligned (int usexp, int yy, int oriwid, int usingfont, const char *text, int align) {

  if (align == SCALIGN_CENTRE)
    usexp = usexp + (oriwid / 2) - (wgettextwidth_compensate(text, usingfont) / 2);
  else if (align == SCALIGN_RIGHT)
    usexp = usexp + (oriwid - wgettextwidth_compensate(text, usingfont));

  wouttext_outline(usexp, yy, usingfont, (char *)text);
}

int user_to_internal_skip_speech(int userval) {
  // 0 = click mouse or key to skip
  if (userval == 0)
    return SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_MOUSECLICK;
  // 1 = key only
  else if (userval == 1)
    return SKIP_AUTOTIMER | SKIP_KEYPRESS;
  // 2 = can't skip at all
  else if (userval == 2)
    return SKIP_AUTOTIMER;
  // 3 = only on keypress, no auto timer
  else if (userval == 3)
    return SKIP_KEYPRESS | SKIP_MOUSECLICK;
  // 4 = mouse only
  else if (userval == 4)
    return SKIP_AUTOTIMER | SKIP_MOUSECLICK;
  else
    quit("user_to_internal_skip_speech: unknown userval");

  return 0;
}

bool ShouldAntiAliasText() {
  return (game.options[OPT_ANTIALIASFONTS] != 0);
}

// Pass yy = -1 to find Y co-ord automatically
// allowShrink = 0 for none, 1 for leftwards, 2 for rightwards
// pass blocking=2 to create permanent overlay
int _display_main(int xx,int yy,int wii,char*todis,int blocking,int usingfont,int asspch, int isThought, int allowShrink, bool overlayPositionFixed) 
{
  bool alphaChannel = false;
  ensure_text_valid_for_font(todis, usingfont);
  break_up_text_into_lines(wii-6,usingfont,todis);
  texthit = wgetfontheight(usingfont);

  // if it's a normal message box and the game was being skipped,
  // ensure that the screen is up to date before the message box
  // is drawn on top of it
  if ((play.skip_until_char_stops >= 0) && (blocking == 1))
    render_graphics();

  EndSkippingUntilCharStops();

  if (topBar.wantIt) {
    // ensure that the window is wide enough to display
    // any top bar text
    int topBarWid = wgettextwidth_compensate(topBar.text, topBar.font);
    topBarWid += multiply_up_coordinate(play.top_bar_borderwidth + 2) * 2;
    if (longestline < topBarWid)
      longestline = topBarWid;
    // the top bar should behave like DisplaySpeech wrt blocking
    blocking = 0;
  }

  if (asspch > 0) {
    // update the all_buttons_disabled variable in advance
    // of the adjust_x/y_for_guis calls
    play.disabled_user_interface++;
    update_gui_disabled_status();
    play.disabled_user_interface--;
  }

  if (xx == OVR_AUTOPLACE) ;
  // centre text in middle of screen
  else if (yy<0) yy=(scrnhit/2-(numlines*texthit)/2)-3;
  // speech, so it wants to be above the character's head
  else if (asspch > 0) {
    yy-=numlines*texthit;
    if (yy < 5) yy=5;
    yy = adjust_y_for_guis (yy);
  }

  if (longestline < wii - get_fixed_pixel_size(6)) {
    // shrink the width of the dialog box to fit the text
    int oldWid = wii;
    //if ((asspch >= 0) || (allowShrink > 0))
    // If it's not speech, or a shrink is allowed, then shrink it
    if ((asspch == 0) || (allowShrink > 0))
      wii = longestline + get_fixed_pixel_size(6);
    
    // shift the dialog box right to align it, if necessary
    if ((allowShrink == 2) && (xx >= 0))
      xx += (oldWid - wii);
  }

  if (xx<-1) { 
    xx=(-xx)-wii/2;
    if (xx < 0)
      xx = 0;

    xx = adjust_x_for_guis (xx, yy);

    if (xx + wii >= scrnwid)
      xx = (scrnwid - wii) - 5;
  }
  else if (xx<0) xx=scrnwid/2-wii/2;

  int ee, extraHeight = get_fixed_pixel_size(6);
  wtextcolor(15);
  if (blocking < 2)
    remove_screen_overlay(OVER_TEXTMSG);

  screenop = create_bitmap_ex(final_col_dep, (wii > 0) ? wii : 2, numlines*texthit + extraHeight);
  wsetscreen(screenop);
  clear_to_color(screenop,bitmap_mask_color(screenop));

  // inform draw_text_window to free the old bitmap
  wantFreeScreenop = 1;
  
  if ((strlen (todis) < 1) || (strcmp (todis, "  ") == 0) || (wii == 0)) ;
    // if it's an empty speech line, don't draw anything
  else if (asspch) { //wtextcolor(12);
    int ttxleft = 0, ttxtop = get_fixed_pixel_size(3), oriwid = wii - 6;
    int usingGui = -1, drawBackground = 0;
 
    if ((asspch < 0) && (game.options[OPT_SPEECHTYPE] >= 2)) {
      usingGui = play.speech_textwindow_gui;
      drawBackground = 1;
    }
    else if ((isThought) && (game.options[OPT_THOUGHTGUI] > 0)) {
      usingGui = game.options[OPT_THOUGHTGUI];
      // make it treat it as drawing inside a window now
      if (asspch > 0)
        asspch = -asspch;
      drawBackground = 1;
    }

    if (drawBackground)
      draw_text_window_and_bar(&ttxleft, &ttxtop, &xx, &yy, &wii, 0, usingGui);
    else if ((ShouldAntiAliasText()) && (final_col_dep >= 24))
      alphaChannel = true;

    for (ee=0;ee<numlines;ee++) {
      //int ttxp=wii/2 - wgettextwidth_compensate(lines[ee], usingfont)/2;
      int ttyp=ttxtop+ee*texthit;
      // asspch < 0 means that it's inside a text box so don't
      // centre the text
      if (asspch < 0) {
        if ((usingGui >= 0) && 
            ((game.options[OPT_SPEECHTYPE] >= 2) || (isThought)))
          wtextcolor(guis[usingGui].fgcol);
        else
          wtextcolor(-asspch);
        
        wouttext_aligned(ttxleft, ttyp, oriwid, usingfont, lines[ee], play.text_align);
      }
      else {
        wtextcolor(asspch);
        //wouttext_outline(ttxp,ttyp,usingfont,lines[ee]);
        wouttext_aligned(ttxleft, ttyp, wii, usingfont, lines[ee], play.speech_text_align);
      }
    }
  }
  else {
    int xoffs,yoffs, oriwid = wii - 6;
    draw_text_window_and_bar(&xoffs,&yoffs,&xx,&yy,&wii);

    if (game.options[OPT_TWCUSTOM] > 0)
    {
      alphaChannel = guis[game.options[OPT_TWCUSTOM]].is_alpha();
    }

    adjust_y_coordinate_for_text(&yoffs, usingfont);

    for (ee=0;ee<numlines;ee++)
      wouttext_aligned (xoffs, yoffs + ee * texthit, oriwid, usingfont, lines[ee], play.text_align);
  }

  wantFreeScreenop = 0;

  int ovrtype = OVER_TEXTMSG;
  if (blocking == 2) ovrtype=OVER_CUSTOM;
  else if (blocking >= OVER_CUSTOM) ovrtype=blocking;

  int nse = add_screen_overlay(xx, yy, ovrtype, screenop, alphaChannel);

  wsetscreen(virtual_screen);
  if (blocking>=2) {
    return screenover[nse].type;
  }

  if (blocking) {
    if (play.fast_forward) {
      remove_screen_overlay(OVER_TEXTMSG);
      play.messagetime=-1;
      return 0;
    }

/*    wputblock(xx,yy,screenop,1);
    remove_screen_overlay(OVER_TEXTMSG);*/

    if (!play.mouse_cursor_hidden)
      domouse(1);
    // play.skip_display has same values as SetSkipSpeech:
    // 0 = click mouse or key to skip
    // 1 = key only
    // 2 = can't skip at all
    // 3 = only on keypress, no auto timer
    // 4 = mouse only
    int countdown = GetTextDisplayTime (todis);
    int skip_setting = user_to_internal_skip_speech(play.skip_display);
    while (1) {
      timerloop = 0;
      NEXT_ITERATION();
/*      if (!play.mouse_cursor_hidden)
        domouse(0);
      write_screen();*/

      render_graphics();

      update_polled_stuff_and_crossfade();
      if (mgetbutton()>NONE) {
        // If we're allowed, skip with mouse
        if (skip_setting & SKIP_MOUSECLICK)
          break;
      }
      if (kbhit()) {
        // discard keypress, and don't leave extended keys over
        int kp = getch();
        if (kp == 0) getch();

        // let them press ESC to skip the cutscene
        check_skip_cutscene_keypress (kp);
        if (play.fast_forward)
          break;

        if (skip_setting & SKIP_KEYPRESS)
          break;
      }
      while ((timerloop == 0) && (play.fast_forward == 0)) {
        update_polled_stuff();
        platform->YieldCPU();
      }
      countdown--;

      if (channels[SCHAN_SPEECH] != NULL) {
        // extend life of text if the voice hasn't finished yet
        if ((!rec_isSpeechFinished()) && (play.fast_forward == 0)) {
          if (countdown <= 1)
            countdown = 1;
        }
        else  // if the voice has finished, remove the speech
          countdown = 0;
      }

      if ((countdown < 1) && (skip_setting & SKIP_AUTOTIMER))
      {
        play.ignore_user_input_until_time = globalTimerCounter + (play.ignore_user_input_after_text_timeout_ms / time_between_timers);
        break;
      }
      // if skipping cutscene, don't get stuck on No Auto Remove
      // text boxes
      if ((countdown < 1) && (play.fast_forward))
        break;
    }
    if (!play.mouse_cursor_hidden)
      domouse(2);
    remove_screen_overlay(OVER_TEXTMSG);

    construct_virtual_screen(true);
  }
  else {
    // if the speech does not time out, but we are skipping a cutscene,
    // allow it to time out
    if ((play.messagetime < 0) && (play.fast_forward))
      play.messagetime = 2;

    if (!overlayPositionFixed)
    {
      screenover[nse].positionRelativeToScreen = false;
      screenover[nse].x += offsetx;
      screenover[nse].y += offsety;
    }

    do_main_cycle(UNTIL_NOOVERLAY,0);
  }

  play.messagetime=-1;
  return 0;
}

void _display_at(int xx,int yy,int wii,char*todis,int blocking,int asspch, int isThought, int allowShrink, bool overlayPositionFixed) {
  int usingfont=FONT_NORMAL;
  if (asspch) usingfont=FONT_SPEECH;
  int needStopSpeech = 0;

  EndSkippingUntilCharStops();

  if (todis[0]=='&') {
    // auto-speech
    int igr=atoi(&todis[1]);
    while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
    if (todis[0]==' ') todis++;
    if (igr <= 0)
      quit("Display: auto-voice symbol '&' not followed by valid integer");
    if (play_speech(play.narrator_speech,igr)) {
      // if Voice Only, then blank out the text
      if (play.want_speech == 2)
        todis = "  ";
    }
    needStopSpeech = 1;
  }
  _display_main(xx,yy,wii,todis,blocking,usingfont,asspch, isThought, allowShrink, overlayPositionFixed);

  if (needStopSpeech)
    stop_speech();
}

void SetTextWindowGUI (int guinum) {
  if ((guinum < -1) | (guinum >= game.numgui))
    quit("!SetTextWindowGUI: invalid GUI number");

  if (guinum < 0) ;  // disable it
  else if (!guis[guinum].is_textwindow())
    quit("!SetTextWindowGUI: specified GUI is not a text window");

  if (play.speech_textwindow_gui == game.options[OPT_TWCUSTOM])
    play.speech_textwindow_gui = guinum;
  game.options[OPT_TWCUSTOM] = guinum;
}

// *** OVERLAY SCRIPT FUNCTINS



void RemoveOverlay(int ovrid) {
  if (find_overlay_of_type(ovrid) < 0) quit("!RemoveOverlay: invalid overlay id passed");
  remove_screen_overlay(ovrid);
}

void Overlay_Remove(ScriptOverlay *sco) {
  sco->Remove();
}

int CreateGraphicOverlay(int xx,int yy,int slott,int trans) {
  multiply_up_coordinates(&xx, &yy);

  block screeno=create_bitmap_ex(final_col_dep, spritewidth[slott],spriteheight[slott]);
  wsetscreen(screeno);
  clear_to_color(screeno,bitmap_mask_color(screeno));
  wputblock(0,0,spriteset[slott],trans);

  bool hasAlpha = (game.spriteflags[slott] & SPF_ALPHACHANNEL) != 0;
  int nse = add_screen_overlay(xx, yy, OVER_CUSTOM, screeno, hasAlpha);

  wsetscreen(virtual_screen);
  return screenover[nse].type;
}

int crovr_id=2;  // whether using SetTextOverlay or CreateTextOvelay
int CreateTextOverlayCore(int xx, int yy, int wii, int fontid, int clr, const char *tex, int allowShrink) {
  if (wii<8) wii=scrnwid/2;
  if (xx<0) xx=scrnwid/2-wii/2;
  if (clr==0) clr=16;
  int blcode = crovr_id;
  crovr_id = 2;
  return _display_main(xx,yy,wii, (char*)tex, blcode,fontid,-clr, 0, allowShrink, false);
}

int CreateTextOverlay(int xx,int yy,int wii,int fontid,int clr,char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf,get_translation(texx),ap);
  va_end(ap);

  int allowShrink = 0;

  if (xx != OVR_AUTOPLACE) {
    multiply_up_coordinates(&xx,&yy);
    wii = multiply_up_coordinate(wii);
  }
  else  // allow DisplaySpeechBackground to be shrunk
    allowShrink = 1;

  return CreateTextOverlayCore(xx, yy, wii, fontid, clr, displbuf, allowShrink);
}

void SetTextOverlay(int ovrid,int xx,int yy,int wii,int fontid,int clr,char*texx,...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf,get_translation(texx),ap);
  va_end(ap);
  RemoveOverlay(ovrid);
  crovr_id=ovrid;
  if (CreateTextOverlay(xx,yy,wii,fontid,clr,displbuf)!=ovrid)
    quit("SetTextOverlay internal error: inconsistent type ids");
  }

void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf,get_translation(texx),ap);
  va_end(ap);

  int ovri=find_overlay_of_type(scover->overlayId);
  if (ovri<0)
    quit("!Overlay.SetText: invalid overlay ID specified");
  int xx = divide_down_coordinate(screenover[ovri].x) - scover->borderWidth;
  int yy = divide_down_coordinate(screenover[ovri].y) - scover->borderHeight;

  RemoveOverlay(scover->overlayId);
  crovr_id = scover->overlayId;

  if (CreateTextOverlay(xx,yy,wii,fontid,clr,displbuf) != scover->overlayId)
    quit("SetTextOverlay internal error: inconsistent type ids");
}

int Overlay_GetX(ScriptOverlay *scover) {
  int ovri = find_overlay_of_type(scover->overlayId);
  if (ovri < 0)
    quit("!invalid overlay ID specified");

  int tdxp, tdyp;
  get_overlay_position(ovri, &tdxp, &tdyp);

  return divide_down_coordinate(tdxp);
}

void Overlay_SetX(ScriptOverlay *scover, int newx) {
  int ovri = find_overlay_of_type(scover->overlayId);
  if (ovri < 0)
    quit("!invalid overlay ID specified");

  screenover[ovri].x = multiply_up_coordinate(newx);
}

int Overlay_GetY(ScriptOverlay *scover) {
  int ovri = find_overlay_of_type(scover->overlayId);
  if (ovri < 0)
    quit("!invalid overlay ID specified");

  int tdxp, tdyp;
  get_overlay_position(ovri, &tdxp, &tdyp);

  return divide_down_coordinate(tdyp);
}

void Overlay_SetY(ScriptOverlay *scover, int newy) {
  int ovri = find_overlay_of_type(scover->overlayId);
  if (ovri < 0)
    quit("!invalid overlay ID specified");

  screenover[ovri].y = multiply_up_coordinate(newy);
}

void MoveOverlay(int ovrid, int newx,int newy) {
  multiply_up_coordinates(&newx, &newy);
  
  int ovri=find_overlay_of_type(ovrid);
  if (ovri<0) quit("!MoveOverlay: invalid overlay ID specified");
  screenover[ovri].x=newx;
  screenover[ovri].y=newy;
}

int IsOverlayValid(int ovrid) {
  if (find_overlay_of_type(ovrid) < 0)
    return 0;

  return 1;
}

int Overlay_GetValid(ScriptOverlay *scover) {
  if (scover->overlayId == -1)
    return 0;

  if (!IsOverlayValid(scover->overlayId)) {
    scover->overlayId = -1;
    return 0;
  }

  return 1;
}


ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent) {
  ScriptOverlay *sco = new ScriptOverlay();
  sco->overlayId = CreateGraphicOverlay(x, y, slot, transparent);
  sco->borderHeight = 0;
  sco->borderWidth = 0;
  sco->isBackgroundSpeech = 0;

  ccRegisterManagedObject(sco, sco);
  return sco;
}

ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text, ...) {
  ScriptOverlay *sco = new ScriptOverlay();

  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,text);
  my_sprintf(displbuf,get_translation(text),ap);
  va_end(ap);

  multiply_up_coordinates(&x, &y);
  width = multiply_up_coordinate(width);

  sco->overlayId = CreateTextOverlayCore(x, y, width, font, colour, displbuf, 0);

  int ovri = find_overlay_of_type(sco->overlayId);
  sco->borderWidth = divide_down_coordinate(screenover[ovri].x - x);
  sco->borderHeight = divide_down_coordinate(screenover[ovri].y - y);
  sco->isBackgroundSpeech = 0;

  ccRegisterManagedObject(sco, sco);
  return sco;
}


int DisplaySpeechBackground(int charid,char*speel) {
  // remove any previous background speech for this character
  int cc;
  for (cc = 0; cc < numscreenover; cc++) {
    if (screenover[cc].bgSpeechForChar == charid) {
      remove_screen_overlay_index(cc);
      cc--;
    }
  }

  int ovrl=CreateTextOverlay(OVR_AUTOPLACE,charid,scrnwid/2,FONT_SPEECH,
    -game.chars[charid].talkcolor, get_translation(speel));

  int scid = find_overlay_of_type(ovrl);
  screenover[scid].bgSpeechForChar = charid;
  screenover[scid].timeout = GetTextDisplayTime(speel, 1);
  return ovrl;
}

void DisplayAt(int xxp,int yyp,int widd,char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf, get_translation(texx), ap);
  va_end(ap);

  multiply_up_coordinates(&xxp, &yyp);
  widd = multiply_up_coordinate(widd);
  
  if (widd<1) widd=scrnwid/2;
  if (xxp<0) xxp=scrnwid/2-widd/2;
  _display_at(xxp,yyp,widd,displbuf,1,0, 0, 0, false);
  }

int play_speech(int charid,int sndid) {
  stop_and_destroy_channel (SCHAN_SPEECH);

  // don't play speech if we're skipping a cutscene
  if (play.fast_forward)
    return 0;
  if ((play.want_speech < 1) || (speech_file == NULL))
    return 0;

  SOUNDCLIP *speechmp3;
/*  char finame[40]="~SPEECH.VOX~NARR";
  if (charid >= 0)
    strncpy(&finame[12],game.chars[charid].scrname,4);*/

  char finame[40] = "~";
  strcat(finame, get_filename(speech_file));
  strcat(finame, "~");

  if (charid >= 0) {
    // append the first 4 characters of the script name to the filename
    char theScriptName[5];
    if (game.chars[charid].scrname[0] == 'c')
      strncpy(theScriptName, &game.chars[charid].scrname[1], 4);
    else
      strncpy(theScriptName, game.chars[charid].scrname, 4);
    theScriptName[4] = 0;
    strcat(finame, theScriptName);
  }
  else
    strcat(finame, "NARR");

  // append the speech number
  sprintf(&finame[strlen(finame)],"%d",sndid);

  int ii;  // Compare the base file name to the .pam file name
  char *basefnptr = strchr (&finame[4], '~') + 1;
  curLipLine = -1;  // See if we have voice lip sync for this line
  curLipLinePhenome = -1;
  for (ii = 0; ii < numLipLines; ii++) {
    if (stricmp(splipsync[ii].filename, basefnptr) == 0) {
      curLipLine = ii;
      break;
    }
  }
  // if the lip-sync is being used for voice sync, disable
  // the text-related lipsync
  if (numLipLines > 0)
    game.options[OPT_LIPSYNCTEXT] = 0;

  strcat (finame, ".WAV");
  speechmp3 = my_load_wave (finame, play.speech_volume, 0);

  if (speechmp3 == NULL) {
    strcpy (&finame[strlen(finame)-3], "ogg");
    speechmp3 = my_load_ogg (finame, play.speech_volume);
  }

  if (speechmp3 == NULL) {
    strcpy (&finame[strlen(finame)-3], "mp3");
    speechmp3 = my_load_mp3 (finame, play.speech_volume);
  }

  if (speechmp3 != NULL) {
    if (speechmp3->play() == 0)
      speechmp3 = NULL;
  }

  if (speechmp3 == NULL) {
    debug_log ("Speech load failure: '%s'",finame);
    curLipLine = -1;
    return 0;
  }

  channels[SCHAN_SPEECH] = speechmp3;
  play.music_vol_was = play.music_master_volume;

  // Negative value means set exactly; positive means drop that amount
  if (play.speech_music_drop < 0)
    play.music_master_volume = -play.speech_music_drop;
  else
    play.music_master_volume -= play.speech_music_drop;

  apply_volume_drop_modifier(true);
  update_music_volume();
  update_music_at = 0;
  mvolcounter = 0;

  update_ambient_sound_vol();

  // change Sierra w/bgrnd  to Sierra without background when voice
  // is available (for Tierra)
  if ((game.options[OPT_SPEECHTYPE] == 2) && (play.no_textbg_when_voice > 0)) {
    game.options[OPT_SPEECHTYPE] = 1;
    play.no_textbg_when_voice = 2;
  }

  return 1;
}

void stop_speech() {
  if (channels[SCHAN_SPEECH] != NULL) {
    play.music_master_volume = play.music_vol_was;
    // update the music in a bit (fixes two speeches follow each other
    // and music going up-then-down)
    update_music_at = 20;
    mvolcounter = 1;
    stop_and_destroy_channel (SCHAN_SPEECH);
    curLipLine = -1;

    if (play.no_textbg_when_voice == 2) {
      // set back to Sierra w/bgrnd
      play.no_textbg_when_voice = 1;
      game.options[OPT_SPEECHTYPE] = 2;
    }
  }
}
void SetSpeechVolume(int newvol) {
  if ((newvol<0) | (newvol>255))
    quit("!SetSpeechVolume: invalid volume - must be from 0-255");

  if (channels[SCHAN_SPEECH])
    channels[SCHAN_SPEECH]->set_volume (newvol);

  play.speech_volume = newvol;
  }

void SetSpeechFont (int fontnum) {
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!SetSpeechFont: invalid font number.");
  play.speech_font = fontnum;
  }
void SetNormalFont (int fontnum) {
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!SetNormalFont: invalid font number.");
  play.normal_font = fontnum;
  }
int Game_GetSpeechFont() {
  return play.speech_font;
}
int Game_GetNormalFont() {
  return play.normal_font;
}

void SetSpeechStyle (int newstyle) {
  if ((newstyle < 0) || (newstyle > 3))
    quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
  game.options[OPT_SPEECHTYPE] = newstyle;
}

void __scr_play_speech(int who, int which) {
  // *** implement this - needs to call stop_speech as well
  // to reset the volume
  quit("PlaySpeech not yet implemented");
}

// 0 = text only
// 1 = voice & text
// 2 = voice only
void SetVoiceMode (int newmod) {
  if ((newmod < 0) | (newmod > 2))
    quit("!SetVoiceMode: invalid mode number (must be 0,1,2)");
  // If speech is turned off, store the mode anyway in case the
  // user adds the VOX file later
  if (play.want_speech < 0)
    play.want_speech = (-newmod) - 1;
  else
    play.want_speech = newmod;
}

int IsVoxAvailable() {
  if (play.want_speech < 0)
    return 0;
  return 1;
}

int IsMusicVoxAvailable () {
  return play.seperate_music_lib;
}

void _displayspeech(char*texx, int aschar, int xx, int yy, int widd, int isThought) {
  if (!is_valid_character(aschar))
    quit("!DisplaySpeech: invalid character");

  CharacterInfo *speakingChar = &game.chars[aschar];
  if ((speakingChar->view < 0) || (speakingChar->view >= game.numviews))
    quit("!DisplaySpeech: character has invalid view");

  if (is_text_overlay > 0)
    quit("!DisplaySpeech: speech was already displayed (nested DisplaySpeech, perhaps room script and global script conflict?)");

  EndSkippingUntilCharStops();

  said_speech_line = 1;

  int aa;
  if (play.bgspeech_stay_on_display == 0) {
    // remove any background speech
    for (aa=0;aa<numscreenover;aa++) {
      if (screenover[aa].timeout > 0) {
        remove_screen_overlay(screenover[aa].type);
        aa--;
      }
    }
  }
  said_text = 1;

  // the strings are pre-translated
  //texx = get_translation(texx);
  our_eip=150;

  int isPause = 1;
  // if the message is all .'s, don't display anything
  for (aa = 0; texx[aa] != 0; aa++) {
    if (texx[aa] != '.') {
      isPause = 0;
      break;
    }
  }

  play.messagetime = GetTextDisplayTime(texx);

  if (isPause) {
    if (update_music_at > 0)
      update_music_at += play.messagetime;
    do_main_cycle(UNTIL_INTISNEG,(int)&play.messagetime);
    return;
  }

  int textcol = speakingChar->talkcolor;
  
  // if it's 0, it won't be recognised as speech
  if (textcol == 0)
    textcol = 16;

  int allowShrink = 0;
  int bwidth = widd;
  if (bwidth < 0)
    bwidth = scrnwid/2 + scrnwid/4;

  our_eip=151;

  int useview = speakingChar->talkview;
  if (isThought) {
    useview = speakingChar->thinkview;
    // view 0 is not valid for think views
    if (useview == 0)
      useview = -1;
    // speech bubble can shrink to fit
    allowShrink = 1;
    if (speakingChar->room != displayed_room) {
      // not in room, centre it
      xx = -1;
      yy = -1;
    }
  }

  if (useview >= game.numviews)
    quitprintf("!Character.Say: attempted to use view %d for animation, but it does not exist", useview + 1);

  int tdxp = xx,tdyp = yy;
  int oldview=-1, oldloop = -1;
  int ovr_type = 0;

  text_lips_offset = 0;
  text_lips_text = texx;

  block closeupface=NULL;
  if (texx[0]=='&') {
    // auto-speech
    int igr=atoi(&texx[1]);
    while ((texx[0]!=' ') & (texx[0]!=0)) texx++;
    if (texx[0]==' ') texx++;
    if (igr <= 0)
      quit("DisplaySpeech: auto-voice symbol '&' not followed by valid integer");

    text_lips_text = texx;

    if (play_speech(aschar,igr)) {
      if (play.want_speech == 2)
        texx = "  ";  // speech only, no text.
    }
  }
  if (game.options[OPT_SPEECHTYPE] == 3)
    remove_screen_overlay(OVER_COMPLETE);
  our_eip=1500;

  if (game.options[OPT_SPEECHTYPE] == 0)
    allowShrink = 1;

  if (speakingChar->idleleft < 0)  {
    // if idle anim in progress for the character, stop it
    ReleaseCharacterView(aschar);
//    speakingChar->idleleft = speakingChar->idletime;
  }

  bool overlayPositionFixed = false;
  int charFrameWas = 0;
  int viewWasLocked = 0;
  if (speakingChar->flags & CHF_FIXVIEW)
    viewWasLocked = 1;

  /*if ((speakingChar->room == displayed_room) ||
      ((useview >= 0) && (game.options[OPT_SPEECHTYPE] > 0)) ) {*/

  if (speakingChar->room == displayed_room) {
    // If the character is in this room, go for it - otherwise
    // run the "else" clause which  does text in the middle of
    // the screen.
    our_eip=1501;
    if (tdxp < 0)
      tdxp = multiply_up_coordinate(speakingChar->x) - offsetx;
    if (tdxp < 2)
      tdxp=2;

    if (speakingChar->walking)
      StopMoving(aschar);

    // save the frame we need to go back to
    // if they were moving, this will be 0 (because we just called
    // StopMoving); otherwise, it might be a specific animation 
    // frame which we should return to
    if (viewWasLocked)
      charFrameWas = speakingChar->frame;

    // if the current loop doesn't exist in talking view, use loop 0
    if (speakingChar->loop >= views[speakingChar->view].numLoops)
      speakingChar->loop = 0;

    if ((speakingChar->view < 0) || 
        (speakingChar->loop >= views[speakingChar->view].numLoops) ||
        (views[speakingChar->view].loops[speakingChar->loop].numFrames < 1))
    {
      quitprintf("Unable to display speech because the character %s has an invalid view frame (View %d, loop %d, frame %d)", speakingChar->scrname, speakingChar->view + 1, speakingChar->loop, speakingChar->frame);
    }

    our_eip=1504;

    if (tdyp < 0) 
    {
      int sppic = views[speakingChar->view].loops[speakingChar->loop].frames[0].pic;
      tdyp = multiply_up_coordinate(speakingChar->get_effective_y()) - offsety - get_fixed_pixel_size(5);
      if (charextra[aschar].height < 1)
        tdyp -= spriteheight[sppic];
      else
        tdyp -= charextra[aschar].height;
      // if it's a thought, lift it a bit further up
      if (isThought)  
        tdyp -= get_fixed_pixel_size(10);
    }

    our_eip=1505;
    if (tdyp < 5)
      tdyp=5;
      
    tdxp=-tdxp;  // tell it to centre it
    our_eip=152;

    if ((useview >= 0) && (game.options[OPT_SPEECHTYPE] > 0)) {
      // Sierra-style close-up portrait

      if (play.swap_portrait_lastchar != aschar) {
        // if the portraits are set to Alternate, OR they are
        // set to Left but swap_portrait has been set to 1 (the old
        // method for enabling it), then swap them round
        if ((game.options[OPT_PORTRAITSIDE] == PORTRAIT_ALTERNATE) ||
            ((game.options[OPT_PORTRAITSIDE] == 0) &&
             (play.swap_portrait_side > 0))) {

          if (play.swap_portrait_side == 2)
            play.swap_portrait_side = 1;
          else
            play.swap_portrait_side = 2;
        }

        if (game.options[OPT_PORTRAITSIDE] == PORTRAIT_XPOSITION) {
          // Portrait side based on character X-positions
          if (play.swap_portrait_lastchar < 0) {
            // no previous character been spoken to
            // therefore, find another character in this room
            // that it could be
            for (int ce = 0; ce < game.numcharacters; ce++) {
              if ((game.chars[ce].room == speakingChar->room) &&
                  (game.chars[ce].on == 1) &&
                  (ce != aschar)) {
                play.swap_portrait_lastchar = ce;
                break;
              }
            }
          }

          if (play.swap_portrait_lastchar >= 0) {
            // if this character is right of the one before, put the
            // portrait on the right
            if (speakingChar->x > game.chars[play.swap_portrait_lastchar].x)
              play.swap_portrait_side = -1;
            else
              play.swap_portrait_side = 0;
          }
        }

        play.swap_portrait_lastchar = aschar;
      }

      // Determine whether to display the portrait on the left or right
      int portrait_on_right = 0;

      if (game.options[OPT_SPEECHTYPE] == 3) 
        { }  // always on left with QFG-style speech
      else if ((play.swap_portrait_side == 1) ||
          (play.swap_portrait_side == -1) ||
          (game.options[OPT_PORTRAITSIDE] == PORTRAIT_RIGHT))
        portrait_on_right = 1;


      int bigx=0,bigy=0,kk;
      ViewStruct*viptr=&views[useview];
      for (kk = 0; kk < viptr->loops[0].numFrames; kk++) 
      {
        int tw = spritewidth[viptr->loops[0].frames[kk].pic];
        if (tw > bigx) bigx=tw;
        tw = spriteheight[viptr->loops[0].frames[kk].pic];
        if (tw > bigy) bigy=tw;
      }

      // if they accidentally used a large full-screen image as the sierra-style
      // talk view, correct it
      if ((game.options[OPT_SPEECHTYPE] != 3) && (bigx > scrnwid - get_fixed_pixel_size(50)))
        bigx = scrnwid - get_fixed_pixel_size(50);

      if (widd > 0)
        bwidth = widd - bigx;

      our_eip=153;
      int draw_yp = 0, ovr_yp = get_fixed_pixel_size(20);
      if (game.options[OPT_SPEECHTYPE] == 3) {
        // QFG4-style whole screen picture
        closeupface = create_bitmap_ex(bitmap_color_depth(spriteset[viptr->loops[0].frames[0].pic]), scrnwid, scrnhit);
        clear_to_color(closeupface, 0);
        draw_yp = scrnhit/2 - spriteheight[viptr->loops[0].frames[0].pic]/2;
        bigx = scrnwid/2 - get_fixed_pixel_size(20);
        ovr_type = OVER_COMPLETE;
        ovr_yp = 0;
        tdyp = -1;  // center vertically
      }
      else {
        // KQ6-style close-up face picture
        if (yy < 0)
          ovr_yp = adjust_y_for_guis (ovr_yp);
        else
          ovr_yp = yy;

        closeupface = create_bitmap_ex(bitmap_color_depth(spriteset[viptr->loops[0].frames[0].pic]),bigx+1,bigy+1);
        clear_to_color(closeupface,bitmap_mask_color(closeupface));
        ovr_type = OVER_PICTURE;

        if (yy < 0)
          tdyp = ovr_yp + get_textwindow_top_border_height(play.speech_textwindow_gui);
      }
      //draw_sprite(closeupface,spriteset[viptr->frames[0][0].pic],0,draw_yp);
      DrawViewFrame(closeupface, &viptr->loops[0].frames[0], 0, draw_yp);

      int overlay_x = get_fixed_pixel_size(10);

      if (xx < 0) {
        tdxp = get_fixed_pixel_size(16) + bigx + get_textwindow_border_width(play.speech_textwindow_gui) / 2;

        int maxWidth = (scrnwid - tdxp) - get_fixed_pixel_size(5) - 
             get_textwindow_border_width (play.speech_textwindow_gui) / 2;

        if (bwidth > maxWidth)
          bwidth = maxWidth;
      }
      else {
        tdxp = xx + bigx + get_fixed_pixel_size(8);
        overlay_x = xx;
      }

      // allow the text box to be shrunk to fit the text
      allowShrink = 1;

      // if the portrait's on the right, swap it round
      if (portrait_on_right) {
        if ((xx < 0) || (widd < 0)) {
          overlay_x = (scrnwid - bigx) - get_fixed_pixel_size(5);
          tdxp = get_fixed_pixel_size(9);
        }
        else {
          overlay_x = (xx + widd - bigx) - get_fixed_pixel_size(5);
          tdxp = xx;
        }
        tdxp += get_textwindow_border_width(play.speech_textwindow_gui) / 2;
        allowShrink = 2;
      }
      if (game.options[OPT_SPEECHTYPE] == 3)
        overlay_x = 0;
      face_talking=add_screen_overlay(overlay_x,ovr_yp,ovr_type,closeupface);
      facetalkframe = 0;
      facetalkwait = viptr->loops[0].frames[0].speed + GetCharacterSpeechAnimationDelay(speakingChar);
      facetalkloop = 0;
      facetalkview = useview;
      facetalkrepeat = (isThought) ? 0 : 1;
      facetalkBlinkLoop = 0;
      facetalkAllowBlink = 1;
      if ((isThought) && (speakingChar->flags & CHF_NOBLINKANDTHINK))
        facetalkAllowBlink = 0;
      facetalkchar = &game.chars[aschar];
      if (facetalkchar->blinktimer < 0)
        facetalkchar->blinktimer = facetalkchar->blinkinterval;
      textcol=-textcol;
      overlayPositionFixed = true;
    }
    else if (useview >= 0) {
      // Lucasarts-style speech
      our_eip=154;

      oldview = speakingChar->view;
      oldloop = speakingChar->loop;
      speakingChar->animating = 1 | (GetCharacterSpeechAnimationDelay(speakingChar) << 8);
      // only repeat if speech, not thought
      if (!isThought)
        speakingChar->animating |= CHANIM_REPEAT;

      speakingChar->view = useview;
      speakingChar->frame=0;
      speakingChar->flags|=CHF_FIXVIEW;

      if (speakingChar->loop >= views[speakingChar->view].numLoops)
      {
        // current character loop is outside the normal talking directions
        speakingChar->loop = 0;
      }

      facetalkBlinkLoop = speakingChar->loop;

      if ((speakingChar->loop >= views[speakingChar->view].numLoops) ||
          (views[speakingChar->view].loops[speakingChar->loop].numFrames < 1))
      {
        quitprintf("!Unable to display speech because the character %s has an invalid speech view (View %d, loop %d, frame %d)", speakingChar->scrname, speakingChar->view + 1, speakingChar->loop, speakingChar->frame);
      }

      // set up the speed of the first frame
      speakingChar->wait = GetCharacterSpeechAnimationDelay(speakingChar) + 
                           views[speakingChar->view].loops[speakingChar->loop].frames[0].speed;

      if (widd < 0) {
        bwidth = scrnwid/2 + scrnwid/6;
        // If they are close to the screen edge, make the text narrower
        int relx = multiply_up_coordinate(speakingChar->x) - offsetx;
        if ((relx < scrnwid / 4) || (relx > scrnwid - (scrnwid / 4)))
          bwidth -= scrnwid / 5;
      }
/*   this causes the text to bob up and down as they talk
      tdxp = OVR_AUTOPLACE;
      tdyp = aschar;*/
      if (!isThought)  // set up the lip sync if not thinking
        char_speaking = aschar;

    }
  }
  else
    allowShrink = 1;

  // it wants the centred position, so make it so
  if ((xx >= 0) && (tdxp < 0))
    tdxp -= widd / 2;

  // if they used DisplaySpeechAt, then use the supplied width
  if ((widd > 0) && (isThought == 0))
    allowShrink = 0;

  our_eip=155;
  _display_at(tdxp,tdyp,bwidth,texx,0,textcol, isThought, allowShrink, overlayPositionFixed);
  our_eip=156;
  if ((play.in_conversation > 0) && (game.options[OPT_SPEECHTYPE] == 3))
    closeupface = NULL;
  if (closeupface!=NULL)
    remove_screen_overlay(ovr_type);
  screen_is_dirty = 1;
  face_talking = -1;
  facetalkchar = NULL;
  our_eip=157;
  if (oldview>=0) {
    speakingChar->flags &= ~CHF_FIXVIEW;
    if (viewWasLocked)
      speakingChar->flags |= CHF_FIXVIEW;
    speakingChar->view=oldview;
    speakingChar->loop = oldloop;
    speakingChar->animating=0;
    speakingChar->frame = charFrameWas;
    speakingChar->wait=0;
    speakingChar->idleleft = speakingChar->idletime;
    // restart the idle animation straight away
    charextra[aschar].process_idle_this_time = 1;
  }
  char_speaking = -1;
  stop_speech();
}

int get_character_currently_talking() {
  if ((face_talking >= 0) && (facetalkrepeat))
    return facetalkchar->index_id;
  else if (char_speaking >= 0)
    return char_speaking;

  return -1;
}

int Character_GetSpeakingFrame(CharacterInfo *chaa) {

  if ((face_talking >= 0) && (facetalkrepeat))
  {
    if (facetalkchar->index_id == chaa->index_id)
    {
      return facetalkframe;
    }
  }
  else if (char_speaking >= 0)
  {
    if (char_speaking == chaa->index_id)
    {
      return chaa->frame;
    }
  }

  quit("!Character.SpeakingFrame: character is not currently speaking");
  return -1;
}


void DisplaySpeech(char*texx, int aschar) {
  _displayspeech (texx, aschar, -1, -1, -1, 0);
}

// **** THIS IS UNDOCUMENTED BECAUSE IT DOESN'T WORK PROPERLY
// **** AT 640x400 AND DOESN'T USE THE RIGHT SPEECH STYLE
void DisplaySpeechAt (int xx, int yy, int wii, int aschar, char*spch) {
  multiply_up_coordinates(&xx, &yy);
  wii = multiply_up_coordinate(wii);
  _displayspeech (get_translation(spch), aschar, xx, yy, wii, 0);
}

void SetGlobalInt(int index,int valu) {
  if ((index<0) | (index>=MAXGSVALUES))
    quit("!SetGlobalInt: invalid index");

  if (play.globalscriptvars[index] != valu) {
    DEBUG_CONSOLE("GlobalInt %d set to %d", index, valu);
  }

  play.globalscriptvars[index]=valu;
}
int GetGlobalInt(int index) {
  if ((index<0) | (index>=MAXGSVALUES))
    quit("!GetGlobalInt: invalid index");
  return play.globalscriptvars[index];
}

void SetGlobalString (int index, char *newval) {
  if ((index<0) | (index >= MAXGLOBALSTRINGS))
    quit("!SetGlobalString: invalid index");
  DEBUG_CONSOLE("GlobalString %d set to '%s'", index, newval);
  strncpy(play.globalstrings[index], newval, MAX_MAXSTRLEN);
  // truncate it to 200 chars, to be sure
  play.globalstrings[index][MAX_MAXSTRLEN - 1] = 0;
}

void GetGlobalString (int index, char *strval) {
  if ((index<0) | (index >= MAXGLOBALSTRINGS))
    quit("!GetGlobalString: invalid index");
  strcpy (strval, play.globalstrings[index]);
}

const char* Game_GetGlobalStrings(int index) {
  if ((index < 0) || (index >= MAXGLOBALSTRINGS))
    quit("!Game.GlobalStrings: invalid index");

  return CreateNewScriptString(play.globalstrings[index]);
}


void SetScreenTransition(int newtrans) {
  if ((newtrans < 0) || (newtrans > FADE_LAST))
    quit("!SetScreenTransition: invalid transition type");

  play.fade_effect = newtrans;

  DEBUG_CONSOLE("Screen transition changed");
}

void SetNextScreenTransition(int newtrans) {
  if ((newtrans < 0) || (newtrans > FADE_LAST))
    quit("!SetNextScreenTransition: invalid transition type");

  play.next_screen_transition = newtrans;

  DEBUG_CONSOLE("SetNextScreenTransition engaged");
}

void SetFadeColor(int red, int green, int blue) {
  if ((red < 0) || (red > 255) || (green < 0) || (green > 255) ||
      (blue < 0) || (blue > 255))
    quit("!SetFadeColor: Red, Green and Blue must be 0-255");

  play.fade_to_red = red;
  play.fade_to_green = green;
  play.fade_to_blue = blue;
}

// 0 = click mouse or key to skip
// 1 = key only
// 2 = can't skip at all
// 3 = only on keypress, no auto timer
// 4 = mouseclick only
void SetSkipSpeech (int newval) {
  if ((newval < 0) || (newval > 4))
    quit("!SetSkipSpeech: invalid skip mode specified (0-4)");

  DEBUG_CONSOLE("SkipSpeech style set to %d", newval);
  play.cant_skip_speech = user_to_internal_skip_speech(newval);
}

void DisplayAtY (int ypos, char *texx) {
  if ((ypos < -1) || (ypos >= GetMaxScreenHeight()))
    quitprintf("!DisplayAtY: invalid Y co-ordinate supplied (used: %d; valid: 0..%d)", ypos, GetMaxScreenHeight());

  // Display("") ... a bit of a stupid thing to do, so ignore it
  if (texx[0] == 0)
    return;

  if (ypos > 0)
    ypos = multiply_up_coordinate(ypos);

  if (game.options[OPT_ALWAYSSPCH])
    DisplaySpeechAt(-1, (ypos > 0) ? divide_down_coordinate(ypos) : ypos, -1, game.playercharacter, texx);
  else { 
    // Normal "Display" in text box

    if (screen_is_dirty) {
      // erase any previous DisplaySpeech
      play.disabled_user_interface ++;
      mainloop();
      play.disabled_user_interface --;
    }

    _display_at(-1,ypos,scrnwid/2+scrnwid/4,get_translation(texx),1,0, 0, 0, false);
  }
}

void Display(char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf, get_translation(texx), ap);
  va_end(ap);
  DisplayAtY (-1, displbuf);
}

void _DisplaySpeechCore(int chid, char *displbuf) {
  if (displbuf[0] == 0) {
    // no text, just update the current character who's speaking
    // this allows the portrait side to be switched with an empty
    // speech line
    play.swap_portrait_lastchar = chid;
    return;
  }

  // adjust timing of text (so that DisplaySpeech("%s", str) pauses
  // for the length of the string not 2 frames)
  if ((int)strlen(displbuf) > source_text_length + 3)
    source_text_length = strlen(displbuf);

  DisplaySpeech(displbuf, chid);
}

void __sc_displayspeech(int chid,char*texx, ...) {
  if ((chid<0) || (chid>=game.numcharacters))
    quit("!DisplaySpeech: invalid character specified");

  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf, get_translation(texx), ap);
  va_end(ap);

  _DisplaySpeechCore(chid, displbuf);

}

void DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char*texx, ...) {

  strcpy(topBar.text, get_translation(title));

  char displbuf[3001];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf, get_translation(texx), ap);
  va_end(ap);

  if (ypos > 0)
    play.top_bar_ypos = ypos;
  if (ttexcol > 0)
    play.top_bar_textcolor = ttexcol;
  if (backcol > 0)
    play.top_bar_backcolor = backcol;

  topBar.wantIt = 1;
  topBar.font = FONT_NORMAL;
  topBar.height = wgetfontheight(topBar.font);
  topBar.height += multiply_up_coordinate(play.top_bar_borderwidth) * 2 + get_fixed_pixel_size(1);

  // they want to customize the font
  if (play.top_bar_font >= 0)
    topBar.font = play.top_bar_font;

  // DisplaySpeech normally sets this up, but since we're not going via it...
  if (play.cant_skip_speech & SKIP_AUTOTIMER)
    play.messagetime = GetTextDisplayTime(texx);

  DisplayAtY(play.top_bar_ypos, displbuf);
}

// Display a room/global message in the bar
void DisplayMessageBar(int ypos, int ttexcol, int backcol, char *title, int msgnum) {
  char msgbufr[3001];
  get_message_text(msgnum, msgbufr);
  DisplayTopBar(ypos, ttexcol, backcol, title, "%s", msgbufr);
}

void _DisplayThoughtCore(int chid, const char *displbuf) {
  // adjust timing of text (so that DisplayThought("%s", str) pauses
  // for the length of the string not 2 frames)
  if ((int)strlen(displbuf) > source_text_length + 3)
    source_text_length = strlen(displbuf);

  int xpp = -1, ypp = -1, width = -1;

  if ((game.options[OPT_SPEECHTYPE] == 0) || (game.chars[chid].thinkview <= 0)) {
    // lucasarts-style, so we want a speech bubble actually above
    // their head (or if they have no think anim in Sierra-style)
    width = multiply_up_coordinate(play.speech_bubble_width);
    xpp = (multiply_up_coordinate(game.chars[chid].x) - offsetx) - width / 2;
    if (xpp < 0)
      xpp = 0;
    // -1 will automatically put it above the char's head
    ypp = -1;
  }

  _displayspeech ((char*)displbuf, chid, xpp, ypp, width, 1);
}

void DisplayThought(int chid, const char*texx, ...) {
  if ((chid < 0) || (chid >= game.numcharacters))
    quit("!DisplayThought: invalid character specified");

  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf,get_translation(texx),ap);
  va_end(ap);

  _DisplayThoughtCore(chid, displbuf);
}

void replace_tokens(char*srcmes,char*destm, int maxlen = 99999) {
  int indxdest=0,indxsrc=0;
  char*srcp,*destp;
  while (srcmes[indxsrc]!=0) {
    srcp=&srcmes[indxsrc];
    destp=&destm[indxdest];
    if ((strncmp(srcp,"@IN",3)==0) | (strncmp(srcp,"@GI",3)==0)) {
      int tokentype=0;
      if (srcp[1]=='I') tokentype=1;
      else tokentype=2;
      int inx=atoi(&srcp[3]);
      srcp++;
      indxsrc+=2;
      while (srcp[0]!='@') {
        if (srcp[0]==0) quit("!Display: special token not terminated");
        srcp++;
        indxsrc++;
        }
      char tval[10];
      if (tokentype==1) {
        if ((inx<1) | (inx>=game.numinvitems))
          quit("!Display: invalid inv item specified in @IN@");
        sprintf(tval,"%d",playerchar->inv[inx]);
        }
      else {
        if ((inx<0) | (inx>=MAXGSVALUES))
          quit("!Display: invalid global int index speicifed in @GI@");
        sprintf(tval,"%d",GetGlobalInt(inx));
        }
      strcpy(destp,tval);
      indxdest+=strlen(tval);
      }
    else {
      destp[0]=srcp[0];
      indxdest++;
      indxsrc++;
      }
    if (indxdest >= maxlen - 3)
      break;
    }
  destm[indxdest]=0;
  }

char *get_global_message (int msnum) {
  if (game.messages[msnum-500] == NULL)
    return "";
  return get_translation(game.messages[msnum-500]);
}

int display_message_aschar=0;
void get_message_text (int msnum, char *buffer, char giveErr) {
  int maxlen = 9999;
  if (!giveErr)
    maxlen = MAX_MAXSTRLEN;

  if (msnum>=500) { //quit("global message requseted, nto yet supported");

    if ((msnum >= MAXGLOBALMES + 500) || (game.messages[msnum-500]==NULL)) {
      if (giveErr)
        quit("!DisplayGlobalMessage: message does not exist");
      buffer[0] = 0;
      return;
    }
    buffer[0] = 0;
    replace_tokens(get_translation(game.messages[msnum-500]), buffer, maxlen);
    return;
  }
  else if (msnum >= thisroom.nummes) {
    if (giveErr)
      quit("!DisplayMessage: Invalid message number to display");
    buffer[0] = 0;
    return;
  }

  buffer[0]=0;
  replace_tokens(get_translation(thisroom.message[msnum]), buffer, maxlen);

}

void GetMessageText (int msg, char *buffer) {
  VALIDATE_STRING(buffer);
  get_message_text (msg, buffer, 0);
}

const char* Room_GetMessages(int index) {
  if ((index < 0) || (index >= thisroom.nummes)) {
    return NULL;
  }
  char buffer[STD_BUFFER_SIZE];
  buffer[0]=0;
  replace_tokens(get_translation(thisroom.message[index]), buffer, STD_BUFFER_SIZE);
  return CreateNewScriptString(buffer);
}

const char* Game_GetGlobalMessages(int index) {
  if ((index < 500) || (index >= MAXGLOBALMES + 500)) {
    return NULL;
  }
  char buffer[STD_BUFFER_SIZE];
  buffer[0] = 0;
  replace_tokens(get_translation(get_global_message(index)), buffer, STD_BUFFER_SIZE);
  return CreateNewScriptString(buffer);
}

void DisplayMessageAtY(int msnum, int ypos) {
  char msgbufr[3001];
  if (msnum>=500) { //quit("global message requseted, nto yet supported");
    get_message_text (msnum, msgbufr);
    if (display_message_aschar > 0)
      DisplaySpeech(msgbufr, display_message_aschar);
    else
      DisplayAtY(ypos, msgbufr);
    display_message_aschar=0;
    return;
  }

  if (display_message_aschar > 0) {
    display_message_aschar=0;
    quit("!DisplayMessage: data column specified a character for local\n"
         "message; use the message editor to select the character for room\n"
         "messages.\n");
  }

  int repeatloop=1;
  while (repeatloop) {
    get_message_text (msnum, msgbufr);

    if (thisroom.msgi[msnum].displayas>0) {
      DisplaySpeech(msgbufr, thisroom.msgi[msnum].displayas - 1);
    }
    else {
      // time out automatically if they have set that
      int oldGameSkipDisp = play.skip_display;
      if (thisroom.msgi[msnum].flags & MSG_TIMELIMIT)
        play.skip_display = 0;

      DisplayAtY(ypos, msgbufr);

      play.skip_display = oldGameSkipDisp;
    }
    if (thisroom.msgi[msnum].flags & MSG_DISPLAYNEXT) {
      msnum++;
      repeatloop=1;
    }
    else
      repeatloop=0;
  }

}

void DisplayMessage(int msnum) {
  DisplayMessageAtY (msnum, -1);
}

// Raw screen writing routines - similar to old CapturedStuff
#define RAW_START() block oldabuf=abuf; abuf=thisroom.ebscene[play.bg_frame]; play.raw_modified[play.bg_frame] = 1
#define RAW_END() abuf = oldabuf
// RawSaveScreen: copy the current screen to a backup bitmap
void RawSaveScreen () {
  if (raw_saved_screen != NULL)
    wfreeblock(raw_saved_screen);
  block source = thisroom.ebscene[play.bg_frame];
  raw_saved_screen = wallocblock(source->w, source->h);
  blit(source, raw_saved_screen, 0, 0, 0, 0, source->w, source->h);
}
// RawRestoreScreen: copy backup bitmap back to screen; we
// deliberately don't free the block cos they can multiple restore
// and it gets freed on room exit anyway
void RawRestoreScreen() {
  if (raw_saved_screen == NULL) {
    debug_log("RawRestoreScreen: unable to restore, since the screen hasn't been saved previously.");
    return;
  }
  block deston = thisroom.ebscene[play.bg_frame];
  blit(raw_saved_screen, deston, 0, 0, 0, 0, deston->w, deston->h);
  invalidate_screen();
  mark_current_background_dirty();
}
// Restores the backup bitmap, but tints it to the specified level
void RawRestoreScreenTinted(int red, int green, int blue, int opacity) {
  if (raw_saved_screen == NULL) {
    debug_log("RawRestoreScreenTinted: unable to restore, since the screen hasn't been saved previously.");
    return;
  }
  if ((red < 0) || (green < 0) || (blue < 0) ||
      (red > 255) || (green > 255) || (blue > 255) ||
      (opacity < 1) || (opacity > 100))
    quit("!RawRestoreScreenTinted: invalid parameter. R,G,B must be 0-255, opacity 1-100");

  DEBUG_CONSOLE("RawRestoreTinted RGB(%d,%d,%d) %d%%", red, green, blue, opacity);

  block deston = thisroom.ebscene[play.bg_frame];
  tint_image(raw_saved_screen, deston, red, green, blue, opacity);
  invalidate_screen();
  mark_current_background_dirty();
}

void RawDrawFrameTransparent (int frame, int translev) {
  if ((frame < 0) || (frame >= thisroom.num_bscenes) ||
      (translev < 0) || (translev > 99))
    quit("!RawDrawFrameTransparent: invalid parameter (transparency must be 0-99, frame a valid BG frame)");

  if (bitmap_color_depth(thisroom.ebscene[frame]) <= 8)
    quit("!RawDrawFrameTransparent: 256-colour backgrounds not supported");

  if (frame == play.bg_frame)
    quit("!RawDrawFrameTransparent: cannot draw current background onto itself");

  if (translev == 0) {
    // just draw it over the top, no transparency
    blit(thisroom.ebscene[frame], thisroom.ebscene[play.bg_frame], 0, 0, 0, 0, thisroom.ebscene[frame]->w, thisroom.ebscene[frame]->h);
    play.raw_modified[play.bg_frame] = 1;
    return;
  }
  // Draw it transparently
  RAW_START();
  trans_mode = ((100-translev) * 25) / 10;
  put_sprite_256 (0, 0, thisroom.ebscene[frame]);
  invalidate_screen();
  mark_current_background_dirty();
  RAW_END();
}

void RawClear (int clr) {
  play.raw_modified[play.bg_frame] = 1;
  clr = get_col8_lookup(clr);
  clear_to_color (thisroom.ebscene[play.bg_frame], clr);
  invalidate_screen();
  mark_current_background_dirty();
}
void RawSetColor (int clr) {
  push_screen();
  wsetscreen(thisroom.ebscene[play.bg_frame]);
  // set the colour at the appropriate depth for the background
  play.raw_color = get_col8_lookup(clr);
  pop_screen();
}
void RawSetColorRGB(int red, int grn, int blu) {
  if ((red < 0) || (red > 255) || (grn < 0) || (grn > 255) ||
      (blu < 0) || (blu > 255))
    quit("!RawSetColorRGB: colour values must be 0-255");

  play.raw_color = makecol_depth(bitmap_color_depth(thisroom.ebscene[play.bg_frame]), red, grn, blu);
}
void RawPrint (int xx, int yy, char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf,get_translation(texx),ap);
  va_end(ap);
  // don't use wtextcolor because it will do a 16->32 conversion
  textcol = play.raw_color;
  RAW_START();
  wtexttransparent(TEXTFG);
  if ((bitmap_color_depth(abuf) <= 8) && (play.raw_color > 255)) {
    wtextcolor(1);
    debug_log ("RawPrint: Attempted to use hi-color on 256-col background");
  }
  multiply_up_coordinates(&xx, &yy);
  wouttext_outline(xx, yy, play.normal_font, displbuf);
  // we must invalidate the entire screen because these are room
  // co-ordinates, not screen co-ords which it works with
  invalidate_screen();
  mark_current_background_dirty();
  RAW_END();
}
void RawPrintMessageWrapped (int xx, int yy, int wid, int font, int msgm) {
  char displbuf[3000];
  int texthit = wgetfontheight(font);
  multiply_up_coordinates(&xx, &yy);
  wid = multiply_up_coordinate(wid);

  get_message_text (msgm, displbuf);
  // it's probably too late but check anyway
  if (strlen(displbuf) > 2899)
    quit("!RawPrintMessageWrapped: message too long");
  break_up_text_into_lines (wid, font, displbuf);

  textcol = play.raw_color;
  RAW_START();
  wtexttransparent(TEXTFG);
  for (int i = 0; i < numlines; i++)
    wouttext_outline(xx, yy + texthit*i, font, lines[i]);
  invalidate_screen();
  mark_current_background_dirty();
  RAW_END();
}

void RawDrawImageCore(int xx, int yy, int slot) {
  if ((slot < 0) || (slot >= MAX_SPRITES) || (spriteset[slot] == NULL))
    quit("!RawDrawImage: invalid sprite slot number specified");
  RAW_START();

  if (bitmap_color_depth(spriteset[slot]) != bitmap_color_depth(abuf)) {
    debug_log("RawDrawImage: Sprite %d colour depth %d-bit not same as background depth %d-bit", slot, bitmap_color_depth(spriteset[slot]), bitmap_color_depth(abuf));
  }

  draw_sprite_support_alpha(xx, yy, spriteset[slot], slot);
  invalidate_screen();
  mark_current_background_dirty();
  RAW_END();
}

void RawDrawImage(int xx, int yy, int slot) {
  multiply_up_coordinates(&xx, &yy);
  RawDrawImageCore(xx, yy, slot);
}

void RawDrawImageOffset(int xx, int yy, int slot) {

  if ((current_screen_resolution_multiplier == 1) && (game.default_resolution >= 3)) {
    // running a 640x400 game at 320x200, adjust
    xx /= 2;
    yy /= 2;
  }
  else if ((current_screen_resolution_multiplier > 1) && (game.default_resolution <= 2)) {
    // running a 320x200 game at 640x400, adjust
    xx *= 2;
    yy *= 2;
  }

  RawDrawImageCore(xx, yy, slot);
}

void RawDrawImageTransparent(int xx, int yy, int slot, int trans) {
  if ((trans < 0) || (trans > 100))
    quit("!RawDrawImageTransparent: invalid transparency setting");

  // since RawDrawImage uses putsprite256, we can just select the
  // transparency mode and call it
  trans_mode = (trans * 255) / 100;
  RawDrawImage(xx, yy, slot);

  update_polled_stuff();  // this operation can be slow so stop music skipping
}
void RawDrawImageResized(int xx, int yy, int gotSlot, int width, int height) {
  if ((gotSlot < 0) || (gotSlot >= MAX_SPRITES) || (spriteset[gotSlot] == NULL))
    quit("!RawDrawImageResized: invalid sprite slot number specified");
  // very small, don't draw it
  if ((width < 1) || (height < 1))
    return;

  multiply_up_coordinates(&xx, &yy);
  multiply_up_coordinates(&width, &height);

  // resize the sprite to the requested size
  block newPic = create_bitmap_ex(bitmap_color_depth(spriteset[gotSlot]), width, height);

  stretch_blit(spriteset[gotSlot], newPic,
               0, 0, spritewidth[gotSlot], spriteheight[gotSlot],
               0, 0, width, height);

  RAW_START();
  if (bitmap_color_depth(newPic) != bitmap_color_depth(abuf))
    quit("!RawDrawImageResized: image colour depth mismatch: the background image must have the same colour depth as the sprite being drawn");

  put_sprite_256(xx, yy, newPic);
  destroy_bitmap(newPic);
  invalidate_screen();
  mark_current_background_dirty();
  update_polled_stuff();  // this operation can be slow so stop music skipping
  RAW_END();
}
void RawDrawLine (int fromx, int fromy, int tox, int toy) {
  multiply_up_coordinates(&fromx, &fromy);
  multiply_up_coordinates(&tox, &toy);

  play.raw_modified[play.bg_frame] = 1;
  int ii,jj;
  // draw a line thick enough to look the same at all resolutions
  for (ii = 0; ii < get_fixed_pixel_size(1); ii++) {
    for (jj = 0; jj < get_fixed_pixel_size(1); jj++)
      line (thisroom.ebscene[play.bg_frame], fromx+ii, fromy+jj, tox+ii, toy+jj, play.raw_color);
  }
  invalidate_screen();
  mark_current_background_dirty();
}
void RawDrawCircle (int xx, int yy, int rad) {
  multiply_up_coordinates(&xx, &yy);
  rad = multiply_up_coordinate(rad);

  play.raw_modified[play.bg_frame] = 1;
  circlefill (thisroom.ebscene[play.bg_frame], xx, yy, rad, play.raw_color);
  invalidate_screen();
  mark_current_background_dirty();
}
void RawDrawRectangle(int x1, int y1, int x2, int y2) {
  play.raw_modified[play.bg_frame] = 1;
  multiply_up_coordinates(&x1, &y1);
  multiply_up_coordinates_round_up(&x2, &y2);

  rectfill(thisroom.ebscene[play.bg_frame], x1,y1,x2,y2, play.raw_color);
  invalidate_screen();
  mark_current_background_dirty();
}
void RawDrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3) {
  play.raw_modified[play.bg_frame] = 1;
  multiply_up_coordinates(&x1, &y1);
  multiply_up_coordinates(&x2, &y2);
  multiply_up_coordinates(&x3, &y3);

  triangle (thisroom.ebscene[play.bg_frame], x1,y1,x2,y2,x3,y3, play.raw_color);
  invalidate_screen();
  mark_current_background_dirty();
}

int SaveScreenShot(char*namm) {
  char fileName[MAX_PATH];

  if (strchr(namm,'.') == NULL)
    sprintf(fileName, "%s%s.bmp", saveGameDirectory, namm);
  else
    sprintf(fileName, "%s%s", saveGameDirectory, namm);

  if (gfxDriver->RequiresFullRedrawEachFrame()) 
  {
    BITMAP *buffer = create_bitmap_ex(32, scrnwid, scrnhit);
    gfxDriver->GetCopyOfScreenIntoBitmap(buffer);

    if (save_bitmap(fileName, buffer, palette)!=0)
    {
      destroy_bitmap(buffer);
      return 0;
    }
    destroy_bitmap(buffer);
  }
  else if (save_bitmap(fileName, virtual_screen, palette)!=0)
    return 0; // failed

  return 1;  // successful
}

void SetObjectView(int obn,int vii) {
  if (!is_valid_object(obn)) quit("!SetObjectView: invalid object number specified");
  DEBUG_CONSOLE("Object %d set to view %d", obn, vii);
  if ((vii < 1) || (vii > game.numviews)) {
    char buffer[150];
    sprintf (buffer, "!SetObjectView: invalid view number (You said %d, max is %d)", vii, game.numviews);
    quit(buffer);
  }
  vii--;

  objs[obn].view=vii;
  objs[obn].frame=0;
  if (objs[obn].loop >= views[vii].numLoops)
    objs[obn].loop=0;
  objs[obn].cycling=0;
  objs[obn].num = views[vii].loops[0].frames[0].pic;
  }

void SetObjectFrame(int obn,int viw,int lop,int fra) {
  if (!is_valid_object(obn)) quit("!SetObjectFrame: invalid object number specified");
  viw--;
  if (viw>=game.numviews) quit("!SetObjectFrame: invalid view number used");
  if (lop>=views[viw].numLoops) quit("!SetObjectFrame: invalid loop number used");
  objs[obn].view=viw;
  if (fra >= 0)
    objs[obn].frame=fra;
  if (lop >= 0)
    objs[obn].loop=lop;

  if (objs[obn].loop >= views[viw].numLoops)
    objs[obn].loop = 0;
  if (objs[obn].frame >= views[viw].loops[objs[obn].loop].numFrames)
    objs[obn].frame = 0;

  if (views[viw].loops[objs[obn].loop].numFrames == 0) 
    quit("!SetObjectFrame: specified loop has no frames");

  objs[obn].cycling=0;
  objs[obn].num = views[viw].loops[objs[obn].loop].frames[objs[obn].frame].pic;
  CheckViewFrame(viw, objs[obn].loop, objs[obn].frame);
}

void Object_SetView(ScriptObject *objj, int view, int loop, int frame) {
  SetObjectFrame(objj->id, view, loop, frame);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetObjectTransparency(int obn,int trans) {
  if (!is_valid_object(obn)) quit("!SetObjectTransparent: invalid object number specified");
  if ((trans < 0) || (trans > 100)) quit("!SetObjectTransparent: transparency value must be between 0 and 100");
  if (trans == 0)
    objs[obn].transparent=0;
  else if (trans == 100)
    objs[obn].transparent = 255;
  else
    objs[obn].transparent=((100-trans) * 25) / 10;
}

void Object_SetTransparency(ScriptObject *objj, int trans) {
  SetObjectTransparency(objj->id, trans);
}

int Object_GetTransparency(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.Transparent: invalid object number specified");

  if (objj->obj->transparent == 0)
    return 0;
  if (objj->obj->transparent == 255)
    return 100;

  return 100 - ((objj->obj->transparent * 10) / 25);

}

void SetObjectBaseline (int obn, int basel) {
  if (!is_valid_object(obn)) quit("!SetObjectBaseline: invalid object number specified");
  // baseline has changed, invalidate the cache
  if (objs[obn].baseline != basel) {
    objcache[obn].ywas = -9999;
    objs[obn].baseline = basel;
  }
}

void Object_SetBaseline(ScriptObject *objj, int basel) {
  SetObjectBaseline(objj->id, basel);
}

int GetObjectBaseline(int obn) {
  if (!is_valid_object(obn)) quit("!GetObjectBaseline: invalid object number specified");

  if (objs[obn].baseline < 1)
    return 0;
  
  return objs[obn].baseline;
}

int Object_GetBaseline(ScriptObject *objj) {
  return GetObjectBaseline(objj->id);
}

void SetCharacterBaseline (int obn, int basel) {
  if (!is_valid_character(obn)) quit("!SetCharacterBaseline: invalid object number specified");
  
  Character_SetBaseline(&game.chars[obn], basel);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetCharacterTransparency(int obn,int trans) {
  if (!is_valid_character(obn))
    quit("!SetCharTransparent: invalid character number specified");
  
  Character_SetTransparency(&game.chars[obn], trans);
}

void scAnimateCharacter (int chh, int loopn, int sppd, int rept) {
  if (!is_valid_character(chh))
    quit("AnimateCharacter: invalid character");

  animate_character(&game.chars[chh], loopn, sppd, rept);
}

void AnimateCharacterEx(int chh, int loopn, int sppd, int rept, int direction, int blocking) {
  if ((direction < 0) || (direction > 1))
    quit("!AnimateCharacterEx: invalid direction");
  if (!is_valid_character(chh))
    quit("AnimateCharacter: invalid character");

  if (direction)
    direction = BACKWARDS;
  else
    direction = FORWARDS;

  if (blocking)
    blocking = BLOCKING;
  else
    blocking = IN_BACKGROUND;

  Character_Animate(&game.chars[chh], loopn, sppd, rept, blocking, direction);

}

void animate_character(CharacterInfo *chap, int loopn,int sppd,int rept, int noidleoverride, int direction) {

  if ((chap->view < 0) || (chap->view > game.numviews)) {
    quitprintf("!AnimateCharacter: you need to set the view number first\n"
      "(trying to animate '%s' using loop %d. View is currently %d).",chap->name,loopn,chap->view+1);
  }
  DEBUG_CONSOLE("%s: Start anim view %d loop %d, spd %d, repeat %d", chap->scrname, chap->view+1, loopn, sppd, rept);
  if ((chap->idleleft < 0) && (noidleoverride == 0)) {
    // if idle view in progress for the character (and this is not the
    // "start idle animation" animate_character call), stop the idle anim
    Character_UnlockView(chap);
    chap->idleleft=chap->idletime;
  }
  if ((loopn < 0) || (loopn >= views[chap->view].numLoops))
    quit("!AnimateCharacter: invalid loop number specified");
  Character_StopMoving(chap);
  chap->animating=1;
  if (rept) chap->animating |= CHANIM_REPEAT;
  if (direction) chap->animating |= CHANIM_BACKWARDS;

  chap->animating|=((sppd << 8) & 0xff00);
  chap->loop=loopn;
  
  if (direction) {
    chap->frame = views[chap->view].loops[loopn].numFrames - 1;
  }
  else
    chap->frame=0;

  chap->wait = sppd + views[chap->view].loops[loopn].frames[chap->frame].speed;
  CheckViewFrameForCharacter(chap);
}

void AnimateObjectEx(int obn,int loopn,int spdd,int rept, int direction, int blocking) {
  if (obn>=MANOBJNUM) {
    scAnimateCharacter(obn - 100,loopn,spdd,rept);
    return;
  }
  if (!is_valid_object(obn))
    quit("!AnimateObject: invalid object number specified");
  if (objs[obn].view < 0)
    quit("!AnimateObject: object has not been assigned a view");
  if (loopn >= views[objs[obn].view].numLoops)
    quit("!AnimateObject: invalid loop number specified");
  if ((direction < 0) || (direction > 1))
    quit("!AnimateObjectEx: invalid direction");
  if ((rept < 0) || (rept > 2))
    quit("!AnimateObjectEx: invalid repeat value");
  if (views[objs[obn].view].loops[loopn].numFrames < 1)
    quit("!AnimateObject: no frames in the specified view loop");

  DEBUG_CONSOLE("Obj %d start anim view %d loop %d, speed %d, repeat %d", obn, objs[obn].view+1, loopn, spdd, rept);

  objs[obn].cycling = rept+1 + (direction * 10);
  objs[obn].loop=loopn;
  if (direction == 0)
    objs[obn].frame = 0;
  else {
    objs[obn].frame = views[objs[obn].view].loops[loopn].numFrames - 1;
  }

  objs[obn].overall_speed=spdd;
  objs[obn].wait = spdd+views[objs[obn].view].loops[loopn].frames[objs[obn].frame].speed;
  objs[obn].num = views[objs[obn].view].loops[loopn].frames[objs[obn].frame].pic;
  CheckViewFrame (objs[obn].view, loopn, objs[obn].frame);

  if (blocking)
    do_main_cycle(UNTIL_CHARIS0,(int)&objs[obn].cycling);
}

void Object_Animate(ScriptObject *objj, int loop, int delay, int repeat, int blocking, int direction) {
  if (direction == FORWARDS)
    direction = 0;
  else if (direction == BACKWARDS)
    direction = 1;
  else
    quit("!Object.Animate: Invalid DIRECTION parameter");

  if ((blocking == BLOCKING) || (blocking == 1))
    blocking = 1;
  else if ((blocking == IN_BACKGROUND) || (blocking == 0))
    blocking = 0;
  else
    quit("!Object.Animate: Invalid BLOCKING parameter");

  AnimateObjectEx(objj->id, loop, delay, repeat, direction, blocking);
}

void Object_StopAnimating(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.StopAnimating: invalid object number");

  if (objs[objj->id].cycling) {
    objs[objj->id].cycling = 0;
    objs[objj->id].wait = 0;
  }
}

void AnimateObject(int obn,int loopn,int spdd,int rept) {
  AnimateObjectEx (obn, loopn, spdd, rept, 0, 0);
}

void MergeObject(int obn) {
  if (!is_valid_object(obn)) quit("!MergeObject: invalid object specified");
  int theHeight;

  construct_object_gfx(obn, NULL, &theHeight, true);

  block oldabuf = abuf;
  abuf = thisroom.ebscene[play.bg_frame];
  if (bitmap_color_depth(abuf) != bitmap_color_depth(actsps[obn]))
    quit("!MergeObject: unable to merge object due to color depth differences");

  int xpos = multiply_up_coordinate(objs[obn].x);
  int ypos = (multiply_up_coordinate(objs[obn].y) - theHeight);

  draw_sprite_support_alpha(xpos, ypos, actsps[obn], objs[obn].num);
  invalidate_screen();
  mark_current_background_dirty();

  abuf = oldabuf;
  // mark the sprite as merged
  objs[obn].on = 2;
  DEBUG_CONSOLE("Object %d merged into background", obn);
}

void Object_MergeIntoBackground(ScriptObject *objj) {
  MergeObject(objj->id);
}

void StopObjectMoving(int objj) {
  if (!is_valid_object(objj))
    quit("!StopObjectMoving: invalid object number");
  objs[objj].moving = 0;

  DEBUG_CONSOLE("Object %d stop moving", objj);
}

void Object_StopMoving(ScriptObject *objj) {
  StopObjectMoving(objj->id);
}

void ObjectOff(int obn) {
  if (!is_valid_object(obn)) quit("!ObjectOff: invalid object specified");
  // don't change it if on == 2 (merged)
  if (objs[obn].on == 1) {
    objs[obn].on = 0;
    DEBUG_CONSOLE("Object %d turned off", obn);
    StopObjectMoving(obn);
  }
}

void ObjectOn(int obn) {
  if (!is_valid_object(obn)) quit("!ObjectOn: invalid object specified");
  if (objs[obn].on == 0) {
    objs[obn].on = 1;
    DEBUG_CONSOLE("Object %d turned on", obn);
  }
}

void Object_SetVisible(ScriptObject *objj, int onoroff) {
  if (onoroff)
    ObjectOn(objj->id);
  else
    ObjectOff(objj->id);
}

int IsObjectOn (int objj) {
  if (!is_valid_object(objj)) quit("!IsObjectOn: invalid object number");
  
  // ==1 is on, ==2 is merged into background
  if (objs[objj].on == 1)
    return 1;

  return 0;
}

int Object_GetView(ScriptObject *objj) {
  if (objj->obj->view < 0)
    return 0;
  return objj->obj->view + 1;
}

int Object_GetLoop(ScriptObject *objj) {
  if (objj->obj->view < 0)
    return 0;
  return objj->obj->loop;
}

int Object_GetFrame(ScriptObject *objj) {
  if (objj->obj->view < 0)
    return 0;
  return objj->obj->frame;
}

int Object_GetVisible(ScriptObject *objj) {
  return IsObjectOn(objj->id);
}

void SetObjectGraphic(int obn,int slott) {
  if (!is_valid_object(obn)) quit("!SetObjectGraphic: invalid object specified");

  if (objs[obn].num != slott) {
    objs[obn].num = slott;
    DEBUG_CONSOLE("Object %d graphic changed to slot %d", obn, slott);
  }
  objs[obn].cycling=0;
  objs[obn].frame = 0;
  objs[obn].loop = 0;
  objs[obn].view = -1;
}

void Object_SetGraphic(ScriptObject *objj, int slott) {
  SetObjectGraphic(objj->id, slott);
}

int GetObjectGraphic(int obn) {
  if (!is_valid_object(obn)) quit("!GetObjectGraphic: invalid object specified");
  return objs[obn].num;
}

int Object_GetGraphic(ScriptObject *objj) {
  return GetObjectGraphic(objj->id);
}

#define OVERLAPPING_OBJECT 1000
struct Rect {
  int x1,y1,x2,y2;
};

int GetThingRect(int thing, Rect *rect) {
  if (is_valid_character(thing)) {
    if (game.chars[thing].room != displayed_room)
      return 0;
    
    int charwid = divide_down_coordinate(GetCharacterWidth(thing));
    rect->x1 = game.chars[thing].x - (charwid / 2);
    rect->x2 = rect->x1 + charwid;
    rect->y1 = game.chars[thing].get_effective_y() - divide_down_coordinate(GetCharacterHeight(thing));
    rect->y2 = game.chars[thing].get_effective_y();
  }
  else if (is_valid_object(thing - OVERLAPPING_OBJECT)) {
    int objid = thing - OVERLAPPING_OBJECT;
    if (objs[objid].on != 1)
      return 0;
    rect->x1 = objs[objid].x;
    rect->x2 = objs[objid].x + divide_down_coordinate(objs[objid].get_width());
    rect->y1 = objs[objid].y - divide_down_coordinate(objs[objid].get_height());
    rect->y2 = objs[objid].y;
  }
  else
    quit("!AreThingsOverlapping: invalid parameter");

  return 1;
}

int AreThingsOverlapping(int thing1, int thing2) {
  Rect r1, r2;
  // get the bounding rectangles, and return 0 if the object/char
  // is currently turned off
  if (GetThingRect(thing1, &r1) == 0)
    return 0;
  if (GetThingRect(thing2, &r2) == 0)
    return 0;

  if ((r1.x2 > r2.x1) && (r1.x1 < r2.x2) &&
      (r1.y2 > r2.y1) && (r1.y1 < r2.y2)) {
    // determine how far apart they are
    // take the smaller of the X distances as the overlapping amount
    int xdist = abs(r1.x2 - r2.x1);
    if (abs(r1.x1 - r2.x2) < xdist)
      xdist = abs(r1.x1 - r2.x2);
    // take the smaller of the Y distances
    int ydist = abs(r1.y2 - r2.y1);
    if (abs(r1.y1 - r2.y2) < ydist)
      ydist = abs(r1.y1 - r2.y2);
    // the overlapping amount is the smaller of the X and Y ovrlap
    if (xdist < ydist)
      return xdist;
    else
      return ydist;
//    return 1;
  }
  return 0;
}

int AreObjectsColliding(int obj1,int obj2) {
  if ((!is_valid_object(obj1)) | (!is_valid_object(obj2)))
    quit("!AreObjectsColliding: invalid object specified");

  return (AreThingsOverlapping(obj1 + OVERLAPPING_OBJECT, obj2 + OVERLAPPING_OBJECT)) ? 1 : 0;
}

int Object_IsCollidingWithObject(ScriptObject *objj, ScriptObject *obj2) {
  return AreObjectsColliding(objj->id, obj2->id);
}

int my_getpixel(BITMAP *blk, int x, int y) {
  if ((x < 0) || (y < 0) || (x >= blk->w) || (y >= blk->h))
    return -1;

  // strip the alpha channel
  return blk->vtable->getpixel(blk, x, y) & 0x00ffffff;
}

block GetCharacterImage(int charid, int *isFlipped) 
{
  if (!gfxDriver->HasAcceleratedStretchAndFlip())
  {
    if (actsps[charid + MAX_INIT_SPR] != NULL) 
    {
      // the actsps image is pre-flipped, so no longer register the image as such
      if (isFlipped)
        *isFlipped = 0;
      return actsps[charid + MAX_INIT_SPR];
    }
  }
  CharacterInfo*chin=&game.chars[charid];
  int sppic = views[chin->view].loops[chin->loop].frames[chin->frame].pic;
  return spriteset[sppic];
}

block GetObjectImage(int obj, int *isFlipped) 
{
  if (!gfxDriver->HasAcceleratedStretchAndFlip())
  {
    if (actsps[obj] != NULL) {
      // the actsps image is pre-flipped, so no longer register the image as such
      if (isFlipped)
        *isFlipped = 0;

      return actsps[obj];
    }
  }
  return spriteset[objs[obj].num];
}

int AreCharObjColliding(int charid,int objid) {
  if (!is_valid_character(charid))
    quit("!AreCharObjColliding: invalid character");
  if (!is_valid_object(objid))
    quit("!AreCharObjColliding: invalid object number");

  return Character_IsCollidingWithObject(&game.chars[charid], &scrObj[objid]);
}

int AreCharactersColliding(int cchar1,int cchar2) {
  if (!is_valid_character(cchar1))
    quit("!AreCharactersColliding: invalid char1");
  if (!is_valid_character(cchar2))
    quit("!AreCharactersColliding: invalid char2");

  return Character_IsCollidingWithChar(&game.chars[cchar1], &game.chars[cchar2]);
}

void scrWait(int nloops) {
  if (nloops < 1)
    quit("!Wait: must wait at least 1 loop");

  play.wait_counter = nloops;
  play.key_skip_wait = 0;
  do_main_cycle(UNTIL_MOVEEND,(int)&play.wait_counter);
  }

int WaitKey(int nloops) {
  if (nloops < 1)
    quit("!WaitKey: must wait at least 1 loop");

  play.wait_counter = nloops;
  play.key_skip_wait = 1;
  do_main_cycle(UNTIL_MOVEEND,(int)&play.wait_counter);
  if (play.wait_counter < 0)
    return 1;
  return 0;
}

int WaitMouseKey(int nloops) {
  if (nloops < 1)
    quit("!WaitMouseKey: must wait at least 1 loop");

  play.wait_counter = nloops;
  play.key_skip_wait = 3;
  do_main_cycle(UNTIL_MOVEEND,(int)&play.wait_counter);
  if (play.wait_counter < 0)
    return 1;
  return 0;
}

// unfortunately MSVC and GCC automatically push floats as doubles
// to functions, thus we need to manually access it as 32-bit
#define SCRIPT_FLOAT(x) long __script_float##x
#define INIT_SCRIPT_FLOAT(x) float x; memcpy(&x, &__script_float##x, sizeof(float))
#define FLOAT_RETURN_TYPE long
#define RETURN_FLOAT(x) long __ret##x; memcpy(&__ret##x, &x, sizeof(float)); return __ret##x

enum RoundDirections {
  eRoundDown = 0,
  eRoundNearest = 1,
  eRoundUp = 2
};

int FloatToInt(SCRIPT_FLOAT(value), int roundDirection) {
  INIT_SCRIPT_FLOAT(value);

  int intval;

  if (value >= 0.0) {
    if (roundDirection == eRoundDown)
      intval = (int)value;
    else if (roundDirection == eRoundNearest)
      intval = (int)(value + 0.5);
    else if (roundDirection == eRoundUp)
      intval = (int)(value + 0.999999);
    else
      quit("!FloatToInt: invalid round direction");
  }
  else {
    // negative number
    if (roundDirection == eRoundUp)
      intval = (int)value; // this just truncates
    else if (roundDirection == eRoundNearest)
      intval = (int)(value - 0.5);
    else if (roundDirection == eRoundDown)
      intval = (int)(value - 0.999999);
    else
      quit("!FloatToInt: invalid round direction");
  }

  return intval;
}

FLOAT_RETURN_TYPE IntToFloat(int value) {
  float fval = value;

  RETURN_FLOAT(fval);
}

FLOAT_RETURN_TYPE StringToFloat(const char *theString) {
  float fval = atof(theString);

  RETURN_FLOAT(fval);
}

FLOAT_RETURN_TYPE Math_Cos(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = ::cos(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Sin(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = ::sin(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Tan(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = ::tan(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcCos(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = ::acos(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcSin(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = ::asin(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcTan(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = ::atan(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcTan2(SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval)) {
  INIT_SCRIPT_FLOAT(yval);
  INIT_SCRIPT_FLOAT(xval);

  float value = ::atan2(yval, xval);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Log(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::log(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Log10(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::log10(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Exp(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::exp(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Cosh(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::cosh(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Sinh(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::sinh(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Tanh(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::tanh(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_RaiseToPower(SCRIPT_FLOAT(base), SCRIPT_FLOAT(exp)) {
  INIT_SCRIPT_FLOAT(base);
  INIT_SCRIPT_FLOAT(exp);

  float value = ::pow(base, exp);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_DegreesToRadians(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = value * (M_PI / 180.0);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_RadiansToDegrees(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = value * (180.0 / M_PI);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_GetPi() {
  float pi = M_PI;

  RETURN_FLOAT(pi);
}

FLOAT_RETURN_TYPE Math_Sqrt(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  if (value < 0.0)
    quit("!Sqrt: cannot perform square root of negative number");

  value = ::sqrt(value);

  RETURN_FLOAT(value);
}


int StringToInt(char*stino) {
  return atoi(stino);
  }

int StrGetCharAt (char *strin, int posn) {
  if ((posn < 0) || (posn >= (int)strlen(strin)))
    return 0;
  return strin[posn];
}

void StrSetCharAt (char *strin, int posn, int nchar) {
  if ((posn < 0) || (posn > (int)strlen(strin)) || (posn >= MAX_MAXSTRLEN))
    quit("!StrSetCharAt: tried to write past end of string");

  if (posn == (int)strlen(strin))
    strin[posn+1] = 0;
  strin[posn] = nchar;
}

ScriptDrawingSurface* Room_GetDrawingSurfaceForBackground(int backgroundNumber)
{
  if (displayed_room < 0)
    quit("!Room.GetDrawingSurfaceForBackground: no room is currently loaded");

  if (backgroundNumber == SCR_NO_VALUE)
  {
    backgroundNumber = play.bg_frame;
  }

  if ((backgroundNumber < 0) || (backgroundNumber >= thisroom.num_bscenes))
    quit("!Room.GetDrawingSurfaceForBackground: invalid background number specified");


  ScriptDrawingSurface *surface = new ScriptDrawingSurface();
  surface->roomBackgroundNumber = backgroundNumber;
  ccRegisterManagedObject(surface, surface);
  return surface;
}

ScriptDateTime* DateTime_Now_Core() {
  ScriptDateTime *sdt = new ScriptDateTime();
  sdt->rawUnixTime = time(NULL);

  platform->GetSystemTime(sdt);

  return sdt;
}

ScriptDateTime* DateTime_Now() {
  ScriptDateTime *sdt = DateTime_Now_Core();
  ccRegisterManagedObject(sdt, sdt);
  return sdt;
}

int DateTime_GetYear(ScriptDateTime *sdt) {
  return sdt->year;
}

int DateTime_GetMonth(ScriptDateTime *sdt) {
  return sdt->month;
}

int DateTime_GetDayOfMonth(ScriptDateTime *sdt) {
  return sdt->day;
}

int DateTime_GetHour(ScriptDateTime *sdt) {
  return sdt->hour;
}

int DateTime_GetMinute(ScriptDateTime *sdt) {
  return sdt->minute;
}

int DateTime_GetSecond(ScriptDateTime *sdt) {
  return sdt->second;
}

int DateTime_GetRawTime(ScriptDateTime *sdt) {
  return sdt->rawUnixTime;
}

int sc_GetTime(int whatti) {
  ScriptDateTime *sdt = DateTime_Now_Core();
  int returnVal;

  if (whatti == 1) returnVal = sdt->hour;
  else if (whatti == 2) returnVal = sdt->minute;
  else if (whatti == 3) returnVal = sdt->second;
  else if (whatti == 4) returnVal = sdt->day;
  else if (whatti == 5) returnVal = sdt->month;
  else if (whatti == 6) returnVal = sdt->year;
  else quit("!GetTime: invalid parameter passed");

  delete sdt;

  return returnVal;
}

int GetRawTime () {
  return time(NULL);
}

char gamefilenamebuf[200];
#define RAGMODE_PRESERVEGLOBALINT 1
#define RAGMODE_LOADNOW 0x8000000  // just to make sure it's non-zero
int RunAGSGame (char *newgame, unsigned int mode, int data) {

  can_run_delayed_command();

  int AllowedModes = RAGMODE_PRESERVEGLOBALINT | RAGMODE_LOADNOW;

  if ((mode & (~AllowedModes)) != 0)
    quit("!RunAGSGame: mode value unknown");

  if (use_compiled_folder_as_current_dir || editor_debugging_enabled)
  {
    quit("!RunAGSGame cannot be used while running the game from within the AGS Editor. You must build the game EXE and run it from there to use this function.");
  }

  if ((mode & RAGMODE_LOADNOW) == 0) {
    // need to copy, since the script gets destroyed
    get_current_dir_path(gamefilenamebuf, newgame);
    game_file_name = &gamefilenamebuf[0];
    usetup.main_data_filename = game_file_name;
    play.takeover_data = data;
    load_new_game_restore = -1;

    if (inside_script) {
      curscript->queue_action(ePSARunAGSGame, mode | RAGMODE_LOADNOW, "RunAGSGame");
      ccAbortInstance (ccGetCurrentInstance ());
    }
    else
      load_new_game = mode | RAGMODE_LOADNOW;

    return 0;
  }

  int result, ee;

  unload_old_room();
  displayed_room = -10;

  unload_game_file();

  if (csetlib(game_file_name,""))
    quitprintf("!RunAGSGame: unable to load new game file '%s'", game_file_name);

  clear(abuf);
  show_preload();

  if ((result = load_game_file ()) != 0) {
    quitprintf("!RunAGSGame: error %d loading new game file", result);
  }

  spriteset.reset();
  if (spriteset.initFile ("acsprset.spr"))
    quit("!RunAGSGame: error loading new sprites");

  if ((mode & RAGMODE_PRESERVEGLOBALINT) == 0) {
    // reset GlobalInts
    for (ee = 0; ee < MAXGSVALUES; ee++)
      play.globalscriptvars[ee] = 0;  
  }

  init_game_settings();
  play.screen_is_faded_out = 1;

  if (load_new_game_restore >= 0) {
    load_game (load_new_game_restore, NULL, NULL);
    load_new_game_restore = -1;
  }
  else
    start_game();

  return 0;
}

static void display_switch_out() {
  // this is only called if in SWITCH_PAUSE mode
  //debug_log("display_switch_out");

  switching_away_from_game++;

  platform->DisplaySwitchOut();

  // allow background running temporarily to halt the sound
  if (set_display_switch_mode(SWITCH_BACKGROUND) == -1)
    set_display_switch_mode(SWITCH_BACKAMNESIA);

  // stop the sound stuttering
  for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) {
    if ((channels[i] != NULL) && (channels[i]->done == 0)) {
      channels[i]->pause();
    }
  }

  rest(1000);

  // restore the callbacks
  SetMultitasking(0);

  switching_away_from_game--;
}

static void display_switch_in() {
  for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) {
    if ((channels[i] != NULL) && (channels[i]->done == 0)) {
      channels[i]->resume();
    }
  }

  if (gfxDriver->UsesMemoryBackBuffer())  // make sure all borders are cleared
    gfxDriver->ClearRectangle(0, 0, final_scrn_wid - 1, final_scrn_hit - 1, NULL);

  platform->DisplaySwitchIn();
}

void SetMultitasking (int mode) {
  if ((mode < 0) | (mode > 1))
    quit("!SetMultitasking: invalid mode parameter");

  // Don't allow background running if full screen
  if ((mode == 1) && (usetup.windowed == 0))
    mode = 0;

  if (mode == 0) {
    if (set_display_switch_mode(SWITCH_PAUSE) == -1)
      set_display_switch_mode(SWITCH_AMNESIA);
    // install callbacks to stop the sound when switching away
    set_display_switch_callback(SWITCH_IN, display_switch_in);
    set_display_switch_callback(SWITCH_OUT, display_switch_out);
  }
  else {
    if (set_display_switch_mode (SWITCH_BACKGROUND) == -1)
      set_display_switch_mode(SWITCH_BACKAMNESIA);
  }
}


// FLIC player start
block fli_buffer;
short fliwidth,fliheight;
int canabort=0, stretch_flc = 1;
block hicol_buf=NULL;
IDriverDependantBitmap *fli_ddb;
BITMAP *fli_target;
int fliTargetWidth, fliTargetHeight;
int check_if_user_input_should_cancel_video()
{
  NEXT_ITERATION();
  if (kbhit()) {
    if ((getch()==27) && (canabort==1))
      return 1;
    if (canabort >= 2)
      return 1;  // skip on any key
  }
  if (canabort == 3) {  // skip on mouse click
    if (mgetbutton()!=NONE) return 1;
  }
  return 0;
}

#if defined(WINDOWS_VERSION)
int __cdecl fli_callback() {
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" int fli_callback() {
#else
int fli_callback(...) {
#endif
  block usebuf = fli_buffer;

  update_polled_stuff_and_crossfade ();

  if (game.color_depth > 1) {
    blit(fli_buffer,hicol_buf,0,0,0,0,fliwidth,fliheight);
    usebuf=hicol_buf;
  }
  if (stretch_flc == 0)
    blit(usebuf, fli_target, 0,0,scrnwid/2-fliwidth/2,scrnhit/2-fliheight/2,scrnwid,scrnhit);
  else 
    stretch_blit(usebuf, fli_target, 0,0,fliwidth,fliheight,0,0,scrnwid,scrnhit);

  gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
  gfxDriver->DrawSprite(0, 0, fli_ddb);
  render_to_screen(fli_target, 0, 0);

  return check_if_user_input_should_cancel_video();
}

void play_flc_file(int numb,int playflags) {
  color oldpal[256];

  if (play.fast_forward)
    return;

  wreadpalette(0,255,oldpal);

  int clearScreenAtStart = 1;
  canabort = playflags % 10;
  playflags -= canabort;

  if (canabort == 2) // convert to PlayVideo-compatible setting
    canabort = 3;

  if (playflags % 100 == 0)
    stretch_flc = 1;
  else
    stretch_flc = 0;

  if (playflags / 100)
    clearScreenAtStart = 0;

  char flicnam[20]; sprintf(flicnam,"flic%d.flc",numb);
  FILE*iii=clibfopen(flicnam,"rb");
  if (iii==NULL) { sprintf(flicnam,"flic%d.fli",numb);
    iii=clibfopen(flicnam,"rb"); }
  if (iii==NULL) {
    debug_log("FLIC animation FLIC%d.FLC not found",numb);
    return;
    }
  fseek(iii,8,SEEK_CUR);
  fread(&fliwidth,2,1,iii);
  fread(&fliheight,2,1,iii);
  fclose(iii);
  if (game.color_depth > 1) {
    hicol_buf=create_bitmap_ex(final_col_dep,fliwidth,fliheight);
    clear(hicol_buf);
    }
  // override the stretch option if necessary
  if ((fliwidth==scrnwid) && (fliheight==scrnhit))
    stretch_flc = 0;
  else if ((fliwidth > scrnwid) || (fliheight > scrnhit))
    stretch_flc = 1;
  fli_buffer=create_bitmap_ex(8,fliwidth,fliheight); //640,400); //scrnwid,scrnhit);
  if (fli_buffer==NULL) quit("Not enough memory to play animation");
  clear(fli_buffer);

  if (clearScreenAtStart) {
    clear(screen);
    render_to_screen(screen, 0, 0);
  }

  fli_target = create_bitmap_ex(final_col_dep, screen->w, screen->h);
  fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);

  if (play_fli(flicnam,fli_buffer,0,fli_callback)==FLI_ERROR)
    quit("FLI/FLC animation play error");

  wfreeblock(fli_buffer);
  clear(screen);
  wsetpalette(0,255,oldpal);
  render_to_screen(screen, 0, 0);

  destroy_bitmap(fli_target);
  gfxDriver->DestroyDDB(fli_ddb);
  fli_ddb = NULL;

  if (hicol_buf!=NULL) {
    wfreeblock(hicol_buf);
    hicol_buf=NULL; }
//  wsetscreen(screen); wputblock(0,0,backbuffer,0);
  while (mgetbutton()!=NONE) ;
  invalidate_screen();
}
// FLIC player end

int theora_playing_callback(BITMAP *theoraBuffer)
{
  if (theoraBuffer == NULL)
  {
    // No video, only sound
    return check_if_user_input_should_cancel_video();
  }

  int drawAtX = 0, drawAtY = 0;
  if (fli_ddb == NULL)
  {
    fli_ddb = gfxDriver->CreateDDBFromBitmap(theoraBuffer, false, true);
  }
  if (stretch_flc) 
  {
    drawAtX = scrnwid / 2 - fliTargetWidth / 2;
    drawAtY = scrnhit / 2 - fliTargetHeight / 2;
    if (!gfxDriver->HasAcceleratedStretchAndFlip())
    {
      stretch_blit(theoraBuffer, fli_target, 0, 0, theoraBuffer->w, theoraBuffer->h, 
                   drawAtX, drawAtY, fliTargetWidth, fliTargetHeight);
      gfxDriver->UpdateDDBFromBitmap(fli_ddb, fli_target, false);
      drawAtX = 0;
      drawAtY = 0;
    }
    else
    {
      gfxDriver->UpdateDDBFromBitmap(fli_ddb, theoraBuffer, false);
      fli_ddb->SetStretch(fliTargetWidth, fliTargetHeight);
    }
  }
  else
  {
    gfxDriver->UpdateDDBFromBitmap(fli_ddb, theoraBuffer, false);
    drawAtX = scrnwid / 2 - theoraBuffer->w / 2;
    drawAtY = scrnhit / 2 - theoraBuffer->h / 2;
  }

  gfxDriver->DrawSprite(drawAtX, drawAtY, fli_ddb);
  render_to_screen(virtual_screen, 0, 0);
  update_polled_stuff_and_crossfade ();

  return check_if_user_input_should_cancel_video();
}

APEG_STREAM* get_theora_size(const char *fileName, int *width, int *height)
{
  APEG_STREAM* oggVid = apeg_open_stream(fileName);
  if (oggVid != NULL)
  {
    apeg_get_video_size(oggVid, width, height);
  }
  else
  {
    *width = 0;
    *height = 0;
  }
  return oggVid;
}

void calculate_destination_size_maintain_aspect_ratio(int vidWidth, int vidHeight, int *targetWidth, int *targetHeight)
{
  float aspectRatioVideo = (float)vidWidth / (float)vidHeight;
  float aspectRatioScreen = (float)scrnwid / (float)scrnhit;

  if (aspectRatioVideo == aspectRatioScreen)
  {
	  *targetWidth = scrnwid;
	  *targetHeight = scrnhit;
  }
  else if (aspectRatioVideo > aspectRatioScreen)
  {
    *targetWidth = scrnwid;
    *targetHeight = (int)(((float)scrnwid / aspectRatioVideo) + 0.5f);
  }
  else
  {
    *targetHeight = scrnhit;
    *targetWidth = (float)scrnhit * aspectRatioVideo;
  }

}

void play_theora_video(const char *name, int skip, int flags)
{
  apeg_set_display_depth(bitmap_color_depth(screen));
  // we must disable length detection, otherwise it takes ages to start
  // playing if the file is large because it seeks through the whole thing
  apeg_disable_length_detection(TRUE);
  apeg_enable_framedrop(TRUE);
  update_polled_stuff();

  stretch_flc = (flags % 10);
  canabort = skip;
  apeg_ignore_audio((flags >= 10) ? 1 : 0);

  int videoWidth, videoHeight;
  APEG_STREAM *oggVid = get_theora_size(name, &videoWidth, &videoHeight);

  if (videoWidth == 0)
  {
    Display("Unable to load theora video '%s'", name);
    return;
  }

  if (flags < 10)
  {
    stop_all_sound_and_music();
  }

  fli_target = NULL;
  //fli_buffer = create_bitmap_ex(final_col_dep, videoWidth, videoHeight);
  calculate_destination_size_maintain_aspect_ratio(videoWidth, videoHeight, &fliTargetWidth, &fliTargetHeight);

  if ((fliTargetWidth == videoWidth) && (fliTargetHeight == videoHeight) && (stretch_flc))
  {
    // don't need to stretch after all
    stretch_flc = 0;
  }

  if ((stretch_flc) && (!gfxDriver->HasAcceleratedStretchAndFlip()))
  {
    fli_target = create_bitmap_ex(final_col_dep, scrnwid, scrnhit);
    clear(fli_target);
    fli_ddb = gfxDriver->CreateDDBFromBitmap(fli_target, false, true);
  }
  else
  {
    fli_ddb = NULL;
  }

  update_polled_stuff();

  clear(virtual_screen);

  if (apeg_play_apeg_stream(oggVid, NULL, 0, theora_playing_callback) == APEG_ERROR)
  {
    Display("Error playing theora video '%s'", name);
  }
  apeg_close_stream(oggVid);

  //destroy_bitmap(fli_buffer);
  if (fli_target != NULL)
    destroy_bitmap(fli_target);
  gfxDriver->DestroyDDB(fli_ddb);
  fli_ddb = NULL;
  invalidate_screen();
}

void pause_sound_if_necessary_and_play_video(const char *name, int skip, int flags)
{
  int musplaying = play.cur_music_number, i;
  int ambientWas[MAX_SOUND_CHANNELS];
  for (i = 1; i < MAX_SOUND_CHANNELS; i++)
    ambientWas[i] = ambient[i].channel;

  if ((strlen(name) > 3) && (stricmp(&name[strlen(name) - 3], "ogv") == 0))
  {
    play_theora_video(name, skip, flags);
  }
  else
  {
    char videoFilePath[MAX_PATH];
    get_current_dir_path(videoFilePath, name);

    platform->PlayVideo(videoFilePath, skip, flags);
  }

  if (flags < 10) 
  {
    update_music_volume();
    // restart the music
    if (musplaying >= 0)
      newmusic (musplaying);
    for (i = 1; i < MAX_SOUND_CHANNELS; i++) {
      if (ambientWas[i] > 0)
        PlayAmbientSound(ambientWas[i], ambient[i].num, ambient[i].vol, ambient[i].x, ambient[i].y);
    }
  }
}

void scrPlayVideo(const char* name, int skip, int flags) {
  EndSkippingUntilCharStops();

  if (play.fast_forward)
    return;
  if (debug_flags & DBG_NOVIDEO)
    return;

  if ((flags < 10) && (usetup.digicard == DIGI_NONE)) {
    // if game audio is disabled in Setup, then don't
    // play any sound on the video either
    flags += 10;
  }

  pause_sound_if_necessary_and_play_video(name, skip, flags);
}

// returns -1 on failure, channel number on success
int PlaySoundEx(int val1, int channel) {

  if (debug_flags & DBG_NOSFX)
    return -1;

  // if no sound, ignore it
  if (usetup.digicard == DIGI_NONE)
    return -1;

  if ((channel < SCHAN_NORMAL) || (channel >= MAX_SOUND_CHANNELS))
    quit("!PlaySoundEx: invalid channel specified, must be 3-7");

  // if an ambient sound is playing on this channel, abort it
  StopAmbientSound(channel);

  if (val1 < 0) {
    stop_and_destroy_channel (channel);
    return -1;
  }
  // if skipping a cutscene, don't try and play the sound
  if (play.fast_forward)
    return -1;
  
  // that sound is already in memory, play it
  if ((last_sound_played[channel] == val1) && (channels[channel] != NULL)) {
    DEBUG_CONSOLE("Playing sound %d on channel %d; cached", val1, channel);
    channels[channel]->restart();
    channels[channel]->set_volume (play.sound_volume);
    return channel;
  }
  // free the old sound
  stop_and_destroy_channel (channel);
  DEBUG_CONSOLE("Playing sound %d on channel %d", val1, channel);

  last_sound_played[channel] = val1;

  SOUNDCLIP *soundfx = load_sound_from_path(val1, play.sound_volume, 0);

  if (soundfx == NULL) {
    debug_log("Sound sample load failure: cannot load sound %d", val1);
    DEBUG_CONSOLE("FAILED to load sound %d", val1);
    return -1;
  }

  channels[channel] = soundfx;
  channels[channel]->priority = 10;
  channels[channel]->set_volume (play.sound_volume);
  return channel;
}

void StopAllSounds(int evenAmbient) {
  // backwards-compatible hack -- stop Type 3 (default Sound Type)
  Game_StopAudio(3);

  if (evenAmbient)
    Game_StopAudio(1);
}

// the sound will only be played if there is a free channel or
// it has a priority >= an existing sound to override
int play_sound_priority (int val1, int priority) {
  int lowest_pri = 9999, lowest_pri_id = -1;

  // find a free channel to play it on
  for (int i = SCHAN_NORMAL; i < numSoundChannels; i++) {
    if (val1 < 0) {
      // Playing sound -1 means iterate through and stop all sound
      if ((channels[i] != NULL) && (channels[i]->done == 0))
        stop_and_destroy_channel (i);
    }
    else if ((channels[i] == NULL) || (channels[i]->done != 0)) {
      if (PlaySoundEx(val1, i) >= 0)
        channels[i]->priority = priority;
      return i;
    }
    else if (channels[i]->priority < lowest_pri) {
      lowest_pri = channels[i]->priority;
      lowest_pri_id = i;
    }
      
  }
  if (val1 < 0)
    return -1;

  // no free channels, see if we have a high enough priority
  // to override one
  if (priority >= lowest_pri) {
    if (PlaySoundEx(val1, lowest_pri_id) >= 0) {
      channels[lowest_pri_id]->priority = priority;
      return lowest_pri_id;
    }
  }

  return -1;
}

int play_sound(int val1) {
  return play_sound_priority(val1, 10);
}

void add_inventory(int inum) {
  if ((inum < 0) || (inum >= MAX_INV))
    quit("!AddInventory: invalid inventory number");

  Character_AddInventory(playerchar, &scrInv[inum], SCR_NO_VALUE);

  play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
}

void lose_inventory(int inum) {
  if ((inum < 0) || (inum >= MAX_INV))
    quit("!LoseInventory: invalid inventory number");

  Character_LoseInventory(playerchar, &scrInv[inum]);

  play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
}

void AddInventoryToCharacter(int charid, int inum) {
  if (!is_valid_character(charid))
    quit("!AddInventoryToCharacter: invalid character specified");
  if ((inum < 1) || (inum >= game.numinvitems))
    quit("!AddInventory: invalid inv item specified");

  Character_AddInventory(&game.chars[charid], &scrInv[inum], SCR_NO_VALUE);
}

void LoseInventoryFromCharacter(int charid, int inum) {
  if (!is_valid_character(charid))
    quit("!LoseInventoryFromCharacter: invalid character specified");
  if ((inum < 1) || (inum >= game.numinvitems))
    quit("!AddInventory: invalid inv item specified");
  
  Character_LoseInventory(&game.chars[charid], &scrInv[inum]);
}

void RunDialog(int tum) {
  if ((tum<0) | (tum>=game.numdialog))
    quit("!RunDialog: invalid topic number specified");

  can_run_delayed_command();

  if (play.stop_dialog_at_end != DIALOG_NONE) {
    if (play.stop_dialog_at_end == DIALOG_RUNNING)
      play.stop_dialog_at_end = DIALOG_NEWTOPIC + tum;
    else
      quit("!NewRoom: two NewRoom/RunDiaolg/StopDialog requests within dialog");
    return;
  }

  if (inside_script) 
    curscript->queue_action(ePSARunDialog, tum, "RunDialog");
  else
    do_conversation(tum);
}

int GetGUIAt (int xx,int yy) {
  multiply_up_coordinates(&xx, &yy);
  
  int aa, ll;
  for (ll = game.numgui - 1; ll >= 0; ll--) {
    aa = play.gui_draw_order[ll];
    if (guis[aa].on<1) continue;
    if (guis[aa].flags & GUIF_NOCLICK) continue;
    if ((xx>=guis[aa].x) & (yy>=guis[aa].y) &
      (xx<=guis[aa].x+guis[aa].wid) & (yy<=guis[aa].y+guis[aa].hit))
      return aa;
  }
  return -1;
}

ScriptGUI *GetGUIAtLocation(int xx, int yy) {
  int guiid = GetGUIAt(xx, yy);
  if (guiid < 0)
    return NULL;
  return &scrGui[guiid];
}

GUIObject *GetGUIControlAtLocation(int xx, int yy) {
  int guinum = GetGUIAt(xx, yy);
  if (guinum == -1)
    return NULL;

  multiply_up_coordinates(&xx, &yy);

  int oldmousex = mousex, oldmousey = mousey;
  mousex = xx - guis[guinum].x;
  mousey = yy - guis[guinum].y;
  int toret = guis[guinum].find_object_under_mouse(0, false);
  mousex = oldmousex;
  mousey = oldmousey;
  if (toret < 0)
    return NULL;

  return guis[guinum].objs[toret];
}

int GetGUIObjectAt (int xx, int yy) {
  GUIObject *toret = GetGUIControlAtLocation(xx, yy);
  if (toret == NULL)
    return -1;

  return toret->objn;
}


int IsGUIOn (int guinum) {
  if ((guinum < 0) || (guinum >= game.numgui))
    quit("!IsGUIOn: invalid GUI number specified");
  return (guis[guinum].on >= 1) ? 1 : 0;
}

// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int FindGUIID (const char* GUIName) {
  for (int ii = 0; ii < game.numgui; ii++) {
    if (strcmp(guis[ii].name, GUIName) == 0)
      return ii;
    if ((guis[ii].name[0] == 'g') && (stricmp(&guis[ii].name[1], GUIName) == 0))
      return ii;
  }
  quit("FindGUIID: No matching GUI found: GUI may have been deleted");
  return -1;
}

int isposinbox(int mmx,int mmy,int lf,int tp,int rt,int bt) {
  if ((mmx>=lf) & (mmx<=rt) & (mmy>=tp) & (mmy<=bt)) return TRUE;
  else return FALSE;
  }

// xx,yy is the position in room co-ordinates that we are checking
// arx,ary is the sprite x/y co-ordinates
int is_pos_in_sprite(int xx,int yy,int arx,int ary, block sprit, int spww,int sphh, int flipped = 0) {
  if (spww==0) spww = divide_down_coordinate(sprit->w) - 1;
  if (sphh==0) sphh = divide_down_coordinate(sprit->h) - 1;

  if (isposinbox(xx,yy,arx,ary,arx+spww,ary+sphh)==FALSE)
    return FALSE;

  if (game.options[OPT_PIXPERFECT]) 
  {
    // if it's transparent, or off the edge of the sprite, ignore
    int xpos = multiply_up_coordinate(xx - arx);
    int ypos = multiply_up_coordinate(yy - ary);

    if (gfxDriver->HasAcceleratedStretchAndFlip())
    {
      // hardware acceleration, so the sprite in memory will not have
      // been stretched, it will be original size. Thus, adjust our
      // calculations to compensate
      multiply_up_coordinates(&spww, &sphh);

      if (spww != sprit->w)
        xpos = (xpos * sprit->w) / spww;
      if (sphh != sprit->h)
        ypos = (ypos * sprit->h) / sphh;
    }

    if (flipped)
      xpos = (sprit->w - 1) - xpos;

    int gpcol = my_getpixel(sprit, xpos, ypos);

    if ((gpcol == bitmap_mask_color(sprit)) || (gpcol == -1))
      return FALSE;
  }
  return TRUE;
}

// Used for deciding whether a char or obj was closer
int char_lowest_yp, obj_lowest_yp;

int GetObjectAt(int xx,int yy) {
  int aa,bestshotyp=-1,bestshotwas=-1;
  // translate screen co-ordinates to room co-ordinates
  xx += divide_down_coordinate(offsetx);
  yy += divide_down_coordinate(offsety);
  // Iterate through all objects in the room
  for (aa=0;aa<croom->numobj;aa++) {
    if (objs[aa].on != 1) continue;
    if (objs[aa].flags & OBJF_NOINTERACT)
      continue;
    int xxx=objs[aa].x,yyy=objs[aa].y;
    int isflipped = 0;
    int spWidth = divide_down_coordinate(objs[aa].get_width());
    int spHeight = divide_down_coordinate(objs[aa].get_height());
    if (objs[aa].view >= 0)
      isflipped = views[objs[aa].view].loops[objs[aa].loop].frames[objs[aa].frame].flags & VFLG_FLIPSPRITE;

    block theImage = GetObjectImage(aa, &isflipped);

    if (is_pos_in_sprite(xx, yy, xxx, yyy - spHeight, theImage,
                         spWidth, spHeight, isflipped) == FALSE)
      continue;

    int usebasel = objs[aa].get_baseline();   
    if (usebasel < bestshotyp) continue;

    bestshotwas = aa;
    bestshotyp = usebasel;
  }
  obj_lowest_yp = bestshotyp;
  return bestshotwas;
}

ScriptObject *GetObjectAtLocation(int xx, int yy) {
  int hsnum = GetObjectAt(xx, yy);
  if (hsnum < 0)
    return NULL;
  return &scrObj[hsnum];
}

void RunObjectInteraction (int aa, int mood) {
  if (!is_valid_object(aa))
    quit("!RunObjectInteraction: invalid object number for current room");
  int passon=-1,cdata=-1;
  if (mood==MODE_LOOK) passon=0;
  else if (mood==MODE_HAND) passon=1;
  else if (mood==MODE_TALK) passon=2;
  else if (mood==MODE_PICKUP) passon=5;
  else if (mood==MODE_CUSTOM1) passon = 6;
  else if (mood==MODE_CUSTOM2) passon = 7;
  else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata; }
  evblockbasename="object%d"; evblocknum=aa;

  if (thisroom.objectScripts != NULL) 
  {
    if (passon>=0) 
    {
      if (run_interaction_script(thisroom.objectScripts[aa], passon, 4, (passon == 3)))
        return;
    }
    run_interaction_script(thisroom.objectScripts[aa], 4);  // any click on obj
  }
  else
  {
    if (passon>=0) {
      if (run_interaction_event(&croom->intrObject[aa],passon, 4, (passon == 3)))
        return;
    }
    run_interaction_event(&croom->intrObject[aa],4);  // any click on obj
  }
}

void Object_RunInteraction(ScriptObject *objj, int mode) {
  RunObjectInteraction(objj->id, mode);
}

// X and Y co-ordinates must be in 320x200 format
int check_click_on_object(int xx,int yy,int mood) {
  int aa = GetObjectAt(xx - divide_down_coordinate(offsetx), yy - divide_down_coordinate(offsety));
  if (aa < 0) return 0;
  RunObjectInteraction(aa, mood);
  return 1;
  }

int is_pos_on_character(int xx,int yy) {
  int cc,sppic,lowestyp=0,lowestwas=-1;
  for (cc=0;cc<game.numcharacters;cc++) {
    if (game.chars[cc].room!=displayed_room) continue;
    if (game.chars[cc].on==0) continue;
    if (game.chars[cc].flags & CHF_NOINTERACT) continue;
    if (game.chars[cc].view < 0) continue;
    CharacterInfo*chin=&game.chars[cc];

    if ((chin->view < 0) || 
        (chin->loop >= views[chin->view].numLoops) ||
        (chin->frame >= views[chin->view].loops[chin->loop].numFrames))
    {
      continue;
    }

    sppic=views[chin->view].loops[chin->loop].frames[chin->frame].pic;
    int usewid = charextra[cc].width;
    int usehit = charextra[cc].height;
    if (usewid==0) usewid=spritewidth[sppic];
    if (usehit==0) usehit=spriteheight[sppic];
    int xxx = chin->x - divide_down_coordinate(usewid) / 2;
    int yyy = chin->get_effective_y() - divide_down_coordinate(usehit);

    int mirrored = views[chin->view].loops[chin->loop].frames[chin->frame].flags & VFLG_FLIPSPRITE;
    block theImage = GetCharacterImage(cc, &mirrored);

    if (is_pos_in_sprite(xx,yy,xxx,yyy, theImage,
                         divide_down_coordinate(usewid),
                         divide_down_coordinate(usehit), mirrored) == FALSE)
      continue;

    int use_base = chin->get_baseline();
    if (use_base < lowestyp) continue;
    lowestyp=use_base;
    lowestwas=cc;
  }
  char_lowest_yp = lowestyp;
  return lowestwas;
}

int GetCharacterAt (int xx, int yy) {
  xx += divide_down_coordinate(offsetx);
  yy += divide_down_coordinate(offsety);
  return is_pos_on_character(xx,yy);
}

CharacterInfo *GetCharacterAtLocation(int xx, int yy) {
  int hsnum = GetCharacterAt(xx, yy);
  if (hsnum < 0)
    return NULL;
  return &game.chars[hsnum];
}

void RunCharacterInteraction (int cc, int mood) {
  if (!is_valid_character(cc))
    quit("!RunCharacterInteraction: invalid character");

  int passon=-1,cdata=-1;
  if (mood==MODE_LOOK) passon=0;
  else if (mood==MODE_HAND) passon=1;
  else if (mood==MODE_TALK) passon=2;
  else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata;
    }
  else if (mood==MODE_PICKUP) passon = 5;
  else if (mood==MODE_CUSTOM1) passon = 6;
  else if (mood==MODE_CUSTOM2) passon = 7;

  evblockbasename="character%d"; evblocknum=cc;
  if (game.charScripts != NULL) 
  {
    if (passon>=0)
      run_interaction_script(game.charScripts[cc], passon, 4, (passon == 3));
    run_interaction_script(game.charScripts[cc], 4);  // any click on char
  }
  else 
  {
    if (passon>=0)
      run_interaction_event(game.intrChar[cc],passon, 4, (passon == 3));
    run_interaction_event(game.intrChar[cc],4);  // any click on char
  }
}

void Character_RunInteraction(CharacterInfo *chaa, int mood) {

  RunCharacterInteraction(chaa->index_id, mood);
}


int check_click_on_character(int xx,int yy,int mood) {
  int lowestwas=is_pos_on_character(xx,yy);
  if (lowestwas>=0) {
    RunCharacterInteraction (lowestwas, mood);
    return 1;
  }
  return 0;
}


int GetObjectX (int objj) {
  if (!is_valid_object(objj)) quit("!GetObjectX: invalid object number");
  return objs[objj].x;
}

int Object_GetX(ScriptObject *objj) {
  return GetObjectX(objj->id);
}

int GetObjectY (int objj) {
  if (!is_valid_object(objj)) quit("!GetObjectY: invalid object number");
  return objs[objj].y;
}

int Object_GetY(ScriptObject *objj) {
  return GetObjectY(objj->id);
}

int IsObjectAnimating(int objj) {
  if (!is_valid_object(objj)) quit("!IsObjectAnimating: invalid object number");
  return (objs[objj].cycling != 0) ? 1 : 0;
}

int Object_GetAnimating(ScriptObject *objj) {
  return IsObjectAnimating(objj->id);
}

int IsObjectMoving(int objj) {
  if (!is_valid_object(objj)) quit("!IsObjectMoving: invalid object number");
  return (objs[objj].moving > 0) ? 1 : 0;
}

int Object_GetMoving(ScriptObject *objj) {
  return IsObjectMoving(objj->id);
}

void remove_walkable_areas_from_temp(int fromx, int cwidth, int starty, int endy) {

  fromx = convert_to_low_res(fromx);
  cwidth = convert_to_low_res(cwidth);
  starty = convert_to_low_res(starty);
  endy = convert_to_low_res(endy);

  int yyy;
  if (endy >= walkable_areas_temp->h)
    endy = walkable_areas_temp->h - 1;
  if (starty < 0)
    starty = 0;
  
  for (; cwidth > 0; cwidth --) {
    for (yyy = starty; yyy <= endy; yyy++)
      _putpixel (walkable_areas_temp, fromx, yyy, 0);
    fromx ++;
  }

}

block prepare_walkable_areas (int sourceChar) {
  // copy the walkable areas to the temp bitmap
  blit (thisroom.walls, walkable_areas_temp, 0,0,0,0,thisroom.walls->w,thisroom.walls->h);
  // if the character who's moving doesn't block, don't bother checking
  if (sourceChar < 0) ;
  else if (game.chars[sourceChar].flags & CHF_NOBLOCKING)
    return walkable_areas_temp;

  int ww;
  // for each character in the current room, make the area under
  // them unwalkable
  for (ww = 0; ww < game.numcharacters; ww++) {
    if (game.chars[ww].on != 1) continue;
    if (game.chars[ww].room != displayed_room) continue;
    if (ww == sourceChar) continue;
    if (game.chars[ww].flags & CHF_NOBLOCKING) continue;
    if (convert_to_low_res(game.chars[ww].y) >= walkable_areas_temp->h) continue;
    if (convert_to_low_res(game.chars[ww].x) >= walkable_areas_temp->w) continue;
    if ((game.chars[ww].y < 0) || (game.chars[ww].x < 0)) continue;

    CharacterInfo *char1 = &game.chars[ww];
    int cwidth, fromx;

    if (is_char_on_another(sourceChar, ww, &fromx, &cwidth))
      continue;
    if ((sourceChar >= 0) && (is_char_on_another(ww, sourceChar, NULL, NULL)))
      continue;

    remove_walkable_areas_from_temp(fromx, cwidth, char1->get_blocking_top(), char1->get_blocking_bottom());
  }

  // check for any blocking objects in the room, and deal with them
  // as well
  for (ww = 0; ww < croom->numobj; ww++) {
    if (objs[ww].on != 1) continue;
    if ((objs[ww].flags & OBJF_SOLID) == 0)
      continue;
    if (convert_to_low_res(objs[ww].y) >= walkable_areas_temp->h) continue;
    if (convert_to_low_res(objs[ww].x) >= walkable_areas_temp->w) continue;
    if ((objs[ww].y < 0) || (objs[ww].x < 0)) continue;

    int x1, y1, width, y2;
    get_object_blocking_rect(ww, &x1, &y1, &width, &y2);

    // if the character is currently standing on the object, ignore
    // it so as to allow him to escape
    if ((sourceChar >= 0) &&
        (is_point_in_rect(game.chars[sourceChar].x, game.chars[sourceChar].y, 
                          x1, y1, x1 + width, y2)))
      continue;

    remove_walkable_areas_from_temp(x1, width, y1, y2);
  }

  return walkable_areas_temp;
}

void SetObjectPosition(int objj, int tox, int toy) {
  if (!is_valid_object(objj))
    quit("!SetObjectPosition: invalid object number");

  if (objs[objj].moving > 0)
    quit("!Object.SetPosition: cannot set position while object is moving");

  objs[objj].x = tox;
  objs[objj].y = toy;
}

void Object_SetPosition(ScriptObject *objj, int xx, int yy) {
  SetObjectPosition(objj->id, xx, yy);
}

void Object_SetX(ScriptObject *objj, int xx) {
  SetObjectPosition(objj->id, xx, objj->obj->y);
}

void Object_SetY(ScriptObject *objj, int yy) {
  SetObjectPosition(objj->id, objj->obj->x, yy);
}

void convert_move_path_to_high_res(MoveList *ml)
{
  ml->fromx *= current_screen_resolution_multiplier;
  ml->fromy *= current_screen_resolution_multiplier;
  ml->lastx *= current_screen_resolution_multiplier;
  ml->lasty *= current_screen_resolution_multiplier;

  for (int i = 0; i < ml->numstage; i++)
  {
    short lowPart = (ml->pos[i] & 0x0000ffff) * current_screen_resolution_multiplier;
    short highPart = ((ml->pos[i] >> 16) & 0x0000ffff) * current_screen_resolution_multiplier;
    ml->pos[i] = (highPart << 16) | lowPart;

    ml->xpermove[i] *= current_screen_resolution_multiplier;
    ml->ypermove[i] *= current_screen_resolution_multiplier;
  }
}

void move_object(int objj,int tox,int toy,int spee,int ignwal) {

  if (!is_valid_object(objj))
    quit("!MoveObject: invalid object number");

  DEBUG_CONSOLE("Object %d start move to %d,%d", objj, tox, toy);

  int objX = convert_to_low_res(objs[objj].x);
  int objY = convert_to_low_res(objs[objj].y);
  tox = convert_to_low_res(tox);
  toy = convert_to_low_res(toy);

  set_route_move_speed(spee, spee);
  set_color_depth(8);
  int mslot=find_route(objX, objY, tox, toy, prepare_walkable_areas(-1), objj+1, 1, ignwal);
  set_color_depth(final_col_dep);
  if (mslot>0) {
    objs[objj].moving = mslot;
    mls[mslot].direct = ignwal;

    if ((game.options[OPT_NATIVECOORDINATES] != 0) &&
        (game.default_resolution > 2))
    {
      convert_move_path_to_high_res(&mls[mslot]);
    }
  }
}



void RunRegionInteraction (int regnum, int mood) {
  if ((regnum < 0) || (regnum >= MAX_REGIONS))
    quit("!RunRegionInteraction: invalid region speicfied");
  if ((mood < 0) || (mood > 2))
    quit("!RunRegionInteraction: invalid event specified");

  // We need a backup, because region interactions can run
  // while another interaction (eg. hotspot) is in a Wait
  // command, and leaving our basename would call the wrong
  // script later on
  char *oldbasename = evblockbasename;
  int   oldblocknum = evblocknum;

  evblockbasename = "region%d";
  evblocknum = regnum;

  if (thisroom.regionScripts != NULL)
  {
    run_interaction_script(thisroom.regionScripts[regnum], mood);
  }
  else
  {
    run_interaction_event(&croom->intrRegion[regnum], mood);
  }

  evblockbasename = oldbasename;
  evblocknum = oldblocknum;
}

void Region_RunInteraction(ScriptRegion *ssr, int mood) {
  RunRegionInteraction(ssr->id, mood);
}

void RunHotspotInteraction (int hotspothere, int mood) {

  int passon=-1,cdata=-1;
  if (mood==MODE_TALK) passon=4;
  else if (mood==MODE_WALK) passon=0;
  else if (mood==MODE_LOOK) passon=1;
  else if (mood==MODE_HAND) passon=2;
  else if (mood==MODE_PICKUP) passon=7;
  else if (mood==MODE_CUSTOM1) passon = 8;
  else if (mood==MODE_CUSTOM2) passon = 9;
  else if (mood==MODE_USE) { passon=3;
    cdata=playerchar->activeinv;
    play.usedinv=cdata;
  }

  if ((game.options[OPT_WALKONLOOK]==0) & (mood==MODE_LOOK)) ;
  else if (play.auto_use_walkto_points == 0) ;
  else if ((mood!=MODE_WALK) && (play.check_interaction_only == 0))
    MoveCharacterToHotspot(game.playercharacter,hotspothere);

  // can't use the setevent functions because this ProcessClick is only
  // executed once in a eventlist
  char *oldbasename = evblockbasename;
  int   oldblocknum = evblocknum;

  evblockbasename="hotspot%d";
  evblocknum=hotspothere;

  if (thisroom.hotspotScripts != NULL) 
  {
    if (passon>=0)
      run_interaction_script(thisroom.hotspotScripts[hotspothere], passon, 5, (passon == 3));
    run_interaction_script(thisroom.hotspotScripts[hotspothere], 5);  // any click on hotspot
  }
  else
  {
    if (passon>=0) {
      if (run_interaction_event(&croom->intrHotspot[hotspothere],passon, 5, (passon == 3))) {
        evblockbasename = oldbasename;
        evblocknum = oldblocknum;
        return;
      }
    }
    // run the 'any click on hs' event
    run_interaction_event(&croom->intrHotspot[hotspothere],5);
  }

  evblockbasename = oldbasename;
  evblocknum = oldblocknum;
}

void Hotspot_RunInteraction (ScriptHotspot *hss, int mood) {
  RunHotspotInteraction(hss->id, mood);
}

void ProcessClick(int xx,int yy,int mood) {
  getloctype_throughgui = 1;
  int loctype = GetLocationType (xx, yy);
  xx += divide_down_coordinate(offsetx); 
  yy += divide_down_coordinate(offsety);

  if ((mood==MODE_WALK) && (game.options[OPT_NOWALKMODE]==0)) {
    int hsnum=get_hotspot_at(xx,yy);
    if (hsnum<1) ;
    else if (thisroom.hswalkto[hsnum].x<1) ;
    else if (play.auto_use_walkto_points == 0) ;
    else {
      xx=thisroom.hswalkto[hsnum].x;
      yy=thisroom.hswalkto[hsnum].y;
      DEBUG_CONSOLE("Move to walk-to point hotspot %d", hsnum);
    }
    walk_character(game.playercharacter,xx,yy,0, true);
    return;
  }
  play.usedmode=mood;

  if (loctype == 0) {
    // click on nothing -> hotspot 0
    getloctype_index = 0;
    loctype = LOCTYPE_HOTSPOT;
  }
  
  if (loctype == LOCTYPE_CHAR) {
    if (check_click_on_character(xx,yy,mood)) return;
  }
  else if (loctype == LOCTYPE_OBJ) {
    if (check_click_on_object(xx,yy,mood)) return;
  }
  else if (loctype == LOCTYPE_HOTSPOT) 
    RunHotspotInteraction (getloctype_index, mood);
}

// ** GetGameParameter replacement functions

int Game_GetInventoryItemCount() {
  // because of the dummy item 0, this is always one higher than it should be
  return game.numinvitems - 1;
}

int Game_GetFontCount() {
  return game.numfonts;
}

int Game_GetMouseCursorCount() {
  return game.numcursors;
}

int Game_GetCharacterCount() {
  return game.numcharacters;
}

int Game_GetGUICount() {
  return game.numgui;
}

int Game_GetViewCount() {
  return game.numviews;
}

int Game_GetUseNativeCoordinates()
{
  if (game.options[OPT_NATIVECOORDINATES] != 0)
  {
    return 1;
  }
  return 0;
}

int Game_GetSpriteWidth(int spriteNum) {
  if ((spriteNum < 0) || (spriteNum >= MAX_SPRITES))
    return 0;

  if (!spriteset.doesSpriteExist(spriteNum))
    return 0;

  return divide_down_coordinate(spritewidth[spriteNum]);
}

int Game_GetSpriteHeight(int spriteNum) {
  if ((spriteNum < 0) || (spriteNum >= MAX_SPRITES))
    return 0;

  if (!spriteset.doesSpriteExist(spriteNum))
    return 0;

  return divide_down_coordinate(spriteheight[spriteNum]);
}

int Game_GetLoopCountForView(int viewNumber) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");

  return views[viewNumber - 1].numLoops;
}

int Game_GetRunNextSettingForLoop(int viewNumber, int loopNumber) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");
  if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
    quit("!GetGameParameter: invalid loop specified");

  return (views[viewNumber - 1].loops[loopNumber].RunNextLoop()) ? 1 : 0;
}

int Game_GetFrameCountForLoop(int viewNumber, int loopNumber) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");
  if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
    quit("!GetGameParameter: invalid loop specified");

  return views[viewNumber - 1].loops[loopNumber].numFrames;
}

ScriptViewFrame* Game_GetViewFrame(int viewNumber, int loopNumber, int frame) {
  if ((viewNumber < 1) || (viewNumber > game.numviews))
    quit("!GetGameParameter: invalid view specified");
  if ((loopNumber < 0) || (loopNumber >= views[viewNumber - 1].numLoops))
    quit("!GetGameParameter: invalid loop specified");
  if ((frame < 0) || (frame >= views[viewNumber - 1].loops[loopNumber].numFrames))
    quit("!GetGameParameter: invalid frame specified");

  ScriptViewFrame *sdt = new ScriptViewFrame(viewNumber - 1, loopNumber, frame);
  ccRegisterManagedObject(sdt, sdt);
  return sdt;
}

int Game_DoOnceOnly(const char *token) 
{
  if (strlen(token) > 199)
    quit("!Game.DoOnceOnly: token length cannot be more than 200 chars");

  for (int i = 0; i < play.num_do_once_tokens; i++)
  {
    if (strcmp(play.do_once_tokens[i], token) == 0)
    {
      return 0;
    }
  }
  play.do_once_tokens = (char**)realloc(play.do_once_tokens, sizeof(char*) * (play.num_do_once_tokens + 1));
  play.do_once_tokens[play.num_do_once_tokens] = (char*)malloc(strlen(token) + 1);
  strcpy(play.do_once_tokens[play.num_do_once_tokens], token);
  play.num_do_once_tokens++;
  return 1;
}

int Room_GetObjectCount() {
  return croom->numobj;
}

int Room_GetWidth() {
  return thisroom.width;
}

int Room_GetHeight() {
  return thisroom.height;
}

int Room_GetColorDepth() {
  return bitmap_color_depth(thisroom.ebscene[0]);
}

int Room_GetLeftEdge() {
  return thisroom.left;
}

int Room_GetRightEdge() {
  return thisroom.right;
}

int Room_GetTopEdge() {
  return thisroom.top;
}

int Room_GetBottomEdge() {
  return thisroom.bottom;
}

int Room_GetMusicOnLoad() {
  return thisroom.options[ST_TUNE];
}

int ViewFrame_GetFlipped(ScriptViewFrame *svf) {
  if (views[svf->view].loops[svf->loop].frames[svf->frame].flags & VFLG_FLIPSPRITE)
    return 1;
  return 0;
}

int ViewFrame_GetGraphic(ScriptViewFrame *svf) {
  return views[svf->view].loops[svf->loop].frames[svf->frame].pic;
}

void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic) {
  views[svf->view].loops[svf->loop].frames[svf->frame].pic = newPic;
}

ScriptAudioClip* ViewFrame_GetLinkedAudio(ScriptViewFrame *svf) 
{
  int soundIndex = views[svf->view].loops[svf->loop].frames[svf->frame].sound;
  if (soundIndex < 0)
    return NULL;

  return &game.audioClips[soundIndex];
}

void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip* clip) 
{
  int newSoundIndex = -1;
  if (clip != NULL)
    newSoundIndex = clip->id;

  views[svf->view].loops[svf->loop].frames[svf->frame].sound = newSoundIndex;
}

int ViewFrame_GetSound(ScriptViewFrame *svf) {
  // convert audio clip to old-style sound number
  int soundIndex = views[svf->view].loops[svf->loop].frames[svf->frame].sound;
  if (soundIndex >= 0)
  {
    if (sscanf(game.audioClips[soundIndex].scriptName, "aSound%d", &soundIndex) == 1)
      return soundIndex;
  }
  return 0;
}

void ViewFrame_SetSound(ScriptViewFrame *svf, int newSound) 
{
  if (newSound < 1)
  {
    views[svf->view].loops[svf->loop].frames[svf->frame].sound = -1;
  }
  else
  {
    // convert sound number to audio clip
    ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, newSound);
    if (clip == NULL)
      quitprintf("!SetFrameSound: audio clip aSound%d not found", newSound);

    views[svf->view].loops[svf->loop].frames[svf->frame].sound = clip->id;
  }
}

int ViewFrame_GetSpeed(ScriptViewFrame *svf) {
  return views[svf->view].loops[svf->loop].frames[svf->frame].speed;
}

int ViewFrame_GetView(ScriptViewFrame *svf) {
  return svf->view + 1;
}

int ViewFrame_GetLoop(ScriptViewFrame *svf) {
  return svf->loop;
}

int ViewFrame_GetFrame(ScriptViewFrame *svf) {
  return svf->frame;
}

#define GP_SPRITEWIDTH   1
#define GP_SPRITEHEIGHT  2
#define GP_NUMLOOPS      3
#define GP_NUMFRAMES     4
#define GP_ISRUNNEXTLOOP 5
#define GP_FRAMESPEED    6
#define GP_FRAMEIMAGE    7
#define GP_FRAMESOUND    8
#define GP_NUMGUIS       9
#define GP_NUMOBJECTS    10
#define GP_NUMCHARACTERS 11
#define GP_NUMINVITEMS   12
#define GP_ISFRAMEFLIPPED 13

int GetGameParameter (int parm, int data1, int data2, int data3) {
  switch (parm) {
   case GP_SPRITEWIDTH:
     return Game_GetSpriteWidth(data1);
   case GP_SPRITEHEIGHT:
     return Game_GetSpriteHeight(data1);
   case GP_NUMLOOPS:
     return Game_GetLoopCountForView(data1);
   case GP_NUMFRAMES:
     return Game_GetFrameCountForLoop(data1, data2);
   case GP_FRAMESPEED:
   case GP_FRAMEIMAGE:
   case GP_FRAMESOUND:
   case GP_ISFRAMEFLIPPED:
     {
     if ((data1 < 1) || (data1 > game.numviews))
       quit("!GetGameParameter: invalid view specified");
     if ((data2 < 0) || (data2 >= views[data1 - 1].numLoops))
       quit("!GetGameParameter: invalid loop specified");
     if ((data3 < 0) || (data3 >= views[data1 - 1].loops[data2].numFrames))
       quit("!GetGameParameter: invalid frame specified");

     ViewFrame *pvf = &views[data1 - 1].loops[data2].frames[data3];

     if (parm == GP_FRAMESPEED)
       return pvf->speed;
     else if (parm == GP_FRAMEIMAGE)
       return pvf->pic;
     else if (parm == GP_FRAMESOUND)
       return pvf->sound;
     else if (parm == GP_ISFRAMEFLIPPED)
       return (pvf->flags & VFLG_FLIPSPRITE) ? 1 : 0;
     else
       quit("GetGameParameter internal error");
     }
   case GP_ISRUNNEXTLOOP:
     return Game_GetRunNextSettingForLoop(data1, data2);
   case GP_NUMGUIS:
     return game.numgui;
   case GP_NUMOBJECTS:
     return croom->numobj;
   case GP_NUMCHARACTERS:
     return game.numcharacters;
   case GP_NUMINVITEMS:
     return game.numinvitems;
   default:
     quit("!GetGameParameter: unknown parameter specified");
  }
  return 0;
}


int System_GetColorDepth() {
  return final_col_dep;
}

int System_GetOS() {
  return scsystem.os;
}

int System_GetScreenWidth() {
  return final_scrn_wid;
}

int System_GetScreenHeight() {
  return final_scrn_hit;
}

int System_GetViewportHeight() {
  return divide_down_coordinate(scrnhit);
}

int System_GetViewportWidth() {
  return divide_down_coordinate(scrnwid);
}

const char *System_GetVersion() {
  return CreateNewScriptString(ACI_VERSION_TEXT);
}

int System_GetHardwareAcceleration() 
{
  return gfxDriver->HasAcceleratedStretchAndFlip() ? 1 : 0;
}

int System_GetNumLock()
{
  return (key_shifts & KB_NUMLOCK_FLAG) ? 1 : 0;
}

int System_GetCapsLock()
{
  return (key_shifts & KB_CAPSLOCK_FLAG) ? 1 : 0;
}

int System_GetScrollLock()
{
  return (key_shifts & KB_SCROLOCK_FLAG) ? 1 : 0;
}

void System_SetNumLock(int newValue)
{
  // doesn't work ... maybe allegro doesn't implement this on windows
  int ledState = key_shifts & (KB_SCROLOCK_FLAG | KB_CAPSLOCK_FLAG);
  if (newValue)
  {
    ledState |= KB_NUMLOCK_FLAG;
  }
  set_leds(ledState);
}

int System_GetVsync() {
  return scsystem.vsync;
}

void System_SetVsync(int newValue) {
  scsystem.vsync = newValue;
}

int System_GetWindowed() {
  if (usetup.windowed)
    return 1;
  return 0;
}


int System_GetSupportsGammaControl() {
  return gfxDriver->SupportsGammaControl();
}

int System_GetGamma() {
  return play.gamma_adjustment;
}

void System_SetGamma(int newValue) {
  if ((newValue < 0) || (newValue > 200))
    quitprintf("!System.Gamma: value must be between 0-200 (not %d)", newValue);

  if (play.gamma_adjustment != newValue) {
    DEBUG_CONSOLE("Gamma control set to %d", newValue);
    play.gamma_adjustment = newValue;

    if (gfxDriver->SupportsGammaControl())
      gfxDriver->SetGamma(newValue);
  }
}

int Game_GetTextReadingSpeed()
{
  return play.text_speed;
}

void Game_SetTextReadingSpeed(int newTextSpeed)
{
  if (newTextSpeed < 1)
    quitprintf("!Game.TextReadingSpeed: %d is an invalid speed", newTextSpeed);

  play.text_speed = newTextSpeed;
}

int Game_GetMinimumTextDisplayTimeMs()
{
  return play.text_min_display_time_ms;
}

void Game_SetMinimumTextDisplayTimeMs(int newTextMinTime)
{
  play.text_min_display_time_ms = newTextMinTime;
}

int Game_GetIgnoreUserInputAfterTextTimeoutMs()
{
  return play.ignore_user_input_after_text_timeout_ms;
}

void Game_SetIgnoreUserInputAfterTextTimeoutMs(int newValueMs)
{
  play.ignore_user_input_after_text_timeout_ms = newValueMs;
}

const char *Game_GetFileName() {
  return CreateNewScriptString(usetup.main_data_filename);
}

const char *Game_GetName() {
  return CreateNewScriptString(play.game_name);
}

void Game_SetName(const char *newName) {
  strncpy(play.game_name, newName, 99);
  play.game_name[99] = 0;

#if (ALLEGRO_DATE > 19990103)
  set_window_title(play.game_name);
#endif
}

// begin custom property functions

// Get an integer property
int get_int_property (CustomProperties *cprop, const char *property) {
  int idx = game.propSchema.findProperty(property);

  if (idx < 0)
    quit("!GetProperty: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

  if (game.propSchema.propType[idx] == PROP_TYPE_STRING)
    quit("!GetProperty: need to use GetPropertyString for a text property");

  const char *valtemp = cprop->getPropertyValue(property);
  if (valtemp == NULL) {
    valtemp = game.propSchema.defaultValue[idx];
  }
  return atoi(valtemp);
}

// Get a string property
void get_text_property (CustomProperties *cprop, const char *property, char *bufer) {
  int idx = game.propSchema.findProperty(property);

  if (idx < 0)
    quit("!GetPropertyText: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

  if (game.propSchema.propType[idx] != PROP_TYPE_STRING)
    quit("!GetPropertyText: need to use GetProperty for a non-text property");

  const char *valtemp = cprop->getPropertyValue(property);
  if (valtemp == NULL) {
    valtemp = game.propSchema.defaultValue[idx];
  }
  strcpy (bufer, valtemp);
}

const char* get_text_property_dynamic_string(CustomProperties *cprop, const char *property) {
  int idx = game.propSchema.findProperty(property);

  if (idx < 0)
    quit("!GetTextProperty: no such property found in schema. Make sure you are using the property's name, and not its description, when calling this command.");

  if (game.propSchema.propType[idx] != PROP_TYPE_STRING)
    quit("!GetTextProperty: need to use GetProperty for a non-text property");

  const char *valtemp = cprop->getPropertyValue(property);
  if (valtemp == NULL) {
    valtemp = game.propSchema.defaultValue[idx];
  }

  return CreateNewScriptString(valtemp);
}

int GetInvProperty (int item, const char *property) {
  return get_int_property (&game.invProps[item], property);
}
int InventoryItem_GetProperty(ScriptInvItem *scii, const char *property) {
  return get_int_property (&game.invProps[scii->id], property);
}
int GetCharacterProperty (int cha, const char *property) {
  if (!is_valid_character(cha))
    quit("!GetCharacterProperty: invalid character");
  return get_int_property (&game.charProps[cha], property);
}
int Character_GetProperty(CharacterInfo *chaa, const char *property) {

  return get_int_property(&game.charProps[chaa->index_id], property);

}
int GetHotspotProperty (int hss, const char *property) {
  return get_int_property (&thisroom.hsProps[hss], property);
}
int Hotspot_GetProperty (ScriptHotspot *hss, const char *property) {
  return get_int_property (&thisroom.hsProps[hss->id], property);
}

int GetObjectProperty (int hss, const char *property) {
  if (!is_valid_object(hss))
    quit("!GetObjectProperty: invalid object");
  return get_int_property (&thisroom.objProps[hss], property);
}
int Object_GetProperty (ScriptObject *objj, const char *property) {
  return GetObjectProperty(objj->id, property);
}

int GetRoomProperty (const char *property) {
  return get_int_property (&thisroom.roomProps, property);
}


void GetInvPropertyText (int item, const char *property, char *bufer) {
  get_text_property (&game.invProps[item], property, bufer);
}
void InventoryItem_GetPropertyText(ScriptInvItem *scii, const char *property, char *bufer) {
  get_text_property(&game.invProps[scii->id], property, bufer);
}
const char* InventoryItem_GetTextProperty(ScriptInvItem *scii, const char *property) {
  return get_text_property_dynamic_string(&game.invProps[scii->id], property);
}
void GetCharacterPropertyText (int item, const char *property, char *bufer) {
  get_text_property (&game.charProps[item], property, bufer);
}
void Character_GetPropertyText(CharacterInfo *chaa, const char *property, char *bufer) {
  get_text_property(&game.charProps[chaa->index_id], property, bufer);
}
const char* Character_GetTextProperty(CharacterInfo *chaa, const char *property) {
  return get_text_property_dynamic_string(&game.charProps[chaa->index_id], property);
}
void GetHotspotPropertyText (int item, const char *property, char *bufer) {
  get_text_property (&thisroom.hsProps[item], property, bufer);
}
void Hotspot_GetPropertyText (ScriptHotspot *hss, const char *property, char *bufer) {
  get_text_property (&thisroom.hsProps[hss->id], property, bufer);
}
const char* Hotspot_GetTextProperty(ScriptHotspot *hss, const char *property) {
  return get_text_property_dynamic_string(&thisroom.hsProps[hss->id], property);
}
void GetObjectPropertyText (int item, const char *property, char *bufer) {
  get_text_property (&thisroom.objProps[item], property, bufer);
}
void Object_GetPropertyText(ScriptObject *objj, const char *property, char *bufer) {
  GetObjectPropertyText(objj->id, property, bufer);
}
const char* Object_GetTextProperty(ScriptObject *objj, const char *property) {
  return get_text_property_dynamic_string(&thisroom.objProps[objj->id], property);
}
void GetRoomPropertyText (const char *property, char *bufer) {
  get_text_property (&thisroom.roomProps, property, bufer);
}
const char* Room_GetTextProperty(const char *property) {
  return get_text_property_dynamic_string(&thisroom.roomProps, property);
}

// end custom property functions

int IsInventoryInteractionAvailable (int item, int mood) {
  if ((item < 0) || (item >= MAX_INV))
    quit("!IsInventoryInteractionAvailable: invalid inventory number");

  play.check_interaction_only = 1;

  RunInventoryInteraction(item, mood);

  int ciwas = play.check_interaction_only;
  play.check_interaction_only = 0;

  if (ciwas == 2)
    return 1;

  return 0;
}

int InventoryItem_CheckInteractionAvailable(ScriptInvItem *iitem, int mood) {
  return IsInventoryInteractionAvailable(iitem->id, mood);
}

int IsInteractionAvailable (int xx,int yy,int mood) {
  getloctype_throughgui = 1;
  int loctype = GetLocationType (xx, yy);
  xx += divide_down_coordinate(offsetx); 
  yy += divide_down_coordinate(offsety);

  // You can always walk places
  if ((mood==MODE_WALK) && (game.options[OPT_NOWALKMODE]==0))
    return 1;

  play.check_interaction_only = 1;

  if (loctype == 0) {
    // click on nothing -> hotspot 0
    getloctype_index = 0;
    loctype = LOCTYPE_HOTSPOT;
  }
  
  if (loctype == LOCTYPE_CHAR) {
    check_click_on_character(xx,yy,mood);
  }
  else if (loctype == LOCTYPE_OBJ) {
    check_click_on_object(xx,yy,mood);
  }
  else if (loctype == LOCTYPE_HOTSPOT)
    RunHotspotInteraction (getloctype_index, mood);

  int ciwas = play.check_interaction_only;
  play.check_interaction_only = 0;

  if (ciwas == 2)
    return 1;

  return 0;
}


int do_movelist_move(short*mlnum,int*xx,int*yy) {
  int need_to_fix_sprite=0;
  if (mlnum[0]<1) quit("movelist_move: attempted to move on a non-exist movelist");
  MoveList*cmls; cmls=&mls[mlnum[0]];
  fixed xpermove=cmls->xpermove[cmls->onstage],ypermove=cmls->ypermove[cmls->onstage];

  short targetx=short((cmls->pos[cmls->onstage+1] >> 16) & 0x00ffff);
  short targety=short(cmls->pos[cmls->onstage+1] & 0x00ffff);
  int xps=xx[0],yps=yy[0];
  if (cmls->doneflag & 1) {
    // if the X-movement has finished, and the Y-per-move is < 1, finish
    // This can cause jump at the end, but without it the character will
    // walk on the spot for a while if the Y-per-move is for example 0.2
//    if ((ypermove & 0xfffff000) == 0) cmls->doneflag|=2;
//    int ypmm=(ypermove >> 16) & 0x0000ffff;

    // NEW 2.15 SR-1 plan: if X-movement has finished, and Y-per-move is < 1,
    // allow it to finish more easily by moving target zone

    int adjAmnt = 3;
    // 2.70: if the X permove is also <=1, don't do the skipping
    if (((xpermove & 0xffff0000) == 0xffff0000) ||
        ((xpermove & 0xffff0000) == 0x00000000))
      adjAmnt = 2;

    // 2.61 RC1: correct this to work with > -1 as well as < 1
    if (ypermove == 0) { }
    // Y per move is < 1, so finish the move
    else if ((ypermove & 0xffff0000) == 0)
      targety -= adjAmnt;
    // Y per move is -1 exactly, don't snap to finish
    else if (ypermove == 0xffff0000) { }
    // Y per move is > -1, so finish the move
    else if ((ypermove & 0xffff0000) == 0xffff0000)
      targety += adjAmnt;
  }
  else xps=cmls->fromx+(int)(fixtof(xpermove)*(float)cmls->onpart);

  if (cmls->doneflag & 2) {
    // Y-movement has finished

    int adjAmnt = 3;

    // if the Y permove is also <=1, don't skip as far
    if (((ypermove & 0xffff0000) == 0xffff0000) ||
        ((ypermove & 0xffff0000) == 0x00000000))
      adjAmnt = 2;

    if (xpermove == 0) { }
    // Y per move is < 1, so finish the move
    else if ((xpermove & 0xffff0000) == 0)
      targetx -= adjAmnt;
    // X per move is -1 exactly, don't snap to finish
    else if (xpermove == 0xffff0000) { }
    // X per move is > -1, so finish the move
    else if ((xpermove & 0xffff0000) == 0xffff0000)
      targetx += adjAmnt;

/*    int xpmm=(xpermove >> 16) & 0x0000ffff;
//    if ((xpmm==0) | (xpmm==0xffff)) cmls->doneflag|=1;
    if (xpmm==0) cmls->doneflag|=1;*/
    }
  else yps=cmls->fromy+(int)(fixtof(ypermove)*(float)cmls->onpart);
  // check if finished horizontal movement
  if (((xpermove > 0) && (xps >= targetx)) ||
      ((xpermove < 0) && (xps <= targetx))) {
    cmls->doneflag|=1;
    xps = targetx;
    // if the Y is almost there too, finish it
    // this is new in v2.40
    // removed in 2.70
    /*if (abs(yps - targety) <= 2)
      yps = targety;*/
  }
  else if (xpermove == 0)
    cmls->doneflag|=1;
  // check if finished vertical movement
  if ((ypermove > 0) & (yps>=targety)) {
    cmls->doneflag|=2;
    yps = targety;
  }
  else if ((ypermove < 0) & (yps<=targety)) {
    cmls->doneflag|=2;
    yps = targety;
  }
  else if (ypermove == 0)
    cmls->doneflag|=2;

  if ((cmls->doneflag & 0x03)==3) {
    // this stage is done, go on to the next stage
    // signed shorts to ensure that numbers like -20 do not become 65515
    cmls->fromx=(signed short)((cmls->pos[cmls->onstage+1] >> 16) & 0x000ffff);
    cmls->fromy=(signed short)(cmls->pos[cmls->onstage+1] & 0x000ffff);
    if ((cmls->fromx > 65000) || (cmls->fromy > 65000))
      quit("do_movelist: int to short rounding error");

    cmls->onstage++; cmls->onpart=-1; cmls->doneflag&=0xf0;
    cmls->lastx=-1;
    if (cmls->onstage < cmls->numstage) {
      xps=cmls->fromx; yps=cmls->fromy; }
    if (cmls->onstage>=cmls->numstage-1) {  // last stage is just dest pos
      cmls->numstage=0;
      mlnum[0]=0;
      need_to_fix_sprite=1;
      }
    else need_to_fix_sprite=2;
    }
  cmls->onpart++;
  xx[0]=xps; yy[0]=yps;
  return need_to_fix_sprite;
  }

void RemoveWalkableArea(int areanum) {
  if ((areanum<1) | (areanum>15))
    quit("!RemoveWalkableArea: invalid area number specified (1-15).");
  play.walkable_areas_on[areanum]=0;
  redo_walkable_areas();
  DEBUG_CONSOLE("Walkable area %d removed", areanum);
}

void RestoreWalkableArea(int areanum) {
  if ((areanum<1) | (areanum>15))
    quit("!RestoreWalkableArea: invalid area number specified (1-15).");
  play.walkable_areas_on[areanum]=1;
  redo_walkable_areas();
  DEBUG_CONSOLE("Walkable area %d restored", areanum);
}

void DisableHotspot(int hsnum) {
  if ((hsnum<1) | (hsnum>=MAX_HOTSPOTS))
    quit("!DisableHotspot: invalid hotspot specified");
  croom->hotspot_enabled[hsnum]=0;
  DEBUG_CONSOLE("Hotspot %d disabled", hsnum);
}

void EnableHotspot(int hsnum) {
  if ((hsnum<1) | (hsnum>=MAX_HOTSPOTS))
    quit("!EnableHotspot: invalid hotspot specified");
  croom->hotspot_enabled[hsnum]=1;
  DEBUG_CONSOLE("Hotspot %d re-enabled", hsnum);
}

void Hotspot_SetEnabled(ScriptHotspot *hss, int newval) {
  if (newval)
    EnableHotspot(hss->id);
  else
    DisableHotspot(hss->id);
}

int Hotspot_GetEnabled(ScriptHotspot *hss) {
  return croom->hotspot_enabled[hss->id];
}

int Hotspot_GetID(ScriptHotspot *hss) {
  return hss->id;
}

void DisableRegion(int hsnum) {
  if ((hsnum < 0) || (hsnum >= MAX_REGIONS))
    quit("!DisableRegion: invalid region specified");

  croom->region_enabled[hsnum] = 0;
  DEBUG_CONSOLE("Region %d disabled", hsnum);
}

void EnableRegion(int hsnum) {
  if ((hsnum < 0) || (hsnum >= MAX_REGIONS))
    quit("!EnableRegion: invalid region specified");

  croom->region_enabled[hsnum] = 1;
  DEBUG_CONSOLE("Region %d enabled", hsnum);
}

void Region_SetEnabled(ScriptRegion *ssr, int enable) {
  if (enable)
    EnableRegion(ssr->id);
  else
    DisableRegion(ssr->id);
}

int Region_GetEnabled(ScriptRegion *ssr) {
  return croom->region_enabled[ssr->id];
}

int Region_GetID(ScriptRegion *ssr) {
  return ssr->id;
}

void DisableGroundLevelAreas(int alsoEffects) {
  if ((alsoEffects < 0) || (alsoEffects > 1))
    quit("!DisableGroundLevelAreas: invalid parameter: must be 0 or 1");

  play.ground_level_areas_disabled = GLED_INTERACTION;

  if (alsoEffects)
    play.ground_level_areas_disabled |= GLED_EFFECTS;

  DEBUG_CONSOLE("Ground-level areas disabled");
}

void EnableGroundLevelAreas() {
  play.ground_level_areas_disabled = 0;

  DEBUG_CONSOLE("Ground-level areas re-enabled");
}

void SetWalkBehindBase(int wa,int bl) {
  if ((wa < 1) || (wa >= MAX_OBJ))
    quit("!SetWalkBehindBase: invalid walk-behind area specified");

  if (bl != croom->walkbehind_base[wa]) {
    walk_behind_baselines_changed = 1;
    invalidate_cached_walkbehinds();
    croom->walkbehind_base[wa] = bl;
    DEBUG_CONSOLE("Walk-behind %d baseline changed to %d", wa, bl);
  }
}

void FlipScreen(int amount) {
  if ((amount<0) | (amount>3)) quit("!FlipScreen: invalid argument (0-3)");
  play.screen_flipped=amount;
}

void stopmusic() {

  if (crossFading > 0) {
    // stop in the middle of a new track fading in
    // Abort the new track, and let the old one finish fading out
    stop_and_destroy_channel (crossFading);
    crossFading = -1;
  }
  else if (crossFading < 0) {
    // the music is already fading out
    if (game.options[OPT_CROSSFADEMUSIC] <= 0) {
      // If they have since disabled crossfading, stop the fadeout
      stop_and_destroy_channel(SCHAN_MUSIC);
      crossFading = 0;
      crossFadeStep = 0;
      update_music_volume();
    }
  }
  else if ((game.options[OPT_CROSSFADEMUSIC] > 0)
      && (channels[SCHAN_MUSIC] != NULL)
      && (channels[SCHAN_MUSIC]->done == 0)
      && (current_music_type != 0)
      && (current_music_type != MUS_MIDI)
      && (current_music_type != MUS_MOD)) {

    crossFading = -1;
    crossFadeStep = 0;
    crossFadeVolumePerStep = game.options[OPT_CROSSFADEMUSIC];
    crossFadeVolumeAtStart = calculate_max_volume();
  }
  else
    stop_and_destroy_channel (SCHAN_MUSIC);

  play.cur_music_number = -1;
  current_music_type = 0;
}

void scr_StopMusic() {
  play.music_queue_size = 0;
  stopmusic();
}

void SeekMODPattern(int patnum) {
  if (current_music_type == MUS_MOD) {
    channels[SCHAN_MUSIC]->seek (patnum);
    DEBUG_CONSOLE("Seek MOD/XM to pattern %d", patnum);
  }
}

int Game_GetMODPattern() {
  if (current_music_type == MUS_MOD) {
    return channels[SCHAN_MUSIC]->get_pos();
  }
  return -1;
}

void SeekMP3PosMillis (int posn) {
  if (current_music_type) {
    DEBUG_CONSOLE("Seek MP3/OGG to %d ms", posn);
    if (crossFading)
      channels[crossFading]->seek (posn);
    else
      channels[SCHAN_MUSIC]->seek (posn);
  }
}

int GetMP3PosMillis () {
  // in case they have "while (GetMP3PosMillis() < 5000) "
  if (play.fast_forward)
    return 999999;

  if (current_music_type) {
    int result = channels[SCHAN_MUSIC]->get_pos_ms();
    if (result >= 0)
      return result;

    return channels[SCHAN_MUSIC]->get_pos ();
  }

  return 0;
}

void update_music_volume() {

  if ((current_music_type) || (crossFading < 0)) 
  {
    // targetVol is the maximum volume we're fading in to
    // newvol is the starting volume that we faded out from
    int targetVol = calculate_max_volume();
    int newvol;
    if (crossFading)
      newvol = crossFadeVolumeAtStart;
    else
      newvol = targetVol;

    // fading out old track, target volume is silence
    if (crossFading < 0)
      targetVol = 0;

    if (crossFading) {
      int curvol = crossFadeVolumePerStep * crossFadeStep;

      if ((curvol > targetVol) && (curvol > newvol)) {
        // it has fully faded to the new track
        newvol = targetVol;
        stop_and_destroy_channel_ex(SCHAN_MUSIC, false);
        if (crossFading > 0) {
          channels[SCHAN_MUSIC] = channels[crossFading];
          channels[crossFading] = NULL;
        }
        crossFading = 0;
      }
      else {
        if (crossFading > 0)
          channels[crossFading]->set_volume((curvol > targetVol) ? targetVol : curvol);

        newvol -= curvol;
        if (newvol < 0)
          newvol = 0;
      }
    }
    if (channels[SCHAN_MUSIC])
      channels[SCHAN_MUSIC]->set_volume (newvol);
  }
}

void SetMusicVolume(int newvol) {
  if ((newvol < -3) || (newvol > 5))
    quit("!SetMusicVolume: invalid volume number. Must be from -3 to 5.");
  thisroom.options[ST_VOLUME]=newvol;
  update_music_volume();
  }

void SetMusicMasterVolume(int newvol) {
  if ((newvol<0) | (newvol>100))
    quit("!SetMusicMasterVolume: invalid volume - must be from 0-100");
  play.music_master_volume=newvol+60;
  update_music_volume();
  }

void SetSoundVolume(int newvol) {
  if ((newvol<0) | (newvol>255))
    quit("!SetSoundVolume: invalid volume - must be from 0-255");
  play.sound_volume = newvol;
  Game_SetAudioTypeVolume(AUDIOTYPE_LEGACY_AMBIENT_SOUND, (newvol * 100) / 255, VOL_BOTH);
  Game_SetAudioTypeVolume(AUDIOTYPE_LEGACY_SOUND, (newvol * 100) / 255, VOL_BOTH);
  update_ambient_sound_vol ();
}

void SetChannelVolume(int chan, int newvol) {
  if ((newvol<0) || (newvol>255))
    quit("!SetChannelVolume: invalid volume - must be from 0-255");
  if ((chan < 0) || (chan >= MAX_SOUND_CHANNELS))
    quit("!SetChannelVolume: invalid channel id");

  if ((channels[chan] != NULL) && (channels[chan]->done == 0)) {
    if (chan == ambient[chan].channel) {
      ambient[chan].vol = newvol;
      update_ambient_sound_vol();
    }
    else
      channels[chan]->set_volume (newvol);
  }
}

void SetDigitalMasterVolume (int newvol) {
  if ((newvol<0) | (newvol>100))
    quit("!SetDigitalMasterVolume: invalid volume - must be from 0-100");
  play.digital_master_volume = newvol;
  set_volume ((newvol * 255) / 100, -1);
}

int System_GetVolume() 
{
  return play.digital_master_volume;
}

void System_SetVolume(int newvol) 
{
  if ((newvol < 0) || (newvol > 100))
    quit("!System.Volume: invalid volume - must be from 0-100");

  if (newvol == play.digital_master_volume)
    return;

  play.digital_master_volume = newvol;
  set_volume((newvol * 255) / 100, (newvol * 255) / 100);

  // allegro's set_volume can lose the volumes of all the channels
  // if it was previously set low; so restore them
  for (int i = 0; i <= MAX_SOUND_CHANNELS; i++) 
  {
    if ((channels[i] != NULL) && (channels[i]->done == 0)) 
    {
      channels[i]->set_volume(channels[i]->vol);
    }
  }
}

int GetCurrentMusic() {
  return play.cur_music_number;
  }

void SetMusicRepeat(int loopflag) {
  play.music_repeat=loopflag;
}

// Ensures crossfader is stable after loading (or failing to load)
// new music
void post_new_music_check (int newchannel) {
  if ((crossFading > 0) && (channels[crossFading] == NULL)) {
    crossFading = 0;
    // Was fading out but then they played invalid music, continue
    // to fade out
    if (channels[SCHAN_MUSIC] != NULL)
      crossFading = -1;
  }

}

// Sets up the crossfading for playing the new music track,
// and returns the channel number to use
int prepare_for_new_music () {
  int useChannel = SCHAN_MUSIC;
  
  if ((game.options[OPT_CROSSFADEMUSIC] > 0)
      && (channels[SCHAN_MUSIC] != NULL)
      && (channels[SCHAN_MUSIC]->done == 0)
      && (current_music_type != MUS_MIDI)
      && (current_music_type != MUS_MOD)) {
      
    if (crossFading > 0) {
      // It's still crossfading to the previous track
      stop_and_destroy_channel_ex(SCHAN_MUSIC, false);
      channels[SCHAN_MUSIC] = channels[crossFading];
      channels[crossFading] = NULL;
      crossFading = 0;
      update_music_volume();
    }
    else if (crossFading < 0) {
      // an old track is still fading out, no new music yet
      // Do nothing, and keep the current crossfade step
    }
    else {
      // start crossfading
      crossFadeStep = 0;
      crossFadeVolumePerStep = game.options[OPT_CROSSFADEMUSIC];
      crossFadeVolumeAtStart = calculate_max_volume();
    }
    useChannel = SPECIAL_CROSSFADE_CHANNEL;
    crossFading = useChannel;
  }
  else {
    // crossfading is now turned off
    stopmusic();
    // ensure that any traces of old tunes fading are eliminated
    // (otherwise the new track will be faded out)
    crossFading = 0;
  }

  // Just make sure, because it will be overwritten in a sec
  if (channels[useChannel] != NULL)
    stop_and_destroy_channel (useChannel);

  return useChannel;
}

void PlayMP3File (char *filename) {
  if (strlen(filename) >= PLAYMP3FILE_MAX_FILENAME_LEN)
    quit("!PlayMP3File: filename too long");

  DEBUG_CONSOLE("PlayMP3File %s", filename);

  char pathToFile[MAX_PATH];
  get_current_dir_path(pathToFile, filename);

  int useChan = prepare_for_new_music ();
  bool doLoop = (play.music_repeat > 0);
  
  if ((channels[useChan] = my_load_static_ogg(pathToFile, 150, doLoop)) != NULL) {
    channels[useChan]->play();
    current_music_type = MUS_OGG;
    play.cur_music_number = 1000;
    // save the filename (if it's not what we were supplied with)
    if (filename != &play.playmp3file_name[0])
      strcpy (play.playmp3file_name, filename);
  }
  else if ((channels[useChan] = my_load_static_mp3(pathToFile, 150, doLoop)) != NULL) {
    channels[useChan]->play();
    current_music_type = MUS_MP3;
    play.cur_music_number = 1000;
    // save the filename (if it's not what we were supplied with)
    if (filename != &play.playmp3file_name[0])
      strcpy (play.playmp3file_name, filename);
  }
  else
    debug_log ("PlayMP3File: file '%s' not found or cannot play", filename);

  post_new_music_check(useChan);

  update_music_volume();
}

void PlaySilentMIDI (int mnum) {
  if (current_music_type == MUS_MIDI)
    quit("!PlaySilentMIDI: proper midi music is in progress");

  set_volume (-1, 0);
  play.silent_midi = mnum;
  play.silent_midi_channel = SCHAN_SPEECH;
  stop_and_destroy_channel(play.silent_midi_channel);
  channels[play.silent_midi_channel] = load_sound_clip_from_old_style_number(true, mnum, false);
  if (channels[play.silent_midi_channel] == NULL)
  {
    quitprintf("!PlaySilentMIDI: failed to load aMusic%d", mnum);
  }
  channels[play.silent_midi_channel]->play();
  channels[play.silent_midi_channel]->set_volume(0);
  channels[play.silent_midi_channel]->volAsPercentage = 0;
}

SOUNDCLIP *load_music_from_disk(int mnum, bool doRepeat) {

  if (mnum >= QUEUED_MUSIC_REPEAT) {
    mnum -= QUEUED_MUSIC_REPEAT;
    doRepeat = true;
  }

  SOUNDCLIP *loaded = load_sound_clip_from_old_style_number(true, mnum, doRepeat);

  if ((loaded == NULL) && (mnum > 0)) 
  {
    debug_log("Music %d not found",mnum);
    DEBUG_CONSOLE("FAILED to load music %d", mnum);
  }

  return loaded;
}


void play_new_music(int mnum, SOUNDCLIP *music) {
  if (debug_flags & DBG_NOMUSIC)
    return;
  if (usetup.midicard == MIDI_NONE)
    return;

  if ((play.cur_music_number == mnum) && (music == NULL)) {
    DEBUG_CONSOLE("PlayMusic %d but already playing", mnum);
    return;  // don't play the music if it's already playing
  }

  int useChannel = SCHAN_MUSIC;
  DEBUG_CONSOLE("Playing music %d", mnum);

  if (mnum<0) {
    stopmusic();
    return;
  }

  if (play.fast_forward) {
    // while skipping cutscene, don't change the music
    play.end_cutscene_music = mnum;
    return;
  }

  useChannel = prepare_for_new_music ();

  play.cur_music_number=mnum;
  current_music_type = 0;
  channels[useChannel] = NULL;

  play.current_music_repeating = play.music_repeat;
  // now that all the previous music is unloaded, load in the new one

  if (music != NULL) {
    channels[useChannel] = music;
    music = NULL;
  }
  else {
    channels[useChannel] = load_music_from_disk(mnum, (play.music_repeat > 0));
  }

  if (channels[useChannel] != NULL) {

    if (channels[useChannel]->play() == 0)
      channels[useChannel] = NULL;
    else
      current_music_type = channels[useChannel]->get_sound_type();
  }

  post_new_music_check(useChannel);

  update_music_volume();

}

void newmusic(int mnum) {
  play_new_music(mnum, NULL);
}

void SetPlayerCharacter(int newchar) {
  if (!is_valid_character(newchar))
    quit("!SetPlayerCharacter: Invalid character specified");

  Character_SetAsPlayer(&game.chars[newchar]);
}

void FollowCharacterEx(int who, int tofollow, int distaway, int eagerness) {
  if (!is_valid_character(who))
    quit("!FollowCharacter: Invalid character specified");
  CharacterInfo *chtofollow;
  if (tofollow == -1)
    chtofollow = NULL;
  else if (!is_valid_character(tofollow))
    quit("!FollowCharacterEx: invalid character to follow");
  else
    chtofollow = &game.chars[tofollow];

  Character_FollowCharacter(&game.chars[who], chtofollow, distaway, eagerness);
}

void FollowCharacter(int who, int tofollow) {
  FollowCharacterEx(who,tofollow,10,97);
  }

void SetCharacterIgnoreLight (int who, int yesorno) {
  if (!is_valid_character(who))
    quit("!SetCharacterIgnoreLight: Invalid character specified");

  Character_SetIgnoreLighting(&game.chars[who], yesorno);
}

void SetCharacterProperty (int who, int flag, int yesorno) {
  if (!is_valid_character(who))
    quit("!SetCharacterProperty: Invalid character specified");

  Character_SetOption(&game.chars[who], flag, yesorno);
}

void QuitGame(int dialog) {
  if (dialog) {
    int rcode;
    setup_for_dialog();
    rcode=quitdialog();
    restore_after_dialog();
    if (rcode==0) return;
    }
  quit("|You have exited.");
  }

void sc_inputbox(const char*msg,char*bufr) {
  VALIDATE_STRING(bufr);
  setup_for_dialog();
  enterstringwindow(get_translation(msg),bufr);
  restore_after_dialog();
  }

const char* Game_InputBox(const char *msg) {
  char buffer[STD_BUFFER_SIZE];
  sc_inputbox(msg, buffer);
  return CreateNewScriptString(buffer);
}

int find_word_in_dictionary (char *lookfor) {
  int j;
  if (game.dict == NULL)
    return -1;

  for (j = 0; j < game.dict->num_words; j++) {
    if (stricmp(lookfor, game.dict->word[j]) == 0) {
      return game.dict->wordnum[j];
    }
  }
  if (lookfor[0] != 0) {
    // If the word wasn't found, but it ends in 'S', see if there's
    // a non-plural version
    char *ptat = &lookfor[strlen(lookfor)-1];
    char lastletter = *ptat;
    if ((lastletter == 's') || (lastletter == 'S') || (lastletter == '\'')) {
      *ptat = 0;
      int reslt = find_word_in_dictionary (lookfor);
      *ptat = lastletter;
      return reslt;
    } 
  }
  return -1;
}

int SaidUnknownWord (char*buffer) {
  VALIDATE_STRING(buffer);
  strcpy (buffer, play.bad_parsed_word);
  if (play.bad_parsed_word[0] == 0)
    return 0;
  return 1;
}

const char* Parser_SaidUnknownWord() {
  if (play.bad_parsed_word[0] == 0)
    return NULL;
  return CreateNewScriptString(play.bad_parsed_word);
}

int is_valid_word_char(char theChar) {
  if ((isalnum(theChar)) || (theChar == '\'') || (theChar == '-')) {
    return 1;
  }
  return 0;
}

int FindMatchingMultiWordWord(char *thisword, char **text) {
  // see if there are any multi-word words
  // that match -- if so, use them
  const char *tempptr = *text;
  char tempword[150] = "";
  if (thisword != NULL)
    strcpy(tempword, thisword);

  int bestMatchFound = -1, word;
  const char *tempptrAtBestMatch = tempptr;

  do {
    // extract and concat the next word
    strcat(tempword, " ");
    while (tempptr[0] == ' ') tempptr++;
    char chbuffer[2];
    while (is_valid_word_char(tempptr[0])) {
      sprintf(chbuffer, "%c", tempptr[0]);
      strcat(tempword, chbuffer);
      tempptr++;
    }
    // is this it?
    word = find_word_in_dictionary(tempword);
    // take the longest match we find
    if (word >= 0) {
      bestMatchFound = word;
      tempptrAtBestMatch = tempptr;
    }

  } while (tempptr[0] == ' ');

  word = bestMatchFound;

  if (word >= 0) {
    // yes, a word like "pick up" was found
    *text = (char*)tempptrAtBestMatch;
    if (thisword != NULL)
      strcpy(thisword, tempword);
  }

  return word;
}

// parse_sentence: pass compareto as NULL to parse the sentence, or
// compareto as non-null to check if it matches the passed sentence
int parse_sentence (char*text, int *numwords, short*wordarray, short*compareto, int comparetonum) {
  char thisword[150] = "\0";
  int  i = 0, comparing = 0;
  char in_optional = 0, do_word_now = 0;
  int  optional_start = 0;

  numwords[0] = 0;
  if (compareto == NULL)
    play.bad_parsed_word[0] = 0;
  strlwr(text);
  while (1) {
    if ((compareto != NULL) && (compareto[comparing] == RESTOFLINE))
      return 1;

    if ((text[0] == ']') && (compareto != NULL)) {
      if (!in_optional)
        quit("!Said: unexpected ']'");
      do_word_now = 1;
    }

    if (is_valid_word_char(text[0])) {
      // Part of a word, add it on
      thisword[i] = text[0];
      i++;
    }
    else if ((text[0] == '[') && (compareto != NULL)) {
      if (in_optional)
        quit("!Said: nested optional words");

      in_optional = 1;
      optional_start = comparing;
    }
    else if ((thisword[0] != 0) || ((text[0] == 0) && (i > 0)) || (do_word_now == 1)) {
      // End of word, so process it
      thisword[i] = 0;
      i = 0;
      int word = -1;

      if (text[0] == ' ') {
        word = FindMatchingMultiWordWord(thisword, &text);
      }

      if (word < 0) {
        // just a normal word
        word = find_word_in_dictionary(thisword);
      }

      // "look rol"
      if (word == RESTOFLINE)
        return 1;
      if (compareto) {
        // check string is longer than user input
        if (comparing >= comparetonum) {
          if (in_optional) {
            // eg. "exit [door]" - there's no more user input
            // but the optional word is still there
            if (do_word_now) {
              in_optional = 0;
              do_word_now = 0;
            }
            thisword[0] = 0;
            text++;
            continue;
          }
          return 0;
        }
        if (word <= 0)
          quitprintf("!Said: supplied word '%s' is not in dictionary or is an ignored word", thisword);
        if (word == ANYWORD) { }
        else if (word != compareto[comparing]) {
          // words don't match - if a comma then a list of possibles,
          // so allow retry
          if (text[0] == ',')
            comparing--;
          else {
            // words don't match
            if (in_optional) {
              // inside an optional clause, so skip it
              while (text[0] != ']') {
                if (text[0] == 0)
                  quit("!Said: unterminated [optional]");
                text++;
              }
              // -1 because it's about to be ++'d again
              comparing = optional_start - 1;
            }
            // words don't match outside an optional clause, abort
            else
              return 0;
          }
        }
        else if (text[0] == ',') {
          // this alternative matched, but there are more
          // so skip the other alternatives
          int continueSearching = 1;
          while (continueSearching) {

            const char *textStart = &text[1];

            while ((text[0] == ',') || (isalnum(text[0]) != 0))
              text++;

            continueSearching = 0;

            if (text[0] == ' ') {
              strcpy(thisword, textStart);
              thisword[text - textStart] = 0;
              // forward past any multi-word alternatives
              if (FindMatchingMultiWordWord(thisword, &text) >= 0)
                continueSearching = 1;
            }
          }

          if ((text[0] == ']') && (in_optional)) {
            // [go,move]  we just matched "go", so skip over "move"
            in_optional = 0;
            text++;
          }

          // go back cos it'll be ++'d in a minute
          text--;
        }
        comparing++;
      }
      else if (word != 0) {
        // it's not an ignore word (it's a known word, or an unknown
        // word, so save its index)
        wordarray[numwords[0]] = word;
        numwords[0]++;
        if (numwords[0] >= MAX_PARSED_WORDS)
          return 0;
        // if it's an unknown word, store it for use in messages like
        // "you can't use the word 'xxx' in this game"
        if ((word < 0) && (play.bad_parsed_word[0] == 0))
          strcpy(play.bad_parsed_word, thisword);
      }

      if (do_word_now) {
        in_optional = 0;
        do_word_now = 0;
      }
  
      thisword[0] = 0;
    }
    if (text[0] == 0)
      break;
    text++;
  }
  // If the user input is longer than the Said string, it's wrong
  // eg Said("look door") and they type "look door jibble"
  // rol should be used instead to enable this
  if (comparing < comparetonum)
    return 0;
  return 1;
}

void ParseText (char*text) {
  parse_sentence (text, &play.num_parsed_words, play.parsed_words, NULL, 0);
}

int Parser_FindWordID(const char *wordToFind)
{
  return find_word_in_dictionary((char*)wordToFind);
}

// Said: call with argument for example "get apple"; we then check
// word by word if it matches (using dictonary ID equivalence to match
// synonyms). Returns 1 if it does, 0 if not.
int Said (char*checkwords) {
  int numword = 0;
  short words[MAX_PARSED_WORDS];
  return parse_sentence (checkwords, &numword, &words[0], play.parsed_words, play.num_parsed_words);
}

int MAXSTRLEN = MAX_MAXSTRLEN;
void check_strlen(char*ptt) {
  MAXSTRLEN = MAX_MAXSTRLEN;
  long charstart = (long)&game.chars[0];
  long charend = charstart + sizeof(CharacterInfo)*game.numcharacters;
  if (((long)&ptt[0] >= charstart) && ((long)&ptt[0] <= charend))
    MAXSTRLEN=30;
}


#define MAX_OPEN_SCRIPT_FILES 10
FILE*valid_handles[MAX_OPEN_SCRIPT_FILES+1];
int num_open_script_files = 0;
int check_valid_file_handle(FILE*hann, char*msg) {
  int aa;
  if (hann != NULL) {
    for (aa=0; aa < num_open_script_files; aa++) {
      if (hann == valid_handles[aa])
        return aa;
    }
  }
  char exmsg[100];
  sprintf(exmsg,"!%s: invalid file handle; file not previously opened or has been closed",msg);
  quit(exmsg);
  return -1;
  }

bool validate_user_file_path(const char *fnmm, char *output, bool currentDirOnly)
{
  if (strncmp(fnmm, "$SAVEGAMEDIR$", 13) == 0) 
  {
    fnmm += 14;
    sprintf(output, "%s%s", saveGameDirectory, fnmm);
  }
  else if (strncmp(fnmm, "$APPDATADIR$", 12) == 0) 
  {
    fnmm += 13;
    const char *appDataDir = platform->GetAllUsersDataDirectory();
    if (appDataDir == NULL) appDataDir = ".";
    if (game.saveGameFolderName[0] != 0)
    {
      sprintf(output, "%s/%s", appDataDir, game.saveGameFolderName);
      fix_filename_slashes(output);
      mkdir(output
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
                  , 0755
#endif
      );
    }
    else 
    {
      strcpy(output, appDataDir);
    }
    put_backslash(output);
    strcat(output, fnmm);
  }
  else
  {
    get_current_dir_path(output, fnmm);
  }

  // don't allow access to files outside current dir
  if (!currentDirOnly) { }
  else if ((strchr (fnmm, '/') != NULL) || (strchr(fnmm, '\\') != NULL) ||
    (strstr(fnmm, "..") != NULL) || (strchr(fnmm, ':') != NULL)) {
    debug_log("Attempt to access file '%s' denied (not current directory)", fnmm);
    return false;
  }

  return true;
}

FILE* FileOpen(const char*fnmm, const char* mode) {
  int useindx = 0;
  char fileToOpen[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToOpen, strcmp(mode, "rb") != 0))
    return NULL;

  // find a free file handle to use
  for (useindx = 0; useindx < num_open_script_files; useindx++) 
  {
    if (valid_handles[useindx] == NULL)
      break;
  }

  valid_handles[useindx] = fopen(fileToOpen, mode);

  if (valid_handles[useindx] == NULL)
    return NULL;

  if (useindx >= num_open_script_files) 
  {
    if (num_open_script_files >= MAX_OPEN_SCRIPT_FILES)
      quit("!FileOpen: tried to open more than 10 files simultaneously - close some first");
    num_open_script_files++;
  }
  return valid_handles[useindx];
}

void FileClose(FILE*hha) {
  valid_handles[check_valid_file_handle(hha,"FileClose")] = NULL;
  fclose(hha);
  }
void FileWrite(FILE*haa, const char *towrite) {
  check_valid_file_handle(haa,"FileWrite");
  putw(strlen(towrite)+1,haa);
  fwrite(towrite,strlen(towrite)+1,1,haa);
  }
void FileWriteRawLine(FILE*haa, const char*towrite) {
  check_valid_file_handle(haa,"FileWriteRawLine");
  fwrite(towrite,strlen(towrite),1,haa);
  fputc (13, haa);
  fputc (10, haa);
  }
void FileRead(FILE*haa,char*toread) {
  VALIDATE_STRING(toread);
  check_valid_file_handle(haa,"FileRead");
  if (feof(haa)) {
    toread[0] = 0;
    return;
  }
  int lle=getw(haa);
  if ((lle>=200) | (lle<1)) quit("!FileRead: file was not written by FileWrite");
  fread(toread,lle,1,haa);
  }
int FileIsEOF (FILE *haa) {
  check_valid_file_handle(haa,"FileIsEOF");
  if (feof(haa))
    return 1;
  if (ferror (haa))
    return 1;
  if (ftell (haa) >= filelength (fileno(haa)))
    return 1;
  return 0;
}
int FileIsError(FILE *haa) {
  check_valid_file_handle(haa,"FileIsError");
  if (ferror(haa))
    return 1;
  return 0;
}
void FileWriteInt(FILE*haa,int into) {
  check_valid_file_handle(haa,"FileWriteInt");
  fputc('I',haa);
  putw(into,haa);
  }
int FileReadInt(FILE*haa) {
  check_valid_file_handle(haa,"FileReadInt");
  if (feof(haa))
    return -1;
  if (fgetc(haa)!='I')
    quit("!FileReadInt: File read back in wrong order");
  return getw(haa);
  }
char FileReadRawChar(FILE*haa) {
  check_valid_file_handle(haa,"FileReadRawChar");
  if (feof(haa))
    return -1;
  return fgetc(haa);
  }
int FileReadRawInt(FILE*haa) {
  check_valid_file_handle(haa,"FileReadRawInt");
  if (feof(haa))
    return -1;
  return getw(haa);
}
void FileWriteRawChar(FILE *haa, int chartoWrite) {
  check_valid_file_handle(haa,"FileWriteRawChar");
  if ((chartoWrite < 0) || (chartoWrite > 255))
    quit("!FileWriteRawChar: can only write values 0-255");

  fputc(chartoWrite, haa);
}

// object-based File routines

const char *fopenModes[] = {NULL, "rb", "wb", "ab"};

int sc_File::OpenFile(const char *filename, int mode) {
  handle = FileOpen(filename, fopenModes[mode]);
  if (handle == NULL)
    return 0;
  return 1;
}

void sc_File::Close() {
  if (handle) {
    FileClose(handle);
    handle = NULL;
  }
}

int File_Exists(const char *fnmm) {
  char fileToCheck[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToCheck, false))
    return 0;

  FILE *iii = fopen(fileToCheck, "rb");
  if (iii == NULL)
    return 0;

  fclose(iii);
  return 1;
}

int File_Delete(const char *fnmm) {

  char fileToDelete[MAX_PATH];

  if (!validate_user_file_path(fnmm, fileToDelete, true))
    return 0;

  unlink(fileToDelete);

  return 1;
}

void *sc_OpenFile(const char *fnmm, int mode) {
  if ((mode < scFileRead) || (mode > scFileAppend))
    quit("!OpenFile: invalid file mode");

  sc_File *scf = new sc_File();
  if (scf->OpenFile(fnmm, mode) == 0) {
    delete scf;
    return 0;
  }
  ccRegisterManagedObject(scf, scf);
  return scf;
}

void File_Close(sc_File *fil) {
  fil->Close();
}

void File_WriteString(sc_File *fil, const char *towrite) {
  FileWrite(fil->handle, towrite);
}

void File_WriteInt(sc_File *fil, int towrite) {
  FileWriteInt(fil->handle, towrite);
}

void File_WriteRawChar(sc_File *fil, int towrite) {
  FileWriteRawChar(fil->handle, towrite);
}

void File_WriteRawLine(sc_File *fil, const char *towrite) {
  FileWriteRawLine(fil->handle, towrite);
}

void File_ReadRawLine(sc_File *fil, char* buffer) {
  check_valid_file_handle(fil->handle, "File.ReadRawLine");
  check_strlen(buffer);
  int i = 0;
  while (i < MAXSTRLEN - 1) {
    buffer[i] = fgetc(fil->handle);
    if (buffer[i] == 13) {
      // CR -- skip LF and abort
      fgetc(fil->handle);
      break;
    }
    if (buffer[i] == 10)  // LF only -- abort
      break;
    if (feof(fil->handle))  // EOF -- abort
      break;
    i++;
  }
  buffer[i] = 0;
}

const char* File_ReadRawLineBack(sc_File *fil) {
  char readbuffer[MAX_MAXSTRLEN + 1];
  File_ReadRawLine(fil, readbuffer);
  return CreateNewScriptString(readbuffer);
}

void File_ReadString(sc_File *fil, char *toread) {
  FileRead(fil->handle, toread);
}

const char* File_ReadStringBack(sc_File *fil) {
  check_valid_file_handle(fil->handle, "File.ReadStringBack");
  if (feof(fil->handle)) {
    return CreateNewScriptString("");
  }

  int lle = getw(fil->handle);
  if ((lle >= 20000) || (lle < 1))
    quit("!File.ReadStringBack: file was not written by WriteString");

  char *retVal = (char*)malloc(lle);
  fread(retVal, lle, 1, fil->handle);

  return CreateNewScriptString(retVal, false);
}

int File_ReadInt(sc_File *fil) {
  return FileReadInt(fil->handle);
}

int File_ReadRawChar(sc_File *fil) {
  return FileReadRawChar(fil->handle);
}

int File_ReadRawInt(sc_File *fil) {
  return FileReadRawInt(fil->handle);
}

int File_GetEOF(sc_File *fil) {
  if (fil->handle == NULL)
    return 1;
  return FileIsEOF(fil->handle);
}

int File_GetError(sc_File *fil) {
  if (fil->handle == NULL)
    return 1;
  return FileIsError(fil->handle);
}


void InterfaceOn(int ifn) {
  if ((ifn<0) | (ifn>=game.numgui))
    quit("!GUIOn: invalid GUI specified");

  EndSkippingUntilCharStops();

  if (guis[ifn].on == 1) {
    DEBUG_CONSOLE("GUIOn(%d) ignored (already on)", ifn);
    return;
  }
  guis_need_update = 1;
  guis[ifn].on=1;
  DEBUG_CONSOLE("GUI %d turned on", ifn);
  // modal interface
  if (guis[ifn].popup==POPUP_SCRIPT) PauseGame();
  else if (guis[ifn].popup==POPUP_MOUSEY) guis[ifn].on=0;
  // clear the cached mouse position
  guis[ifn].control_positions_changed();
  guis[ifn].poll();
}

void InterfaceOff(int ifn) {
  if ((ifn<0) | (ifn>=game.numgui)) quit("!GUIOff: invalid GUI specified");
  if ((guis[ifn].on==0) && (guis[ifn].popup!=POPUP_MOUSEY)) {
    DEBUG_CONSOLE("GUIOff(%d) ignored (already off)", ifn);
    return;
  }
  DEBUG_CONSOLE("GUI %d turned off", ifn);
  guis[ifn].on=0;
  if (guis[ifn].mouseover>=0) {
    // Make sure that the overpic is turned off when the GUI goes off
    guis[ifn].objs[guis[ifn].mouseover]->MouseLeave();
    guis[ifn].mouseover = -1;
  }
  guis[ifn].control_positions_changed();
  guis_need_update = 1;
  // modal interface
  if (guis[ifn].popup==POPUP_SCRIPT) UnPauseGame();
  else if (guis[ifn].popup==POPUP_MOUSEY) guis[ifn].on=-1;
}

void GUI_SetVisible(ScriptGUI *tehgui, int isvisible) {
  if (isvisible)
    InterfaceOn(tehgui->id);
  else
    InterfaceOff(tehgui->id);
}

int GUI_GetVisible(ScriptGUI *tehgui) {
  // GUI_GetVisible is slightly different from IsGUIOn, because
  // with a mouse ypos gui it returns 1 if the GUI is enabled,
  // whereas IsGUIOn actually checks if it is displayed
  if (tehgui->gui->on != 0)
    return 1;
  return 0;
}

int GUIControl_GetVisible(GUIObject *guio) {
  return guio->IsVisible();
}

void GUIControl_SetVisible(GUIObject *guio, int visible) 
{
  if (visible != guio->IsVisible()) 
  {
    if (visible)
      guio->Show();
    else
      guio->Hide();

    guis[guio->guin].control_positions_changed();
    guis_need_update = 1;
  }
}

int GUIControl_GetClickable(GUIObject *guio) {
  if (guio->IsClickable())
    return 1;
  return 0;
}

void GUIControl_SetClickable(GUIObject *guio, int enabled) {
  if (enabled)
    guio->SetClickable(true);
  else
    guio->SetClickable(false);

  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetEnabled(GUIObject *guio) {
  if (guio->IsDisabled())
    return 0;
  return 1;
}

void GUIControl_SetEnabled(GUIObject *guio, int enabled) {
  if (enabled)
    guio->Enable();
  else
    guio->Disable();

  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void SetGUIObjectEnabled(int guin, int objn, int enabled) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIObjectEnabled: invalid GUI number");
  if ((objn<0) || (objn>=guis[guin].numobjs))
    quit("!SetGUIObjectEnabled: invalid object number");

  GUIControl_SetEnabled(guis[guin].objs[objn], enabled);
}

int GUIControl_GetID(GUIObject *guio) {
  return guio->objn;
}

ScriptGUI* GUIControl_GetOwningGUI(GUIObject *guio) {
  return &scrGui[guio->guin];
}

GUIButton* GUIControl_GetAsButton(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_BUTTON)
    return NULL;

  return (GUIButton*)guio;
}

GUIInv* GUIControl_GetAsInvWindow(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_INVENTORY)
    return NULL;

  return (GUIInv*)guio;
}

GUILabel* GUIControl_GetAsLabel(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_LABEL)
    return NULL;

  return (GUILabel*)guio;
}

GUIListBox* GUIControl_GetAsListBox(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_LISTBOX)
    return NULL;

  return (GUIListBox*)guio;
}

GUISlider* GUIControl_GetAsSlider(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_SLIDER)
    return NULL;

  return (GUISlider*)guio;
}

GUITextBox* GUIControl_GetAsTextBox(GUIObject *guio) {
  if (guis[guio->guin].get_control_type(guio->objn) != GOBJ_TEXTBOX)
    return NULL;

  return (GUITextBox*)guio;
}

int GUIControl_GetX(GUIObject *guio) {
  return divide_down_coordinate(guio->x);
}

void GUIControl_SetX(GUIObject *guio, int xx) {
  guio->x = multiply_up_coordinate(xx);
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetY(GUIObject *guio) {
  return divide_down_coordinate(guio->y);
}

void GUIControl_SetY(GUIObject *guio, int yy) {
  guio->y = multiply_up_coordinate(yy);
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void GUIControl_SetPosition(GUIObject *guio, int xx, int yy) {
  GUIControl_SetX(guio, xx);
  GUIControl_SetY(guio, yy);
}

void SetGUIObjectPosition(int guin, int objn, int xx, int yy) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIObjectPosition: invalid GUI number");
  if ((objn<0) || (objn>=guis[guin].numobjs))
    quit("!SetGUIObjectPosition: invalid object number");

  GUIControl_SetPosition(guis[guin].objs[objn], xx, yy);
}

int GUI_GetX(ScriptGUI *tehgui) {
  return divide_down_coordinate(tehgui->gui->x);
}

void GUI_SetX(ScriptGUI *tehgui, int xx) {
  if (xx >= thisroom.width)
    quit("!GUI.X: co-ordinates specified are out of range.");

  tehgui->gui->x = multiply_up_coordinate(xx);
}

int GUI_GetY(ScriptGUI *tehgui) {
  return divide_down_coordinate(tehgui->gui->y);
}

void GUI_SetY(ScriptGUI *tehgui, int yy) {
  if (yy >= thisroom.height)
    quit("!GUI.Y: co-ordinates specified are out of range.");

  tehgui->gui->y = multiply_up_coordinate(yy);
}

void GUI_SetPosition(ScriptGUI *tehgui, int xx, int yy) {
  GUI_SetX(tehgui, xx);
  GUI_SetY(tehgui, yy);
}

void SetGUIPosition(int ifn,int xx,int yy) {
  if ((ifn<0) || (ifn>=game.numgui))
    quit("!SetGUIPosition: invalid GUI number");
  
  GUI_SetPosition(&scrGui[ifn], xx, yy);
}

int GUIControl_GetWidth(GUIObject *guio) {
  return divide_down_coordinate(guio->wid);
}

void GUIControl_SetWidth(GUIObject *guio, int newwid) {
  guio->wid = multiply_up_coordinate(newwid);
  guio->Resized();
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

int GUIControl_GetHeight(GUIObject *guio) {
  return divide_down_coordinate(guio->hit);
}

void GUIControl_SetHeight(GUIObject *guio, int newhit) {
  guio->hit = multiply_up_coordinate(newhit);
  guio->Resized();
  guis[guio->guin].control_positions_changed();
  guis_need_update = 1;
}

void GUIControl_SetSize(GUIObject *guio, int newwid, int newhit) {
  if ((newwid < 2) || (newhit < 2))
    quit("!SetGUIObjectSize: new size is too small (must be at least 2x2)");

  DEBUG_CONSOLE("SetGUIObject %d,%d size %d,%d", guio->guin, guio->objn, newwid, newhit);
  GUIControl_SetWidth(guio, newwid);
  GUIControl_SetHeight(guio, newhit);
}

void GUIControl_SendToBack(GUIObject *guio) {
  if (guis[guio->guin].send_to_back(guio->objn))
    guis_need_update = 1;
}

void GUIControl_BringToFront(GUIObject *guio) {
  if (guis[guio->guin].bring_to_front(guio->objn))
    guis_need_update = 1;
}

void SetGUIObjectSize(int ifn, int objn, int newwid, int newhit) {
  if ((ifn<0) || (ifn>=game.numgui))
    quit("!SetGUIObjectSize: invalid GUI number");

  if ((objn<0) || (objn >= guis[ifn].numobjs))
    quit("!SetGUIObjectSize: invalid object number");

  GUIControl_SetSize(guis[ifn].objs[objn], newwid, newhit);
}

void recreate_guibg_image(GUIMain *tehgui)
{
  int ifn = tehgui->guiId;
  destroy_bitmap(guibg[ifn]);
  guibg[ifn] = create_bitmap_ex (final_col_dep, tehgui->wid, tehgui->hit);
  if (guibg[ifn] == NULL)
    quit("SetGUISize: internal error: unable to reallocate gui cache");
  guibg[ifn] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[ifn]);

  if (guibgbmp[ifn] != NULL)
  {
    gfxDriver->DestroyDDB(guibgbmp[ifn]);
    guibgbmp[ifn] = NULL;
  }
}

void GUI_SetSize(ScriptGUI *sgui, int widd, int hitt) {
  if ((widd < 1) || (hitt < 1) || (widd > BASEWIDTH) || (hitt > GetMaxScreenHeight()))
    quitprintf("!SetGUISize: invalid dimensions (tried to set to %d x %d)", widd, hitt);

  GUIMain *tehgui = sgui->gui;
  multiply_up_coordinates(&widd, &hitt);

  if ((tehgui->wid == widd) && (tehgui->hit == hitt))
    return;
  
  tehgui->wid = widd;
  tehgui->hit = hitt;
  
  recreate_guibg_image(tehgui);

  guis_need_update = 1;
}

int GUI_GetWidth(ScriptGUI *sgui) {
  return divide_down_coordinate(sgui->gui->wid);
}

int GUI_GetHeight(ScriptGUI *sgui) {
  return divide_down_coordinate(sgui->gui->hit);
}

void GUI_SetWidth(ScriptGUI *sgui, int newwid) {
  GUI_SetSize(sgui, newwid, GUI_GetHeight(sgui));
}

void GUI_SetHeight(ScriptGUI *sgui, int newhit) {
  GUI_SetSize(sgui, GUI_GetWidth(sgui), newhit);
}

void SetGUISize (int ifn, int widd, int hitt) {
  if ((ifn<0) || (ifn>=game.numgui))
    quit("!SetGUISize: invalid GUI number");

  GUI_SetSize(&scrGui[ifn], widd, hitt);
}

void GUI_SetZOrder(ScriptGUI *tehgui, int z) {
  tehgui->gui->zorder = z;
  update_gui_zorder();
}

int GUI_GetZOrder(ScriptGUI *tehgui) {
  return tehgui->gui->zorder;
}

void SetGUIZOrder(int guin, int z) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIZOrder: invalid GUI number");

  GUI_SetZOrder(&scrGui[guin], z);
}

void GUI_SetClickable(ScriptGUI *tehgui, int clickable) {
  tehgui->gui->flags &= ~GUIF_NOCLICK;
  if (clickable == 0)
    tehgui->gui->flags |= GUIF_NOCLICK;
}

int GUI_GetClickable(ScriptGUI *tehgui) {
  if (tehgui->gui->flags & GUIF_NOCLICK)
    return 0;
  return 1;
}

void SetGUIClickable(int guin, int clickable) {
  if ((guin<0) || (guin>=game.numgui))
    quit("!SetGUIClickable: invalid GUI number");

  GUI_SetClickable(&scrGui[guin], clickable);
}

int GUI_GetID(ScriptGUI *tehgui) {
  return tehgui->id;
}

GUIObject* GUI_GetiControls(ScriptGUI *tehgui, int idx) {
  if ((idx < 0) || (idx >= tehgui->gui->numobjs))
    return NULL;
  return tehgui->gui->objs[idx];
}

int GUI_GetControlCount(ScriptGUI *tehgui) {
  return tehgui->gui->numobjs;
}

void GUI_SetTransparency(ScriptGUI *tehgui, int trans) {
  if ((trans < 0) | (trans > 100))
    quit("!SetGUITransparency: transparency value must be between 0 and 100");

  tehgui->gui->SetTransparencyAsPercentage(trans);
}

int GUI_GetTransparency(ScriptGUI *tehgui) {
  if (tehgui->gui->transparency == 0)
    return 0;
  if (tehgui->gui->transparency == 255)
    return 100;

  return 100 - ((tehgui->gui->transparency * 10) / 25);
}

// pass trans=0 for fully solid, trans=100 for fully transparent
void SetGUITransparency(int ifn, int trans) {
  if ((ifn < 0) | (ifn >= game.numgui))
    quit("!SetGUITransparency: invalid GUI number");

  GUI_SetTransparency(&scrGui[ifn], trans);
}

void GUI_Centre(ScriptGUI *sgui) {
  GUIMain *tehgui = sgui->gui;
  tehgui->x = scrnwid / 2 - tehgui->wid / 2;
  tehgui->y = scrnhit / 2 - tehgui->hit / 2;
}

void CentreGUI (int ifn) {
  if ((ifn<0) | (ifn>=game.numgui))
    quit("!CentreGUI: invalid GUI number");

  GUI_Centre(&scrGui[ifn]);
}

int GetTextWidth(char *text, int fontnum) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetTextWidth: invalid font number.");

  return divide_down_coordinate(wgettextwidth_compensate(text, fontnum));
}

int GetTextHeight(char *text, int fontnum, int width) {
  VALIDATE_STRING(text);
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!GetTextHeight: invalid font number.");

  int texthit = wgetfontheight(fontnum);

  break_up_text_into_lines(multiply_up_coordinate(width), fontnum, text);

  return divide_down_coordinate(texthit * numlines);
}


// ** TEXT BOX FUNCTIONS

const char* TextBox_GetText_New(GUITextBox *texbox) {
  return CreateNewScriptString(texbox->text);
}

void TextBox_GetText(GUITextBox *texbox, char *buffer) {
  strcpy(buffer, texbox->text);
}

void TextBox_SetText(GUITextBox *texbox, const char *newtex) {
  if (strlen(newtex) > 190)
    quit("!SetTextBoxText: text too long");

  if (strcmp(texbox->text, newtex)) {
    strcpy(texbox->text, newtex);
    guis_need_update = 1;
  }
}

int TextBox_GetTextColor(GUITextBox *guit) {
  return guit->textcol;
}

void TextBox_SetTextColor(GUITextBox *guit, int colr)
{
  if (guit->textcol != colr) 
  {
    guit->textcol = colr;
    guis_need_update = 1;
  }
}

int TextBox_GetFont(GUITextBox *guit) {
  return guit->font;
}

void TextBox_SetFont(GUITextBox *guit, int fontnum) {
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!SetTextBoxFont: invalid font number.");

  if (guit->font != fontnum) {
    guit->font = fontnum;
    guis_need_update = 1;
  }
}


void SetTextBoxFont(int guin,int objn, int fontnum) {

  if ((guin<0) | (guin>=game.numgui)) quit("!SetTextBoxFont: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetTextBoxFont: invalid object number");
  if (guis[guin].get_control_type(objn) != GOBJ_TEXTBOX)
    quit("!SetTextBoxFont: specified control is not a text box");

  GUITextBox *guit = (GUITextBox*)guis[guin].objs[objn];
  TextBox_SetFont(guit, fontnum);
}

void GetTextBoxText(int guin, int objn, char*txbuf) {
  VALIDATE_STRING(txbuf);
  if ((guin<0) | (guin>=game.numgui)) quit("!GetTextBoxText: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!GetTextBoxText: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_TEXTBOX)
    quit("!GetTextBoxText: specified control is not a text box");

  GUITextBox*guisl=(GUITextBox*)guis[guin].objs[objn];
  TextBox_GetText(guisl, txbuf);
}

void SetTextBoxText(int guin, int objn, char*txbuf) {
  if ((guin<0) | (guin>=game.numgui)) quit("!SetTextBoxText: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetTextBoxText: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_TEXTBOX)
    quit("!SetTextBoxText: specified control is not a text box");

  GUITextBox*guisl=(GUITextBox*)guis[guin].objs[objn];
  TextBox_SetText(guisl, txbuf);
}


// *** LIST BOX FUNCTIONS

int ListBox_AddItem(GUIListBox *lbb, const char *text) {
  if (lbb->AddItem(text) < 0)
    return 0;

  guis_need_update = 1;
  return 1;
}

int ListBox_InsertItemAt(GUIListBox *lbb, int index, const char *text) {
  if (lbb->InsertItem(index, text) < 0)
    return 0;

  guis_need_update = 1;
  return 1;
}

void ListBox_Clear(GUIListBox *listbox) {
  listbox->Clear();
  guis_need_update = 1;
}

void ListBox_FillDirList(GUIListBox *listbox, const char *filemask) {
  char searchPath[MAX_PATH];
  validate_user_file_path(filemask, searchPath, false);

  listbox->Clear();
  al_ffblk dfb;
  int	dun = al_findfirst(searchPath, &dfb, FA_SEARCH);
  while (!dun) {
    listbox->AddItem(dfb.name);
    dun = al_findnext(&dfb);
  }
  al_findclose(&dfb);
  guis_need_update = 1;
}

int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBox.SaveGameSlot: index out of range");

  return listbox->saveGameIndex[index];
}

int ListBox_FillSaveGameList(GUIListBox *listbox) {
  listbox->Clear();

  int numsaves=0;
  int bufix=0;
  al_ffblk ffb; 
  long filedates[MAXSAVEGAMES];
  char buff[200];

  char searchPath[260];
  sprintf(searchPath, "%s""agssave.*", saveGameDirectory);

  int don = al_findfirst(searchPath, &ffb, FA_SEARCH);
  while (!don) {
    bufix=0;
    if (numsaves >= MAXSAVEGAMES)
      break;
    // only list games .000 to .099 (to allow higher slots for other perposes)
    if (strstr(ffb.name,".0")==NULL) {
      don = al_findnext(&ffb);
      continue;
    }
    const char *numberExtension = strstr(ffb.name, ".0") + 1;
    int saveGameSlot = atoi(numberExtension);
    GetSaveSlotDescription(saveGameSlot, buff);
    listbox->AddItem(buff);
    listbox->saveGameIndex[numsaves] = saveGameSlot;
    filedates[numsaves]=(long int)ffb.time;
    numsaves++;
    don = al_findnext(&ffb);
  }
  al_findclose(&ffb);

  int nn;
  for (nn=0;nn<numsaves-1;nn++) {
    for (int kk=0;kk<numsaves-1;kk++) {  // Date order the games

      if (filedates[kk] < filedates[kk+1]) {   // swap them round
        char*tempptr = listbox->items[kk];
        listbox->items[kk] = listbox->items[kk+1];
        listbox->items[kk+1] = tempptr;
        int numtem = listbox->saveGameIndex[kk];
        listbox->saveGameIndex[kk] = listbox->saveGameIndex[kk+1];
        listbox->saveGameIndex[kk+1] = numtem;
        long numted=filedates[kk]; filedates[kk]=filedates[kk+1];
        filedates[kk+1]=numted;
      }
    }
  }

  // update the global savegameindex[] array for backward compatibilty
  for (nn = 0; nn < numsaves; nn++) {
    play.filenumbers[nn] = listbox->saveGameIndex[nn];
  }

  guis_need_update = 1;
  listbox->exflags |= GLF_SGINDEXVALID;

  if (numsaves >= MAXSAVEGAMES)
    return 1;
  return 0;
}

int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y) {

  if (guis[listbox->guin].on < 1)
    return -1;

  multiply_up_coordinates(&x, &y);
  x = (x - listbox->x) - guis[listbox->guin].x;
  y = (y - listbox->y) - guis[listbox->guin].y;

  if ((x < 0) || (y < 0) || (x >= listbox->wid) || (y >= listbox->hit))
    return -1;
  
  return listbox->GetIndexFromCoordinates(x, y);
}

char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBoxGetItemText: invalid item specified");
  strncpy(buffer, listbox->items[index],198);
  buffer[199] = 0;
  return buffer;
}

const char* ListBox_GetItems(GUIListBox *listbox, int index) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBox.Items: invalid index specified");

  return CreateNewScriptString(listbox->items[index]);
}

void ListBox_SetItemText(GUIListBox *listbox, int index, const char *newtext) {
  if ((index < 0) || (index >= listbox->numItems))
    quit("!ListBoxSetItemText: invalid item specified");

  if (strcmp(listbox->items[index], newtext)) {
    listbox->SetItemText(index, newtext);
    guis_need_update = 1;
  }
}

void ListBox_RemoveItem(GUIListBox *listbox, int itemIndex) {
  
  if ((itemIndex < 0) || (itemIndex >= listbox->numItems))
    quit("!ListBoxRemove: invalid listindex specified");

  listbox->RemoveItem(itemIndex);
  guis_need_update = 1;
}

int ListBox_GetItemCount(GUIListBox *listbox) {
  return listbox->numItems;
}

int ListBox_GetFont(GUIListBox *listbox) {
  return listbox->font;
}

void ListBox_SetFont(GUIListBox *listbox, int newfont) {

  if ((newfont < 0) || (newfont >= game.numfonts))
    quit("!ListBox.Font: invalid font number.");

  if (newfont != listbox->font) {
    listbox->ChangeFont(newfont);
    guis_need_update = 1;
  }

}

int ListBox_GetHideBorder(GUIListBox *listbox) {
  return (listbox->exflags & GLF_NOBORDER) ? 1 : 0;
}

void ListBox_SetHideBorder(GUIListBox *listbox, int newValue) {
  listbox->exflags &= ~GLF_NOBORDER;
  if (newValue)
    listbox->exflags |= GLF_NOBORDER;
  guis_need_update = 1;
}

int ListBox_GetHideScrollArrows(GUIListBox *listbox) {
  return (listbox->exflags & GLF_NOARROWS) ? 1 : 0;
}

void ListBox_SetHideScrollArrows(GUIListBox *listbox, int newValue) {
  listbox->exflags &= ~GLF_NOARROWS;
  if (newValue)
    listbox->exflags |= GLF_NOARROWS;
  guis_need_update = 1;
}

int ListBox_GetSelectedIndex(GUIListBox *listbox) {
  if ((listbox->selected < 0) || (listbox->selected >= listbox->numItems))
    return -1;
  return listbox->selected;
}

void ListBox_SetSelectedIndex(GUIListBox *guisl, int newsel) {

  if (newsel >= guisl->numItems)
    newsel = -1;

  if (guisl->selected != newsel) {
    guisl->selected = newsel;
    if (newsel >= 0) {
      if (newsel < guisl->topItem)
        guisl->topItem = newsel;
      if (newsel >= guisl->topItem + guisl->num_items_fit)
        guisl->topItem = (newsel - guisl->num_items_fit) + 1;
    }
    guis_need_update = 1;
  }

}

int ListBox_GetTopItem(GUIListBox *listbox) {
  return listbox->topItem;
}

void ListBox_SetTopItem(GUIListBox *guisl, int item) {
  if ((guisl->numItems == 0) && (item == 0))
    ;  // allow resetting an empty box to the top
  else if ((item >= guisl->numItems) || (item < 0))
    quit("!ListBoxSetTopItem: tried to set top to beyond top or bottom of list");

  guisl->topItem = item;
  guis_need_update = 1;
}

int ListBox_GetRowCount(GUIListBox *listbox) {
  return listbox->num_items_fit;
}

void ListBox_ScrollDown(GUIListBox *listbox) {
  if (listbox->topItem + listbox->num_items_fit < listbox->numItems) {
    listbox->topItem++;
    guis_need_update = 1;
  }
}

void ListBox_ScrollUp(GUIListBox *listbox) {
  if (listbox->topItem > 0) {
    listbox->topItem--;
    guis_need_update = 1;
  }
}


GUIListBox* is_valid_listbox (int guin, int objn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!ListBox: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!ListBox: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LISTBOX)
    quit("!ListBox: specified control is not a list box");
  guis_need_update = 1;
  return (GUIListBox*)guis[guin].objs[objn];
}

void ListBoxClear(int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  ListBox_Clear(guisl);
}
void ListBoxAdd(int guin, int objn, const char*newitem) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  ListBox_AddItem(guisl, newitem);
}
void ListBoxRemove(int guin, int objn, int itemIndex) {
  GUIListBox*guisl = is_valid_listbox(guin,objn);
  ListBox_RemoveItem(guisl, itemIndex);
}
int ListBoxGetSelected(int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_GetSelectedIndex(guisl);
}
int ListBoxGetNumItems(int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_GetItemCount(guisl);
}
char* ListBoxGetItemText(int guin, int objn, int item, char*buffer) {
  VALIDATE_STRING(buffer);
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_GetItemText(guisl, item, buffer);
}
void ListBoxSetSelected(int guin, int objn, int newsel) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  ListBox_SetSelectedIndex(guisl, newsel);
}
void ListBoxSetTopItem (int guin, int objn, int item) {
  GUIListBox*guisl = is_valid_listbox(guin,objn);
  ListBox_SetTopItem(guisl, item);
}

int ListBoxSaveGameList (int guin, int objn) {
  GUIListBox*guisl=is_valid_listbox(guin,objn);
  return ListBox_FillSaveGameList(guisl);
}

void ListBoxDirList (int guin, int objn, const char*filemask) {
  GUIListBox *guisl = is_valid_listbox(guin,objn);
  ListBox_FillDirList(guisl, filemask);
}



// ** LABEL FUNCTIONS

const char* Label_GetText_New(GUILabel *labl) {
  return CreateNewScriptString(labl->GetText());
}

void Label_GetText(GUILabel *labl, char *buffer) {
  strcpy(buffer, labl->GetText());
}

void Label_SetText(GUILabel *labl, const char *newtx) {
  newtx = get_translation(newtx);

  if (strcmp(labl->GetText(), newtx)) {
    guis_need_update = 1;
    labl->SetText(newtx);
  }
}

int Label_GetColor(GUILabel *labl) {
  return labl->textcol;
}

void Label_SetColor(GUILabel *labl, int colr) {
  if (labl->textcol != colr) {
    labl->textcol = colr;
    guis_need_update = 1;
  }
}

int Label_GetFont(GUILabel *labl) {
  return labl->font;
}

void Label_SetFont(GUILabel *guil, int fontnum) {
  if ((fontnum < 0) || (fontnum >= game.numfonts))
    quit("!SetLabelFont: invalid font number.");

  if (fontnum != guil->font) {
    guil->font = fontnum;
    guis_need_update = 1;
  }
}


void SetLabelColor(int guin,int objn, int colr) {
  if ((guin<0) | (guin>=game.numgui))
    quit("!SetLabelColor: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs))
    quit("!SetLabelColor: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
    quit("!SetLabelColor: specified control is not a label");

  GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
  Label_SetColor(guil, colr);
}

void SetLabelText(int guin,int objn,char*newtx) {
  VALIDATE_STRING(newtx);
  if ((guin<0) | (guin>=game.numgui)) quit("!SetLabelText: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetLabelTexT: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
    quit("!SetLabelText: specified control is not a label");

  GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
  Label_SetText(guil, newtx);
}

void SetLabelFont(int guin,int objn, int fontnum) {

  if ((guin<0) | (guin>=game.numgui)) quit("!SetLabelFont: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetLabelFont: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_LABEL)
    quit("!SetLabelFont: specified control is not a label");

  GUILabel*guil=(GUILabel*)guis[guin].objs[objn];
  Label_SetFont(guil, fontnum);
}


// *** INV WINDOW FUNCTIONS

void InvWindow_SetCharacterToUse(GUIInv *guii, CharacterInfo *chaa) {
  if (chaa == NULL)
    guii->charId = -1;
  else
    guii->charId = chaa->index_id;
  // reset to top of list
  guii->topIndex = 0;

  guis_need_update = 1;
}

CharacterInfo* InvWindow_GetCharacterToUse(GUIInv *guii) {
  if (guii->charId < 0)
    return NULL;

  return &game.chars[guii->charId];
}

void InvWindow_SetItemWidth(GUIInv *guii, int newwidth) {
  guii->itemWidth = newwidth;
  guii->Resized();
}

int InvWindow_GetItemWidth(GUIInv *guii) {
  return guii->itemWidth;
}

void InvWindow_SetItemHeight(GUIInv *guii, int newhit) {
  guii->itemHeight = newhit;
  guii->Resized();
}

int InvWindow_GetItemHeight(GUIInv *guii) {
  return guii->itemHeight;
}

void InvWindow_SetTopItem(GUIInv *guii, int topitem) {
  if (guii->topIndex != topitem) {
    guii->topIndex = topitem;
    guis_need_update = 1;
  }
}

int InvWindow_GetTopItem(GUIInv *guii) {
  return guii->topIndex;
}

int InvWindow_GetItemsPerRow(GUIInv *guii) {
  return guii->itemsPerLine;
}

int InvWindow_GetItemCount(GUIInv *guii) {
  return charextra[guii->CharToDisplay()].invorder_count;
}

int InvWindow_GetRowCount(GUIInv *guii) {
  return guii->numLines;
}

void InvWindow_ScrollDown(GUIInv *guii) {
  if ((charextra[guii->CharToDisplay()].invorder_count) >
      (guii->topIndex + (guii->itemsPerLine * guii->numLines))) { 
    guii->topIndex += guii->itemsPerLine;
    guis_need_update = 1;
  }
}

void InvWindow_ScrollUp(GUIInv *guii) {
  if (guii->topIndex > 0) {
    guii->topIndex -= guii->itemsPerLine;
    if (guii->topIndex < 0)
      guii->topIndex = 0;

    guis_need_update = 1;
  }
}

ScriptInvItem* InvWindow_GetItemAtIndex(GUIInv *guii, int index) {
  if ((index < 0) || (index >= charextra[guii->CharToDisplay()].invorder_count))
    return NULL;
  return &scrInv[charextra[guii->CharToDisplay()].invorder[index]];
}


// *** SLIDER FUNCTIONS

void Slider_SetMax(GUISlider *guisl, int valn) {

  if (valn != guisl->max) {
    guisl->max = valn;

    if (guisl->value > guisl->max)
      guisl->value = guisl->max;
    if (guisl->min > guisl->max)
      quit("!Slider.Max: minimum cannot be greater than maximum");

    guis_need_update = 1;
  }

}

int Slider_GetMax(GUISlider *guisl) {
  return guisl->max;
}

void Slider_SetMin(GUISlider *guisl, int valn) {

  if (valn != guisl->min) {
    guisl->min = valn;

    if (guisl->value < guisl->min)
      guisl->value = guisl->min;
    if (guisl->min > guisl->max)
      quit("!Slider.Min: minimum cannot be greater than maximum");

    guis_need_update = 1;
  }

}

int Slider_GetMin(GUISlider *guisl) {
  return guisl->min;
}

void Slider_SetValue(GUISlider *guisl, int valn) {
  if (valn > guisl->max) valn = guisl->max;
  if (valn < guisl->min) valn = guisl->min;

  if (valn != guisl->value) {
    guisl->value = valn;
    guis_need_update = 1;
  }
}

int Slider_GetValue(GUISlider *guisl) {
  return guisl->value;
}

void SetSliderValue(int guin,int objn, int valn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!SetSliderValue: invalid GUI number");
  if (guis[guin].get_control_type(objn)!=GOBJ_SLIDER)
    quit("!SetSliderValue: specified control is not a slider");

  GUISlider*guisl=(GUISlider*)guis[guin].objs[objn];
  Slider_SetValue(guisl, valn);
}

int GetSliderValue(int guin,int objn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!GetSliderValue: invalid GUI number");
  if (guis[guin].get_control_type(objn)!=GOBJ_SLIDER)
    quit("!GetSliderValue: specified control is not a slider");

  GUISlider*guisl=(GUISlider*)guis[guin].objs[objn];
  return Slider_GetValue(guisl);
}

int Slider_GetBackgroundGraphic(GUISlider *guisl) {
  return (guisl->bgimage > 0) ? guisl->bgimage : 0;
}

void Slider_SetBackgroundGraphic(GUISlider *guisl, int newImage) 
{
  if (newImage != guisl->bgimage)
  {
    guisl->bgimage = newImage;
    guis_need_update = 1;
  }
}

int Slider_GetHandleGraphic(GUISlider *guisl) {
  return (guisl->handlepic > 0) ? guisl->handlepic : 0;
}

void Slider_SetHandleGraphic(GUISlider *guisl, int newImage) 
{
  if (newImage != guisl->handlepic)
  {
    guisl->handlepic = newImage;
    guis_need_update = 1;
  }
}

int Slider_GetHandleOffset(GUISlider *guisl) {
  return guisl->handleoffset;
}

void Slider_SetHandleOffset(GUISlider *guisl, int newOffset) 
{
  if (newOffset != guisl->handleoffset)
  {
    guisl->handleoffset = newOffset;
    guis_need_update = 1;
  }
}

void GUI_SetBackgroundGraphic(ScriptGUI *tehgui, int slotn) {
  if (tehgui->gui->bgpic != slotn) {
    tehgui->gui->bgpic = slotn;
    guis_need_update = 1;
  }
}

int GUI_GetBackgroundGraphic(ScriptGUI *tehgui) {
  if (tehgui->gui->bgpic < 1)
    return 0;
  return tehgui->gui->bgpic;
}

void SetGUIBackgroundPic (int guin, int slotn) {
  if ((guin<0) | (guin>=game.numgui))
    quit("!SetGUIBackgroundPic: invalid GUI number");

  GUI_SetBackgroundGraphic(&scrGui[guin], slotn);
}


// *** BUTTON FUNCTIONS


void Button_Animate(GUIButton *butt, int view, int loop, int speed, int repeat) {
  int guin = butt->guin;
  int objn = butt->objn;
  
  if ((view < 1) || (view > game.numviews))
    quit("!AnimateButton: invalid view specified");
  view--;

  if ((loop < 0) || (loop >= views[view].numLoops))
    quit("!AnimateButton: invalid loop specified for view");

  // if it's already animating, stop it
  FindAndRemoveButtonAnimation(guin, objn);

  if (numAnimButs >= MAX_ANIMATING_BUTTONS)
    quit("!AnimateButton: too many animating GUI buttons at once");

  int buttonId = guis[guin].objrefptr[objn] & 0x000ffff;

  guibuts[buttonId].pushedpic = 0;
  guibuts[buttonId].overpic = 0;

  animbuts[numAnimButs].ongui = guin;
  animbuts[numAnimButs].onguibut = objn;
  animbuts[numAnimButs].buttonid = buttonId;
  animbuts[numAnimButs].view = view;
  animbuts[numAnimButs].loop = loop;
  animbuts[numAnimButs].speed = speed;
  animbuts[numAnimButs].repeat = repeat;
  animbuts[numAnimButs].frame = -1;
  animbuts[numAnimButs].wait = 0;
  numAnimButs++;
  // launch into the first frame
  if (UpdateAnimatingButton(numAnimButs - 1))
    quit("!AnimateButton: no frames to animate");
}

const char* Button_GetText_New(GUIButton *butt) {
  return CreateNewScriptString(butt->text);
}

void Button_GetText(GUIButton *butt, char *buffer) {
  strcpy(buffer, butt->text);
}

void Button_SetText(GUIButton *butt, const char *newtx) {
  newtx = get_translation(newtx);
  if (strlen(newtx) > 49) quit("!SetButtonText: text too long, button has 50 chars max");

  if (strcmp(butt->text, newtx)) {
    guis_need_update = 1;
    strcpy(butt->text,newtx);
  }
}

void Button_SetFont(GUIButton *butt, int newFont) {
  if ((newFont < 0) || (newFont >= game.numfonts))
    quit("!Button.Font: invalid font number.");

  if (butt->font != newFont) {
    butt->font = newFont;
    guis_need_update = 1;
  }
}

int Button_GetFont(GUIButton *butt) {
  return butt->font;
}

int Button_GetClipImage(GUIButton *butt) {
  if (butt->flags & GUIF_CLIP)
    return 1;
  return 0;
}

void Button_SetClipImage(GUIButton *butt, int newval) {
  butt->flags &= ~GUIF_CLIP;
  if (newval)
    butt->flags |= GUIF_CLIP;

  guis_need_update = 1;
}

int Button_GetGraphic(GUIButton *butt) {
  // return currently displayed pic
  if (butt->usepic < 0)
    return butt->pic;
  return butt->usepic;
}

int Button_GetMouseOverGraphic(GUIButton *butt) {
  return butt->overpic;
}

void Button_SetMouseOverGraphic(GUIButton *guil, int slotn) {
  DEBUG_CONSOLE("GUI %d Button %d mouseover set to slot %d", guil->guin, guil->objn, slotn);

  if ((guil->isover != 0) && (guil->ispushed == 0))
    guil->usepic = slotn;
  guil->overpic = slotn;

  guis_need_update = 1;
  FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetNormalGraphic(GUIButton *butt) {
  return butt->pic;
}

void Button_SetNormalGraphic(GUIButton *guil, int slotn) {
  DEBUG_CONSOLE("GUI %d Button %d normal set to slot %d", guil->guin, guil->objn, slotn);
  // normal pic - update if mouse is not over, or if there's no overpic
  if (((guil->isover == 0) || (guil->overpic < 1)) && (guil->ispushed == 0))
    guil->usepic = slotn;
  guil->pic = slotn;
  // update the clickable area to the same size as the graphic
  guil->wid = spritewidth[slotn];
  guil->hit = spriteheight[slotn];

  guis_need_update = 1;
  FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetPushedGraphic(GUIButton *butt) {
  return butt->pushedpic;
}

void Button_SetPushedGraphic(GUIButton *guil, int slotn) {
  DEBUG_CONSOLE("GUI %d Button %d pushed set to slot %d", guil->guin, guil->objn, slotn);

  if (guil->ispushed)
    guil->usepic = slotn;
  guil->pushedpic = slotn;

  guis_need_update = 1;
  FindAndRemoveButtonAnimation(guil->guin, guil->objn);
}

int Button_GetTextColor(GUIButton *butt) {
  return butt->textcol;
}

void Button_SetTextColor(GUIButton *butt, int newcol) {
  if (butt->textcol != newcol) {
    butt->textcol = newcol;
    guis_need_update = 1;
  }
}

void SetButtonText(int guin,int objn,char*newtx) {
  VALIDATE_STRING(newtx);
  if ((guin<0) | (guin>=game.numgui))
    quit("!SetButtonText: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs))
    quit("!SetButtonText: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
    quit("!SetButtonText: specified control is not a button");

  GUIButton*guil=(GUIButton*)guis[guin].objs[objn];
  Button_SetText(guil, newtx);
}


void AnimateButton(int guin, int objn, int view, int loop, int speed, int repeat) {
  if ((guin<0) | (guin>=game.numgui)) quit("!AnimateButton: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!AnimateButton: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
    quit("!AnimateButton: specified control is not a button");

  Button_Animate((GUIButton*)guis[guin].objs[objn], view, loop, speed, repeat);
}


int GetButtonPic(int guin, int objn, int ptype) {
  if ((guin<0) | (guin>=game.numgui)) quit("!GetButtonPic: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!GetButtonPic: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
    quit("!GetButtonPic: specified control is not a button");
  if ((ptype < 0) | (ptype > 3)) quit("!GetButtonPic: invalid pic type");

  GUIButton*guil=(GUIButton*)guis[guin].objs[objn];

  if (ptype == 0) {
    // currently displayed pic
    if (guil->usepic < 0)
      return guil->pic;
    return guil->usepic;
  }
  else if (ptype==1) {
    // nomal pic
    return guil->pic;
  }
  else if (ptype==2) {
    // mouseover pic
    return guil->overpic;
  }
  else { // pushed pic
    return guil->pushedpic;
  }

  quit("internal error in getbuttonpic");
}

void SetButtonPic(int guin,int objn,int ptype,int slotn) {
  if ((guin<0) | (guin>=game.numgui)) quit("!SetButtonPic: invalid GUI number");
  if ((objn<0) | (objn>=guis[guin].numobjs)) quit("!SetButtonPic: invalid object number");
  if (guis[guin].get_control_type(objn)!=GOBJ_BUTTON)
    quit("!SetButtonPic: specified control is not a button");
  if ((ptype<1) | (ptype>3)) quit("!SetButtonPic: invalid pic type");

  GUIButton*guil=(GUIButton*)guis[guin].objs[objn];
  if (ptype==1) {
    Button_SetNormalGraphic(guil, slotn);
  }
  else if (ptype==2) {
    // mouseover pic
    Button_SetMouseOverGraphic(guil, slotn);
  }
  else { // pushed pic
    Button_SetPushedGraphic(guil, slotn);
  }
}

void DisableInterface() {
  play.disabled_user_interface++;
  guis_need_update = 1;
  set_mouse_cursor(CURS_WAIT);
  }
void EnableInterface() {
  guis_need_update = 1;
  play.disabled_user_interface--;
  if (play.disabled_user_interface<1) {
    play.disabled_user_interface=0;
    set_default_cursor();
    }
  }
// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled() {
  return (play.disabled_user_interface > 0) ? 0 : 1;
}

void NewRoom(int nrnum) {
  if (nrnum < 0)
    quitprintf("!NewRoom: room change requested to invalid room number %d.", nrnum);

  if (displayed_room < 0) {
    // called from game_start; change the room where the game will start
    playerchar->room = nrnum;
    return;
  }

  
  DEBUG_CONSOLE("Room change requested to room %d", nrnum);
  EndSkippingUntilCharStops();

  can_run_delayed_command();

  if (play.stop_dialog_at_end != DIALOG_NONE) {
    if (play.stop_dialog_at_end == DIALOG_RUNNING)
      play.stop_dialog_at_end = DIALOG_NEWROOM + nrnum;
    else
      quit("!NewRoom: two NewRoom/RunDialog/StopDialog requests within dialog");
    return;
  }

  if (in_leaves_screen >= 0) {
    // NewRoom called from the Player Leaves Screen event -- just
    // change which room it will go to
    in_leaves_screen = nrnum;
  }
  else if (in_enters_screen) {
    setevent(EV_NEWROOM,nrnum);
    return;
  }
  else if (in_inv_screen) {
    inv_screen_newroom = nrnum;
    return;
  }
  else if ((inside_script==0) & (in_graph_script==0)) {
    new_room(nrnum,playerchar);
    return;
  }
  else if (inside_script) {
    curscript->queue_action(ePSANewRoom, nrnum, "NewRoom");
    // we might be within a MoveCharacterBlocking -- the room
    // change should abort it
    if ((playerchar->walking > 0) && (playerchar->walking < TURNING_AROUND)) {
      // nasty hack - make sure it doesn't move the character
      // to a walkable area
      mls[playerchar->walking].direct = 1;
      StopMoving(game.playercharacter);
    }
  }
  else if (in_graph_script)
    gs_to_newroom = nrnum;
}

void NewRoomEx(int nrnum,int newx,int newy) {

  Character_ChangeRoom(playerchar, nrnum, newx, newy);

}

void NewRoomNPC(int charid, int nrnum, int newx, int newy) {
  if (!is_valid_character(charid))
    quit("!NewRoomNPC: invalid character");
  if (charid == game.playercharacter)
    quit("!NewRoomNPC: use NewRoomEx with the player character");

  Character_ChangeRoom(&game.chars[charid], nrnum, newx, newy);
}

void ResetRoom(int nrnum) {
  if (nrnum == displayed_room)
    quit("!ResetRoom: cannot reset current room");
  if ((nrnum<0) | (nrnum>=MAX_ROOMS))
    quit("!ResetRoom: invalid room number");
  if (roomstats[nrnum].beenhere) {
    if (roomstats[nrnum].tsdata!=NULL)
      free(roomstats[nrnum].tsdata);
    roomstats[nrnum].tsdata=NULL;
    roomstats[nrnum].tsdatasize=0;
    }
  roomstats[nrnum].beenhere=0;
  DEBUG_CONSOLE("Room %d reset to original state", nrnum);
}

int HasPlayerBeenInRoom(int roomnum) {
  if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
    return 0;
  return roomstats[roomnum].beenhere;
}

void SetRestartPoint() {
  save_game(RESTART_POINT_SAVE_GAME_NUMBER, "Restart Game Auto-Save");
}

void CallRoomScript (int value) {
  can_run_delayed_command();

  if (!inside_script)
    quit("!CallRoomScript: not inside a script???");

  play.roomscript_finished = 0;
  curscript->run_another("$on_call", value, 0);
}

void SetGameSpeed(int newspd) {
  // if Ctrl+E has been used to max out frame rate, lock it there
  if ((frames_per_second == 1000) && (display_fps == 2))
    return;

  newspd += play.game_speed_modifier;
  if (newspd>1000) newspd=1000;
  if (newspd<10) newspd=10;
  set_game_speed(newspd);
  DEBUG_CONSOLE("Game speed set to %d", newspd);
}

int GetGameSpeed() {
  return frames_per_second - play.game_speed_modifier;
}

int SetGameOption (int opt, int setting) {
  if (((opt < 1) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    quit("!SetGameOption: invalid option specified");

  if (opt == OPT_ANTIGLIDE)
  {
    for (int i = 0; i < game.numcharacters; i++)
    {
      if (setting)
        game.chars[i].flags |= CHF_ANTIGLIDE;
      else
        game.chars[i].flags &= ~CHF_ANTIGLIDE;
    }
  }

  if ((opt == OPT_CROSSFADEMUSIC) && (game.audioClipTypeCount > AUDIOTYPE_LEGACY_MUSIC))
  {
    // legacy compatibility -- changing crossfade speed here also
    // updates the new audio clip type style
    game.audioClipTypes[AUDIOTYPE_LEGACY_MUSIC].crossfadeSpeed = setting;
  }
  
  int oldval = game.options[opt];
  game.options[opt] = setting;

  if (opt == OPT_DUPLICATEINV)
    update_invorder();
  else if (opt == OPT_DISABLEOFF)
    gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
  else if (opt == OPT_PORTRAITSIDE) {
    if (setting == 0)  // set back to Left
      play.swap_portrait_side = 0;
  }

  return oldval;
}

int GetGameOption (int opt) {
  if (((opt < 1) || (opt > OPT_HIGHESTOPTION)) && (opt != OPT_LIPSYNCTEXT))
    quit("!GetGameOption: invalid option specified");

  return game.options[opt];
}

void StopDialog() {
  if (play.stop_dialog_at_end == DIALOG_NONE) {
    debug_log("StopDialog called, but was not in a dialog");
    DEBUG_CONSOLE("StopDialog called but no dialog");
    return;
  }
  play.stop_dialog_at_end = DIALOG_STOP;
}

void SetDialogOption(int dlg,int opt,int onoroff) {
  if ((dlg<0) | (dlg>=game.numdialog))
    quit("!SetDialogOption: Invalid topic number specified");
  if ((opt<1) | (opt>dialog[dlg].numoptions))
    quit("!SetDialogOption: Invalid option number specified");
  opt--;

  dialog[dlg].optionflags[opt]&=~DFLG_ON;
  if ((onoroff==1) & ((dialog[dlg].optionflags[opt] & DFLG_OFFPERM)==0))
    dialog[dlg].optionflags[opt]|=DFLG_ON;
  else if (onoroff==2)
    dialog[dlg].optionflags[opt]|=DFLG_OFFPERM;
}

int GetDialogOption (int dlg, int opt) {
  if ((dlg<0) | (dlg>=game.numdialog))
    quit("!GetDialogOption: Invalid topic number specified");
  if ((opt<1) | (opt>dialog[dlg].numoptions))
    quit("!GetDialogOption: Invalid option number specified");
  opt--;

  if (dialog[dlg].optionflags[opt] & DFLG_OFFPERM)
    return 2;
  if (dialog[dlg].optionflags[opt] & DFLG_ON)
    return 1;
  return 0;
}

int Game_GetDialogCount()
{
  return game.numdialog;
}

void Dialog_Start(ScriptDialog *sd) {
  RunDialog(sd->id);
}

#define CHOSE_TEXTPARSER -3053
#define SAYCHOSEN_USEFLAG 1
#define SAYCHOSEN_YES 2
#define SAYCHOSEN_NO  3 

int Dialog_DisplayOptions(ScriptDialog *sd, int sayChosenOption) 
{
  if ((sayChosenOption < 1) || (sayChosenOption > 3))
    quit("!Dialog.DisplayOptions: invalid parameter passed");

  int chose = show_dialog_options(sd->id, sayChosenOption, (game.options[OPT_RUNGAMEDLGOPTS] != 0));
  if (chose != CHOSE_TEXTPARSER)
  {
    chose++;
  }
  return chose;
}

void Dialog_SetOptionState(ScriptDialog *sd, int option, int newState) {
  SetDialogOption(sd->id, option, newState);
}

int Dialog_GetOptionState(ScriptDialog *sd, int option) {
  return GetDialogOption(sd->id, option);
}

int Dialog_HasOptionBeenChosen(ScriptDialog *sd, int option)
{
  if ((option < 1) || (option > dialog[sd->id].numoptions))
    quit("!Dialog.HasOptionBeenChosen: Invalid option number specified");
  option--;

  if (dialog[sd->id].optionflags[option] & DFLG_HASBEENCHOSEN)
    return 1;
  return 0;
}

int Dialog_GetOptionCount(ScriptDialog *sd)
{
  return dialog[sd->id].numoptions;
}

int Dialog_GetShowTextParser(ScriptDialog *sd)
{
  return (dialog[sd->id].topicFlags & DTFLG_SHOWPARSER) ? 1 : 0;
}

const char* Dialog_GetOptionText(ScriptDialog *sd, int option)
{
  if ((option < 1) || (option > dialog[sd->id].numoptions))
    quit("!Dialog.GetOptionText: Invalid option number specified");

  option--;

  return CreateNewScriptString(get_translation(dialog[sd->id].optionnames[option]));
}

int Dialog_GetID(ScriptDialog *sd) {
  return sd->id;
}

void ShakeScreen(int severe) {
  EndSkippingUntilCharStops();

  if (play.fast_forward)
    return;

  int hh;
  block oldsc=abuf;
  severe = multiply_up_coordinate(severe);

  if (gfxDriver->RequiresFullRedrawEachFrame())
  {
    play.shakesc_length = 10;
    play.shakesc_delay = 2;
    play.shakesc_amount = severe;
    play.mouse_cursor_hidden++;

    for (hh = 0; hh < 40; hh++) {
      loopcounter++;
      platform->Delay(50);

      render_graphics();

      update_polled_stuff();
    }

    play.mouse_cursor_hidden--;
    clear_letterbox_borders();
    play.shakesc_length = 0;
  }
  else
  {
    block tty = create_bitmap(scrnwid, scrnhit);
    gfxDriver->GetCopyOfScreenIntoBitmap(tty);
    for (hh=0;hh<40;hh++) {
      platform->Delay(50);

      if (hh % 2 == 0) 
        render_to_screen(tty, 0, 0);
      else
        render_to_screen(tty, 0, severe);

      update_polled_stuff();
    }
    clear_letterbox_borders();
    render_to_screen(tty, 0, 0);
    wfreeblock(tty);
  }

  abuf=oldsc;
}

void ShakeScreenBackground (int delay, int amount, int length) {
  if (delay < 2) 
    quit("!ShakeScreenBackground: invalid delay parameter");

  if (amount < play.shakesc_amount)
  {
    // from a bigger to smaller shake, clear up the borders
    clear_letterbox_borders();
  }

  play.shakesc_amount = amount;
  play.shakesc_delay = delay;
  play.shakesc_length = length;
}

void CyclePalette(int strt,int eend) {
  // hi-color game must invalidate screen since the palette changes
  // the effect of the drawing operations
  if (game.color_depth > 1)
    invalidate_screen();

  if ((strt < 0) || (strt > 255) || (eend < 0) || (eend > 255))
    quit("!CyclePalette: start and end must be 0-255");

  if (eend > strt) {
    // forwards
    wcolrotate(strt, eend, 0, palette);
    wsetpalette(strt, eend, palette);
  }
  else {
    // backwards
    wcolrotate(eend, strt, 1, palette);
    wsetpalette(eend, strt, palette);
  }
  
}
void SetPalRGB(int inndx,int rr,int gg,int bb) {
  if (game.color_depth > 1)
    invalidate_screen();

  wsetrgb(inndx,rr,gg,bb,palette);
  wsetpalette(inndx,inndx,palette);
}
/*void scSetPal(color*pptr) {
  wsetpalette(0,255,pptr);
  }
void scGetPal(color*pptr) {
  get_palette(pptr);
  }*/
void FadeIn(int sppd) {
  EndSkippingUntilCharStops();

  if (play.fast_forward)
    return;

  my_fade_in(palette,sppd);
}

int __Rand(int upto) {
  upto++;
  if (upto < 1)
    quit("!Random: invalid parameter passed -- must be at least 0.");
  return rand()%upto;
  }

void RefreshMouse() {
  domouse(DOMOUSE_NOCURSOR);
  scmouse.x = divide_down_coordinate(mousex);
  scmouse.y = divide_down_coordinate(mousey);
}

void SetMousePosition (int newx, int newy) {
  if (newx < 0)
    newx = 0;
  if (newy < 0)
    newy = 0;
  if (newx >= BASEWIDTH)
    newx = BASEWIDTH - 1;
  if (newy >= GetMaxScreenHeight())
    newy = GetMaxScreenHeight() - 1;

  multiply_up_coordinates(&newx, &newy);
  filter->SetMousePosition(newx, newy);
  RefreshMouse();
}

int GetCursorMode() {
  return cur_mode;
}

int GetMouseCursor() {
  return cur_cursor;
}

void GiveScore(int amnt) 
{
  guis_need_update = 1;
  play.score += amnt;

  if ((amnt > 0) && (play.score_sound >= 0))
    play_audio_clip_by_index(play.score_sound);

  run_on_event (GE_GOT_SCORE, amnt);
}

int GetHotspotPointX (int hotspot) {
  if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
    quit("!GetHotspotPointX: invalid hotspot");

  if (thisroom.hswalkto[hotspot].x < 1)
    return -1;

  return thisroom.hswalkto[hotspot].x;
}

int Hotspot_GetWalkToX(ScriptHotspot *hss) {
  return GetHotspotPointX(hss->id);
}

int GetHotspotPointY (int hotspot) {
  if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
    quit("!GetHotspotPointY: invalid hotspot");

  if (thisroom.hswalkto[hotspot].x < 1)
    return -1;

  return thisroom.hswalkto[hotspot].y;
}

int Hotspot_GetWalkToY(ScriptHotspot *hss) {
  return GetHotspotPointY(hss->id);
}

int GetHotspotAt(int xxx,int yyy) {
  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;
  return get_hotspot_at(xxx,yyy);
}

ScriptHotspot *GetHotspotAtLocation(int xx, int yy) {
  int hsnum = GetHotspotAt(xx, yy);
  if (hsnum <= 0)
    return &scrHotspot[0];
  return &scrHotspot[hsnum];
}

int GetWalkableAreaAt(int xxx,int yyy) {
  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;
  int result = get_walkable_area_pixel(xxx, yyy);
  if (result <= 0)
    return 0;
  return result;
}

// allowHotspot0 defines whether Hotspot 0 returns LOCTYPE_HOTSPOT
// or whether it returns 0
int __GetLocationType(int xxx,int yyy, int allowHotspot0) {
  getloctype_index = 0;
  // If it's not in ProcessClick, then return 0 when over a GUI
  if ((GetGUIAt(xxx, yyy) >= 0) && (getloctype_throughgui == 0))
    return 0;

  getloctype_throughgui = 0;

  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;

  // check characters, objects and walkbehinds, work out which is
  // foremost visible to the player
  int charat = is_pos_on_character(xxx,yyy);
  int hsat = get_hotspot_at(xxx,yyy);
  int objat = GetObjectAt(xxx - divide_down_coordinate(offsetx), yyy - divide_down_coordinate(offsety));

  multiply_up_coordinates(&xxx, &yyy);

  int wbat = getpixel(thisroom.object, xxx, yyy);

  if (wbat <= 0) wbat = 0;
  else wbat = croom->walkbehind_base[wbat];

  int winner = 0;
  // if it's an Ignore Walkbehinds object, then ignore the walkbehind
  if ((objat >= 0) && ((objs[objat].flags & OBJF_NOWALKBEHINDS) != 0))
    wbat = 0;
  if ((charat >= 0) && ((game.chars[charat].flags & CHF_NOWALKBEHINDS) != 0))
    wbat = 0;
  
  if ((charat >= 0) && (objat >= 0)) {
    if ((wbat > obj_lowest_yp) && (wbat > char_lowest_yp))
      winner = LOCTYPE_HOTSPOT;
    else if (obj_lowest_yp > char_lowest_yp)
      winner = LOCTYPE_OBJ;
    else
      winner = LOCTYPE_CHAR;
  }
  else if (charat >= 0) {
    if (wbat > char_lowest_yp)
      winner = LOCTYPE_HOTSPOT;
    else
      winner = LOCTYPE_CHAR;
  }
  else if (objat >= 0) {
    if (wbat > obj_lowest_yp)
      winner = LOCTYPE_HOTSPOT;
    else
      winner = LOCTYPE_OBJ;
  }

  if (winner == 0) {
    if (hsat >= 0)
      winner = LOCTYPE_HOTSPOT;
  }

  if ((winner == LOCTYPE_HOTSPOT) && (!allowHotspot0) && (hsat == 0))
    winner = 0;

  if (winner == LOCTYPE_HOTSPOT)
    getloctype_index = hsat;
  else if (winner == LOCTYPE_CHAR)
    getloctype_index = charat;
  else if (winner == LOCTYPE_OBJ)
    getloctype_index = objat;

  return winner;
}

// GetLocationType exported function - just call through
// to the main function with default 0
int GetLocationType(int xxx,int yyy) {
  return __GetLocationType(xxx, yyy, 0);
}

int GetInvAt (int xxx, int yyy) {
  int ongui = GetGUIAt (xxx, yyy);
  if (ongui >= 0) {
    int mxwas = mousex, mywas = mousey;
    mousex = multiply_up_coordinate(xxx) - guis[ongui].x;
    mousey = multiply_up_coordinate(yyy) - guis[ongui].y;
    int onobj = guis[ongui].find_object_under_mouse();
    if (onobj>=0) {
      mouse_ifacebut_xoffs = mousex-(guis[ongui].objs[onobj]->x);
      mouse_ifacebut_yoffs = mousey-(guis[ongui].objs[onobj]->y);
    }
    mousex = mxwas;
    mousey = mywas;
    if ((onobj>=0) && ((guis[ongui].objrefptr[onobj] >> 16)==GOBJ_INVENTORY))
      return offset_over_inv((GUIInv*)guis[ongui].objs[onobj]);
  }
  return -1;
}

ScriptInvItem *GetInvAtLocation(int xx, int yy) {
  int hsnum = GetInvAt(xx, yy);
  if (hsnum <= 0)
    return NULL;
  return &scrInv[hsnum];
}

void SaveCursorForLocationChange() {
  // update the current location name
  char tempo[100];
  GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);

  if (play.get_loc_name_save_cursor != play.get_loc_name_last_time) {
    play.get_loc_name_save_cursor = play.get_loc_name_last_time;
    play.restore_cursor_mode_to = GetCursorMode();
    play.restore_cursor_image_to = GetMouseCursor();
    DEBUG_CONSOLE("Saving mouse: mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
  }
}

void GetObjectName(int obj, char *buffer) {
  VALIDATE_STRING(buffer);
  if (!is_valid_object(obj))
    quit("!GetObjectName: invalid object number");

  strcpy(buffer, get_translation(thisroom.objectnames[obj]));
}

void Object_GetName(ScriptObject *objj, char *buffer) {
  GetObjectName(objj->id, buffer);
}

const char* Object_GetName_New(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.Name: invalid object number");

  return CreateNewScriptString(get_translation(thisroom.objectnames[objj->id]));
}

void GetHotspotName(int hotspot, char *buffer) {
  VALIDATE_STRING(buffer);
  if ((hotspot < 0) || (hotspot >= MAX_HOTSPOTS))
    quit("!GetHotspotName: invalid hotspot number");

  strcpy(buffer, get_translation(thisroom.hotspotnames[hotspot]));
}

void Hotspot_GetName(ScriptHotspot *hss, char *buffer) {
  GetHotspotName(hss->id, buffer);
}

const char* Hotspot_GetName_New(ScriptHotspot *hss) {
  return CreateNewScriptString(get_translation(thisroom.hotspotnames[hss->id]));
}

void GetLocationName(int xxx,int yyy,char*tempo) {
  if (displayed_room < 0)
    quit("!GetLocationName: no room has been loaded");

  VALIDATE_STRING(tempo);
  
  if (GetGUIAt(xxx, yyy) >= 0) {
    tempo[0]=0;
    int mover = GetInvAt (xxx, yyy);
    if (mover > 0) {
      if (play.get_loc_name_last_time != 1000 + mover)
        guis_need_update = 1;
      play.get_loc_name_last_time = 1000 + mover;
      strcpy(tempo,get_translation(game.invinfo[mover].name));
    }
    else if ((play.get_loc_name_last_time > 1000) && (play.get_loc_name_last_time < 1000 + MAX_INV)) {
      // no longer selecting an item
      guis_need_update = 1;
      play.get_loc_name_last_time = -1;
    }
    return;
  }
  int loctype = GetLocationType (xxx, yyy);
  xxx += divide_down_coordinate(offsetx); 
  yyy += divide_down_coordinate(offsety);
  tempo[0]=0;
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return;

  int onhs,aa;
  if (loctype == 0) {
    if (play.get_loc_name_last_time != 0) {
      play.get_loc_name_last_time = 0;
      guis_need_update = 1;
    }
    return;
  }

  // on character
  if (loctype == LOCTYPE_CHAR) {
    onhs = getloctype_index;
    strcpy(tempo,get_translation(game.chars[onhs].name));
    if (play.get_loc_name_last_time != 2000+onhs)
      guis_need_update = 1;
    play.get_loc_name_last_time = 2000+onhs;
    return;
  }
  // on object
  if (loctype == LOCTYPE_OBJ) {
    aa = getloctype_index;
    strcpy(tempo,get_translation(thisroom.objectnames[aa]));
    if (play.get_loc_name_last_time != 3000+aa)
      guis_need_update = 1;
    play.get_loc_name_last_time = 3000+aa;
    return;
  }
  onhs = getloctype_index;
  if (onhs>0) strcpy(tempo,get_translation(thisroom.hotspotnames[onhs]));
  if (play.get_loc_name_last_time != onhs)
    guis_need_update = 1;
  play.get_loc_name_last_time = onhs;
}

const char* Game_GetLocationName(int x, int y) {
  char buffer[STD_BUFFER_SIZE];
  GetLocationName(x, y, buffer);
  return CreateNewScriptString(buffer);
}

void GetInvName(int indx,char*buff) {
  VALIDATE_STRING(buff);
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvName: invalid inventory item specified");
  strcpy(buff,get_translation(game.invinfo[indx].name));
}

void InventoryItem_GetName(ScriptInvItem *iitem, char *buff) {
  GetInvName(iitem->id, buff);
}

const char* InventoryItem_GetName_New(ScriptInvItem *invitem) {
  return CreateNewScriptString(get_translation(game.invinfo[invitem->id].name));
}

int GetInvGraphic(int indx) {
  if ((indx<0) | (indx>=game.numinvitems)) quit("!GetInvGraphic: invalid inventory item specified");

  return game.invinfo[indx].pic;
}

int InventoryItem_GetGraphic(ScriptInvItem *iitem) {
  return game.invinfo[iitem->id].pic;
}

void MoveCharacter(int cc,int xx,int yy) {
  walk_character(cc,xx,yy,0, true);
}
void MoveCharacterDirect(int cc,int xx, int yy) {
  walk_character(cc,xx,yy,1, true);
}
void MoveCharacterStraight(int cc,int xx, int yy) {
  if (!is_valid_character(cc))
    quit("!MoveCharacterStraight: invalid character specified");
  
  Character_WalkStraight(&game.chars[cc], xx, yy, IN_BACKGROUND);
}

// Append to character path
void MoveCharacterPath (int chac, int tox, int toy) {
  if (!is_valid_character(chac))
    quit("!MoveCharacterPath: invalid character specified");

  Character_AddWaypoint(&game.chars[chac], tox, toy);
}


void MoveObject(int objj,int xx,int yy,int spp) {
  move_object(objj,xx,yy,spp,0);
  }
void MoveObjectDirect(int objj,int xx,int yy,int spp) {
  move_object(objj,xx,yy,spp,1);
  }

void Object_Move(ScriptObject *objj, int x, int y, int speed, int blocking, int direct) {
  if ((direct == ANYWHERE) || (direct == 1))
    direct = 1;
  else if ((direct == WALKABLE_AREAS) || (direct == 0))
    direct = 0;
  else
    quit("Object.Move: invalid DIRECT parameter");

  move_object(objj->id, x, y, speed, direct);

  if ((blocking == BLOCKING) || (blocking == 1))
    do_main_cycle(UNTIL_SHORTIS0,(int)&objj->obj->moving);
  else if ((blocking != IN_BACKGROUND) && (blocking != 0))
    quit("Object.Move: invalid BLOCKING paramter");
}

int GetPlayerCharacter() {
  return game.playercharacter;
  }

void SetCharacterSpeedEx(int chaa, int xspeed, int yspeed) {
  if (!is_valid_character(chaa))
    quit("!SetCharacterSpeedEx: invalid character");

  Character_SetSpeed(&game.chars[chaa], xspeed, yspeed);

}

void SetCharacterSpeed(int chaa,int nspeed) {
  SetCharacterSpeedEx(chaa, nspeed, nspeed);
}

void SetTalkingColor(int chaa,int ncol) {
  if (!is_valid_character(chaa)) quit("!SetTalkingColor: invalid character");
  
  Character_SetSpeechColor(&game.chars[chaa], ncol);
}

void SetCharacterSpeechView (int chaa, int vii) {
  if (!is_valid_character(chaa))
    quit("!SetCharacterSpeechView: invalid character specified");
  
  Character_SetSpeechView(&game.chars[chaa], vii);
}

void SetCharacterBlinkView (int chaa, int vii, int intrv) {
  if (!is_valid_character(chaa))
    quit("!SetCharacterBlinkView: invalid character specified");

  Character_SetBlinkView(&game.chars[chaa], vii);
  Character_SetBlinkInterval(&game.chars[chaa], intrv);
}

void SetCharacterView(int chaa,int vii) {
  if (!is_valid_character(chaa))
    quit("!SetCharacterView: invalid character specified");
  
  Character_LockView(&game.chars[chaa], vii);
}

void SetCharacterFrame(int chaa, int view, int loop, int frame) {

  Character_LockViewFrame(&game.chars[chaa], view, loop, frame);
}

// similar to SetCharView, but aligns the frame to make it line up
void SetCharacterViewEx (int chaa, int vii, int loop, int align) {
  
  Character_LockViewAligned(&game.chars[chaa], vii, loop, align);
}

void SetCharacterViewOffset (int chaa, int vii, int xoffs, int yoffs) {

  Character_LockViewOffset(&game.chars[chaa], vii, xoffs, yoffs);
}


void ChangeCharacterView(int chaa,int vii) {
  if (!is_valid_character(chaa))
    quit("!ChangeCharacterView: invalid character specified");
  
  Character_ChangeView(&game.chars[chaa], vii);
}

void SetCharacterClickable (int cha, int clik) {
  if (!is_valid_character(cha))
    quit("!SetCharacterClickable: Invalid character specified");
  // make the character clicklabe (reset "No interaction" bit)
  game.chars[cha].flags&=~CHF_NOINTERACT;
  // if they don't want it clickable, set the relevant bit
  if (clik == 0)
    game.chars[cha].flags|=CHF_NOINTERACT;
  }

void SetCharacterIgnoreWalkbehinds (int cha, int clik) {
  if (!is_valid_character(cha))
    quit("!SetCharacterIgnoreWalkbehinds: Invalid character specified");

  Character_SetIgnoreWalkbehinds(&game.chars[cha], clik);
}

void SetObjectClickable (int cha, int clik) {
  if (!is_valid_object(cha))
    quit("!SetObjectClickable: Invalid object specified");
  objs[cha].flags&=~OBJF_NOINTERACT;
  if (clik == 0)
    objs[cha].flags|=OBJF_NOINTERACT;
  }

void Object_SetClickable(ScriptObject *objj, int clik) {
  SetObjectClickable(objj->id, clik);
}

int Object_GetClickable(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.Clickable: Invalid object specified");

  if (objj->obj->flags & OBJF_NOINTERACT)
    return 0;
  return 1;
}

void Object_SetIgnoreScaling(ScriptObject *objj, int newval) {
  if (!is_valid_object(objj->id))
    quit("!Object.IgnoreScaling: Invalid object specified");

  objj->obj->flags &= ~OBJF_USEROOMSCALING;
  if (!newval)
    objj->obj->flags |= OBJF_USEROOMSCALING;

  // clear the cache
  objcache[objj->id].ywas = -9999;
}

int Object_GetIgnoreScaling(ScriptObject *objj) {
  if (!is_valid_object(objj->id))
    quit("!Object.IgnoreScaling: Invalid object specified");

  if (objj->obj->flags & OBJF_USEROOMSCALING)
    return 0;
  return 1;
}

void Object_SetSolid(ScriptObject *objj, int solid) {
  objj->obj->flags &= ~OBJF_SOLID;
  if (solid)
    objj->obj->flags |= OBJF_SOLID;
}

int Object_GetSolid(ScriptObject *objj) {
  if (objj->obj->flags & OBJF_SOLID)
    return 1;
  return 0;
}

void Object_SetBlockingWidth(ScriptObject *objj, int bwid) {
  objj->obj->blocking_width = bwid;
}

int Object_GetBlockingWidth(ScriptObject *objj) {
  return objj->obj->blocking_width;
}

void Object_SetBlockingHeight(ScriptObject *objj, int bhit) {
  objj->obj->blocking_height = bhit;
}

int Object_GetBlockingHeight(ScriptObject *objj) {
  return objj->obj->blocking_height;
}

void SetObjectIgnoreWalkbehinds (int cha, int clik) {
  if (!is_valid_object(cha))
    quit("!SetObjectIgnoreWalkbehinds: Invalid object specified");
  objs[cha].flags&=~OBJF_NOWALKBEHINDS;
  if (clik)
    objs[cha].flags|=OBJF_NOWALKBEHINDS;
  // clear the cache
  objcache[cha].ywas = -9999;
}

int Object_GetID(ScriptObject *objj) {
  return objj->id;
}

void Object_SetIgnoreWalkbehinds(ScriptObject *chaa, int clik) {
  SetObjectIgnoreWalkbehinds(chaa->id, clik);
}

int Object_GetIgnoreWalkbehinds(ScriptObject *chaa) {
  if (!is_valid_object(chaa->id))
    quit("!Object.IgnoreWalkbehinds: Invalid object specified");

  if (chaa->obj->flags & OBJF_NOWALKBEHINDS)
    return 1;
  return 0;
}

void MoveCharacterToObject(int chaa,int obbj) {
  // invalid object, do nothing
  // this allows MoveCharacterToObject(EGO, GetObjectAt(...));
  if (!is_valid_object(obbj))
    return;

  walk_character(chaa,objs[obbj].x+5,objs[obbj].y+6,0, true);
  do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
}

void MoveCharacterToHotspot(int chaa,int hotsp) {
  if ((hotsp<0) || (hotsp>=MAX_HOTSPOTS))
    quit("!MovecharacterToHotspot: invalid hotspot");
  if (thisroom.hswalkto[hotsp].x<1) return;
  walk_character(chaa,thisroom.hswalkto[hotsp].x,thisroom.hswalkto[hotsp].y,0, true);
  do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
  }

void MoveCharacterBlocking(int chaa,int xx,int yy,int direct) {
  if (!is_valid_character (chaa))
    quit("!MoveCharacterBlocking: invalid character");

  // check if they try to move the player when Hide Player Char is
  // ticked -- otherwise this will hang the game
  if (game.chars[chaa].on != 1)
    quit("!MoveCharacterBlocking: character is turned off (is Hide Player Character selected?) and cannot be moved");

  if (direct)
    MoveCharacterDirect(chaa,xx,yy);
  else
    MoveCharacter(chaa,xx,yy);
  do_main_cycle(UNTIL_MOVEEND,(int)&game.chars[chaa].walking);
  }


/*void GetLanguageString(int indxx,char*buffr) {
  VALIDATE_STRING(buffr);
  char*bptr=get_language_text(indxx);
  if (bptr==NULL) strcpy(buffr,"[language string error]");
  else strncpy(buffr,bptr,199);
  buffr[199]=0;
  }*/

void SetViewport(int offsx,int offsy) {
  DEBUG_CONSOLE("Viewport locked to %d,%d", offsx, offsy);
  offsetx = multiply_up_coordinate(offsx);
  offsety = multiply_up_coordinate(offsy);
  check_viewport_coords();
  play.offsets_locked = 1;
}
void ReleaseViewport() {
  play.offsets_locked = 0;
  DEBUG_CONSOLE("Viewport released back to engine control");
}
int GetViewportX () {
  return divide_down_coordinate(offsetx);
  }
int GetViewportY () {
  return divide_down_coordinate(offsety);
  }

void on_background_frame_change () {

  invalidate_screen();
  mark_current_background_dirty();
  invalidate_cached_walkbehinds();

  // get the new frame's palette
  memcpy (palette, thisroom.bpalettes[play.bg_frame], sizeof(color) * 256);

  // hi-colour, update the palette. It won't have an immediate effect
  // but will be drawn properly when the screen fades in
  if (game.color_depth > 1)
    setpal();

  if (in_enters_screen)
    return;

  // Don't update the palette if it hasn't changed
  if (thisroom.ebpalShared[play.bg_frame])
    return;

  // 256-colours, tell it to update the palette (will actually be done as
  // close as possible to the screen update to prevent flicker problem)
  if (game.color_depth == 1)
    bg_just_changed = 1;
}

void SetBackgroundFrame(int frnum) {
  if ((frnum<-1) | (frnum>=thisroom.num_bscenes))
    quit("!SetBackgrondFrame: invalid frame number specified");
  if (frnum<0) {
    play.bg_frame_locked=0;
    return;
  }

  play.bg_frame_locked = 1;

  if (frnum == play.bg_frame)
  {
    // already on this frame, do nothing
    return;
  }

  play.bg_frame = frnum;
  on_background_frame_change ();
}

int GetBackgroundFrame() {
  return play.bg_frame;
  }

void script_debug(int cmdd,int dataa) {
  if (play.debug_mode==0) return;
  int rr;
  if (cmdd==0) {
    for (rr=1;rr<game.numinvitems;rr++)
      playerchar->inv[rr]=1;
    update_invorder();
//    Display("invorder decided there are %d items[display %d",play.inv_numorder,play.inv_numdisp);
    }
  else if (cmdd==1) {
    char toDisplay[STD_BUFFER_SIZE];
    const char *filterName = filter->GetVersionBoxText();
    sprintf(toDisplay,"Adventure Game Studio run-time engine[ACI version " ACI_VERSION_TEXT
      "[Running %d x %d at %d-bit %s[GFX: %s[%s" "Sprite cache size: %ld KB (limit %ld KB; %ld locked)",
      final_scrn_wid,final_scrn_hit,final_col_dep, (convert_16bit_bgr) ? "BGR" : "",
      gfxDriver->GetDriverName(), filterName,
      spriteset.cachesize / 1024, spriteset.maxCacheSize / 1024, spriteset.lockedSize / 1024);
    if (play.seperate_music_lib)
      strcat(toDisplay,"[AUDIO.VOX enabled");
    if (play.want_speech >= 1)
      strcat(toDisplay,"[SPEECH.VOX enabled");
    if (transtree != NULL) {
      strcat(toDisplay,"[Using translation ");
      strcat(toDisplay, transFileName);
    }
    if (opts.mod_player == 0)
      strcat(toDisplay,"[(mod/xm player discarded)");
    Display(toDisplay);
//    Display("shftR: %d  shftG: %d  shftB: %d", _rgb_r_shift_16, _rgb_g_shift_16, _rgb_b_shift_16);
//    Display("Remaining memory: %d kb",_go32_dpmi_remaining_virtual_memory()/1024);
//Display("Play char bcd: %d",bitmap_color_depth(spriteset[views[playerchar->view].frames[playerchar->loop][playerchar->frame].pic]));
    }
  else if (cmdd==2) 
  {  // show walkable areas from here
    block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
    blit(prepare_walkable_areas(-1),tempw,0,0,0,0,tempw->w,tempw->h);
    block stretched = create_bitmap(scrnwid, scrnhit);
    stretch_sprite(stretched, tempw, -offsetx, -offsety, get_fixed_pixel_size(tempw->w), get_fixed_pixel_size(tempw->h));

    IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(stretched, false, true);
    render_graphics(ddb, 0, 0);

    destroy_bitmap(tempw);
    destroy_bitmap(stretched);
    gfxDriver->DestroyDDB(ddb);
    while (!kbhit()) ;
    getch();
    invalidate_screen();
  }
  else if (cmdd==3) 
  {
    int goToRoom = -1;
    if (game.roomCount == 0)
    {
      char inroomtex[80];
      sprintf(inroomtex, "!Enter new room: (in room %d)", displayed_room);
      setup_for_dialog();
      goToRoom = enternumberwindow(inroomtex);
      restore_after_dialog();
    }
    else
    {
      setup_for_dialog();
      goToRoom = roomSelectorWindow(displayed_room, game.roomCount, game.roomNumbers, game.roomNames);
      restore_after_dialog();
    }
    if (goToRoom >= 0) 
      NewRoom(goToRoom);
  }
  else if (cmdd == 4) {
    if (display_fps != 2)
      display_fps = dataa;
  }
  else if (cmdd == 5) {
    if (dataa == 0) dataa = game.playercharacter;
    if (game.chars[dataa].walking < 1) {
      Display("Not currently moving.");
      return;
    }
    block tempw=create_bitmap(thisroom.walls->w,thisroom.walls->h);
    int mlsnum = game.chars[dataa].walking;
    if (game.chars[dataa].walking >= TURNING_AROUND)
      mlsnum %= TURNING_AROUND;
    MoveList*cmls = &mls[mlsnum];
    clear_to_color(tempw, bitmap_mask_color(tempw));
    for (int i = 0; i < cmls->numstage-1; i++) {
      short srcx=short((cmls->pos[i] >> 16) & 0x00ffff);
      short srcy=short(cmls->pos[i] & 0x00ffff);
      short targetx=short((cmls->pos[i+1] >> 16) & 0x00ffff);
      short targety=short(cmls->pos[i+1] & 0x00ffff);
      line (tempw, srcx, srcy, targetx, targety, get_col8_lookup(i+1));
    }
    stretch_sprite(screen, tempw, -offsetx, -offsety, multiply_up_coordinate(tempw->w), multiply_up_coordinate(tempw->h));
    render_to_screen(screen, 0, 0);
    wfreeblock(tempw);
    while (!kbhit()) ;
    getch();
  }
  else if (cmdd == 99)
    ccSetOption(SCOPT_DEBUGRUN, dataa);
  else quit("!Debug: unknown command code");
}


int init_cd_player() 
{
  use_cdplayer=0;
  return platform->InitializeCDPlayer();
}

int cd_manager(int cmdd,int datt) 
{
  if (!triedToUseCdAudioCommand)
  {
    triedToUseCdAudioCommand = true;
    init_cd_player();
  }
  if (cmdd==0) return use_cdplayer;
  if (use_cdplayer==0) return 0;  // ignore other commands

  return platform->CDPlayerCommand(cmdd, datt);
}

void script_SetTimer(int tnum,int timeout) {
  if ((tnum < 1) || (tnum >= MAX_TIMERS))
    quit("!StartTimer: invalid timer number");
  play.script_timers[tnum] = timeout;
  }

int IsTimerExpired(int tnum) {
  if ((tnum < 1) || (tnum >= MAX_TIMERS))
    quit("!IsTimerExpired: invalid timer number");
  if (play.script_timers[tnum] == 1) {
    play.script_timers[tnum] = 0;
    return 1;
    }
  return 0;
  }

void my_strncpy(char *dest, const char *src, int len) {
  // the normal strncpy pads out the string with zeros up to the
  // max length -- we don't want that
  if (strlen(src) >= (unsigned)len) {
    strncpy(dest, src, len);
    dest[len] = 0;
  }
  else
    strcpy(dest, src);
}

void _sc_strcat(char*s1,char*s2) {
  // make sure they don't try to append a char to the string
  VALIDATE_STRING (s2);
  check_strlen(s1);
  int mosttocopy=(MAXSTRLEN-strlen(s1))-1;
//  int numbf=game.iface[4].numbuttons;
  my_strncpy(&s1[strlen(s1)], s2, mosttocopy);
}

void _sc_strcpy(char*s1,char*s2) {
  check_strlen(s1);
  my_strncpy(s1, s2, MAXSTRLEN - 1);
}

int StrContains (const char *s1, const char *s2) {
  VALIDATE_STRING (s1);
  VALIDATE_STRING (s2);
  char *tempbuf1 = (char*)malloc(strlen(s1) + 1);
  char *tempbuf2 = (char*)malloc(strlen(s2) + 1);
  strcpy(tempbuf1, s1);
  strcpy(tempbuf2, s2);
  strlwr(tempbuf1);
  strlwr(tempbuf2);

  char *offs = strstr (tempbuf1, tempbuf2);
  free(tempbuf1);
  free(tempbuf2);

  if (offs == NULL)
    return -1;

  return (offs - tempbuf1);
}

#ifdef WINDOWS_VERSION
#define strlwr _strlwr
#define strupr _strupr
#endif

void _sc_strlower (char *desbuf) {
  VALIDATE_STRING(desbuf);
  check_strlen (desbuf);
  strlwr (desbuf);
}

void _sc_strupper (char *desbuf) {
  VALIDATE_STRING(desbuf);
  check_strlen (desbuf);
  strupr (desbuf);
}

/*int _sc_strcmp (char *s1, char *s2) {
  return strcmp (get_translation (s1), get_translation(s2));
}

int _sc_stricmp (char *s1, char *s2) {
  return stricmp (get_translation (s1), get_translation(s2));
}*/


// Custom printf, needed because floats are pushed as 8 bytes
void my_sprintf(char *buffer, const char *fmt, va_list ap) {
  int bufidx = 0;
  const char *curptr = fmt;
  const char *endptr;
  char spfbuffer[STD_BUFFER_SIZE];
  char fmtstring[100];
  int numargs = -1;

  while (1) {
    // copy across everything until the next % (or end of string)
    endptr = strchr(curptr, '%');
    if (endptr == NULL)
      endptr = &curptr[strlen(curptr)];
    while (curptr < endptr) {
      buffer[bufidx] = *curptr;
      curptr++;
      bufidx++;
    }
    // at this point, curptr and endptr should be equal and pointing
    // to the % or \0
    if (*curptr == 0)
      break;
    if (curptr[1] == '%') {
      // "%%", so just write a % to the output
      buffer[bufidx] = '%';
      bufidx++;
      curptr += 2;
      continue;
    }
    // find the end of the % clause
    while ((*endptr != 'd') && (*endptr != 'f') && (*endptr != 'c') &&
           (*endptr != 0) && (*endptr != 's') && (*endptr != 'x') &&
           (*endptr != 'X'))
      endptr++;

    if (numargs >= 0) {
      numargs--;
      // if there are not enough arguments, just copy the %d
      // to the output string rather than trying to format it
      if (numargs < 0)
        endptr = &curptr[strlen(curptr)];
    }

    if (*endptr == 0) {
      // something like %p which we don't support, so just write
      // the % to the output
      buffer[bufidx] = '%';
      bufidx++;
      curptr++;
      continue;
    }
    // move endptr to 1 after the end character
    endptr++;

    // copy the %d or whatever
    strncpy(fmtstring, curptr, (endptr - curptr));
    fmtstring[endptr - curptr] = 0;

    unsigned int theArg = va_arg(ap, unsigned int);

    // use sprintf to parse the actual %02d type thing
    if (endptr[-1] == 'f') {
      // floats are pushed as 8-bytes, so ensure that it knows this is a float
      float floatArg;
      memcpy(&floatArg, &theArg, sizeof(float));
      sprintf(spfbuffer, fmtstring, floatArg);
    }
    else if ((theArg == (int)buffer) && (endptr[-1] == 's'))
      quit("Cannot use destination as argument to StrFormat");
    else if ((theArg < 0x10000) && (endptr[-1] == 's'))
      quit("!One of the string arguments supplied was not a string");
    else if (endptr[-1] == 's')
    {
      strncpy(spfbuffer, (const char*)theArg, STD_BUFFER_SIZE);
      spfbuffer[STD_BUFFER_SIZE - 1] = 0;
    }
    else 
      sprintf(spfbuffer, fmtstring, theArg);

    // use the formatted text
    buffer[bufidx] = 0;

    if (bufidx + strlen(spfbuffer) >= STD_BUFFER_SIZE)
      quitprintf("!String.Format: buffer overrun: maximum formatted string length %d chars, this string: %d chars", STD_BUFFER_SIZE, bufidx + strlen(spfbuffer));

    strcat(buffer, spfbuffer);
    bufidx += strlen(spfbuffer);
    curptr = endptr;
  }
  buffer[bufidx] = 0;

}

void _sc_AbortGame(char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE] = "!?";
  va_list ap;
  va_start(ap,texx);
  my_sprintf(&displbuf[2], get_translation(texx), ap);
  va_end(ap);

  quit(displbuf);
}

void _sc_sprintf(char*destt,char*texx, ...) {
  char displbuf[STD_BUFFER_SIZE];
  VALIDATE_STRING(destt);
  check_strlen(destt);
  va_list ap;
  va_start(ap,texx);
  my_sprintf(displbuf, get_translation(texx), ap);
  va_end(ap);

  my_strncpy(destt, displbuf, MAXSTRLEN - 1);
}

int HasBeenToRoom (int roomnum) {
  if ((roomnum < 0) || (roomnum >= MAX_ROOMS))
    quit("!HasBeenToRoom: invalid room number specified");

  if (roomstats[roomnum].beenhere)
    return 1;
  return 0;
}

// **** end of trext script exported functions

InteractionVariable *get_interaction_variable (int varindx) {
  
  if ((varindx >= LOCAL_VARIABLE_OFFSET) && (varindx < LOCAL_VARIABLE_OFFSET + thisroom.numLocalVars))
    return &thisroom.localvars[varindx - LOCAL_VARIABLE_OFFSET];

  if ((varindx < 0) || (varindx >= numGlobalVars))
    quit("!invalid interaction variable specified");

  return &globalvars[varindx];
}

InteractionVariable *FindGraphicalVariable(const char *varName) {
  int ii;
  for (ii = 0; ii < numGlobalVars; ii++) {
    if (stricmp (globalvars[ii].name, varName) == 0)
      return &globalvars[ii];
  }
  for (ii = 0; ii < thisroom.numLocalVars; ii++) {
    if (stricmp (thisroom.localvars[ii].name, varName) == 0)
      return &thisroom.localvars[ii];
  }
  return NULL;
}

int GetGraphicalVariable (const char *varName) {
  InteractionVariable *theVar = FindGraphicalVariable(varName);
  if (theVar == NULL) {
    char quitmessage[120];
    sprintf (quitmessage, "!GetGraphicalVariable: interaction variable '%s' not found", varName);
    quit(quitmessage);
    return 0;
  }
  return theVar->value;
}

void SetGraphicalVariable (const char *varName, int p_value) {
  InteractionVariable *theVar = FindGraphicalVariable(varName);
  if (theVar == NULL) {
    char quitmessage[120];
    sprintf (quitmessage, "!SetGraphicalVariable: interaction variable '%s' not found", varName);
    quit(quitmessage);
  }
  else
    theVar->value = p_value;
}

int get_nivalue (NewInteractionCommandList *nic, int idx, int parm) {
  if (nic->command[idx].data[parm].valType == VALTYPE_VARIABLE) {
    // return the value of the variable
    return get_interaction_variable(nic->command[idx].data[parm].val)->value;
  }
  return nic->command[idx].data[parm].val;
}

#define IPARAM1 get_nivalue(nicl, i, 0)
#define IPARAM2 get_nivalue(nicl, i, 1)
#define IPARAM3 get_nivalue(nicl, i, 2)
#define IPARAM4 get_nivalue(nicl, i, 3)
#define IPARAM5 get_nivalue(nicl, i, 4)

// the 'cmdsrun' parameter counts how many commands are run.
// if a 'Inv Item Was Used' check does not pass, it doesn't count
// so cmdsrun remains 0 if no inventory items matched
int run_interaction_commandlist (NewInteractionCommandList *nicl, int *timesrun, int*cmdsrun) {
  int i;

  if (nicl == NULL)
    return -1;

  for (i = 0; i < nicl->numCommands; i++) {
    cmdsrun[0] ++;
    int room_was = play.room_changes;
    
    switch (nicl->command[i].type) {
      case 0:  // Do nothing
        break;
      case 1:  // Run script
        { 
        TempEip tempip(4001);
        UPDATE_MP3
        if ((strstr(evblockbasename,"character")!=0) || (strstr(evblockbasename,"inventory")!=0)) {
          // Character or Inventory (global script)
          char *torun = make_ts_func_name(evblockbasename,evblocknum,nicl->command[i].data[0].val);
          // we are already inside the mouseclick event of the script, can't nest calls
          if (inside_script) 
            curscript->run_another (torun, 0, 0);
          else run_text_script(gameinst,torun);
          }
        else {
          // Other (room script)
          if (inside_script) {
            char funcName[60];
            strcpy(funcName,"|");
            strcat(funcName,make_ts_func_name(evblockbasename,evblocknum,nicl->command[i].data[0].val));
            curscript->run_another (funcName, 0, 0);
            }
          else
            run_text_script(roominst,make_ts_func_name(evblockbasename,evblocknum,nicl->command[i].data[0].val));
          }
        UPDATE_MP3
        break;
      }
      case 2:  // Add score (first time)
        if (timesrun[0] > 0)
          break;
        timesrun[0] ++;
      case 3:  // Add score
        GiveScore (IPARAM1);
        break;
      case 4:  // Display Message
/*        if (comprdata<0)
          display_message_aschar=evb->data[ss];*/
        DisplayMessage(IPARAM1);
        break;
      case 5:  // Play Music
        PlayMusicResetQueue(IPARAM1);
        break;
      case 6:  // Stop Music
        stopmusic ();
        break;
      case 7:  // Play Sound
        play_sound (IPARAM1);
        break;
      case 8:  // Play Flic
        play_flc_file(IPARAM1, IPARAM2);
        break;
      case 9:  // Run Dialog
		{ int room_was = play.room_changes;
        RunDialog(IPARAM1);
		// if they changed room within the dialog script,
		// the interaction command list is no longer valid
        if (room_was != play.room_changes)
          return -1;
		}
        break;
      case 10: // Enable Dialog Option
        SetDialogOption (IPARAM1, IPARAM2, 1);
        break;
      case 11: // Disable Dialog Option
        SetDialogOption (IPARAM1, IPARAM2, 0);
        break;
      case 12: // Go To Screen
        Character_ChangeRoomAutoPosition(playerchar, IPARAM1, IPARAM2);
        return -1;
      case 13: // Add Inventory
        add_inventory (IPARAM1);
        break;
      case 14: // Move Object
        MoveObject (IPARAM1, IPARAM2, IPARAM3, IPARAM4);
        // if they want to wait until finished, do so
        if (IPARAM5)
          do_main_cycle(UNTIL_MOVEEND,(int)&objs[IPARAM1].moving);
        break;
      case 15: // Object Off
        ObjectOff (IPARAM1);
        break;
      case 16: // Object On
        ObjectOn (IPARAM1);
        break;
      case 17: // Set Object View
        SetObjectView (IPARAM1, IPARAM2);
        break;
      case 18: // Animate Object
        AnimateObject (IPARAM1, IPARAM2, IPARAM3, IPARAM4);
        break;
      case 19: // Move Character
        if (IPARAM4)
          MoveCharacterBlocking (IPARAM1, IPARAM2, IPARAM3, 0);
        else
          MoveCharacter (IPARAM1, IPARAM2, IPARAM3);
        break;
      case 20: // If Inventory Item was used
        if (play.usedinv == IPARAM1) {
          if (game.options[OPT_NOLOSEINV] == 0)
            lose_inventory (play.usedinv);
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        }
        else
          cmdsrun[0] --;
        break;
      case 21: // if player has inventory item
        if (playerchar->inv[IPARAM1] > 0)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 22: // if a character is moving
        if (game.chars[IPARAM1].walking)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 23: // if two variables are equal
        if (IPARAM1 == IPARAM2)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 24: // Stop character walking
        StopMoving (IPARAM1);
        break;
      case 25: // Go to screen at specific co-ordinates
        NewRoomEx (IPARAM1, IPARAM2, IPARAM3);
        return -1;
      case 26: // Move NPC to different room
        if (!is_valid_character(IPARAM1))
          quit("!Move NPC to different room: invalid character specified");
        game.chars[IPARAM1].room = IPARAM2;
        break;
      case 27: // Set character view
        SetCharacterView (IPARAM1, IPARAM2);
        break;
      case 28: // Release character view
        ReleaseCharacterView (IPARAM1);
        break;
      case 29: // Follow character
        FollowCharacter (IPARAM1, IPARAM2);
        break;
      case 30: // Stop following
        FollowCharacter (IPARAM1, -1);
        break;
      case 31: // Disable hotspot
        DisableHotspot (IPARAM1);
        break;
      case 32: // Enable hotspot
        EnableHotspot (IPARAM1);
        break;
      case 33: // Set variable value
        get_interaction_variable(nicl->command[i].data[0].val)->value = IPARAM2;
        break;
      case 34: // Run animation
        scAnimateCharacter(IPARAM1, IPARAM2, IPARAM3, 0);
        do_main_cycle(UNTIL_SHORTIS0,(int)&game.chars[IPARAM1].animating);
        break;
      case 35: // Quick animation
        SetCharacterView (IPARAM1, IPARAM2);
        scAnimateCharacter(IPARAM1, IPARAM3, IPARAM4, 0);
        do_main_cycle(UNTIL_SHORTIS0,(int)&game.chars[IPARAM1].animating);
        ReleaseCharacterView (IPARAM1);
        break;
      case 36: // Set idle animation
        SetCharacterIdle (IPARAM1, IPARAM2, IPARAM3);
        break;
      case 37: // Disable idle animation
        SetCharacterIdle (IPARAM1, -1, -1);
        break;
      case 38: // Lose inventory item
        lose_inventory (IPARAM1);
        break;
      case 39: // Show GUI
        InterfaceOn (IPARAM1);
        break;
      case 40: // Hide GUI
        InterfaceOff (IPARAM1);
        break;
      case 41: // Stop running more commands
        return -1;
      case 42: // Face location
        FaceLocation (IPARAM1, IPARAM2, IPARAM3);
        break;
      case 43: // Pause command processor
        scrWait (IPARAM1);
        break;
      case 44: // Change character view
        ChangeCharacterView (IPARAM1, IPARAM2);
        break;
      case 45: // If player character is
        if (GetPlayerCharacter() == IPARAM1)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 46: // if cursor mode is
        if (GetCursorMode() == IPARAM1)
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      case 47: // if player has been to room
        if (HasBeenToRoom(IPARAM1))
          if (run_interaction_commandlist (nicl->command[i].get_child_list(), timesrun, cmdsrun))
            return -1;
        break;
      default:
        quit("unknown new interaction command");
        break;
    }

    // if the room changed within the action, nicl is no longer valid
    if (room_was != play.room_changes)
      return -1;
  }
  return 0;

}

void run_unhandled_event (int evnt) {

  if (play.check_interaction_only)
    return;

  int evtype=0;
  if (strnicmp(evblockbasename,"hotspot",7)==0) evtype=1;
  else if (strnicmp(evblockbasename,"object",6)==0) evtype=2;
  else if (strnicmp(evblockbasename,"character",9)==0) evtype=3;
  else if (strnicmp(evblockbasename,"inventory",9)==0) evtype=5;
  else if (strnicmp(evblockbasename,"region",6)==0)
    return;  // no unhandled_events for regions

  // clicked Hotspot 0, so change the type code
  if ((evtype == 1) & (evblocknum == 0) & (evnt != 0) & (evnt != 5) & (evnt != 6))
    evtype = 4;
  if ((evtype==1) & ((evnt==0) | (evnt==5) | (evnt==6)))
    ;  // character stands on hotspot, mouse moves over hotspot, any click
  else if ((evtype==2) & (evnt==4)) ;  // any click on object
  else if ((evtype==3) & (evnt==4)) ;  // any click on character
  else if (evtype > 0) {
    can_run_delayed_command();

    if (inside_script)
      curscript->run_another ("#unhandled_event", evtype, evnt);
    else
      run_text_script_2iparam(gameinst,"unhandled_event",evtype,evnt);
  }

}

// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_event (NewInteraction *nint, int evnt, int chkAny, int isInv) {

  if ((nint->response[evnt] == NULL) || (nint->response[evnt]->numCommands == 0)) {
    // no response defined for this event
    // If there is a response for "Any Click", then abort now so as to
    // run that instead
    if (chkAny < 0) ;
    else if ((nint->response[chkAny] != NULL) && (nint->response[chkAny]->numCommands > 0))
      return 0;

    // Otherwise, run unhandled_event
    run_unhandled_event(evnt);
    
    return 0;
  }

  if (play.check_interaction_only) {
    play.check_interaction_only = 2;
    return -1;
  }

  int cmdsrun = 0, retval = 0;
  // Right, so there were some commands defined in response to the event.
  retval = run_interaction_commandlist (nint->response[evnt], &nint->timesRun[evnt], &cmdsrun);

  // An inventory interaction, but the wrong item was used
  if ((isInv) && (cmdsrun == 0))
    run_unhandled_event (evnt);

  return retval;
}

// Returns 0 normally, or -1 to indicate that the NewInteraction has
// become invalid and don't run another interaction on it
// (eg. a room change occured)
int run_interaction_script(InteractionScripts *nint, int evnt, int chkAny, int isInv) {

  if ((nint->scriptFuncNames[evnt] == NULL) || (nint->scriptFuncNames[evnt][0] == 0)) {
    // no response defined for this event
    // If there is a response for "Any Click", then abort now so as to
    // run that instead
    if (chkAny < 0) ;
    else if ((nint->scriptFuncNames[chkAny] != NULL) && (nint->scriptFuncNames[chkAny][0] != 0))
      return 0;

    // Otherwise, run unhandled_event
    run_unhandled_event(evnt);
    
    return 0;
  }

  if (play.check_interaction_only) {
    play.check_interaction_only = 2;
    return -1;
  }

  int room_was = play.room_changes;

  UPDATE_MP3
  if ((strstr(evblockbasename,"character")!=0) || (strstr(evblockbasename,"inventory")!=0)) {
    // Character or Inventory (global script)
    if (inside_script) 
      curscript->run_another (nint->scriptFuncNames[evnt], 0, 0);
    else run_text_script(gameinst, nint->scriptFuncNames[evnt]);
    }
  else {
    // Other (room script)
    if (inside_script) {
      char funcName[60];
      sprintf(funcName, "|%s", nint->scriptFuncNames[evnt]);
      curscript->run_another (funcName, 0, 0);
    }
    else
      run_text_script(roominst, nint->scriptFuncNames[evnt]);
  }
  UPDATE_MP3

  int retval = 0;
  // if the room changed within the action
  if (room_was != play.room_changes)
    retval = -1;

  return retval;
}

int run_dialog_request (int parmtr) {
  play.stop_dialog_at_end = DIALOG_RUNNING;
  run_text_script_iparam(gameinst, "dialog_request", parmtr);

  if (play.stop_dialog_at_end == DIALOG_STOP) {
    play.stop_dialog_at_end = DIALOG_NONE;
    return -2;
  }
  if (play.stop_dialog_at_end >= DIALOG_NEWTOPIC) {
    int tval = play.stop_dialog_at_end - DIALOG_NEWTOPIC;
    play.stop_dialog_at_end = DIALOG_NONE;
    return tval;
  }
  if (play.stop_dialog_at_end >= DIALOG_NEWROOM) {
    int roomnum = play.stop_dialog_at_end - DIALOG_NEWROOM;
    play.stop_dialog_at_end = DIALOG_NONE;
    NewRoom(roomnum);
    return -2;
  }
  play.stop_dialog_at_end = DIALOG_NONE;
  return -1;
}

#define RUN_DIALOG_STOP_DIALOG   -2
#define RUN_DIALOG_GOTO_PREVIOUS -4
// dialog manager stuff

int run_dialog_script(DialogTopic*dtpp, int dialogID, int offse, int optionIndex) {
  said_speech_line = 0;
  int result;

  char funcName[100];
  sprintf(funcName, "_run_dialog%d", dialogID);
  run_text_script_iparam(dialogScriptsInst, funcName, optionIndex);
  result = dialogScriptsInst->returnValue;

  if (in_new_room > 0)
    return RUN_DIALOG_STOP_DIALOG;

  if (said_speech_line > 0) {
    // the line below fixes the problem with the close-up face remaining on the
    // screen after they finish talking; however, it makes the dialog options
    // area flicker when going between topics.
    DisableInterface();
    mainloop(); // redraw the screen to make sure it looks right
    EnableInterface();
    // if we're not about to abort the dialog, switch back to arrow
    if (result != RUN_DIALOG_STOP_DIALOG)
      set_mouse_cursor(CURS_ARROW);
  }

  return result;
}

int write_dialog_options(int dlgxp, int curyp, int numdisp, int mouseison, int areawid,
    int bullet_wid, int usingfont, DialogTopic*dtop, char*disporder, short*dispyp,
    int txthit, int utextcol) {
  int ww;

  for (ww=0;ww<numdisp;ww++) {

    if ((dtop->optionflags[disporder[ww]] & DFLG_HASBEENCHOSEN) &&
        (play.read_dialog_option_colour >= 0)) {
      // 'read' colour
      wtextcolor(play.read_dialog_option_colour);
    }
    else {
      // 'unread' colour
      wtextcolor(playerchar->talkcolor);
    }

    if (mouseison==ww) {
      if (textcol==get_col8_lookup(utextcol))
        wtextcolor(13); // the normal colour is the same as highlight col
      else wtextcolor(utextcol);
    }

    break_up_text_into_lines(areawid-(8+bullet_wid),usingfont,get_translation(dtop->optionnames[disporder[ww]]));
    dispyp[ww]=curyp;
    if (game.dialog_bullet > 0)
      wputblock(dlgxp,curyp,spriteset[game.dialog_bullet],1);
    int cc;
    if (game.options[OPT_DIALOGNUMBERED]) {
      char tempbfr[20];
      int actualpicwid = 0;
      if (game.dialog_bullet > 0)
        actualpicwid = spritewidth[game.dialog_bullet]+3;

      sprintf (tempbfr, "%d.", ww + 1);
      wouttext_outline (dlgxp + actualpicwid, curyp, usingfont, tempbfr);
    }
    for (cc=0;cc<numlines;cc++) {
      wouttext_outline(dlgxp+((cc==0) ? 0 : 9)+bullet_wid, curyp, usingfont, lines[cc]);
      curyp+=txthit;
    }
    if (ww < numdisp-1)
      curyp += multiply_up_coordinate(game.options[OPT_DIALOGGAP]);
  }
  return curyp;
}

#define GET_OPTIONS_HEIGHT {\
  needheight = 0;\
  for (ww=0;ww<numdisp;ww++) {\
    break_up_text_into_lines(areawid-(8+bullet_wid),usingfont,get_translation(dtop->optionnames[disporder[ww]]));\
    needheight += (numlines * txthit) + multiply_up_coordinate(game.options[OPT_DIALOGGAP]);\
  }\
  if (parserInput) needheight += parserInput->hit + multiply_up_coordinate(game.options[OPT_DIALOGGAP]);\
 }


void draw_gui_for_dialog_options(GUIMain *guib, int dlgxp, int dlgyp) {
  if (guib->bgcol != 0) {
    wsetcolor(guib->bgcol);
    wbar(dlgxp, dlgyp, dlgxp + guib->wid, dlgyp + guib->hit);
  }
  if (guib->bgpic > 0)
    put_sprite_256 (dlgxp, dlgyp, spriteset[guib->bgpic]);

  wsetcolor(0);
}

bool get_custom_dialog_options_dimensions(int dlgnum)
{
  ccDialogOptionsRendering.Reset();
  ccDialogOptionsRendering.dialogID = dlgnum;

  getDialogOptionsDimensionsFunc.param1 = &ccDialogOptionsRendering;
  run_function_on_non_blocking_thread(&getDialogOptionsDimensionsFunc);

  if ((ccDialogOptionsRendering.width > 0) &&
      (ccDialogOptionsRendering.height > 0))
  {
    return true;
  }
  return false;
}

#define MAX_TOPIC_HISTORY 50
#define DLG_OPTION_PARSER 99

int show_dialog_options(int dlgnum, int sayChosenOption, bool runGameLoopsInBackground) 
{
  int dlgxp,dlgyp = get_fixed_pixel_size(160);
  int usingfont=FONT_NORMAL;
  int txthit = wgetfontheight(usingfont);
  int curswas=cur_cursor;
  int bullet_wid = 0, needheight;
  IDriverDependantBitmap *ddb = NULL;
  BITMAP *subBitmap = NULL;
  GUITextBox *parserInput = NULL;
  DialogTopic*dtop = NULL;

  if ((dlgnum < 0) || (dlgnum >= game.numdialog))
    quit("!RunDialog: invalid dialog number specified");

  can_run_delayed_command();

  play.in_conversation ++;

  update_polled_stuff();

  if (game.dialog_bullet > 0)
    bullet_wid = spritewidth[game.dialog_bullet]+3;

  // numbered options, leave space for the numbers
  if (game.options[OPT_DIALOGNUMBERED])
    bullet_wid += wgettextwidth_compensate("9. ", usingfont);

  said_text = 0;

  update_polled_stuff();

  block tempScrn = create_bitmap_ex(final_col_dep, screen->w, screen->h);

  set_mouse_cursor(CURS_ARROW);

  dtop=&dialog[dlgnum];

  int ww,chose=-1,numdisp=0;

  //get_real_screen();
  wsetscreen(virtual_screen);

  char disporder[MAXTOPICOPTIONS];
  short dispyp[MAXTOPICOPTIONS];
  int parserActivated = 0;
  if ((dtop->topicFlags & DTFLG_SHOWPARSER) && (play.disable_dialog_parser == 0)) {
    parserInput = new GUITextBox();
    parserInput->hit = txthit + get_fixed_pixel_size(4);
    parserInput->exflags = 0;
    parserInput->font = usingfont;
  }

  wtexttransparent(TEXTFG);
  numdisp=0;
  for (ww=0;ww<dtop->numoptions;ww++) {
    if ((dtop->optionflags[ww] & DFLG_ON)==0) continue;
    ensure_text_valid_for_font(dtop->optionnames[ww], usingfont);
    disporder[numdisp]=ww;
    numdisp++;
  }
  if (numdisp<1) quit("!DoDialog: all options have been turned off");
  // Don't display the options if there is only one and the parser
  // is not enabled.
  if ((numdisp > 1) || (parserInput != NULL) || (play.show_single_dialog_option)) {
    wsetcolor(0); //wbar(0,dlgyp-1,scrnwid-1,dlgyp+numdisp*txthit+1);
    int areawid, is_textwindow = 0;
    int forecol = 14, savedwid;

    int mouseison=-1,curyp;
    int mousewason=-10;
    int dirtyx = 0, dirtyy = 0;
    int dirtywidth = virtual_screen->w, dirtyheight = virtual_screen->h;
    bool usingCustomRendering = false;

    dlgxp = 1;
    if (get_custom_dialog_options_dimensions(dlgnum))
    {
      usingCustomRendering = true;
      dirtyx = multiply_up_coordinate(ccDialogOptionsRendering.x);
      dirtyy = multiply_up_coordinate(ccDialogOptionsRendering.y);
      dirtywidth = multiply_up_coordinate(ccDialogOptionsRendering.width);
      dirtyheight = multiply_up_coordinate(ccDialogOptionsRendering.height);
    }
    else if (game.options[OPT_DIALOGIFACE] > 0)
    {
      GUIMain*guib=&guis[game.options[OPT_DIALOGIFACE]];
      if (guib->is_textwindow()) {
        // text-window, so do the QFG4-style speech options
        is_textwindow = 1;
        forecol = guib->fgcol;
      }
      else {
        dlgxp = guib->x;
        dlgyp = guib->y;
        draw_gui_for_dialog_options(guib, dlgxp, dlgyp);

        dirtyx = dlgxp;
        dirtyy = dlgyp;
        dirtywidth = guib->wid;
        dirtyheight = guib->hit;

        areawid=guib->wid - 5;

        GET_OPTIONS_HEIGHT

        if (game.options[OPT_DIALOGUPWARDS]) {
          // They want the options upwards from the bottom
          dlgyp = (guib->y + guib->hit) - needheight;
        }
        
      }
    }
    else {
      //dlgyp=(scrnhit-numdisp*txthit)-1;
      areawid=scrnwid-5;
      GET_OPTIONS_HEIGHT
      dlgyp = scrnhit - needheight;
      wbar(0,dlgyp-1,scrnwid-1,scrnhit-1);

      dirtyx = 0;
      dirtyy = dlgyp - 1;
      dirtywidth = scrnwid;
      dirtyheight = scrnhit - dirtyy;
    }
    if (!is_textwindow)
      areawid -= multiply_up_coordinate(play.dialog_options_x) * 2;

    int orixp = dlgxp, oriyp = dlgyp;
    int wantRefresh = 0;
    mouseison=-10;
    
    update_polled_stuff();
    //blit(virtual_screen, tempScrn, 0, 0, 0, 0, screen->w, screen->h);
    if (!play.mouse_cursor_hidden)
      domouse(1);
    update_polled_stuff();

 redraw_options:

    wantRefresh = 1;

    if (usingCustomRendering)
    {
      tempScrn = recycle_bitmap(tempScrn, final_col_dep, 
        multiply_up_coordinate(ccDialogOptionsRendering.width), 
        multiply_up_coordinate(ccDialogOptionsRendering.height));
    }

    clear_to_color(tempScrn, bitmap_mask_color(tempScrn));
    wsetscreen(tempScrn);

    dlgxp = orixp;
    dlgyp = oriyp;
    // lengthy drawing to screen, so lock it for speed
    //acquire_screen();

    if (usingCustomRendering)
    {
      ccDialogOptionsRendering.surfaceToRenderTo = dialogOptionsRenderingSurface;
      ccDialogOptionsRendering.surfaceAccessed = false;
      dialogOptionsRenderingSurface->linkedBitmapOnly = tempScrn;
      dialogOptionsRenderingSurface->hasAlphaChannel = false;

      renderDialogOptionsFunc.param1 = &ccDialogOptionsRendering;
      run_function_on_non_blocking_thread(&renderDialogOptionsFunc);

      if (!ccDialogOptionsRendering.surfaceAccessed)
        quit("!dialog_options_get_dimensions was implemented, but no dialog_options_render function drew anything to the surface");

      if (parserInput)
      {
        parserInput->x = multiply_up_coordinate(ccDialogOptionsRendering.parserTextboxX);
        curyp = multiply_up_coordinate(ccDialogOptionsRendering.parserTextboxY);
        areawid = multiply_up_coordinate(ccDialogOptionsRendering.parserTextboxWidth);
        if (areawid == 0)
          areawid = tempScrn->w;
      }
    }
    else if (is_textwindow) {
      // text window behind the options
      areawid = multiply_up_coordinate(play.max_dialogoption_width);
      int biggest = 0;
      for (ww=0;ww<numdisp;ww++) {
        break_up_text_into_lines(areawid-(8+bullet_wid),usingfont,get_translation(dtop->optionnames[disporder[ww]]));
        if (longestline > biggest)
          biggest = longestline;
      }
      if (biggest < areawid - (12+bullet_wid))
        areawid = biggest + (12+bullet_wid);

      if (areawid < multiply_up_coordinate(play.min_dialogoption_width)) {
        areawid = multiply_up_coordinate(play.min_dialogoption_width);
        if (play.min_dialogoption_width > play.max_dialogoption_width)
          quit("!game.min_dialogoption_width is larger than game.max_dialogoption_width");
      }

      GET_OPTIONS_HEIGHT

      savedwid = areawid;
      int txoffs=0,tyoffs=0,yspos = scrnhit/2-needheight/2;
      int xspos = scrnwid/2 - areawid/2;
      // shift window to the right if QG4-style full-screen pic
      if ((game.options[OPT_SPEECHTYPE] == 3) && (said_text > 0))
        xspos = (scrnwid - areawid) - get_fixed_pixel_size(10);

      // needs to draw the right text window, not the default
      push_screen();
      draw_text_window(&txoffs,&tyoffs,&xspos,&yspos,&areawid,needheight, game.options[OPT_DIALOGIFACE]);
      pop_screen();
      // snice draw_text_window incrases the width, restore it
      areawid = savedwid;
      //wnormscreen();

      dirtyx = xspos;
      dirtyy = yspos;
      dirtywidth = screenop->w;
      dirtyheight = screenop->h;

      wputblock(xspos,yspos,screenop,1);
      wfreeblock(screenop); screenop = NULL;

      // Ignore the dialog_options_x/y offsets when using a text window
      txoffs += xspos;
      tyoffs += yspos;
      dlgyp = tyoffs;
      curyp = write_dialog_options(txoffs,tyoffs,numdisp,mouseison,areawid,bullet_wid,usingfont,dtop,disporder,dispyp,txthit,forecol);
      if (parserInput)
        parserInput->x = txoffs;
    }
    else {

      if (wantRefresh) {
        // redraw the black background so that anti-alias
        // fonts don't re-alias themselves
        if (game.options[OPT_DIALOGIFACE] == 0) {
          wsetcolor(16);
          wbar(0,dlgyp-1,scrnwid-1,scrnhit-1);
        }
        else {
          GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
          if (!guib->is_textwindow())
            draw_gui_for_dialog_options(guib, dlgxp, dlgyp);
        }
      }

      dirtyx = 0;
      dirtywidth = scrnwid;

      if (game.options[OPT_DIALOGIFACE] > 0) 
      {
        // the whole GUI area should be marked dirty in order
        // to ensure it gets drawn
        GUIMain* guib = &guis[game.options[OPT_DIALOGIFACE]];
        dirtyheight = guib->hit;
        dirtyy = dlgyp;
      }
      else
      {
        dirtyy = dlgyp - 1;
        dirtyheight = needheight + 1;
      }

      dlgxp += multiply_up_coordinate(play.dialog_options_x);
      dlgyp += multiply_up_coordinate(play.dialog_options_y);

      // if they use a negative dialog_options_y, make sure the
      // area gets marked as dirty
      if (dlgyp < dirtyy)
        dirtyy = dlgyp;

      //curyp = dlgyp + 1;
      curyp = dlgyp;
      curyp = write_dialog_options(dlgxp,curyp,numdisp,mouseison,areawid,bullet_wid,usingfont,dtop,disporder,dispyp,txthit,forecol);

      /*if (curyp > scrnhit) {
        dlgyp = scrnhit - (curyp - dlgyp);
        wbar(0,dlgyp-1,scrnwid-1,scrnhit-1);
        goto redraw_options;
      }*/
      if (parserInput)
        parserInput->x = dlgxp;
    }

    if (parserInput) {
      // Set up the text box, if present
      parserInput->y = curyp + multiply_up_coordinate(game.options[OPT_DIALOGGAP]);
      parserInput->wid = areawid - get_fixed_pixel_size(10);
      parserInput->textcol = playerchar->talkcolor;
      if (mouseison == DLG_OPTION_PARSER)
        parserInput->textcol = forecol;

      if (game.dialog_bullet)  // the parser X will get moved in a second
        wputblock(parserInput->x, parserInput->y, spriteset[game.dialog_bullet], 1);

      parserInput->wid -= bullet_wid;
      parserInput->x += bullet_wid;

      parserInput->Draw();
      parserInput->activated = 0;
    }

    wantRefresh = 0;
    wsetscreen(virtual_screen);

    update_polled_stuff();

    subBitmap = recycle_bitmap(subBitmap, bitmap_color_depth(tempScrn), dirtywidth, dirtyheight);
    subBitmap = gfxDriver->ConvertBitmapToSupportedColourDepth(subBitmap);

    update_polled_stuff();

    if (usingCustomRendering)
    {
      blit(tempScrn, subBitmap, 0, 0, 0, 0, tempScrn->w, tempScrn->h);
      invalidate_rect(dirtyx, dirtyy, dirtyx + subBitmap->w, dirtyy + subBitmap->h);
    }
    else
    {
      blit(tempScrn, subBitmap, dirtyx, dirtyy, 0, 0, dirtywidth, dirtyheight);
    }

    if ((ddb != NULL) && 
      ((ddb->GetWidth() != dirtywidth) ||
       (ddb->GetHeight() != dirtyheight)))
    {
      gfxDriver->DestroyDDB(ddb);
      ddb = NULL;
    }
    if (ddb == NULL)
      ddb = gfxDriver->CreateDDBFromBitmap(subBitmap, false, false);
    else
      gfxDriver->UpdateDDBFromBitmap(ddb, subBitmap, false);

    render_graphics(ddb, dirtyx, dirtyy);

    while (1) {

      if (runGameLoopsInBackground)
      {
        play.disabled_user_interface++;
        mainloop(false, ddb, dirtyx, dirtyy);
        play.disabled_user_interface--;
      }
      else
      {
        timerloop = 0;
        NEXT_ITERATION();

        render_graphics(ddb, dirtyx, dirtyy);
      
        update_polled_stuff_and_crossfade();
      }

      if (kbhit()) {
        int gkey = getch();
        if (parserInput) {
          wantRefresh = 1;
          // type into the parser 
          if ((gkey == 361) || ((gkey == ' ') && (strlen(parserInput->text) == 0))) {
            // write previous contents into textbox (F3 or Space when box is empty)
            for (unsigned int i = strlen(parserInput->text); i < strlen(play.lastParserEntry); i++) {
              parserInput->KeyPress(play.lastParserEntry[i]);
            }
            //domouse(2);
            goto redraw_options;
          }
          else if ((gkey >= 32) || (gkey == 13) || (gkey == 8)) {
            parserInput->KeyPress(gkey);
            if (!parserInput->activated) {
              //domouse(2);
              goto redraw_options;
            }
          }
        }
        // Allow selection of options by keyboard shortcuts
        else if ((gkey >= '1') && (gkey <= '9')) {
          gkey -= '1';
          if (gkey < numdisp) {
            chose = disporder[gkey];
            break;
          }
        }
      }
      mousewason=mouseison;
      mouseison=-1;
      if (usingCustomRendering)
      {
        if ((mousex >= dirtyx) && (mousey >= dirtyy) &&
            (mousex < dirtyx + tempScrn->w) &&
            (mousey < dirtyy + tempScrn->h))
        {
          getDialogOptionUnderCursorFunc.param1 = &ccDialogOptionsRendering;
          run_function_on_non_blocking_thread(&getDialogOptionUnderCursorFunc);

          if (!getDialogOptionUnderCursorFunc.atLeastOneImplementationExists)
            quit("!The script function dialog_options_get_active is not implemented. It must be present to use a custom dialogue system.");

          mouseison = ccDialogOptionsRendering.activeOptionID;
        }
        else
        {
          ccDialogOptionsRendering.activeOptionID = -1;
        }
      }
      else if ((mousey <= dlgyp) || (mousey > curyp)) ;
      else {
        mouseison=numdisp-1;
        for (ww=0;ww<numdisp;ww++) {
          if (mousey < dispyp[ww]) { mouseison=ww-1; break; }
        }
        if ((mouseison<0) | (mouseison>=numdisp)) mouseison=-1;
      }

      if (parserInput != NULL) {
        int relativeMousey = mousey;
        if (usingCustomRendering)
          relativeMousey -= dirtyy;

        if ((relativeMousey > parserInput->y) && 
            (relativeMousey < parserInput->y + parserInput->hit))
          mouseison = DLG_OPTION_PARSER;

        if (parserInput->activated)
          parserActivated = 1;
      }

      int mouseButtonPressed = mgetbutton();

      if (mouseButtonPressed != NONE) {
        if (mouseison < 0) 
        {
          if (usingCustomRendering)
          {
            runDialogOptionMouseClickHandlerFunc.param1 = &ccDialogOptionsRendering;
            runDialogOptionMouseClickHandlerFunc.param2 = (void*)(mouseButtonPressed + 1);
            run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);

            if (runDialogOptionMouseClickHandlerFunc.atLeastOneImplementationExists)
              goto redraw_options;
          }
          continue;
        }
        if (mouseison == DLG_OPTION_PARSER) {
          // they clicked the text box
          parserActivated = 1;
        }
        else if (usingCustomRendering)
        {
          chose = mouseison;
          break;
        }
        else {
          chose=disporder[mouseison];
          break;
        }
      }

      if (usingCustomRendering)
      {
        int mouseWheelTurn = check_mouse_wheel();
        if (mouseWheelTurn != 0)
        {
            runDialogOptionMouseClickHandlerFunc.param1 = &ccDialogOptionsRendering;
            runDialogOptionMouseClickHandlerFunc.param2 = (void*)((mouseWheelTurn < 0) ? 9 : 8);
            run_function_on_non_blocking_thread(&runDialogOptionMouseClickHandlerFunc);

            if (runDialogOptionMouseClickHandlerFunc.atLeastOneImplementationExists)
              goto redraw_options;

            continue;
        }
      }

      if (parserActivated) {
        // They have selected a custom parser-based option
        if (parserInput->text[0] != 0) {
          chose = DLG_OPTION_PARSER;
          break;
        }
        else {
          parserActivated = 0;
          parserInput->activated = 0;
        }
      }
      if (mousewason != mouseison) {
        //domouse(2);
        goto redraw_options;
      }
      while ((timerloop == 0) && (play.fast_forward == 0)) {
        update_polled_stuff();
        platform->YieldCPU();
      }

    }
    if (!play.mouse_cursor_hidden)
      domouse(2);
  }
  else 
    chose = disporder[0];  // only one choice, so select it

  while (kbhit()) getch(); // empty keyboard buffer
  //leave_real_screen();
  construct_virtual_screen(true);

  if (parserActivated) 
  {
    strcpy (play.lastParserEntry, parserInput->text);
    ParseText (parserInput->text);
    chose = CHOSE_TEXTPARSER;
  }

  if (parserInput) {
    delete parserInput;
    parserInput = NULL;
  }

  if (ddb != NULL)
    gfxDriver->DestroyDDB(ddb);
  if (subBitmap != NULL)
    destroy_bitmap(subBitmap);

  set_mouse_cursor(curswas);
  // In case it's the QFG4 style dialog, remove the black screen
  play.in_conversation--;
  remove_screen_overlay(OVER_COMPLETE);

  wfreeblock(tempScrn);

  if (chose != CHOSE_TEXTPARSER)
  {
    dtop->optionflags[chose] |= DFLG_HASBEENCHOSEN;

    bool sayTheOption = false;
    if (sayChosenOption == SAYCHOSEN_YES)
    {
      sayTheOption = true;
    }
    else if (sayChosenOption == SAYCHOSEN_USEFLAG)
    {
      sayTheOption = ((dtop->optionflags[chose] & DFLG_NOREPEAT) == 0);
    }

    if (sayTheOption)
      DisplaySpeech(get_translation(dtop->optionnames[chose]), game.playercharacter);
  }

  return chose;
}

void do_conversation(int dlgnum) 
{
  EndSkippingUntilCharStops();

  int dlgnum_was = dlgnum;
  int previousTopics[MAX_TOPIC_HISTORY];
  int numPrevTopics = 0;
  DialogTopic *dtop = &dialog[dlgnum];

  // run the startup script
  int tocar = run_dialog_script(dtop, dlgnum, dtop->startupentrypoint, 0);
  if ((tocar == RUN_DIALOG_STOP_DIALOG) ||
      (tocar == RUN_DIALOG_GOTO_PREVIOUS)) 
  {
    // 'stop' or 'goto-previous' from first startup script
    remove_screen_overlay(OVER_COMPLETE);
    play.in_conversation--;
    return;
  }
  else if (tocar >= 0)
    dlgnum = tocar;

  while (dlgnum >= 0)
  {
    if (dlgnum >= game.numdialog)
      quit("!RunDialog: invalid dialog number specified");

    dtop = &dialog[dlgnum];

    if (dlgnum != dlgnum_was) 
    {
      // dialog topic changed, so play the startup
      // script for the new topic
      tocar = run_dialog_script(dtop, dlgnum, dtop->startupentrypoint, 0);
      dlgnum_was = dlgnum;
      if (tocar == RUN_DIALOG_GOTO_PREVIOUS) {
        if (numPrevTopics < 1) {
          // goto-previous on first topic -- end dialog
          tocar = RUN_DIALOG_STOP_DIALOG;
        }
        else {
          tocar = previousTopics[numPrevTopics - 1];
          numPrevTopics--;
        }
      }
      if (tocar == RUN_DIALOG_STOP_DIALOG)
        break;
      else if (tocar >= 0) {
        // save the old topic number in the history
        if (numPrevTopics < MAX_TOPIC_HISTORY) {
          previousTopics[numPrevTopics] = dlgnum;
          numPrevTopics++;
        }
        dlgnum = tocar;
        continue;
      }
    }

    int chose = show_dialog_options(dlgnum, SAYCHOSEN_USEFLAG, (game.options[OPT_RUNGAMEDLGOPTS] != 0));

    if (chose == CHOSE_TEXTPARSER)
    {
      said_speech_line = 0;
  
      tocar = run_dialog_request(dlgnum);

      if (said_speech_line > 0) {
        // fix the problem with the close-up face remaining on screen
        DisableInterface();
        mainloop(); // redraw the screen to make sure it looks right
        EnableInterface();
        set_mouse_cursor(CURS_ARROW);
      }
    }
    else 
    {
      tocar = run_dialog_script(dtop, dlgnum, dtop->entrypoints[chose], chose + 1);
    }

    if (tocar == RUN_DIALOG_GOTO_PREVIOUS) {
      if (numPrevTopics < 1) {
        tocar = RUN_DIALOG_STOP_DIALOG;
      }
      else {
        tocar = previousTopics[numPrevTopics - 1];
        numPrevTopics--;
      }
    }
    if (tocar == RUN_DIALOG_STOP_DIALOG) break;
    else if (tocar >= 0) {
      // save the old topic number in the history
      if (numPrevTopics < MAX_TOPIC_HISTORY) {
        previousTopics[numPrevTopics] = dlgnum;
        numPrevTopics++;
      }
      dlgnum = tocar;
    }

  }

}

// end dialog manager


// save game functions
#define SGVERSION 8
char*sgsig="Adventure Game Studio saved game";
int sgsiglen=32;
int find_highest_room_entered() {
  int qq,fndas=-1;
  for (qq=0;qq<MAX_ROOMS;qq++) {
    if (roomstats[qq].beenhere!=0) fndas=qq;
  }
  // This is actually legal - they might start in room 400 and save
  //if (fndas<0) quit("find_highest_room: been in no rooms?");
  return fndas;
}

void serialize_bitmap(block thispic, FILE*ooo) {
  if (thispic != NULL) {
    putw(thispic->w,ooo);
    putw(thispic->h,ooo);
    putw(bitmap_color_depth(thispic),ooo);
    for (int cc=0;cc<thispic->h;cc++)
      fwrite(&thispic->line[cc][0],thispic->w,bitmap_color_depth(thispic)/8,ooo);
    }
  }

long write_screen_shot_for_vista(FILE *ooo, block screenshot) 
{
  long fileSize = 0;
  char tempFileName[MAX_PATH];
  sprintf(tempFileName, "%s""_tmpscht.bmp", saveGameDirectory);
  
  save_bitmap(tempFileName, screenshot, palette);

  update_polled_stuff();
  
  if (exists(tempFileName))
  {
    fileSize = file_size(tempFileName);
    char *buffer = (char*)malloc(fileSize);

    FILE *input = fopen(tempFileName, "rb");
    fread(buffer, fileSize, 1, input);
    fclose(input);
    unlink(tempFileName);

    fwrite(buffer, fileSize, 1, ooo);
    free(buffer);
  }
  return fileSize;
}

#define MAGICNUMBER 0xbeefcafe
// Write the save game position to the file
void save_game_data (FILE *ooo, block screenshot) {
  int bb, cc, dd;

  platform->RunPluginHooks(AGSE_PRESAVEGAME, 0);

  putw(SGVERSION,ooo);
  // store the screenshot at the start to make it easily accesible
  putw((screenshot == NULL) ? 0 : 1, ooo);

  if (screenshot)
    serialize_bitmap(screenshot, ooo);

  fputstring(ACI_VERSION_TEXT, ooo);
  fputstring(usetup.main_data_filename, ooo);
  putw(scrnhit,ooo);
  putw(final_col_dep, ooo);
  putw(frames_per_second,ooo);
  putw(cur_mode,ooo);
  putw(cur_cursor,ooo);
  putw(offsetx,ooo); putw(offsety,ooo);
  putw(loopcounter,ooo);

  putw(spriteset.elements, ooo);
  for (bb = 1; bb < spriteset.elements; bb++) {
    if (game.spriteflags[bb] & SPF_DYNAMICALLOC) {
      putw(bb, ooo);
      fputc(game.spriteflags[bb], ooo);
      serialize_bitmap(spriteset[bb], ooo);
    }
  }
  // end of dynamic sprite list
  putw(0, ooo);

  // write the data segment of the global script
  int gdatasize=gameinst->globaldatasize;
  putw(gdatasize,ooo);
  ccFlattenGlobalData (gameinst);
  // MACPORT FIX: just in case gdatasize is 2 or 4, don't want to swap endian
  fwrite(&gameinst->globaldata[0], 1, gdatasize, ooo);
  ccUnFlattenGlobalData (gameinst);
  // write the script modules data segments
  putw(numScriptModules, ooo);
  for (bb = 0; bb < numScriptModules; bb++) {
    int glsize = moduleInst[bb]->globaldatasize;
    putw(glsize, ooo);
    if (glsize > 0) {
      ccFlattenGlobalData(moduleInst[bb]);
      fwrite(&moduleInst[bb]->globaldata[0], 1, glsize, ooo);
      ccUnFlattenGlobalData(moduleInst[bb]);
    }
  }

  putw(displayed_room, ooo);

  if (displayed_room >= 0) {
    // update the current room script's data segment copy
    if (roominst!=NULL)
      save_room_data_segment();

    // Update the saved interaction variable values
    for (ff = 0; ff < thisroom.numLocalVars; ff++)
      croom->interactionVariableValues[ff] = thisroom.localvars[ff].value;

  }

  // write the room state for all the rooms the player has been in
  for (bb = 0; bb < MAX_ROOMS; bb++) {
    if (roomstats[bb].beenhere) {
      fputc (1, ooo);
      fwrite(&roomstats[bb],sizeof(RoomStatus),1,ooo);
      if (roomstats[bb].tsdatasize>0)
        fwrite(&roomstats[bb].tsdata[0], 1, roomstats[bb].tsdatasize, ooo);
    }
    else
      fputc (0, ooo);
  }

  update_polled_stuff();

  if (play.cur_music_number >= 0) {
    if (IsMusicPlaying() == 0)
      play.cur_music_number = -1;
    }

  fwrite(&play,sizeof(GameState),1,ooo);

  for (bb = 0; bb < play.num_do_once_tokens; bb++)
  {
    fputstring(play.do_once_tokens[bb], ooo);
  }
  fwrite(&play.gui_draw_order[0], sizeof(int), game.numgui, ooo);

  fwrite(&mls[0],sizeof(MoveList), game.numcharacters + MAX_INIT_SPR + 1, ooo);

  fwrite(&game,sizeof(GameSetupStructBase),1,ooo);
  fwrite(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems, ooo);
  fwrite(&game.mcurs[0], sizeof(MouseCursor), game.numcursors, ooo);

  if (game.invScripts == NULL)
  {
    for (bb = 0; bb < game.numinvitems; bb++)
      fwrite (&game.intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo);
    for (bb = 0; bb < game.numcharacters; bb++)
      fwrite (&game.intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo); 
  }

  fwrite (&game.options[0], sizeof(int), OPT_HIGHESTOPTION+1, ooo);
  fputc (game.options[OPT_LIPSYNCTEXT], ooo);

  fwrite(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,ooo);
  fwrite(&charextra[0],sizeof(CharacterExtras),game.numcharacters,ooo);
  fwrite(&palette[0],sizeof(color),256,ooo);
  for (bb=0;bb<game.numdialog;bb++)
    fwrite(&dialog[bb].optionflags[0],sizeof(int),MAXTOPICOPTIONS,ooo);
  putw(mouse_on_iface,ooo);
  putw(mouse_on_iface_button,ooo);
  putw(mouse_pushed_iface,ooo);
  putw (ifacepopped, ooo);
  putw(game_paused,ooo);
  //putw(mi.trk,ooo);
  write_gui(ooo,guis,&game);
  putw(numAnimButs, ooo);
  fwrite(&animbuts[0], sizeof(AnimatingGUIButton), numAnimButs, ooo);

  putw(game.audioClipTypeCount, ooo);
  fwrite(&game.audioClipTypes[0], sizeof(AudioClipType), game.audioClipTypeCount, ooo);

  fwrite(&thisroom.regionLightLevel[0],sizeof(short), MAX_REGIONS,ooo);
  fwrite(&thisroom.regionTintLevel[0],sizeof(int), MAX_REGIONS,ooo);
  fwrite(&thisroom.walk_area_zoom[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);
  fwrite(&thisroom.walk_area_zoom2[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);

  fwrite (&ambient[0], sizeof(AmbientSound), MAX_SOUND_CHANNELS, ooo);
  putw(numscreenover,ooo);
  fwrite(&screenover[0],sizeof(ScreenOverlay),numscreenover,ooo);
  for (bb=0;bb<numscreenover;bb++) {
    serialize_bitmap (screenover[bb].pic, ooo);
  }

  update_polled_stuff();

  for (bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
  {
    if (dynamicallyCreatedSurfaces[bb] == NULL)
    {
      fputc(0, ooo);
    }
    else
    {
      fputc(1, ooo);
      serialize_bitmap(dynamicallyCreatedSurfaces[bb], ooo);
    }
  }

  update_polled_stuff();

  if (displayed_room >= 0) {

    for (bb = 0; bb < MAX_BSCENE; bb++) {
      if (play.raw_modified[bb])
        serialize_bitmap (thisroom.ebscene[bb], ooo);
    }

    putw ((raw_saved_screen == NULL) ? 0 : 1, ooo);
    if (raw_saved_screen)
      serialize_bitmap (raw_saved_screen, ooo);

    // save the current troom, in case they save in room 600 or whatever
    fwrite(&troom,sizeof(RoomStatus),1,ooo);
    if (troom.tsdatasize>0)
      fwrite(&troom.tsdata[0],troom.tsdatasize,1,ooo);

  }

  putw (numGlobalVars, ooo);
  fwrite (&globalvars[0], sizeof(InteractionVariable), numGlobalVars, ooo);

  putw (game.numviews, ooo);
  for (bb = 0; bb < game.numviews; bb++) {
    for (cc = 0; cc < views[bb].numLoops; cc++) {
      for (dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
      {
        putw(views[bb].loops[cc].frames[dd].sound, ooo);
        putw(views[bb].loops[cc].frames[dd].pic, ooo);
      }
    }
  }
  putw (MAGICNUMBER+1, ooo);

  putw(game.audioClipCount, ooo);
  for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
  {
    if ((channels[bb] != NULL) && (channels[bb]->done == 0) && (channels[bb]->sourceClip != NULL))
    {
      putw(((ScriptAudioClip*)channels[bb]->sourceClip)->id, ooo);
      putw(channels[bb]->get_pos(), ooo);
      putw(channels[bb]->priority, ooo);
      putw(channels[bb]->repeat ? 1 : 0, ooo);
      putw(channels[bb]->vol, ooo);
      putw(channels[bb]->panning, ooo);
      putw(channels[bb]->volAsPercentage, ooo);
      putw(channels[bb]->panningAsPercentage, ooo);
    }
    else
    {
      putw(-1, ooo);
    }
  }
  putw(crossFading, ooo);
  putw(crossFadeVolumePerStep, ooo);
  putw(crossFadeStep, ooo);
  putw(crossFadeVolumeAtStart, ooo);

  platform->RunPluginHooks(AGSE_SAVEGAME, (int)ooo);
  putw (MAGICNUMBER, ooo);  // to verify the plugins

  // save the room music volume
  putw(thisroom.options[ST_VOLUME], ooo);

  ccSerializeAllObjects(ooo);

  putw(current_music_type, ooo);

  update_polled_stuff();
}

// Some people have been having crashes with the save game list,
// so make sure the game name is valid
void safeguard_string (unsigned char *descript) {
  int it;
  for (it = 0; it < 50; it++) {
    if ((descript[it] < 1) || (descript[it] > 127))
      break;
  }
  if (descript[it] != 0)
    descript[it] = 0;
}

// On Windows we could just use IIDFromString but this is platform-independant
void convert_guid_from_text_to_binary(const char *guidText, unsigned char *buffer) 
{
  guidText++; // skip {
  for (int bytesDone = 0; bytesDone < 16; bytesDone++)
  {
    if (*guidText == '-')
      guidText++;

    char tempString[3];
    tempString[0] = guidText[0];
    tempString[1] = guidText[1];
    tempString[2] = 0;
    int thisByte = 0;
    sscanf(tempString, "%X", &thisByte);

    buffer[bytesDone] = thisByte;
    guidText += 2;
  }

  // Swap bytes to give correct GUID order
  unsigned char temp;
  temp = buffer[0]; buffer[0] = buffer[3]; buffer[3] = temp;
  temp = buffer[1]; buffer[1] = buffer[2]; buffer[2] = temp;
  temp = buffer[4]; buffer[4] = buffer[5]; buffer[5] = temp;
  temp = buffer[6]; buffer[6] = buffer[7]; buffer[7] = temp;
}

void save_game(int slotn, const char*descript) {
  
  // dont allow save in rep_exec_always, because we dont save
  // the state of blocked scripts
  can_run_delayed_command();

  if (inside_script) {
    strcpy(curscript->postScriptSaveSlotDescription[curscript->queue_action(ePSASaveGame, slotn, "SaveGameSlot")], descript);
    return;
  }

  if (platform->GetDiskFreeSpaceMB() < 2) {
    Display("ERROR: There is not enough disk space free to save the game. Clear some disk space and try again.");
    return;
  }

  VALIDATE_STRING(descript);
  char nametouse[260];
  get_save_game_path(slotn, nametouse);

  FILE *ooo = fopen(nametouse, "wb");
  if (ooo == NULL)
    quit("save_game: unable to open savegame file for writing");

  // Initialize and write Vista header
  RICH_GAME_MEDIA_HEADER vistaHeader;
  memset(&vistaHeader, 0, sizeof(RICH_GAME_MEDIA_HEADER));
  memcpy(&vistaHeader.dwMagicNumber, RM_MAGICNUMBER, sizeof(long));
  vistaHeader.dwHeaderVersion = 1;
  vistaHeader.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
  vistaHeader.dwThumbnailOffsetHigherDword = 0;
  vistaHeader.dwThumbnailOffsetLowerDword = 0;
  vistaHeader.dwThumbnailSize = 0;
  convert_guid_from_text_to_binary(game.guid, &vistaHeader.guidGameId[0]);
  uconvert(game.gamename, U_ASCII, (char*)&vistaHeader.szGameName[0], U_UNICODE, RM_MAXLENGTH);
  uconvert(descript, U_ASCII, (char*)&vistaHeader.szSaveName[0], U_UNICODE, RM_MAXLENGTH);
  vistaHeader.szLevelName[0] = 0;
  vistaHeader.szComments[0] = 0;

  fwrite(&vistaHeader, sizeof(RICH_GAME_MEDIA_HEADER), 1, ooo);

  fwrite(sgsig,sgsiglen,1,ooo);

  safeguard_string ((unsigned char*)descript);

  fputstring((char*)descript,ooo);

  block screenShot = NULL;

  if (game.options[OPT_SAVESCREENSHOT]) {
    int usewid = multiply_up_coordinate(play.screenshot_width);
    int usehit = multiply_up_coordinate(play.screenshot_height);
    if (usewid > virtual_screen->w)
      usewid = virtual_screen->w;
    if (usehit > virtual_screen->h)
      usehit = virtual_screen->h;

    if ((play.screenshot_width < 16) || (play.screenshot_height < 16))
      quit("!Invalid game.screenshot_width/height, must be from 16x16 to screen res");

    if (gfxDriver->UsesMemoryBackBuffer())
    {
      screenShot = create_bitmap_ex(bitmap_color_depth(virtual_screen), usewid, usehit);

      stretch_blit(virtual_screen, screenShot, 0, 0,
        virtual_screen->w, virtual_screen->h, 0, 0,
        screenShot->w, screenShot->h);
    }
    else
    {
      block tempBlock = create_bitmap_ex(final_col_dep, virtual_screen->w, virtual_screen->h);
      gfxDriver->GetCopyOfScreenIntoBitmap(tempBlock);

      screenShot = create_bitmap_ex(final_col_dep, usewid, usehit);
      stretch_blit(tempBlock, screenShot, 0, 0,
        tempBlock->w, tempBlock->h, 0, 0,
        screenShot->w, screenShot->h);

      destroy_bitmap(tempBlock);
    }
  }

  update_polled_stuff();

  save_game_data(ooo, screenShot);

  if (screenShot != NULL)
  {
    long screenShotOffset = ftell(ooo) - sizeof(RICH_GAME_MEDIA_HEADER);
    long screenShotSize = write_screen_shot_for_vista(ooo, screenShot);
    fclose(ooo);

    update_polled_stuff();

    ooo = fopen(nametouse, "r+b");
    fseek(ooo, 12, SEEK_SET);
    putw(screenShotOffset, ooo);
    fseek(ooo, 4, SEEK_CUR);
    putw(screenShotSize, ooo);
  }

  if (screenShot != NULL)
    free(screenShot);

  fclose(ooo);
}

block read_serialized_bitmap(FILE* ooo) {
  block thispic;
  int picwid = getw(ooo);
  int pichit = getw(ooo);
  int piccoldep = getw(ooo);
  thispic = create_bitmap_ex(piccoldep,picwid,pichit);
  if (thispic == NULL)
    return NULL;
  for (int vv=0; vv < pichit; vv++)
    fread(&thispic->line[vv][0], picwid, piccoldep/8, ooo);
  return thispic;
}

char rbuffer[200];

void first_room_initialization() {
  starting_room = displayed_room;
  t1 = time(NULL);
  lastcounter=0;
  loopcounter=0;
  mouse_z_was = mouse_z;
}

int restore_game_data (FILE *ooo, const char *nametouse) {
  int vv, bb;

  if (getw(ooo)!=SGVERSION) {
    fclose(ooo);
    return -3;
  }
  int isScreen = getw(ooo);
  if (isScreen) {
    // skip the screenshot
    wfreeblock(read_serialized_bitmap(ooo));
  }

  fgetstring_limit(rbuffer, ooo, 200);
  int vercmp = strcmp(rbuffer, ACI_VERSION_TEXT);
  if ((vercmp > 0) || (strcmp(rbuffer, LOWEST_SGVER_COMPAT) < 0) ||
      (strlen(rbuffer) > strlen(LOWEST_SGVER_COMPAT))) {
    fclose(ooo);
    return -4;
  }
  fgetstring_limit (rbuffer, ooo, 180);
  rbuffer[180] = 0;
  if (stricmp (rbuffer, usetup.main_data_filename)) {
    fclose(ooo);
    return -5;
  }
  int gamescrnhit = getw(ooo);
  // a 320x240 game, they saved in a 320x200 room but try to restore
  // from within a 320x240 room, make it work
  if (final_scrn_hit == (gamescrnhit * 12) / 10)
    gamescrnhit = scrnhit;
  // they saved in a 320x240 room but try to restore from a 320x200
  // room, fix it
  else if (gamescrnhit == final_scrn_hit)
    gamescrnhit = scrnhit;

  if (gamescrnhit != scrnhit) {
    Display("This game was saved with the interpreter running at a different "
      "resolution. It cannot be restored.");
    fclose(ooo);
    return -6;
  }

  if (getw(ooo) != final_col_dep) {
    Display("This game was saved with the engine running at a different colour depth. It cannot be restored.");
    fclose(ooo);
    return -7;
  }

  unload_old_room();

  remove_screen_overlay(-1);
  is_complete_overlay=0; is_text_overlay=0;
  set_game_speed(getw(ooo));
  int sg_cur_mode=getw(ooo);
  int sg_cur_cursor=getw(ooo);
  offsetx = getw(ooo);
  offsety = getw(ooo);
  loopcounter = getw(ooo);

  for (bb = 1; bb < spriteset.elements; bb++) {
    if (game.spriteflags[bb] & SPF_DYNAMICALLOC) {
      // do this early, so that it changing guibuts doesn't
      // affect the restored data
      free_dynamic_sprite(bb);
    }
  }
  // ensure the sprite set is at least as large as it was
  // when the game was saved
  spriteset.enlargeTo(getw(ooo));
  // get serialized dynamic sprites
  int sprnum = getw(ooo);
  while (sprnum) {
    unsigned char spriteflag = fgetc(ooo);
    add_dynamic_sprite(sprnum, read_serialized_bitmap(ooo));
    game.spriteflags[sprnum] = spriteflag;
    sprnum = getw(ooo);
  }

  clear_music_cache();

  for (vv = 0; vv < game.numgui; vv++) {
    if (guibg[vv])
      wfreeblock (guibg[vv]);
    guibg[vv] = NULL;

    if (guibgbmp[vv])
      gfxDriver->DestroyDDB(guibgbmp[vv]);
    guibgbmp[vv] = NULL;
  }
  
  update_polled_stuff();

  ccFreeInstance(gameinstFork);
  ccFreeInstance(gameinst);
  gameinstFork = NULL;
  gameinst = NULL;
  for (vv = 0; vv < numScriptModules; vv++) {
    ccFreeInstance(moduleInstFork[vv]);
    ccFreeInstance(moduleInst[vv]);
    moduleInst[vv] = NULL;
  }

  if (dialogScriptsInst != NULL)
  {
    ccFreeInstance(dialogScriptsInst);
    dialogScriptsInst = NULL;
  }

  update_polled_stuff();

  // read the global script data segment
  int gdatasize = getw(ooo);
  char *newglobaldatabuffer = (char*)malloc(gdatasize);
  fread(newglobaldatabuffer, sizeof(char), gdatasize, ooo);
  //fread(&gameinst->globaldata[0],gdatasize,1,ooo);
  //ccUnFlattenGlobalData (gameinst);

  char *scriptModuleDataBuffers[MAX_SCRIPT_MODULES];
  int scriptModuleDataSize[MAX_SCRIPT_MODULES];

  if (getw(ooo) != numScriptModules)
    quit("wrong script module count; cannot restore game");
  for (vv = 0; vv < numScriptModules; vv++) {
    scriptModuleDataSize[vv] = getw(ooo);
    scriptModuleDataBuffers[vv] = (char*)malloc(scriptModuleDataSize[vv]);
    fread(&scriptModuleDataBuffers[vv][0], sizeof(char), scriptModuleDataSize[vv], ooo);
  }

  displayed_room = getw(ooo);

  // now the rooms
  for (vv=0;vv<MAX_ROOMS;vv++) {
    if (roomstats[vv].tsdata==NULL) ;
    else if (roomstats[vv].tsdatasize>0) {
      free(roomstats[vv].tsdata);
      roomstats[vv].tsdatasize=0; roomstats[vv].tsdata=NULL;
      }
    roomstats[vv].beenhere=0;
    }
  long gobackto=ftell(ooo);
  fclose(ooo);
  ooo=fopen(nametouse,"rb");
  fseek(ooo,gobackto,SEEK_SET);

  // read the room state for all the rooms the player has been in
  for (vv=0;vv<MAX_ROOMS;vv++) {
    if ((roomstats[vv].tsdatasize>0) & (roomstats[vv].tsdata!=NULL))
      free(roomstats[vv].tsdata);
    roomstats[vv].tsdatasize=0;
    roomstats[vv].tsdata=NULL;
    roomstats[vv].beenhere = fgetc (ooo);

    if (roomstats[vv].beenhere) {
      fread(&roomstats[vv],sizeof(RoomStatus),1,ooo);
      if (roomstats[vv].tsdatasize>0) {
        roomstats[vv].tsdata=(char*)malloc(roomstats[vv].tsdatasize+8);
        fread(&roomstats[vv].tsdata[0],roomstats[vv].tsdatasize,1,ooo);
      }
    }
  }

/*  for (vv=0;vv<MAX_ROOMS;vv++) {
    if ((roomstats[vv].tsdatasize>0) & (roomstats[vv].tsdata!=NULL))
      free(roomstats[vv].tsdata);
    roomstats[vv].tsdatasize=0;
    roomstats[vv].tsdata=NULL;
  }
  int numtoread=getw(ooo);
  if ((numtoread < 0) | (numtoread>MAX_ROOMS)) {
    sprintf(rbuffer,"Save game has invalid value for rooms_entered: %d",numtoread);
    quit(rbuffer);
    }
  fread(&roomstats[0],sizeof(RoomStatus),numtoread,ooo);
  for (vv=0;vv<numtoread;vv++) {
    if (roomstats[vv].tsdatasize>0) {
      roomstats[vv].tsdata=(char*)malloc(roomstats[vv].tsdatasize+5);
      fread(&roomstats[vv].tsdata[0],roomstats[vv].tsdatasize,1,ooo);
      }
    else roomstats[vv].tsdata=NULL;
    }*/

  int speech_was = play.want_speech, musicvox = play.seperate_music_lib;
  // preserve the replay settings
  int playback_was = play.playback, recording_was = play.recording;
  int gamestep_was = play.gamestep;
  int screenfadedout_was = play.screen_is_faded_out;
  int roomchanges_was = play.room_changes;
  // make sure the pointer is preserved
  int *gui_draw_order_was = play.gui_draw_order;

  free_do_once_tokens();

  //fread (&play, 76, 4, ooo);
  //fread (((char*)&play) + 78*4, sizeof(GameState) - 78*4, 1, ooo);
  fread(&play,sizeof(GameState),1,ooo);
  // Preserve whether the music vox is available
  play.seperate_music_lib = musicvox;
  // If they had the vox when they saved it, but they don't now
  if ((speech_was < 0) && (play.want_speech >= 0))
    play.want_speech = (-play.want_speech) - 1;
  // If they didn't have the vox before, but now they do
  else if ((speech_was >= 0) && (play.want_speech < 0))
    play.want_speech = (-play.want_speech) - 1;

  play.screen_is_faded_out = screenfadedout_was;
  play.playback = playback_was;
  play.recording = recording_was;
  play.gamestep = gamestep_was;
  play.room_changes = roomchanges_was;
  play.gui_draw_order = gui_draw_order_was;

  if (play.num_do_once_tokens > 0)
  {
    play.do_once_tokens = (char**)malloc(sizeof(char*) * play.num_do_once_tokens);
    for (bb = 0; bb < play.num_do_once_tokens; bb++)
    {
      fgetstring_limit(rbuffer, ooo, 200);
      play.do_once_tokens[bb] = (char*)malloc(strlen(rbuffer) + 1);
      strcpy(play.do_once_tokens[bb], rbuffer);
    }
  }

  fread(&play.gui_draw_order[0], sizeof(int), game.numgui, ooo);
  fread(&mls[0],sizeof(MoveList), game.numcharacters + MAX_INIT_SPR + 1, ooo);

  // save pointer members before reading
  char* gswas=game.globalscript;
  ccScript* compsc=game.compiled_script;
  CharacterInfo* chwas=game.chars;
  WordsDictionary *olddict = game.dict;
  char* mesbk[MAXGLOBALMES];
  int numchwas = game.numcharacters;
  for (vv=0;vv<MAXGLOBALMES;vv++) mesbk[vv]=game.messages[vv];
  int numdiwas = game.numdialog, numinvwas = game.numinvitems;
  int numviewswas = game.numviews;
  int numGuisWas = game.numgui;

  fread(&game,sizeof(GameSetupStructBase),1,ooo);

  if (game.numdialog!=numdiwas)
    quit("!Restore_Game: Game has changed (dlg), unable to restore");
  if ((numchwas != game.numcharacters) || (numinvwas != game.numinvitems))
    quit("!Restore_Game: Game has changed (inv), unable to restore position");
  if (game.numviews != numviewswas)
    quit("!Restore_Game: Game has changed (views), unable to restore position");

  fread(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems, ooo);
  fread(&game.mcurs[0], sizeof(MouseCursor), game.numcursors, ooo);

  if (game.invScripts == NULL)
  {
    for (bb = 0; bb < game.numinvitems; bb++)
      fread (&game.intrInv[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo);
    for (bb = 0; bb < game.numcharacters; bb++)
      fread (&game.intrChar[bb]->timesRun[0], sizeof (int), MAX_NEWINTERACTION_EVENTS, ooo);
  }

  // restore pointer members
  game.globalscript=gswas;
  game.compiled_script=compsc;
  game.chars=chwas;
  game.dict = olddict;
  for (vv=0;vv<MAXGLOBALMES;vv++) game.messages[vv]=mesbk[vv];

  fread(&game.options[0], sizeof(int), OPT_HIGHESTOPTION+1, ooo);
  game.options[OPT_LIPSYNCTEXT] = fgetc(ooo);

  fread(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,ooo);
  fread(&charextra[0],sizeof(CharacterExtras),game.numcharacters,ooo);
  if (roominst!=NULL) {  // so it doesn't overwrite the tsdata
    ccFreeInstance(roominstFork);
    ccFreeInstance(roominst); 
    roominstFork = NULL;
    roominst=NULL;
  }
  fread(&palette[0],sizeof(color),256,ooo);
  for (vv=0;vv<game.numdialog;vv++)
    fread(&dialog[vv].optionflags[0],sizeof(int),MAXTOPICOPTIONS,ooo);
  mouse_on_iface=getw(ooo);
  mouse_on_iface_button=getw(ooo);
  mouse_pushed_iface=getw(ooo);
  ifacepopped = getw(ooo);
  game_paused=getw(ooo);

  for (vv = 0; vv < game.numgui; vv++)
    unexport_gui_controls(vv);

  read_gui(ooo,guis,&game);

  if (numGuisWas != game.numgui)
    quit("!Restore_Game: Game has changed (GUIs), unable to restore position");

  for (vv = 0; vv < game.numgui; vv++)
    export_gui_controls(vv);

  numAnimButs = getw(ooo);
  fread(&animbuts[0], sizeof(AnimatingGUIButton), numAnimButs, ooo);

  if (getw(ooo) != game.audioClipTypeCount)
    quit("!Restore_Game: game has changed (audio types), unable to restore");

  fread(&game.audioClipTypes[0], sizeof(AudioClipType), game.audioClipTypeCount, ooo);

  short saved_light_levels[MAX_REGIONS];
  int   saved_tint_levels[MAX_REGIONS];
  fread(&saved_light_levels[0], sizeof(short), MAX_REGIONS, ooo);
  fread(&saved_tint_levels[0], sizeof(int), MAX_REGIONS, ooo);

  short saved_zoom_levels1[MAX_WALK_AREAS + 1];
  short saved_zoom_levels2[MAX_WALK_AREAS + 1];
  fread(&saved_zoom_levels1[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);
  fread(&saved_zoom_levels2[0],sizeof(short), MAX_WALK_AREAS + 1, ooo);

  int doAmbient[MAX_SOUND_CHANNELS], cc, dd;
  int crossfadeInChannelWas = play.crossfading_in_channel;
  int crossfadeOutChannelWas = play.crossfading_out_channel;

  for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
  {
    stop_and_destroy_channel_ex(bb, false);
  }

  play.crossfading_in_channel = crossfadeInChannelWas;
  play.crossfading_out_channel = crossfadeOutChannelWas;

  fread(&ambient[0], sizeof(AmbientSound), MAX_SOUND_CHANNELS, ooo);

  for (bb = 1; bb < MAX_SOUND_CHANNELS; bb++) {
    if (ambient[bb].channel == 0)
      doAmbient[bb] = 0;
    else {
      doAmbient[bb] = ambient[bb].num;
      ambient[bb].channel = 0;
    }
  }

  numscreenover = getw(ooo);
  fread(&screenover[0],sizeof(ScreenOverlay),numscreenover,ooo);
  for (bb=0;bb<numscreenover;bb++) {
    if (screenover[bb].pic != NULL)
    {
      screenover[bb].pic = read_serialized_bitmap(ooo);
      screenover[bb].bmp = gfxDriver->CreateDDBFromBitmap(screenover[bb].pic, false);
    }
  }

  update_polled_stuff();

  // load into a temp array since ccUnserialiseObjects will destroy
  // it otherwise
  block dynamicallyCreatedSurfacesFromSaveGame[MAX_DYNAMIC_SURFACES];
  for (bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
  {
    if (fgetc(ooo) == 0)
    {
      dynamicallyCreatedSurfacesFromSaveGame[bb] = NULL;
    }
    else
    {
      dynamicallyCreatedSurfacesFromSaveGame[bb] = read_serialized_bitmap(ooo);
    }
  }

  update_polled_stuff();

  block newbscene[MAX_BSCENE];
  for (bb = 0; bb < MAX_BSCENE; bb++)
    newbscene[bb] = NULL;

  if (displayed_room >= 0) {

    for (bb = 0; bb < MAX_BSCENE; bb++) {
      newbscene[bb] = NULL;
      if (play.raw_modified[bb]) {
        newbscene[bb] = read_serialized_bitmap (ooo);
      }
    }
    bb = getw(ooo);
    if (raw_saved_screen != NULL) {
      wfreeblock(raw_saved_screen);
      raw_saved_screen = NULL;
    }
    if (bb)
      raw_saved_screen = read_serialized_bitmap(ooo);

    if (troom.tsdata != NULL)
      free (troom.tsdata);
    // get the current troom, in case they save in room 600 or whatever
    fread(&troom,sizeof(RoomStatus),1,ooo);
    if (troom.tsdatasize > 0) {
      troom.tsdata=(char*)malloc(troom.tsdatasize+5);
      fread(&troom.tsdata[0],troom.tsdatasize,1,ooo);
    }
    else
      troom.tsdata = NULL;

  }

  if (getw (ooo) != numGlobalVars) 
    quit("!Game has been modified since save; unable to restore game (GM01)");

  fread (&globalvars[0], sizeof(InteractionVariable), numGlobalVars, ooo);

  if (getw(ooo) != game.numviews)
    quit("!Game has been modified since save; unable to restore (GV02)");

  for (bb = 0; bb < game.numviews; bb++) {
    for (cc = 0; cc < views[bb].numLoops; cc++) {
      for (dd = 0; dd < views[bb].loops[cc].numFrames; dd++)
      {
        views[bb].loops[cc].frames[dd].sound = getw(ooo);
        views[bb].loops[cc].frames[dd].pic = getw(ooo);
      }
    }
  }

  if (getw(ooo) != MAGICNUMBER+1)
    quit("!Game has been modified since save; unable to restore (GV03)");

  if (getw(ooo) != game.audioClipCount)
    quit("Game has changed: different audio clip count");

  play.crossfading_in_channel = 0;
  play.crossfading_out_channel = 0;
  int channelPositions[MAX_SOUND_CHANNELS + 1];
  for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
  {
    channelPositions[bb] = 0;
    int audioClipIndex = getw(ooo);
    if (audioClipIndex >= 0)
    {
      if (audioClipIndex >= game.audioClipCount)
        quit("save game error: invalid audio clip index");

      channelPositions[bb] = getw(ooo);
      if (channelPositions[bb] < 0) channelPositions[bb] = 0;
      int priority = getw(ooo);
      int repeat = getw(ooo);
      int vol = getw(ooo);
      int pan = getw(ooo);
      int volAsPercent = getw(ooo);
      int panAsPercent = getw(ooo);
      play_audio_clip_on_channel(bb, &game.audioClips[audioClipIndex], priority, repeat, channelPositions[bb]);
      if (channels[bb] != NULL)
      {
        channels[bb]->set_panning(pan);
        channels[bb]->set_volume(vol);
        channels[bb]->panningAsPercentage = panAsPercent;
        channels[bb]->volAsPercentage = volAsPercent;
      }
    }
  }
  if ((crossfadeInChannelWas > 0) && (channels[crossfadeInChannelWas] != NULL))
    play.crossfading_in_channel = crossfadeInChannelWas;
  if ((crossfadeOutChannelWas > 0) && (channels[crossfadeOutChannelWas] != NULL))
    play.crossfading_out_channel = crossfadeOutChannelWas;

  // If there were synced audio tracks, the time taken to load in the
  // different channels will have thrown them out of sync, so re-time it
  for (bb = 0; bb <= MAX_SOUND_CHANNELS; bb++)
  {
    if ((channelPositions[bb] > 0) && (channels[bb] != NULL) && (channels[bb]->done == 0))
    {
      channels[bb]->seek(channelPositions[bb]);
    }
  }
  crossFading = getw(ooo);
  crossFadeVolumePerStep = getw(ooo);
  crossFadeStep = getw(ooo);
  crossFadeVolumeAtStart = getw(ooo);

  recache_queued_clips_after_loading_save_game();

  platform->RunPluginHooks(AGSE_RESTOREGAME, (int)ooo);
  if (getw(ooo) != (unsigned)MAGICNUMBER)
    quit("!One of the game plugins did not restore its game data correctly.");

  // save the new room music vol for later use
  int newRoomVol = getw(ooo);

  if (ccUnserializeAllObjects(ooo, &ccUnserializer))
    quitprintf("LoadGame: Error during deserialization: %s", ccErrorString);

  // preserve legacy music type setting
  current_music_type = getw(ooo);

  fclose(ooo);

  // restore these to the ones retrieved from the save game
  for (bb = 0; bb < MAX_DYNAMIC_SURFACES; bb++)
  {
    dynamicallyCreatedSurfaces[bb] = dynamicallyCreatedSurfacesFromSaveGame[bb];
  }

  if (create_global_script())
    quitprintf("Unable to recreate global script: %s", ccErrorString);

  if (gameinst->globaldatasize != gdatasize)
    quit("!Restore_game: Global script changed, cannot restore game");

  // read the global data into the newly created script
  memcpy(&gameinst->globaldata[0], newglobaldatabuffer, gdatasize);
  free(newglobaldatabuffer);
  ccUnFlattenGlobalData(gameinst);

  // restore the script module data
  for (bb = 0; bb < numScriptModules; bb++) {
    if (scriptModuleDataSize[bb] != moduleInst[bb]->globaldatasize)
      quit("!Restore Game: script module global data changed, unable to restore");
    memcpy(&moduleInst[bb]->globaldata[0], scriptModuleDataBuffers[bb], scriptModuleDataSize[bb]);
    free(scriptModuleDataBuffers[bb]);
    ccUnFlattenGlobalData(moduleInst[bb]);
  }
  

  setup_player_character(game.playercharacter);

  int gstimer=play.gscript_timer;
  int oldx1 = play.mboundx1, oldx2 = play.mboundx2;
  int oldy1 = play.mboundy1, oldy2 = play.mboundy2;
  int musicWasRepeating = play.current_music_repeating;
  int newms = play.cur_music_number;

  // disable the queue momentarily
  int queuedMusicSize = play.music_queue_size;
  play.music_queue_size = 0;

  update_polled_stuff();

  if (displayed_room >= 0)
    load_new_room(displayed_room,NULL);//&game.chars[game.playercharacter]);

  update_polled_stuff();

  play.gscript_timer=gstimer;

  // restore the correct room volume (they might have modified
  // it with SetMusicVolume)
  thisroom.options[ST_VOLUME] = newRoomVol;

  filter->SetMouseLimit(oldx1,oldy1,oldx2,oldy2);
  
  set_cursor_mode(sg_cur_mode);
  set_mouse_cursor(sg_cur_cursor);
  if (sg_cur_mode == MODE_USE)
    SetActiveInventory (playerchar->activeinv);
  // ensure that the current cursor is locked
  spriteset.precache(game.mcurs[sg_cur_cursor].pic);

#if (ALLEGRO_DATE > 19990103)
  set_window_title(play.game_name);
#endif

  update_polled_stuff();

  if (displayed_room >= 0) {

    for (bb = 0; bb < MAX_BSCENE; bb++) {
      if (newbscene[bb]) {
        wfreeblock(thisroom.ebscene[bb]);
        thisroom.ebscene[bb] = newbscene[bb];
      }
    }

    in_new_room=3;  // don't run "enters screen" events
    // now that room has loaded, copy saved light levels in
    memcpy(&thisroom.regionLightLevel[0],&saved_light_levels[0],sizeof(short)*MAX_REGIONS);
    memcpy(&thisroom.regionTintLevel[0],&saved_tint_levels[0],sizeof(int)*MAX_REGIONS);
    generate_light_table();

    memcpy(&thisroom.walk_area_zoom[0], &saved_zoom_levels1[0], sizeof(short) * (MAX_WALK_AREAS + 1));
    memcpy(&thisroom.walk_area_zoom2[0], &saved_zoom_levels2[0], sizeof(short) * (MAX_WALK_AREAS + 1));

    on_background_frame_change();

  }

  gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);
/*
  play_sound(-1);
  
  stopmusic();
  // use the repeat setting when the current track was started
  int musicRepeatSetting = play.music_repeat;
  SetMusicRepeat(musicWasRepeating);
  if (newms>=0) {
    // restart the background music
    if (newms == 1000)
      PlayMP3File (play.playmp3file_name);
    else {
      play.cur_music_number=2000;  // make sure it gets played
      newmusic(newms);
    }
  }
  SetMusicRepeat(musicRepeatSetting);
  if (play.silent_midi)
    PlaySilentMIDI (play.silent_midi);
  SeekMIDIPosition(midipos);
  //SeekMODPattern (modtrack);
  //SeekMP3PosMillis (mp3mpos);

  if (musicpos > 0) {
    // For some reason, in Prodigal after this Seek line is called
    // it can cause the next update_polled_stuff to crash;
    // must be some sort of bug in AllegroMP3
    if ((crossFading > 0) && (channels[crossFading] != NULL))
      channels[crossFading]->seek(musicpos);
    else if (channels[SCHAN_MUSIC] != NULL)
      channels[SCHAN_MUSIC]->seek(musicpos);
  }*/

  // restore the queue now that the music is playing
  play.music_queue_size = queuedMusicSize;
  
  if (play.digital_master_volume >= 0)
    System_SetVolume(play.digital_master_volume);

  for (vv = 1; vv < MAX_SOUND_CHANNELS; vv++) {
    if (doAmbient[vv])
      PlayAmbientSound(vv, doAmbient[vv], ambient[vv].vol, ambient[vv].x, ambient[vv].y);
  }

  for (vv = 0; vv < game.numgui; vv++) {
    guibg[vv] = create_bitmap_ex (final_col_dep, guis[vv].wid, guis[vv].hit);
    guibg[vv] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[vv]);
  }

  if (gfxDriver->SupportsGammaControl())
    gfxDriver->SetGamma(play.gamma_adjustment);

  guis_need_update = 1;

  play.ignore_user_input_until_time = 0;
  update_polled_stuff();

  platform->RunPluginHooks(AGSE_POSTRESTOREGAME, 0);

  if (displayed_room < 0) {
    // the restart point, no room was loaded
    load_new_room(playerchar->room, playerchar);
    playerchar->prevroom = -1;

    first_room_initialization();
  }

  if ((play.music_queue_size > 0) && (cachedQueuedMusic == NULL)) {
    cachedQueuedMusic = load_music_from_disk(play.music_queue[0], 0);
  }

  return 0;
}

void add_dynamic_sprite(int gotSlot, block redin, bool hasAlpha) {

  spriteset.set(gotSlot, redin);

  game.spriteflags[gotSlot] = SPF_DYNAMICALLOC;

  if (bitmap_color_depth(redin) > 8)
    game.spriteflags[gotSlot] |= SPF_HICOLOR;
  if (bitmap_color_depth(redin) > 16)
    game.spriteflags[gotSlot] |= SPF_TRUECOLOR;
  if (hasAlpha)
    game.spriteflags[gotSlot] |= SPF_ALPHACHANNEL;

  spritewidth[gotSlot] = redin->w;
  spriteheight[gotSlot] = redin->h;
}

void free_dynamic_sprite (int gotSlot) {
  int tt;

  if ((gotSlot < 0) || (gotSlot >= spriteset.elements))
    quit("!FreeDynamicSprite: invalid slot number");

  if ((game.spriteflags[gotSlot] & SPF_DYNAMICALLOC) == 0)
    quitprintf("!DeleteSprite: Attempted to free static sprite %d that was not loaded by the script", gotSlot);

  wfreeblock(spriteset[gotSlot]);
  spriteset.set(gotSlot, NULL);

  game.spriteflags[gotSlot] = 0;
  spritewidth[gotSlot] = 0;
  spriteheight[gotSlot] = 0;

  // ensure it isn't still on any GUI buttons
  for (tt = 0; tt < numguibuts; tt++) {
    if (guibuts[tt].IsDeleted())
      continue;
    if (guibuts[tt].pic == gotSlot)
      guibuts[tt].pic = 0;
    if (guibuts[tt].usepic == gotSlot)
      guibuts[tt].usepic = 0;
    if (guibuts[tt].overpic == gotSlot)
      guibuts[tt].overpic = 0;
    if (guibuts[tt].pushedpic == gotSlot)
      guibuts[tt].pushedpic = 0;
  }

  // force refresh of any object caches using the sprite
  if (croom != NULL) 
  {
    for (tt = 0; tt < croom->numobj; tt++) 
    {
      if (objs[tt].num == gotSlot)
      {
        objs[tt].num = 0;
        objcache[tt].sppic = -1;
      }
      else if (objcache[tt].sppic == gotSlot)
        objcache[tt].sppic = -1;
    }
  }
}

int do_game_load(const char *nametouse, int slotNumber, char *descrp, int *wantShot)
{
  gameHasBeenRestored++;

  FILE*ooo=fopen(nametouse,"rb");
  if (ooo==NULL)
    return -1;

  // skip Vista header
  fseek(ooo, sizeof(RICH_GAME_MEDIA_HEADER), SEEK_SET);

  fread(rbuffer,sgsiglen,1,ooo);
  rbuffer[sgsiglen]=0;
  if (strcmp(rbuffer,sgsig)!=0) {
    // not a save game
    fclose(ooo);
    return -2; 
  }
  int oldeip = our_eip;
  our_eip = 2050;

  fgetstring_limit(rbuffer,ooo, 180);
  rbuffer[180] = 0;
  safeguard_string ((unsigned char*)rbuffer);

  if (descrp!=NULL) {
    // just want slot description, so return
    strcpy(descrp,rbuffer);
    fclose (ooo);
    our_eip = oldeip;
    return 0;
  }

  if (wantShot != NULL) {
    // just want the screenshot
    if (getw(ooo)!=SGVERSION) {
      fclose(ooo);
      return -3;
    }
    int isScreen = getw(ooo);
    *wantShot = 0;

    if (isScreen) {
      int gotSlot = spriteset.findFreeSlot();
      // load the screenshot
      block redin = read_serialized_bitmap(ooo);
      if (gotSlot > 0) {
        // add it into the sprite set
        add_dynamic_sprite(gotSlot, gfxDriver->ConvertBitmapToSupportedColourDepth(redin));

        *wantShot = gotSlot;
      }
      else
      {
        destroy_bitmap(redin);
      }
    }
    fclose (ooo);
    our_eip = oldeip;
    return 0;
  }

  our_eip = 2051;

  // do the actual restore
  int ress = restore_game_data(ooo, nametouse);

  our_eip = oldeip;

  if (ress == -5) {
    // saved in different game
    RunAGSGame (rbuffer, 0, 0);
    load_new_game_restore = slotNumber;
    return 0;
  }

  if (ress)
    return ress;

  run_on_event (GE_RESTORE_GAME, slotNumber);

  // ensure keyboard buffer is clean
  // use the raw versions rather than the rec_ versions so we don't
  // interfere with the replay sync
  while (keypressed()) readkey();

  return 0;
}

int load_game(int slotn, char*descrp, int *wantShot) {
  char nametouse[260];
  get_save_game_path(slotn, nametouse);

  return do_game_load(nametouse, slotn, descrp, wantShot);
}

#define ICONSPERLINE 4

struct DisplayInvItem {
  int num;
  int sprnum;
  };
int __actual_invscreen() {
  
  int BUTTONAREAHEIGHT = get_fixed_pixel_size(30);
  int cmode=CURS_ARROW, toret = -1;
  int top_item = 0, num_visible_items = 0;
  int MAX_ITEMAREA_HEIGHT = ((scrnhit - BUTTONAREAHEIGHT) - get_fixed_pixel_size(20));
  in_inv_screen++;
  inv_screen_newroom = -1;

start_actinv:
  wsetscreen(virtual_screen);
  
  DisplayInvItem dii[MAX_INV];
  int numitems=0,ww,widest=0,highest=0;
  if (charextra[game.playercharacter].invorder_count < 0)
    update_invorder();
  if (charextra[game.playercharacter].invorder_count == 0) {
    DisplayMessage(996);
    in_inv_screen--;
    return -1;
  }

  if (inv_screen_newroom >= 0) {
    in_inv_screen--;
    NewRoom(inv_screen_newroom);
    return -1;
  }

  for (ww = 0; ww < charextra[game.playercharacter].invorder_count; ww++) {
    if (game.invinfo[charextra[game.playercharacter].invorder[ww]].name[0]!=0) {
      dii[numitems].num = charextra[game.playercharacter].invorder[ww];
      dii[numitems].sprnum = game.invinfo[charextra[game.playercharacter].invorder[ww]].pic;
      int snn=dii[numitems].sprnum;
      if (spritewidth[snn] > widest) widest=spritewidth[snn];
      if (spriteheight[snn] > highest) highest=spriteheight[snn];
      numitems++;
      }
    }
  if (numitems != charextra[game.playercharacter].invorder_count)
    quit("inconsistent inventory calculations");

  widest += get_fixed_pixel_size(4);
  highest += get_fixed_pixel_size(4);
  num_visible_items = (MAX_ITEMAREA_HEIGHT / highest) * ICONSPERLINE;

  int windowhit = highest * (numitems/ICONSPERLINE) + get_fixed_pixel_size(4);
  if ((numitems%ICONSPERLINE) !=0) windowhit+=highest;
  if (windowhit > MAX_ITEMAREA_HEIGHT) {
    windowhit = (MAX_ITEMAREA_HEIGHT / highest) * highest + get_fixed_pixel_size(4);
  }
  windowhit += BUTTONAREAHEIGHT;

  int windowwid = widest*ICONSPERLINE + get_fixed_pixel_size(4);
  if (windowwid < get_fixed_pixel_size(105)) windowwid = get_fixed_pixel_size(105);
  int windowxp=scrnwid/2-windowwid/2;
  int windowyp=scrnhit/2-windowhit/2;
  int buttonyp=windowyp+windowhit-BUTTONAREAHEIGHT;
  wsetcolor(play.sierra_inv_color);
  wbar(windowxp,windowyp,windowxp+windowwid,windowyp+windowhit);
  wsetcolor(0); 
  int bartop = windowyp + get_fixed_pixel_size(2);
  int barxp = windowxp + get_fixed_pixel_size(2);
  wbar(barxp,bartop, windowxp + windowwid - get_fixed_pixel_size(2),buttonyp-1);
  for (ww = top_item; ww < numitems; ww++) {
    if (ww >= top_item + num_visible_items)
      break;
    block spof=spriteset[dii[ww].sprnum];
    wputblock(barxp+1+((ww-top_item)%4)*widest+widest/2-wgetblockwidth(spof)/2,
      bartop+1+((ww-top_item)/4)*highest+highest/2-wgetblockheight(spof)/2,spof,1);
    }
  if ((spriteset[2041] == NULL) || (spriteset[2042] == NULL) || (spriteset[2043] == NULL))
    quit("!InventoryScreen: one or more of the inventory screen graphics have been deleted");
  #define BUTTONWID spritewidth[2042]
  // Draw select, look and OK buttons
  wputblock(windowxp+2, buttonyp + get_fixed_pixel_size(2), spriteset[2041], 1);
  wputblock(windowxp+3+BUTTONWID, buttonyp + get_fixed_pixel_size(2), spriteset[2042], 1);
  wputblock(windowxp+4+BUTTONWID*2, buttonyp + get_fixed_pixel_size(2), spriteset[2043], 1);

  // Draw Up and Down buttons if required
  const int ARROWBUTTONWID = 11;
  block arrowblock = create_bitmap (ARROWBUTTONWID, ARROWBUTTONWID);
  clear_to_color(arrowblock, bitmap_mask_color(arrowblock));
  int usecol;
  __my_setcolor(&usecol, 0);
  if (play.sierra_inv_color == 0)
    __my_setcolor(&usecol, 14);

  line(arrowblock,ARROWBUTTONWID/2, 2, ARROWBUTTONWID-2, 9, usecol);
  line(arrowblock,ARROWBUTTONWID/2, 2, 2, 9, usecol);
  line(arrowblock, 2, 9, ARROWBUTTONWID-2, 9, usecol);
  floodfill(arrowblock, ARROWBUTTONWID/2, 4, usecol);

  if (top_item > 0)
    wputblock(windowxp+windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(2), arrowblock, 1);
  if (top_item + num_visible_items < numitems)
    draw_sprite_v_flip (abuf, arrowblock, windowxp+windowwid-ARROWBUTTONWID, buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID);
  wfreeblock(arrowblock);

  domouse(1);
  set_mouse_cursor(cmode);
  int wasonitem=-1;
  while (!kbhit()) {
    timerloop = 0;
    NEXT_ITERATION();
    domouse(0);
    update_polled_stuff_and_crossfade();
    write_screen();

    int isonitem=((mousey-bartop)/highest)*ICONSPERLINE+(mousex-barxp)/widest;
    if (mousey<=bartop) isonitem=-1;
    else if (isonitem >= 0) isonitem += top_item;
    if ((isonitem<0) | (isonitem>=numitems) | (isonitem >= top_item + num_visible_items))
      isonitem=-1;

    int mclick = mgetbutton();
    if (mclick == LEFT) {
      if ((mousey<windowyp) | (mousey>windowyp+windowhit) | (mousex<windowxp) | (mousex>windowxp+windowwid))
        continue;
      if (mousey<buttonyp) {
        int clickedon=isonitem;
        if (clickedon<0) continue;
        evblocknum=dii[clickedon].num;
        play.used_inv_on = dii[clickedon].num;

        if (cmode==MODE_LOOK) {
          domouse(2);
          run_event_block_inv(dii[clickedon].num, 0); 
          // in case the script did anything to the screen, redraw it
          mainloop();
          
          goto start_actinv;
          continue;
        }
        else if (cmode==MODE_USE) {
          // use objects on each other
          play.usedinv=toret;

          // set the activeinv so the script can check it
          int activeinvwas = playerchar->activeinv;
          playerchar->activeinv = toret;

          domouse(2);
          run_event_block_inv(dii[clickedon].num, 3);

          // if the script didn't change it, then put it back
          if (playerchar->activeinv == toret)
            playerchar->activeinv = activeinvwas;

          // in case the script did anything to the screen, redraw it
          mainloop();
          
          // They used the active item and lost it
          if (playerchar->inv[toret] < 1) {
            cmode = CURS_ARROW;
            set_mouse_cursor(cmode);
            toret = -1;
          }
 
          goto start_actinv;
//          continue;
          }
        toret=dii[clickedon].num;
//        int plusng=play.using; play.using=toret;
        update_inv_cursor(toret);
        set_mouse_cursor(MODE_USE);
        cmode=MODE_USE;
//        play.using=plusng;
//        break;
        continue;
        }
      else {
        if (mousex >= windowxp+windowwid-ARROWBUTTONWID) {
          if (mousey < buttonyp + get_fixed_pixel_size(2) + ARROWBUTTONWID) {
            if (top_item > 0) {
              top_item -= ICONSPERLINE;
              domouse(2);
              goto start_actinv;
              }
            }
          else if ((mousey < buttonyp + get_fixed_pixel_size(4) + ARROWBUTTONWID*2) && (top_item + num_visible_items < numitems)) {
            top_item += ICONSPERLINE;
            domouse(2);
            goto start_actinv;
            }
          continue;
          }

        int buton=(mousex-windowxp)-2;
        if (buton<0) continue;
        buton/=BUTTONWID;
        if (buton>=3) continue;
        if (buton==0) { toret=-1; cmode=MODE_LOOK; }
        else if (buton==1) { cmode=CURS_ARROW; toret=-1; }
        else break;
        set_mouse_cursor(cmode);
        }
      }
    else if (mclick == RIGHT) {
      if (cmode == CURS_ARROW)
        cmode = MODE_LOOK;
      else
        cmode = CURS_ARROW;
      toret = -1;
      set_mouse_cursor(cmode);
    }
    else if (isonitem!=wasonitem) { domouse(2);
      int rectxp=barxp+1+(wasonitem%4)*widest;
      int rectyp=bartop+1+((wasonitem - top_item)/4)*highest;
      if (wasonitem>=0) {
        wsetcolor(0);
        wrectangle(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1);
        }
      if (isonitem>=0) { wsetcolor(14);//opts.invrectcol);
        rectxp=barxp+1+(isonitem%4)*widest;
        rectyp=bartop+1+((isonitem - top_item)/4)*highest;
        wrectangle(rectxp,rectyp,rectxp+widest-1,rectyp+highest-1);
        }
      domouse(1);
      }
    wasonitem=isonitem;
    while (timerloop == 0) {
      update_polled_stuff();
      platform->YieldCPU();
    }
  }
  while (kbhit()) getch();
  set_default_cursor();
  domouse(2);
  construct_virtual_screen(true);
  in_inv_screen--;
  return toret;
  }

int invscreen() {
  int selt=__actual_invscreen();
  if (selt<0) return -1;
  playerchar->activeinv=selt;
  guis_need_update = 1;
  set_cursor_mode(MODE_USE);
  return selt;
  }

void sc_invscreen() {
  curscript->queue_action(ePSAInvScreen, 0, "InventoryScreen");
}

void SetInvDimensions(int ww,int hh) {
  play.inv_item_wid = ww;
  play.inv_item_hit = hh;
  play.inv_numdisp = 0;
  // backwards compatibility
  for (int i = 0; i < numguiinv; i++) {
    guiinv[i].itemWidth = ww;
    guiinv[i].itemHeight = hh;
    guiinv[i].Resized();
  }
  guis_need_update = 1;
}

void UpdatePalette() {
  if (game.color_depth > 1)
    invalidate_screen();

  if (!play.fast_forward)  
    setpal();
}

// Helper functions used by StartCutscene/EndCutscene, but also
// by SkipUntilCharacterStops
void initialize_skippable_cutscene() {
  play.end_cutscene_music = -1;
}

void stop_fast_forwarding() {
  // when the skipping of a cutscene comes to an end, update things
  play.fast_forward = 0;
  setpal();
  if (play.end_cutscene_music >= 0)
    newmusic(play.end_cutscene_music);

  // Restore actual volume of sounds
  for (int aa = 0; aa < MAX_SOUND_CHANNELS; aa++)
  {
    if ((channels[aa] != NULL) && (!channels[aa]->done) && 
        (channels[aa]->volAsPercentage == 0) &&
        (channels[aa]->originalVolAsPercentage > 0)) 
    {
      channels[aa]->volAsPercentage = channels[aa]->originalVolAsPercentage;
      channels[aa]->set_volume((channels[aa]->volAsPercentage * 255) / 100);
    }
  }

  update_music_volume();
}

void SkipUntilCharacterStops(int cc) {
  if (!is_valid_character(cc))
    quit("!SkipUntilCharacterStops: invalid character specified");
  if (game.chars[cc].room!=displayed_room)
    quit("!SkipUntilCharacterStops: specified character not in current room");

  // if they are not currently moving, do nothing
  if (!game.chars[cc].walking)
    return;

  if (play.in_cutscene)
    quit("!SkipUntilCharacterStops: cannot be used within a cutscene");

  initialize_skippable_cutscene();
  play.fast_forward = 2;
  play.skip_until_char_stops = cc;
}

void EndSkippingUntilCharStops() {
  // not currently skipping, so ignore
  if (play.skip_until_char_stops < 0)
    return;

  stop_fast_forwarding();
  play.skip_until_char_stops = -1;
}

// skipwith decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
void StartCutscene (int skipwith) {
  if (play.in_cutscene)
    quit("!StartCutscene: already in a cutscene");

  if ((skipwith < 1) || (skipwith > 5))
    quit("!StartCutscene: invalid argument, must be 1 to 5.");

  // make sure they can't be skipping and cutsceneing at the same time
  EndSkippingUntilCharStops();

  play.in_cutscene = skipwith;
  initialize_skippable_cutscene();
}

int EndCutscene () {
  if (play.in_cutscene == 0)
    quit("!EndCutscene: not in a cutscene");

  int retval = play.fast_forward;
  play.in_cutscene = 0;
  // Stop it fast-forwarding
  stop_fast_forwarding();

  // make sure that the screen redraws
  invalidate_screen();

  // Return whether the player skipped it
  return retval;
}

int Game_GetSkippingCutscene()
{
  if (play.fast_forward)
  {
    return 1;
  }
  return 0;
}

int Game_GetInSkippableCutscene()
{
  if (play.in_cutscene)
  {
    return 1;
  }
  return 0;
}


// Stubs for plugin functions.
void ScriptStub_ShellExecute()
{
}
void srSetSnowDriftRange(int min_value, int max_value)
{
}
void srSetSnowDriftSpeed(int min_value, int max_value)
{
}
void srSetSnowFallSpeed(int min_value, int max_value)
{
}
void srChangeSnowAmount(int amount)
{
}
void srSetSnowBaseline(int top, int bottom)
{
}
void srSetSnowTransparency(int min_value, int max_value)
{
}
void srSetSnowDefaultView(int view, int loop)
{
}
void srSetSnowWindSpeed(int value)
{
}
void srSetSnowAmount(int amount)
{
}
void srSetSnowView(int kind_id, int event, int view, int loop)
{
}
void srChangeRainAmount(int amount)
{
}
void srSetRainView(int kind_id, int event, int view, int loop)
{
}
void srSetRainDefaultView(int view, int loop)
{
}
void srSetRainTransparency(int min_value, int max_value)
{
}
void srSetRainWindSpeed(int value)
{
}
void srSetRainBaseline(int top, int bottom)
{
}
void srSetRainAmount(int amount)
{
}
void srSetRainFallSpeed(int min_value, int max_value)
{
}
void srSetWindSpeed(int value)
{
}
void srSetBaseline(int top, int bottom)
{
}
int JoystickCount()
{
  return 0;
}
int Joystick_Open(int a)
{
  return 0;
}
int Joystick_IsButtonDown(int a)
{
  return 0;
}
void Joystick_EnableEvents(int a)
{
}
void Joystick_DisableEvents()
{
}
void Joystick_Click(int a)
{
}
int Joystick_Valid()
{
  return 0;
}
int Joystick_Unplugged()
{
  return 0;
}
int DrawAlpha(int destination, int sprite, int x, int y, int transparency)
{
  return 0;
}
int GetAlpha(int sprite, int x, int y)
{
  return 0;
}
int PutAlpha(int sprite, int x, int y, int alpha)
{
  return 0;
}
int Blur(int sprite, int radius)
{
  return 0;
}
int HighPass(int sprite, int threshold)
{
  return 0;
}
int DrawAdd(int destination, int sprite, int x, int y, float scale)
{
  return 0;
}


#define scAdd_External_Symbol ccAddExternalSymbol
void setup_script_exports() {
  // the ^5 after the function name is the number of params
  // this is to allow an extra parameter to be added in a later
  // version without screwing up the stack in previous versions
  // (just export both the ^5 and the ^6 as seperate funcs)
  register_audio_script_functions();
  scAdd_External_Symbol("Character::AddInventory^2",(void *)Character_AddInventory);
  scAdd_External_Symbol("Character::AddWaypoint^2",(void *)Character_AddWaypoint);
  scAdd_External_Symbol("Character::Animate^5",(void *)Character_Animate);
  scAdd_External_Symbol("Character::ChangeRoom^3",(void *)Character_ChangeRoom);
  scAdd_External_Symbol("Character::ChangeRoomAutoPosition^2",(void *)Character_ChangeRoomAutoPosition);
  scAdd_External_Symbol("Character::ChangeView^1",(void *)Character_ChangeView);
  scAdd_External_Symbol("Character::FaceCharacter^2",(void *)Character_FaceCharacter);
  scAdd_External_Symbol("Character::FaceLocation^3",(void *)Character_FaceLocation);
  scAdd_External_Symbol("Character::FaceObject^2",(void *)Character_FaceObject);
  scAdd_External_Symbol("Character::FollowCharacter^3",(void *)Character_FollowCharacter);
  scAdd_External_Symbol("Character::GetProperty^1",(void *)Character_GetProperty);
  scAdd_External_Symbol("Character::GetPropertyText^2",(void *)Character_GetPropertyText);
  scAdd_External_Symbol("Character::GetTextProperty^1",(void *)Character_GetTextProperty);
  scAdd_External_Symbol("Character::HasInventory^1",(void *)Character_HasInventory);
  scAdd_External_Symbol("Character::IsCollidingWithChar^1",(void *)Character_IsCollidingWithChar);
  scAdd_External_Symbol("Character::IsCollidingWithObject^1",(void *)Character_IsCollidingWithObject);
  scAdd_External_Symbol("Character::LockView^1",(void *)Character_LockView);
  scAdd_External_Symbol("Character::LockViewAligned^3",(void *)Character_LockViewAligned);
  scAdd_External_Symbol("Character::LockViewFrame^3",(void *)Character_LockViewFrame);
  scAdd_External_Symbol("Character::LockViewOffset^3",(void *)Character_LockViewOffset);
  scAdd_External_Symbol("Character::LoseInventory^1",(void *)Character_LoseInventory);
  scAdd_External_Symbol("Character::Move^4",(void *)Character_Move);
  scAdd_External_Symbol("Character::PlaceOnWalkableArea^0",(void *)Character_PlaceOnWalkableArea);
  scAdd_External_Symbol("Character::RemoveTint^0",(void *)Character_RemoveTint);
  scAdd_External_Symbol("Character::RunInteraction^1",(void *)Character_RunInteraction);
  scAdd_External_Symbol("Character::Say^101",(void *)Character_Say);
  scAdd_External_Symbol("Character::SayAt^4",(void *)Character_SayAt);
  scAdd_External_Symbol("Character::SayBackground^1",(void *)Character_SayBackground);
  scAdd_External_Symbol("Character::SetAsPlayer^0",(void *)Character_SetAsPlayer);
  scAdd_External_Symbol("Character::SetIdleView^2",(void *)Character_SetIdleView);
  //scAdd_External_Symbol("Character::SetOption^2",(void *)Character_SetOption);
  scAdd_External_Symbol("Character::SetWalkSpeed^2",(void *)Character_SetSpeed);
  scAdd_External_Symbol("Character::StopMoving^0",(void *)Character_StopMoving);
  scAdd_External_Symbol("Character::Think^101",(void *)Character_Think);
  scAdd_External_Symbol("Character::Tint^5",(void *)Character_Tint);
  scAdd_External_Symbol("Character::UnlockView^0",(void *)Character_UnlockView);
  scAdd_External_Symbol("Character::Walk^4",(void *)Character_Walk);
  scAdd_External_Symbol("Character::WalkStraight^3",(void *)Character_WalkStraight);

  // static
  scAdd_External_Symbol("Character::GetAtScreenXY^2", (void *)GetCharacterAtLocation);

  scAdd_External_Symbol("Character::get_ActiveInventory",(void *)Character_GetActiveInventory);
  scAdd_External_Symbol("Character::set_ActiveInventory",(void *)Character_SetActiveInventory);
  scAdd_External_Symbol("Character::get_Animating", (void *)Character_GetAnimating);
  scAdd_External_Symbol("Character::get_AnimationSpeed", (void *)Character_GetAnimationSpeed);
  scAdd_External_Symbol("Character::set_AnimationSpeed", (void *)Character_SetAnimationSpeed);
  scAdd_External_Symbol("Character::get_Baseline",(void *)Character_GetBaseline);
  scAdd_External_Symbol("Character::set_Baseline",(void *)Character_SetBaseline);
  scAdd_External_Symbol("Character::get_BlinkInterval",(void *)Character_GetBlinkInterval);
  scAdd_External_Symbol("Character::set_BlinkInterval",(void *)Character_SetBlinkInterval);
  scAdd_External_Symbol("Character::get_BlinkView",(void *)Character_GetBlinkView);
  scAdd_External_Symbol("Character::set_BlinkView",(void *)Character_SetBlinkView);
  scAdd_External_Symbol("Character::get_BlinkWhileThinking",(void *)Character_GetBlinkWhileThinking);
  scAdd_External_Symbol("Character::set_BlinkWhileThinking",(void *)Character_SetBlinkWhileThinking);
  scAdd_External_Symbol("Character::get_BlockingHeight",(void *)Character_GetBlockingHeight);
  scAdd_External_Symbol("Character::set_BlockingHeight",(void *)Character_SetBlockingHeight);
  scAdd_External_Symbol("Character::get_BlockingWidth",(void *)Character_GetBlockingWidth);
  scAdd_External_Symbol("Character::set_BlockingWidth",(void *)Character_SetBlockingWidth);
  scAdd_External_Symbol("Character::get_Clickable",(void *)Character_GetClickable);
  scAdd_External_Symbol("Character::set_Clickable",(void *)Character_SetClickable);
  scAdd_External_Symbol("Character::get_DiagonalLoops", (void *)Character_GetDiagonalWalking);
  scAdd_External_Symbol("Character::set_DiagonalLoops", (void *)Character_SetDiagonalWalking);
  scAdd_External_Symbol("Character::get_Frame", (void *)Character_GetFrame);
  scAdd_External_Symbol("Character::set_Frame", (void *)Character_SetFrame);
  scAdd_External_Symbol("Character::get_HasExplicitTint", (void *)Character_GetHasExplicitTint);
  scAdd_External_Symbol("Character::get_ID", (void *)Character_GetID);
  scAdd_External_Symbol("Character::get_IdleView", (void *)Character_GetIdleView);
  scAdd_External_Symbol("Character::geti_InventoryQuantity", (void *)Character_GetIInventoryQuantity);
  scAdd_External_Symbol("Character::seti_InventoryQuantity", (void *)Character_SetIInventoryQuantity);
  scAdd_External_Symbol("Character::get_IgnoreLighting",(void *)Character_GetIgnoreLighting);
  scAdd_External_Symbol("Character::set_IgnoreLighting",(void *)Character_SetIgnoreLighting);
  scAdd_External_Symbol("Character::get_IgnoreScaling", (void *)Character_GetIgnoreScaling);
  scAdd_External_Symbol("Character::set_IgnoreScaling", (void *)Character_SetIgnoreScaling);
  scAdd_External_Symbol("Character::get_IgnoreWalkbehinds",(void *)Character_GetIgnoreWalkbehinds);
  scAdd_External_Symbol("Character::set_IgnoreWalkbehinds",(void *)Character_SetIgnoreWalkbehinds);
  scAdd_External_Symbol("Character::get_Loop", (void *)Character_GetLoop);
  scAdd_External_Symbol("Character::set_Loop", (void *)Character_SetLoop);
  scAdd_External_Symbol("Character::get_ManualScaling", (void *)Character_GetIgnoreScaling);
  scAdd_External_Symbol("Character::set_ManualScaling", (void *)Character_SetManualScaling);
  scAdd_External_Symbol("Character::get_MovementLinkedToAnimation",(void *)Character_GetMovementLinkedToAnimation);
  scAdd_External_Symbol("Character::set_MovementLinkedToAnimation",(void *)Character_SetMovementLinkedToAnimation);
  scAdd_External_Symbol("Character::get_Moving", (void *)Character_GetMoving);
  scAdd_External_Symbol("Character::get_Name", (void *)Character_GetName);
  scAdd_External_Symbol("Character::set_Name", (void *)Character_SetName);
  scAdd_External_Symbol("Character::get_NormalView",(void *)Character_GetNormalView);
  scAdd_External_Symbol("Character::get_PreviousRoom",(void *)Character_GetPreviousRoom);
  scAdd_External_Symbol("Character::get_Room",(void *)Character_GetRoom);
  scAdd_External_Symbol("Character::get_ScaleMoveSpeed", (void *)Character_GetScaleMoveSpeed);
  scAdd_External_Symbol("Character::set_ScaleMoveSpeed", (void *)Character_SetScaleMoveSpeed);
  scAdd_External_Symbol("Character::get_ScaleVolume", (void *)Character_GetScaleVolume);
  scAdd_External_Symbol("Character::set_ScaleVolume", (void *)Character_SetScaleVolume);
  scAdd_External_Symbol("Character::get_Scaling", (void *)Character_GetScaling);
  scAdd_External_Symbol("Character::set_Scaling", (void *)Character_SetScaling);
  scAdd_External_Symbol("Character::get_Solid", (void *)Character_GetSolid);
  scAdd_External_Symbol("Character::set_Solid", (void *)Character_SetSolid);
  scAdd_External_Symbol("Character::get_Speaking", (void *)Character_GetSpeaking);
  scAdd_External_Symbol("Character::get_SpeakingFrame", (void *)Character_GetSpeakingFrame);
  scAdd_External_Symbol("Character::get_SpeechAnimationDelay",(void *)GetCharacterSpeechAnimationDelay);
  scAdd_External_Symbol("Character::set_SpeechAnimationDelay",(void *)Character_SetSpeechAnimationDelay);
  scAdd_External_Symbol("Character::get_SpeechColor",(void *)Character_GetSpeechColor);
  scAdd_External_Symbol("Character::set_SpeechColor",(void *)Character_SetSpeechColor);
  scAdd_External_Symbol("Character::get_SpeechView",(void *)Character_GetSpeechView);
  scAdd_External_Symbol("Character::set_SpeechView",(void *)Character_SetSpeechView);
  scAdd_External_Symbol("Character::get_ThinkView",(void *)Character_GetThinkView);
  scAdd_External_Symbol("Character::set_ThinkView",(void *)Character_SetThinkView);
  scAdd_External_Symbol("Character::get_Transparency",(void *)Character_GetTransparency);
  scAdd_External_Symbol("Character::set_Transparency",(void *)Character_SetTransparency);
  scAdd_External_Symbol("Character::get_TurnBeforeWalking", (void *)Character_GetTurnBeforeWalking);
  scAdd_External_Symbol("Character::set_TurnBeforeWalking", (void *)Character_SetTurnBeforeWalking);
  scAdd_External_Symbol("Character::get_View", (void *)Character_GetView);
  scAdd_External_Symbol("Character::get_WalkSpeedX", (void *)Character_GetWalkSpeedX);
  scAdd_External_Symbol("Character::get_WalkSpeedY", (void *)Character_GetWalkSpeedY);
  scAdd_External_Symbol("Character::get_X", (void *)Character_GetX);
  scAdd_External_Symbol("Character::set_X", (void *)Character_SetX);
  scAdd_External_Symbol("Character::get_x", (void *)Character_GetX);
  scAdd_External_Symbol("Character::set_x", (void *)Character_SetX);
  scAdd_External_Symbol("Character::get_Y", (void *)Character_GetY);
  scAdd_External_Symbol("Character::set_Y", (void *)Character_SetY);
  scAdd_External_Symbol("Character::get_y", (void *)Character_GetY);
  scAdd_External_Symbol("Character::set_y", (void *)Character_SetY);
  scAdd_External_Symbol("Character::get_Z", (void *)Character_GetZ);
  scAdd_External_Symbol("Character::set_Z", (void *)Character_SetZ);
  scAdd_External_Symbol("Character::get_z", (void *)Character_GetZ);
  scAdd_External_Symbol("Character::set_z", (void *)Character_SetZ);

  scAdd_External_Symbol("Object::Animate^5", (void *)Object_Animate);
  scAdd_External_Symbol("Object::IsCollidingWithObject^1", (void *)Object_IsCollidingWithObject);
  scAdd_External_Symbol("Object::GetName^1", (void *)Object_GetName);
  scAdd_External_Symbol("Object::GetProperty^1", (void *)Object_GetProperty);
  scAdd_External_Symbol("Object::GetPropertyText^2", (void *)Object_GetPropertyText);
  scAdd_External_Symbol("Object::GetTextProperty^1",(void *)Object_GetTextProperty);
  scAdd_External_Symbol("Object::MergeIntoBackground^0", (void *)Object_MergeIntoBackground);
  scAdd_External_Symbol("Object::Move^5", (void *)Object_Move);
  scAdd_External_Symbol("Object::RemoveTint^0", (void *)Object_RemoveTint);
  scAdd_External_Symbol("Object::RunInteraction^1", (void *)Object_RunInteraction);
  scAdd_External_Symbol("Object::SetPosition^2", (void *)Object_SetPosition);
  scAdd_External_Symbol("Object::SetView^3", (void *)Object_SetView);
  scAdd_External_Symbol("Object::StopAnimating^0", (void *)Object_StopAnimating);
  scAdd_External_Symbol("Object::StopMoving^0", (void *)Object_StopMoving);
  scAdd_External_Symbol("Object::Tint^5", (void *)Object_Tint);

  // static
  scAdd_External_Symbol("Object::GetAtScreenXY^2", (void *)GetObjectAtLocation);

  scAdd_External_Symbol("Object::get_Animating", (void *)Object_GetAnimating);
  scAdd_External_Symbol("Object::get_Baseline", (void *)Object_GetBaseline);
  scAdd_External_Symbol("Object::set_Baseline", (void *)Object_SetBaseline);
  scAdd_External_Symbol("Object::get_BlockingHeight",(void *)Object_GetBlockingHeight);
  scAdd_External_Symbol("Object::set_BlockingHeight",(void *)Object_SetBlockingHeight);
  scAdd_External_Symbol("Object::get_BlockingWidth",(void *)Object_GetBlockingWidth);
  scAdd_External_Symbol("Object::set_BlockingWidth",(void *)Object_SetBlockingWidth);
  scAdd_External_Symbol("Object::get_Clickable", (void *)Object_GetClickable);
  scAdd_External_Symbol("Object::set_Clickable", (void *)Object_SetClickable);
  scAdd_External_Symbol("Object::get_Frame", (void *)Object_GetFrame);
  scAdd_External_Symbol("Object::get_Graphic", (void *)Object_GetGraphic);
  scAdd_External_Symbol("Object::set_Graphic", (void *)Object_SetGraphic);
  scAdd_External_Symbol("Object::get_ID", (void *)Object_GetID);
  scAdd_External_Symbol("Object::get_IgnoreScaling", (void *)Object_GetIgnoreScaling);
  scAdd_External_Symbol("Object::set_IgnoreScaling", (void *)Object_SetIgnoreScaling);
  scAdd_External_Symbol("Object::get_IgnoreWalkbehinds", (void *)Object_GetIgnoreWalkbehinds);
  scAdd_External_Symbol("Object::set_IgnoreWalkbehinds", (void *)Object_SetIgnoreWalkbehinds);
  scAdd_External_Symbol("Object::get_Loop", (void *)Object_GetLoop);
  scAdd_External_Symbol("Object::get_Moving", (void *)Object_GetMoving);
  scAdd_External_Symbol("Object::get_Name", (void *)Object_GetName_New);
  scAdd_External_Symbol("Object::get_Solid", (void *)Object_GetSolid);
  scAdd_External_Symbol("Object::set_Solid", (void *)Object_SetSolid);
  scAdd_External_Symbol("Object::get_Transparency", (void *)Object_GetTransparency);
  scAdd_External_Symbol("Object::set_Transparency", (void *)Object_SetTransparency);
  scAdd_External_Symbol("Object::get_View", (void *)Object_GetView);
  scAdd_External_Symbol("Object::get_Visible", (void *)Object_GetVisible);
  scAdd_External_Symbol("Object::set_Visible", (void *)Object_SetVisible);
  scAdd_External_Symbol("Object::get_X", (void *)Object_GetX);
  scAdd_External_Symbol("Object::set_X", (void *)Object_SetX);
  scAdd_External_Symbol("Object::get_Y", (void *)Object_GetY);
  scAdd_External_Symbol("Object::set_Y", (void *)Object_SetY);

  scAdd_External_Symbol("Dialog::get_ID", (void *)Dialog_GetID);
  scAdd_External_Symbol("Dialog::get_OptionCount", (void *)Dialog_GetOptionCount);
  scAdd_External_Symbol("Dialog::get_ShowTextParser", (void *)Dialog_GetShowTextParser);
  scAdd_External_Symbol("Dialog::DisplayOptions^1", (void *)Dialog_DisplayOptions);
  scAdd_External_Symbol("Dialog::GetOptionState^1", (void *)Dialog_GetOptionState);
  scAdd_External_Symbol("Dialog::GetOptionText^1", (void *)Dialog_GetOptionText);
  scAdd_External_Symbol("Dialog::HasOptionBeenChosen^1", (void *)Dialog_HasOptionBeenChosen);
  scAdd_External_Symbol("Dialog::SetOptionState^2", (void *)Dialog_SetOptionState);
  scAdd_External_Symbol("Dialog::Start^0", (void *)Dialog_Start);

  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ActiveOptionID", (void *)DialogOptionsRendering_GetActiveOptionID);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ActiveOptionID", (void *)DialogOptionsRendering_SetActiveOptionID);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_DialogToRender", (void *)DialogOptionsRendering_GetDialogToRender);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Height", (void *)DialogOptionsRendering_GetHeight);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Height", (void *)DialogOptionsRendering_SetHeight);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxX", (void *)DialogOptionsRendering_GetParserTextboxX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxX", (void *)DialogOptionsRendering_SetParserTextboxX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxY", (void *)DialogOptionsRendering_GetParserTextboxY);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxY", (void *)DialogOptionsRendering_SetParserTextboxY);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_ParserTextBoxWidth", (void *)DialogOptionsRendering_GetParserTextboxWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_ParserTextBoxWidth", (void *)DialogOptionsRendering_SetParserTextboxWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Surface", (void *)DialogOptionsRendering_GetSurface);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Width", (void *)DialogOptionsRendering_GetWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Width", (void *)DialogOptionsRendering_SetWidth);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_X", (void *)DialogOptionsRendering_GetX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_X", (void *)DialogOptionsRendering_SetX);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::get_Y", (void *)DialogOptionsRendering_GetY);
  scAdd_External_Symbol("DialogOptionsRenderingInfo::set_Y", (void *)DialogOptionsRendering_SetY);

  scAdd_External_Symbol("File::Delete^1",(void *)File_Delete);
  scAdd_External_Symbol("File::Exists^1",(void *)File_Exists);
  scAdd_External_Symbol("File::Open^2",(void *)sc_OpenFile);
  scAdd_External_Symbol("File::Close^0", (void *)File_Close);
  scAdd_External_Symbol("File::ReadInt^0", (void *)File_ReadInt);
  scAdd_External_Symbol("File::ReadRawChar^0", (void *)File_ReadRawChar);
  scAdd_External_Symbol("File::ReadRawInt^0", (void *)File_ReadRawInt);
  scAdd_External_Symbol("File::ReadRawLine^1", (void *)File_ReadRawLine);
  scAdd_External_Symbol("File::ReadRawLineBack^0", (void *)File_ReadRawLineBack);
  scAdd_External_Symbol("File::ReadString^1", (void *)File_ReadString);
  scAdd_External_Symbol("File::ReadStringBack^0", (void *)File_ReadStringBack);
  scAdd_External_Symbol("File::WriteInt^1", (void *)File_WriteInt);
  scAdd_External_Symbol("File::WriteRawChar^1", (void *)File_WriteRawChar);
  scAdd_External_Symbol("File::WriteRawLine^1", (void *)File_WriteRawLine);
  scAdd_External_Symbol("File::WriteString^1", (void *)File_WriteString);
  scAdd_External_Symbol("File::get_EOF", (void *)File_GetEOF);
  scAdd_External_Symbol("File::get_Error", (void *)File_GetError);

  scAdd_External_Symbol("Overlay::CreateGraphical^4", (void *)Overlay_CreateGraphical);
  scAdd_External_Symbol("Overlay::CreateTextual^106", (void *)Overlay_CreateTextual);
  scAdd_External_Symbol("Overlay::SetText^104", (void *)Overlay_SetText);
  scAdd_External_Symbol("Overlay::Remove^0", (void *)Overlay_Remove);
  scAdd_External_Symbol("Overlay::get_Valid", (void *)Overlay_GetValid);
  scAdd_External_Symbol("Overlay::get_X", (void *)Overlay_GetX);
  scAdd_External_Symbol("Overlay::set_X", (void *)Overlay_SetX);
  scAdd_External_Symbol("Overlay::get_Y", (void *)Overlay_GetY);
  scAdd_External_Symbol("Overlay::set_Y", (void *)Overlay_SetY);

  scAdd_External_Symbol("InventoryItem::GetAtScreenXY^2", (void *)GetInvAtLocation);
  scAdd_External_Symbol("InventoryItem::IsInteractionAvailable^1", (void *)InventoryItem_CheckInteractionAvailable);
  scAdd_External_Symbol("InventoryItem::GetName^1", (void *)InventoryItem_GetName);
  scAdd_External_Symbol("InventoryItem::GetProperty^1", (void *)InventoryItem_GetProperty);
  scAdd_External_Symbol("InventoryItem::GetPropertyText^2", (void *)InventoryItem_GetPropertyText);
  scAdd_External_Symbol("InventoryItem::GetTextProperty^1",(void *)InventoryItem_GetTextProperty);
  scAdd_External_Symbol("InventoryItem::RunInteraction^1", (void *)InventoryItem_RunInteraction);
  scAdd_External_Symbol("InventoryItem::SetName^1", (void *)InventoryItem_SetName);
  scAdd_External_Symbol("InventoryItem::get_CursorGraphic", (void *)InventoryItem_GetCursorGraphic);
  scAdd_External_Symbol("InventoryItem::set_CursorGraphic", (void *)InventoryItem_SetCursorGraphic);
  scAdd_External_Symbol("InventoryItem::get_Graphic", (void *)InventoryItem_GetGraphic);
  scAdd_External_Symbol("InventoryItem::set_Graphic", (void *)InventoryItem_SetGraphic);
  scAdd_External_Symbol("InventoryItem::get_ID", (void *)InventoryItem_GetID);
  scAdd_External_Symbol("InventoryItem::get_Name", (void *)InventoryItem_GetName_New);
  scAdd_External_Symbol("InventoryItem::set_Name", (void *)InventoryItem_SetName);

  scAdd_External_Symbol("GUI::Centre^0", (void *)GUI_Centre);
  scAdd_External_Symbol("GUI::GetAtScreenXY^2", (void *)GetGUIAtLocation);
  scAdd_External_Symbol("GUI::SetPosition^2", (void *)GUI_SetPosition);
  scAdd_External_Symbol("GUI::SetSize^2", (void *)GUI_SetSize);
  scAdd_External_Symbol("GUI::get_BackgroundGraphic", (void *)GUI_GetBackgroundGraphic);
  scAdd_External_Symbol("GUI::set_BackgroundGraphic", (void *)GUI_SetBackgroundGraphic);
  scAdd_External_Symbol("GUI::get_Clickable", (void *)GUI_GetClickable);
  scAdd_External_Symbol("GUI::set_Clickable", (void *)GUI_SetClickable);
  scAdd_External_Symbol("GUI::get_ControlCount", (void *)GUI_GetControlCount);
  scAdd_External_Symbol("GUI::geti_Controls", (void *)GUI_GetiControls);
  scAdd_External_Symbol("GUI::get_Height", (void *)GUI_GetHeight);
  scAdd_External_Symbol("GUI::set_Height", (void *)GUI_SetHeight);
  scAdd_External_Symbol("GUI::get_ID", (void *)GUI_GetID);
  scAdd_External_Symbol("GUI::get_Transparency", (void *)GUI_GetTransparency);
  scAdd_External_Symbol("GUI::set_Transparency", (void *)GUI_SetTransparency);
  scAdd_External_Symbol("GUI::get_Visible", (void *)GUI_GetVisible);
  scAdd_External_Symbol("GUI::set_Visible", (void *)GUI_SetVisible);
  scAdd_External_Symbol("GUI::get_Width", (void *)GUI_GetWidth);
  scAdd_External_Symbol("GUI::set_Width", (void *)GUI_SetWidth);
  scAdd_External_Symbol("GUI::get_X", (void *)GUI_GetX);
  scAdd_External_Symbol("GUI::set_X", (void *)GUI_SetX);
  scAdd_External_Symbol("GUI::get_Y", (void *)GUI_GetY);
  scAdd_External_Symbol("GUI::set_Y", (void *)GUI_SetY);
  scAdd_External_Symbol("GUI::get_ZOrder", (void *)GUI_GetZOrder);
  scAdd_External_Symbol("GUI::set_ZOrder", (void *)GUI_SetZOrder);

  scAdd_External_Symbol("GUIControl::BringToFront^0", (void *)GUIControl_BringToFront);
  scAdd_External_Symbol("GUIControl::GetAtScreenXY^2", (void *)GetGUIControlAtLocation);
  scAdd_External_Symbol("GUIControl::SendToBack^0", (void *)GUIControl_SendToBack);
  scAdd_External_Symbol("GUIControl::SetPosition^2", (void *)GUIControl_SetPosition);
  scAdd_External_Symbol("GUIControl::SetSize^2", (void *)GUIControl_SetSize);
  scAdd_External_Symbol("GUIControl::get_AsButton", (void *)GUIControl_GetAsButton);
  scAdd_External_Symbol("GUIControl::get_AsInvWindow", (void *)GUIControl_GetAsInvWindow);
  scAdd_External_Symbol("GUIControl::get_AsLabel", (void *)GUIControl_GetAsLabel);
  scAdd_External_Symbol("GUIControl::get_AsListBox", (void *)GUIControl_GetAsListBox);
  scAdd_External_Symbol("GUIControl::get_AsSlider", (void *)GUIControl_GetAsSlider);
  scAdd_External_Symbol("GUIControl::get_AsTextBox", (void *)GUIControl_GetAsTextBox);
  scAdd_External_Symbol("GUIControl::get_Clickable", (void *)GUIControl_GetClickable);
  scAdd_External_Symbol("GUIControl::set_Clickable", (void *)GUIControl_SetClickable);
  scAdd_External_Symbol("GUIControl::get_Enabled", (void *)GUIControl_GetEnabled);
  scAdd_External_Symbol("GUIControl::set_Enabled", (void *)GUIControl_SetEnabled);
  scAdd_External_Symbol("GUIControl::get_Height", (void *)GUIControl_GetHeight);
  scAdd_External_Symbol("GUIControl::set_Height", (void *)GUIControl_SetHeight);
  scAdd_External_Symbol("GUIControl::get_ID", (void *)GUIControl_GetID);
  scAdd_External_Symbol("GUIControl::get_OwningGUI", (void *)GUIControl_GetOwningGUI);
  scAdd_External_Symbol("GUIControl::get_Visible", (void *)GUIControl_GetVisible);
  scAdd_External_Symbol("GUIControl::set_Visible", (void *)GUIControl_SetVisible);
  scAdd_External_Symbol("GUIControl::get_Width", (void *)GUIControl_GetWidth);
  scAdd_External_Symbol("GUIControl::set_Width", (void *)GUIControl_SetWidth);
  scAdd_External_Symbol("GUIControl::get_X", (void *)GUIControl_GetX);
  scAdd_External_Symbol("GUIControl::set_X", (void *)GUIControl_SetX);
  scAdd_External_Symbol("GUIControl::get_Y", (void *)GUIControl_GetY);
  scAdd_External_Symbol("GUIControl::set_Y", (void *)GUIControl_SetY);

  scAdd_External_Symbol("Label::GetText^1", (void *)Label_GetText);
  scAdd_External_Symbol("Label::SetText^1", (void *)Label_SetText);
  scAdd_External_Symbol("Label::get_Font", (void *)Label_GetFont);
  scAdd_External_Symbol("Label::set_Font", (void *)Label_SetFont);
  scAdd_External_Symbol("Label::get_Text", (void *)Label_GetText_New);
  scAdd_External_Symbol("Label::set_Text", (void *)Label_SetText);
  scAdd_External_Symbol("Label::get_TextColor", (void *)Label_GetColor);
  scAdd_External_Symbol("Label::set_TextColor", (void *)Label_SetColor);

  scAdd_External_Symbol("Button::Animate^4", (void *)Button_Animate);
  scAdd_External_Symbol("Button::GetText^1", (void *)Button_GetText);
  scAdd_External_Symbol("Button::SetText^1", (void *)Button_SetText);
  scAdd_External_Symbol("Button::get_ClipImage", (void *)Button_GetClipImage);
  scAdd_External_Symbol("Button::set_ClipImage", (void *)Button_SetClipImage);
  scAdd_External_Symbol("Button::get_Font", (void *)Button_GetFont);
  scAdd_External_Symbol("Button::set_Font", (void *)Button_SetFont);
  scAdd_External_Symbol("Button::get_Graphic", (void *)Button_GetGraphic);
  scAdd_External_Symbol("Button::get_MouseOverGraphic", (void *)Button_GetMouseOverGraphic);
  scAdd_External_Symbol("Button::set_MouseOverGraphic", (void *)Button_SetMouseOverGraphic);
  scAdd_External_Symbol("Button::get_NormalGraphic", (void *)Button_GetNormalGraphic);
  scAdd_External_Symbol("Button::set_NormalGraphic", (void *)Button_SetNormalGraphic);
  scAdd_External_Symbol("Button::get_PushedGraphic", (void *)Button_GetPushedGraphic);
  scAdd_External_Symbol("Button::set_PushedGraphic", (void *)Button_SetPushedGraphic);
  scAdd_External_Symbol("Button::get_Text", (void *)Button_GetText_New);
  scAdd_External_Symbol("Button::set_Text", (void *)Button_SetText);
  scAdd_External_Symbol("Button::get_TextColor", (void *)Button_GetTextColor);
  scAdd_External_Symbol("Button::set_TextColor", (void *)Button_SetTextColor);

  scAdd_External_Symbol("Slider::get_BackgroundGraphic", (void *)Slider_GetBackgroundGraphic);
  scAdd_External_Symbol("Slider::set_BackgroundGraphic", (void *)Slider_SetBackgroundGraphic);
  scAdd_External_Symbol("Slider::get_HandleGraphic", (void *)Slider_GetHandleGraphic);
  scAdd_External_Symbol("Slider::set_HandleGraphic", (void *)Slider_SetHandleGraphic);
  scAdd_External_Symbol("Slider::get_HandleOffset", (void *)Slider_GetHandleOffset);
  scAdd_External_Symbol("Slider::set_HandleOffset", (void *)Slider_SetHandleOffset);
  scAdd_External_Symbol("Slider::get_Max", (void *)Slider_GetMax);
  scAdd_External_Symbol("Slider::set_Max", (void *)Slider_SetMax);
  scAdd_External_Symbol("Slider::get_Min", (void *)Slider_GetMin);
  scAdd_External_Symbol("Slider::set_Min", (void *)Slider_SetMin);
  scAdd_External_Symbol("Slider::get_Value", (void *)Slider_GetValue);
  scAdd_External_Symbol("Slider::set_Value", (void *)Slider_SetValue);

  scAdd_External_Symbol("TextBox::GetText^1", (void *)TextBox_GetText);
  scAdd_External_Symbol("TextBox::SetText^1", (void *)TextBox_SetText);
  scAdd_External_Symbol("TextBox::get_Font", (void *)TextBox_GetFont);
  scAdd_External_Symbol("TextBox::set_Font", (void *)TextBox_SetFont);
  scAdd_External_Symbol("TextBox::get_Text", (void *)TextBox_GetText_New);
  scAdd_External_Symbol("TextBox::set_Text", (void *)TextBox_SetText);
  scAdd_External_Symbol("TextBox::get_TextColor", (void *)TextBox_GetTextColor);
  scAdd_External_Symbol("TextBox::set_TextColor", (void *)TextBox_SetTextColor);

  scAdd_External_Symbol("InvWindow::ScrollDown^0", (void *)InvWindow_ScrollDown);
  scAdd_External_Symbol("InvWindow::ScrollUp^0", (void *)InvWindow_ScrollUp);
  scAdd_External_Symbol("InvWindow::get_CharacterToUse", (void *)InvWindow_GetCharacterToUse);
  scAdd_External_Symbol("InvWindow::set_CharacterToUse", (void *)InvWindow_SetCharacterToUse);
  scAdd_External_Symbol("InvWindow::geti_ItemAtIndex", (void *)InvWindow_GetItemAtIndex);
  scAdd_External_Symbol("InvWindow::get_ItemCount", (void *)InvWindow_GetItemCount);
  scAdd_External_Symbol("InvWindow::get_ItemHeight", (void *)InvWindow_GetItemHeight);
  scAdd_External_Symbol("InvWindow::set_ItemHeight", (void *)InvWindow_SetItemHeight);
  scAdd_External_Symbol("InvWindow::get_ItemWidth", (void *)InvWindow_GetItemWidth);
  scAdd_External_Symbol("InvWindow::set_ItemWidth", (void *)InvWindow_SetItemWidth);
  scAdd_External_Symbol("InvWindow::get_ItemsPerRow", (void *)InvWindow_GetItemsPerRow);
  scAdd_External_Symbol("InvWindow::get_RowCount", (void *)InvWindow_GetRowCount);
  scAdd_External_Symbol("InvWindow::get_TopItem", (void *)InvWindow_GetTopItem);
  scAdd_External_Symbol("InvWindow::set_TopItem", (void *)InvWindow_SetTopItem);

  scAdd_External_Symbol("ListBox::AddItem^1", (void *)ListBox_AddItem);
  scAdd_External_Symbol("ListBox::Clear^0", (void *)ListBox_Clear);
  scAdd_External_Symbol("ListBox::FillDirList^1", (void *)ListBox_FillDirList);
  scAdd_External_Symbol("ListBox::FillSaveGameList^0", (void *)ListBox_FillSaveGameList);
  scAdd_External_Symbol("ListBox::GetItemAtLocation^2", (void *)ListBox_GetItemAtLocation);
  scAdd_External_Symbol("ListBox::GetItemText^2", (void *)ListBox_GetItemText);
  scAdd_External_Symbol("ListBox::InsertItemAt^2", (void *)ListBox_InsertItemAt);
  scAdd_External_Symbol("ListBox::RemoveItem^1", (void *)ListBox_RemoveItem);
  scAdd_External_Symbol("ListBox::ScrollDown^0", (void *)ListBox_ScrollDown);
  scAdd_External_Symbol("ListBox::ScrollUp^0", (void *)ListBox_ScrollUp);
  scAdd_External_Symbol("ListBox::SetItemText^2", (void *)ListBox_SetItemText);
  scAdd_External_Symbol("ListBox::get_Font", (void *)ListBox_GetFont);
  scAdd_External_Symbol("ListBox::set_Font", (void *)ListBox_SetFont);
  scAdd_External_Symbol("ListBox::get_HideBorder", (void *)ListBox_GetHideBorder);
  scAdd_External_Symbol("ListBox::set_HideBorder", (void *)ListBox_SetHideBorder);
  scAdd_External_Symbol("ListBox::get_HideScrollArrows", (void *)ListBox_GetHideScrollArrows);
  scAdd_External_Symbol("ListBox::set_HideScrollArrows", (void *)ListBox_SetHideScrollArrows);
  scAdd_External_Symbol("ListBox::get_ItemCount", (void *)ListBox_GetItemCount);
  scAdd_External_Symbol("ListBox::geti_Items", (void *)ListBox_GetItems);
  scAdd_External_Symbol("ListBox::seti_Items", (void *)ListBox_SetItemText);
  scAdd_External_Symbol("ListBox::get_RowCount", (void *)ListBox_GetRowCount);
  scAdd_External_Symbol("ListBox::geti_SaveGameSlots", (void *)ListBox_GetSaveGameSlots);
  scAdd_External_Symbol("ListBox::get_SelectedIndex", (void *)ListBox_GetSelectedIndex);
  scAdd_External_Symbol("ListBox::set_SelectedIndex", (void *)ListBox_SetSelectedIndex);
  scAdd_External_Symbol("ListBox::get_TopItem", (void *)ListBox_GetTopItem);
  scAdd_External_Symbol("ListBox::set_TopItem", (void *)ListBox_SetTopItem);

  scAdd_External_Symbol("Mouse::ChangeModeGraphic^2",(void *)ChangeCursorGraphic);
  scAdd_External_Symbol("Mouse::ChangeModeHotspot^3",(void *)ChangeCursorHotspot);
  scAdd_External_Symbol("Mouse::ChangeModeView^2",(void *)Mouse_ChangeModeView);
  scAdd_External_Symbol("Mouse::DisableMode^1",(void *)disable_cursor_mode);
  scAdd_External_Symbol("Mouse::EnableMode^1",(void *)enable_cursor_mode);
  scAdd_External_Symbol("Mouse::GetModeGraphic^1",(void *)Mouse_GetModeGraphic);
  scAdd_External_Symbol("Mouse::IsButtonDown^1",(void *)IsButtonDown);
  scAdd_External_Symbol("Mouse::SaveCursorUntilItLeaves^0",(void *)SaveCursorForLocationChange);
  scAdd_External_Symbol("Mouse::SelectNextMode^0", (void *)SetNextCursor);
  scAdd_External_Symbol("Mouse::SetBounds^4",(void *)SetMouseBounds);
  scAdd_External_Symbol("Mouse::SetPosition^2",(void *)SetMousePosition);
  scAdd_External_Symbol("Mouse::Update^0",(void *)RefreshMouse);
  scAdd_External_Symbol("Mouse::UseDefaultGraphic^0",(void *)set_default_cursor);
  scAdd_External_Symbol("Mouse::UseModeGraphic^1",(void *)set_mouse_cursor);
  scAdd_External_Symbol("Mouse::get_Mode",(void *)GetCursorMode);
  scAdd_External_Symbol("Mouse::set_Mode",(void *)set_cursor_mode);
  scAdd_External_Symbol("Mouse::get_Visible", (void *)Mouse_GetVisible);
  scAdd_External_Symbol("Mouse::set_Visible", (void *)Mouse_SetVisible);

  scAdd_External_Symbol("Maths::ArcCos^1", (void*)Math_ArcCos);
  scAdd_External_Symbol("Maths::ArcSin^1", (void*)Math_ArcSin);
  scAdd_External_Symbol("Maths::ArcTan^1", (void*)Math_ArcTan);
  scAdd_External_Symbol("Maths::ArcTan2^2", (void*)Math_ArcTan2);
  scAdd_External_Symbol("Maths::Cos^1", (void*)Math_Cos);
  scAdd_External_Symbol("Maths::Cosh^1", (void*)Math_Cosh);
  scAdd_External_Symbol("Maths::DegreesToRadians^1", (void*)Math_DegreesToRadians);
  scAdd_External_Symbol("Maths::Exp^1", (void*)Math_Exp);
  scAdd_External_Symbol("Maths::Log^1", (void*)Math_Log);
  scAdd_External_Symbol("Maths::Log10^1", (void*)Math_Log10);
  scAdd_External_Symbol("Maths::RadiansToDegrees^1", (void*)Math_RadiansToDegrees);
  scAdd_External_Symbol("Maths::RaiseToPower^2", (void*)Math_RaiseToPower);
  scAdd_External_Symbol("Maths::Sin^1", (void*)Math_Sin);
  scAdd_External_Symbol("Maths::Sinh^1", (void*)Math_Sinh);
  scAdd_External_Symbol("Maths::Sqrt^1", (void*)Math_Sqrt);
  scAdd_External_Symbol("Maths::Tan^1", (void*)Math_Tan);
  scAdd_External_Symbol("Maths::Tanh^1", (void*)Math_Tanh);
  scAdd_External_Symbol("Maths::get_Pi", (void*)Math_GetPi);

  scAdd_External_Symbol("Hotspot::GetAtScreenXY^2",(void *)GetHotspotAtLocation);
  scAdd_External_Symbol("Hotspot::GetName^1", (void*)Hotspot_GetName);
  scAdd_External_Symbol("Hotspot::GetProperty^1", (void*)Hotspot_GetProperty);
  scAdd_External_Symbol("Hotspot::GetPropertyText^2", (void*)Hotspot_GetPropertyText);
  scAdd_External_Symbol("Hotspot::GetTextProperty^1",(void *)Hotspot_GetTextProperty);
  scAdd_External_Symbol("Hotspot::RunInteraction^1", (void*)Hotspot_RunInteraction);
  scAdd_External_Symbol("Hotspot::get_Enabled", (void*)Hotspot_GetEnabled);
  scAdd_External_Symbol("Hotspot::set_Enabled", (void*)Hotspot_SetEnabled);
  scAdd_External_Symbol("Hotspot::get_ID", (void*)Hotspot_GetID);
  scAdd_External_Symbol("Hotspot::get_Name", (void*)Hotspot_GetName_New);
  scAdd_External_Symbol("Hotspot::get_WalkToX", (void*)Hotspot_GetWalkToX);
  scAdd_External_Symbol("Hotspot::get_WalkToY", (void*)Hotspot_GetWalkToY);

  scAdd_External_Symbol("Region::GetAtRoomXY^2",(void *)GetRegionAtLocation);
  scAdd_External_Symbol("Region::Tint^4", (void*)Region_Tint);
  scAdd_External_Symbol("Region::RunInteraction^1", (void*)Region_RunInteraction);
  scAdd_External_Symbol("Region::get_Enabled", (void*)Region_GetEnabled);
  scAdd_External_Symbol("Region::set_Enabled", (void*)Region_SetEnabled);
  scAdd_External_Symbol("Region::get_ID", (void*)Region_GetID);
  scAdd_External_Symbol("Region::get_LightLevel", (void*)Region_GetLightLevel);
  scAdd_External_Symbol("Region::set_LightLevel", (void*)Region_SetLightLevel);
  scAdd_External_Symbol("Region::get_TintEnabled", (void*)Region_GetTintEnabled);
  scAdd_External_Symbol("Region::get_TintBlue", (void*)Region_GetTintBlue);
  scAdd_External_Symbol("Region::get_TintGreen", (void*)Region_GetTintGreen);
  scAdd_External_Symbol("Region::get_TintRed", (void*)Region_GetTintRed);
  scAdd_External_Symbol("Region::get_TintSaturation", (void*)Region_GetTintSaturation);

  scAdd_External_Symbol("DateTime::get_Now", (void*)DateTime_Now);
  scAdd_External_Symbol("DateTime::get_DayOfMonth", (void*)DateTime_GetDayOfMonth);
  scAdd_External_Symbol("DateTime::get_Hour", (void*)DateTime_GetHour);
  scAdd_External_Symbol("DateTime::get_Minute", (void*)DateTime_GetMinute);
  scAdd_External_Symbol("DateTime::get_Month", (void*)DateTime_GetMonth);
  scAdd_External_Symbol("DateTime::get_RawTime", (void*)DateTime_GetRawTime);
  scAdd_External_Symbol("DateTime::get_Second", (void*)DateTime_GetSecond);
  scAdd_External_Symbol("DateTime::get_Year", (void*)DateTime_GetYear);

  scAdd_External_Symbol("DrawingSurface::Clear^1", (void *)DrawingSurface_Clear);
  scAdd_External_Symbol("DrawingSurface::CreateCopy^0", (void *)DrawingSurface_CreateCopy);
  scAdd_External_Symbol("DrawingSurface::DrawCircle^3", (void *)DrawingSurface_DrawCircle);
  scAdd_External_Symbol("DrawingSurface::DrawImage^6", (void *)DrawingSurface_DrawImage);
  scAdd_External_Symbol("DrawingSurface::DrawLine^5", (void *)DrawingSurface_DrawLine);
  scAdd_External_Symbol("DrawingSurface::DrawMessageWrapped^5", (void *)DrawingSurface_DrawMessageWrapped);
  scAdd_External_Symbol("DrawingSurface::DrawPixel^2", (void *)DrawingSurface_DrawPixel);
  scAdd_External_Symbol("DrawingSurface::DrawRectangle^4", (void *)DrawingSurface_DrawRectangle);
  scAdd_External_Symbol("DrawingSurface::DrawString^104", (void *)DrawingSurface_DrawString);
  scAdd_External_Symbol("DrawingSurface::DrawStringWrapped^6", (void *)DrawingSurface_DrawStringWrapped);
  scAdd_External_Symbol("DrawingSurface::DrawSurface^2", (void *)DrawingSurface_DrawSurface);
  scAdd_External_Symbol("DrawingSurface::DrawTriangle^6", (void *)DrawingSurface_DrawTriangle);
  scAdd_External_Symbol("DrawingSurface::GetPixel^2", (void *)DrawingSurface_GetPixel);
  scAdd_External_Symbol("DrawingSurface::Release^0", (void *)DrawingSurface_Release);
  scAdd_External_Symbol("DrawingSurface::get_DrawingColor", (void *)DrawingSurface_GetDrawingColor);
  scAdd_External_Symbol("DrawingSurface::set_DrawingColor", (void *)DrawingSurface_SetDrawingColor);
  scAdd_External_Symbol("DrawingSurface::get_Height", (void *)DrawingSurface_GetHeight);
  scAdd_External_Symbol("DrawingSurface::get_UseHighResCoordinates", (void *)DrawingSurface_GetUseHighResCoordinates);
  scAdd_External_Symbol("DrawingSurface::set_UseHighResCoordinates", (void *)DrawingSurface_SetUseHighResCoordinates);
  scAdd_External_Symbol("DrawingSurface::get_Width", (void *)DrawingSurface_GetWidth);

  scAdd_External_Symbol("DynamicSprite::ChangeCanvasSize^4", (void*)DynamicSprite_ChangeCanvasSize);
  scAdd_External_Symbol("DynamicSprite::CopyTransparencyMask^1", (void*)DynamicSprite_CopyTransparencyMask);
  scAdd_External_Symbol("DynamicSprite::Crop^4", (void*)DynamicSprite_Crop);
  scAdd_External_Symbol("DynamicSprite::Delete", (void*)DynamicSprite_Delete);
  scAdd_External_Symbol("DynamicSprite::Flip^1", (void*)DynamicSprite_Flip);
  scAdd_External_Symbol("DynamicSprite::GetDrawingSurface^0", (void*)DynamicSprite_GetDrawingSurface);
  scAdd_External_Symbol("DynamicSprite::Resize^2", (void*)DynamicSprite_Resize);
  scAdd_External_Symbol("DynamicSprite::Rotate^3", (void*)DynamicSprite_Rotate);
  scAdd_External_Symbol("DynamicSprite::SaveToFile^1", (void*)DynamicSprite_SaveToFile);
  scAdd_External_Symbol("DynamicSprite::Tint^5", (void*)DynamicSprite_Tint);
  scAdd_External_Symbol("DynamicSprite::get_ColorDepth", (void*)DynamicSprite_GetColorDepth);
  scAdd_External_Symbol("DynamicSprite::get_Graphic", (void*)DynamicSprite_GetGraphic);
  scAdd_External_Symbol("DynamicSprite::get_Height", (void*)DynamicSprite_GetHeight);
  scAdd_External_Symbol("DynamicSprite::get_Width", (void*)DynamicSprite_GetWidth);
  
  scAdd_External_Symbol("DynamicSprite::Create^3", (void*)DynamicSprite_Create);
  scAdd_External_Symbol("DynamicSprite::CreateFromBackground", (void*)DynamicSprite_CreateFromBackground);
  scAdd_External_Symbol("DynamicSprite::CreateFromDrawingSurface^5", (void*)DynamicSprite_CreateFromDrawingSurface);
  scAdd_External_Symbol("DynamicSprite::CreateFromExistingSprite^1", (void*)DynamicSprite_CreateFromExistingSprite_Old);
  scAdd_External_Symbol("DynamicSprite::CreateFromExistingSprite^2", (void*)DynamicSprite_CreateFromExistingSprite);
  scAdd_External_Symbol("DynamicSprite::CreateFromFile", (void*)DynamicSprite_CreateFromFile);
  scAdd_External_Symbol("DynamicSprite::CreateFromSaveGame", (void*)DynamicSprite_CreateFromSaveGame);
  scAdd_External_Symbol("DynamicSprite::CreateFromScreenShot", (void*)DynamicSprite_CreateFromScreenShot);

  scAdd_External_Symbol("String::IsNullOrEmpty^1", (void*)String_IsNullOrEmpty);
  scAdd_External_Symbol("String::Append^1", (void*)String_Append);
  scAdd_External_Symbol("String::AppendChar^1", (void*)String_AppendChar);
  scAdd_External_Symbol("String::CompareTo^2", (void*)String_CompareTo);
  scAdd_External_Symbol("String::Contains^1", (void*)StrContains);
  scAdd_External_Symbol("String::Copy^0", (void*)String_Copy);
  scAdd_External_Symbol("String::EndsWith^2", (void*)String_EndsWith);
  scAdd_External_Symbol("String::Format^101", (void*)String_Format);
  scAdd_External_Symbol("String::IndexOf^1", (void*)StrContains);
  scAdd_External_Symbol("String::LowerCase^0", (void*)String_LowerCase);
  scAdd_External_Symbol("String::Replace^3", (void*)String_Replace);
  scAdd_External_Symbol("String::ReplaceCharAt^2", (void*)String_ReplaceCharAt);
  scAdd_External_Symbol("String::StartsWith^2", (void*)String_StartsWith);
  scAdd_External_Symbol("String::Substring^2", (void*)String_Substring);
  scAdd_External_Symbol("String::Truncate^1", (void*)String_Truncate);
  scAdd_External_Symbol("String::UpperCase^0", (void*)String_UpperCase);
  scAdd_External_Symbol("String::get_AsFloat", (void*)StringToFloat);
  scAdd_External_Symbol("String::get_AsInt", (void*)StringToInt);
  scAdd_External_Symbol("String::geti_Chars", (void*)String_GetChars);
  scAdd_External_Symbol("String::get_Length", (void*)strlen);

  scAdd_External_Symbol("Game::ChangeTranslation^1", (void *)Game_ChangeTranslation);
  scAdd_External_Symbol("Game::DoOnceOnly^1", (void *)Game_DoOnceOnly);
  scAdd_External_Symbol("Game::GetColorFromRGB^3", (void *)Game_GetColorFromRGB);
  scAdd_External_Symbol("Game::GetFrameCountForLoop^2", (void *)Game_GetFrameCountForLoop);
  scAdd_External_Symbol("Game::GetLocationName^2",(void *)Game_GetLocationName);
  scAdd_External_Symbol("Game::GetLoopCountForView^1", (void *)Game_GetLoopCountForView);
  scAdd_External_Symbol("Game::GetMODPattern^0",(void *)Game_GetMODPattern);
  scAdd_External_Symbol("Game::GetRunNextSettingForLoop^2", (void *)Game_GetRunNextSettingForLoop);
  scAdd_External_Symbol("Game::GetSaveSlotDescription^1",(void *)Game_GetSaveSlotDescription);
  scAdd_External_Symbol("Game::GetViewFrame^3",(void *)Game_GetViewFrame);
  scAdd_External_Symbol("Game::InputBox^1",(void *)Game_InputBox);
  scAdd_External_Symbol("Game::SetSaveGameDirectory^1", (void *)Game_SetSaveGameDirectory);
  scAdd_External_Symbol("Game::StopSound^1", (void *)StopAllSounds);
  scAdd_External_Symbol("Game::get_CharacterCount", (void *)Game_GetCharacterCount);
  scAdd_External_Symbol("Game::get_DialogCount", (void *)Game_GetDialogCount);
  scAdd_External_Symbol("Game::get_FileName", (void *)Game_GetFileName);
  scAdd_External_Symbol("Game::get_FontCount", (void *)Game_GetFontCount);
  scAdd_External_Symbol("Game::geti_GlobalMessages",(void *)Game_GetGlobalMessages);
  scAdd_External_Symbol("Game::geti_GlobalStrings",(void *)Game_GetGlobalStrings);
  scAdd_External_Symbol("Game::seti_GlobalStrings",(void *)SetGlobalString);
  scAdd_External_Symbol("Game::get_GUICount", (void *)Game_GetGUICount);
  scAdd_External_Symbol("Game::get_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_GetIgnoreUserInputAfterTextTimeoutMs);
  scAdd_External_Symbol("Game::set_IgnoreUserInputAfterTextTimeoutMs", (void *)Game_SetIgnoreUserInputAfterTextTimeoutMs);
  scAdd_External_Symbol("Game::get_InSkippableCutscene", (void *)Game_GetInSkippableCutscene);
  scAdd_External_Symbol("Game::get_InventoryItemCount", (void *)Game_GetInventoryItemCount);
  scAdd_External_Symbol("Game::get_MinimumTextDisplayTimeMs", (void *)Game_GetMinimumTextDisplayTimeMs);
  scAdd_External_Symbol("Game::set_MinimumTextDisplayTimeMs", (void *)Game_SetMinimumTextDisplayTimeMs);
  scAdd_External_Symbol("Game::get_MouseCursorCount", (void *)Game_GetMouseCursorCount);
  scAdd_External_Symbol("Game::get_Name", (void *)Game_GetName);
  scAdd_External_Symbol("Game::set_Name", (void *)Game_SetName);
  scAdd_External_Symbol("Game::get_NormalFont", (void *)Game_GetNormalFont);
  scAdd_External_Symbol("Game::set_NormalFont", (void *)SetNormalFont);
  scAdd_External_Symbol("Game::get_SkippingCutscene", (void *)Game_GetSkippingCutscene);
  scAdd_External_Symbol("Game::get_SpeechFont", (void *)Game_GetSpeechFont);
  scAdd_External_Symbol("Game::set_SpeechFont", (void *)SetSpeechFont);
  scAdd_External_Symbol("Game::geti_SpriteWidth", (void *)Game_GetSpriteWidth);
  scAdd_External_Symbol("Game::geti_SpriteHeight", (void *)Game_GetSpriteHeight);
  scAdd_External_Symbol("Game::get_TextReadingSpeed", (void *)Game_GetTextReadingSpeed);
  scAdd_External_Symbol("Game::set_TextReadingSpeed", (void *)Game_SetTextReadingSpeed);
  scAdd_External_Symbol("Game::get_TranslationFilename",(void *)Game_GetTranslationFilename);
  scAdd_External_Symbol("Game::get_UseNativeCoordinates", (void *)Game_GetUseNativeCoordinates);
  scAdd_External_Symbol("Game::get_ViewCount", (void *)Game_GetViewCount);

  scAdd_External_Symbol("System::get_CapsLock", (void *)System_GetCapsLock);
  scAdd_External_Symbol("System::get_ColorDepth", (void *)System_GetColorDepth);
  scAdd_External_Symbol("System::get_Gamma", (void *)System_GetGamma);
  scAdd_External_Symbol("System::set_Gamma", (void *)System_SetGamma);
  scAdd_External_Symbol("System::get_HardwareAcceleration", (void *)System_GetHardwareAcceleration);
  scAdd_External_Symbol("System::get_NumLock", (void *)System_GetNumLock);
  scAdd_External_Symbol("System::set_NumLock", (void *)System_SetNumLock);
  scAdd_External_Symbol("System::get_OperatingSystem", (void *)System_GetOS);
  scAdd_External_Symbol("System::get_ScreenHeight", (void *)System_GetScreenHeight);
  scAdd_External_Symbol("System::get_ScreenWidth", (void *)System_GetScreenWidth);
  scAdd_External_Symbol("System::get_ScrollLock", (void *)System_GetScrollLock);
  scAdd_External_Symbol("System::get_SupportsGammaControl", (void *)System_GetSupportsGammaControl);
  scAdd_External_Symbol("System::get_Version", (void *)System_GetVersion);
  scAdd_External_Symbol("SystemInfo::get_Version", (void *)System_GetVersion);
  scAdd_External_Symbol("System::get_ViewportHeight", (void *)System_GetViewportHeight);
  scAdd_External_Symbol("System::get_ViewportWidth", (void *)System_GetViewportWidth);
  scAdd_External_Symbol("System::get_Volume",(void *)System_GetVolume);
  scAdd_External_Symbol("System::set_Volume",(void *)System_SetVolume);
  scAdd_External_Symbol("System::get_VSync", (void *)System_GetVsync);
  scAdd_External_Symbol("System::set_VSync", (void *)System_SetVsync);
  scAdd_External_Symbol("System::get_Windowed", (void *)System_GetWindowed);

  scAdd_External_Symbol("Room::GetDrawingSurfaceForBackground^1", (void *)Room_GetDrawingSurfaceForBackground);
  scAdd_External_Symbol("Room::GetTextProperty^1",(void *)Room_GetTextProperty);
  scAdd_External_Symbol("Room::get_BottomEdge", (void *)Room_GetBottomEdge);
  scAdd_External_Symbol("Room::get_ColorDepth", (void *)Room_GetColorDepth);
  scAdd_External_Symbol("Room::get_Height", (void *)Room_GetHeight);
  scAdd_External_Symbol("Room::get_LeftEdge", (void *)Room_GetLeftEdge);
  scAdd_External_Symbol("Room::geti_Messages",(void *)Room_GetMessages);
  scAdd_External_Symbol("Room::get_MusicOnLoad", (void *)Room_GetMusicOnLoad);
  scAdd_External_Symbol("Room::get_ObjectCount", (void *)Room_GetObjectCount);
  scAdd_External_Symbol("Room::get_RightEdge", (void *)Room_GetRightEdge);
  scAdd_External_Symbol("Room::get_TopEdge", (void *)Room_GetTopEdge);
  scAdd_External_Symbol("Room::get_Width", (void *)Room_GetWidth);

  scAdd_External_Symbol("Parser::FindWordID^1",(void *)Parser_FindWordID);
  scAdd_External_Symbol("Parser::ParseText^1",(void *)ParseText);
  scAdd_External_Symbol("Parser::SaidUnknownWord^0",(void *)Parser_SaidUnknownWord);
  scAdd_External_Symbol("Parser::Said^1",(void *)Said);

  scAdd_External_Symbol("ViewFrame::get_Flipped", (void *)ViewFrame_GetFlipped);
  scAdd_External_Symbol("ViewFrame::get_Frame", (void *)ViewFrame_GetFrame);
  scAdd_External_Symbol("ViewFrame::get_Graphic", (void *)ViewFrame_GetGraphic);
  scAdd_External_Symbol("ViewFrame::set_Graphic", (void *)ViewFrame_SetGraphic);
  scAdd_External_Symbol("ViewFrame::get_LinkedAudio", (void *)ViewFrame_GetLinkedAudio);
  scAdd_External_Symbol("ViewFrame::set_LinkedAudio", (void *)ViewFrame_SetLinkedAudio);
  scAdd_External_Symbol("ViewFrame::get_Loop", (void *)ViewFrame_GetLoop);
  scAdd_External_Symbol("ViewFrame::get_Sound", (void *)ViewFrame_GetSound);
  scAdd_External_Symbol("ViewFrame::set_Sound", (void *)ViewFrame_SetSound);
  scAdd_External_Symbol("ViewFrame::get_Speed", (void *)ViewFrame_GetSpeed);
  scAdd_External_Symbol("ViewFrame::get_View", (void *)ViewFrame_GetView);
  
  scAdd_External_Symbol("AbortGame",(void *)_sc_AbortGame);
  scAdd_External_Symbol("AddInventory",(void *)add_inventory);
  scAdd_External_Symbol("AddInventoryToCharacter",(void *)AddInventoryToCharacter);
  scAdd_External_Symbol("AnimateButton",(void *)AnimateButton);
  scAdd_External_Symbol("AnimateCharacter",(void *)scAnimateCharacter);
  scAdd_External_Symbol("AnimateCharacterEx",(void *)AnimateCharacterEx);
  scAdd_External_Symbol("AnimateObject",(void *)AnimateObject);
  scAdd_External_Symbol("AnimateObjectEx",(void *)AnimateObjectEx);
  scAdd_External_Symbol("AreCharactersColliding",(void *)AreCharactersColliding);
  scAdd_External_Symbol("AreCharObjColliding",(void *)AreCharObjColliding);
  scAdd_External_Symbol("AreObjectsColliding",(void *)AreObjectsColliding);
  scAdd_External_Symbol("AreThingsOverlapping",(void *)AreThingsOverlapping);
  scAdd_External_Symbol("CallRoomScript",(void *)CallRoomScript);
  scAdd_External_Symbol("CDAudio",(void *)cd_manager);
  scAdd_External_Symbol("CentreGUI",(void *)CentreGUI);
  scAdd_External_Symbol("ChangeCharacterView",(void *)ChangeCharacterView);
  scAdd_External_Symbol("ChangeCursorGraphic",(void *)ChangeCursorGraphic);
  scAdd_External_Symbol("ChangeCursorHotspot",(void *)ChangeCursorHotspot);
  scAdd_External_Symbol("ClaimEvent",(void *)ClaimEvent);
  scAdd_External_Symbol("CreateGraphicOverlay",(void *)CreateGraphicOverlay);
  scAdd_External_Symbol("CreateTextOverlay",(void *)CreateTextOverlay);
  scAdd_External_Symbol("CyclePalette",(void *)CyclePalette);
  scAdd_External_Symbol("Debug",(void *)script_debug);
  scAdd_External_Symbol("DeleteSaveSlot",(void *)DeleteSaveSlot);
  scAdd_External_Symbol("DeleteSprite",(void *)free_dynamic_sprite);
  scAdd_External_Symbol("DisableCursorMode",(void *)disable_cursor_mode);
  scAdd_External_Symbol("DisableGroundLevelAreas",(void *)DisableGroundLevelAreas);
  scAdd_External_Symbol("DisableHotspot",(void *)DisableHotspot);
  scAdd_External_Symbol("DisableInterface",(void *)DisableInterface);
  scAdd_External_Symbol("DisableRegion",(void *)DisableRegion);
  scAdd_External_Symbol("Display",(void *)Display);
  scAdd_External_Symbol("DisplayAt",(void *)DisplayAt);
  scAdd_External_Symbol("DisplayAtY",(void *)DisplayAtY);
  scAdd_External_Symbol("DisplayMessage",(void *)DisplayMessage);
  scAdd_External_Symbol("DisplayMessageAtY",(void *)DisplayMessageAtY);
  scAdd_External_Symbol("DisplayMessageBar",(void *)DisplayMessageBar);
  scAdd_External_Symbol("DisplaySpeech",(void *)__sc_displayspeech);
  scAdd_External_Symbol("DisplaySpeechAt", (void *)DisplaySpeechAt);
  scAdd_External_Symbol("DisplaySpeechBackground",(void *)DisplaySpeechBackground);
  scAdd_External_Symbol("DisplayThought",(void *)DisplayThought);
  scAdd_External_Symbol("DisplayTopBar",(void *)DisplayTopBar);
  scAdd_External_Symbol("EnableCursorMode",(void *)enable_cursor_mode);
  scAdd_External_Symbol("EnableGroundLevelAreas",(void *)EnableGroundLevelAreas);
  scAdd_External_Symbol("EnableHotspot",(void *)EnableHotspot);
  scAdd_External_Symbol("EnableInterface",(void *)EnableInterface);
  scAdd_External_Symbol("EnableRegion",(void *)EnableRegion);
  scAdd_External_Symbol("EndCutscene", (void *)EndCutscene);
  scAdd_External_Symbol("FaceCharacter",(void *)FaceCharacter);
  scAdd_External_Symbol("FaceLocation",(void *)FaceLocation);
  scAdd_External_Symbol("FadeIn",(void *)FadeIn);
  scAdd_External_Symbol("FadeOut",(void *)my_fade_out);
  scAdd_External_Symbol("FileClose",(void *)FileClose);
  scAdd_External_Symbol("FileIsEOF",(void *)FileIsEOF);
  scAdd_External_Symbol("FileIsError",(void *)FileIsError);
  scAdd_External_Symbol("FileOpen",(void *)FileOpen);
  scAdd_External_Symbol("FileRead",(void *)FileRead);
  scAdd_External_Symbol("FileReadInt",(void *)FileReadInt);
  scAdd_External_Symbol("FileReadRawChar",(void *)FileReadRawChar);
  scAdd_External_Symbol("FileReadRawInt",(void *)FileReadRawInt);
  scAdd_External_Symbol("FileWrite",(void *)FileWrite);
  scAdd_External_Symbol("FileWriteInt",(void *)FileWriteInt);
  scAdd_External_Symbol("FileWriteRawChar",(void *)FileWriteRawChar);
  scAdd_External_Symbol("FileWriteRawLine", (void *)FileWriteRawLine);
  scAdd_External_Symbol("FindGUIID",(void *)FindGUIID);
  scAdd_External_Symbol("FlipScreen",(void *)FlipScreen);
  scAdd_External_Symbol("FloatToInt",(void *)FloatToInt);
  scAdd_External_Symbol("FollowCharacter",(void *)FollowCharacter);
  scAdd_External_Symbol("FollowCharacterEx",(void *)FollowCharacterEx);
  scAdd_External_Symbol("GetBackgroundFrame",(void *)GetBackgroundFrame);
  scAdd_External_Symbol("GetButtonPic",(void *)GetButtonPic);
  scAdd_External_Symbol("GetCharacterAt",(void *)GetCharacterAt);
  scAdd_External_Symbol("GetCharacterProperty",(void *)GetCharacterProperty);
  scAdd_External_Symbol("GetCharacterPropertyText",(void *)GetCharacterPropertyText);
  scAdd_External_Symbol("GetCurrentMusic",(void *)GetCurrentMusic);
  scAdd_External_Symbol("GetCursorMode",(void *)GetCursorMode);
  scAdd_External_Symbol("GetDialogOption",(void *)GetDialogOption);
  scAdd_External_Symbol("GetGameOption",(void *)GetGameOption);
  scAdd_External_Symbol("GetGameParameter",(void *)GetGameParameter);
  scAdd_External_Symbol("GetGameSpeed",(void *)GetGameSpeed);
  scAdd_External_Symbol("GetGlobalInt",(void *)GetGlobalInt);
  scAdd_External_Symbol("GetGlobalString",(void *)GetGlobalString);
  scAdd_External_Symbol("GetGraphicalVariable",(void *)GetGraphicalVariable);
  scAdd_External_Symbol("GetGUIAt", (void *)GetGUIAt);
  scAdd_External_Symbol("GetGUIObjectAt", (void *)GetGUIObjectAt);
  scAdd_External_Symbol("GetHotspotAt",(void *)GetHotspotAt);
  scAdd_External_Symbol("GetHotspotName",(void *)GetHotspotName);
  scAdd_External_Symbol("GetHotspotPointX",(void *)GetHotspotPointX);
  scAdd_External_Symbol("GetHotspotPointY",(void *)GetHotspotPointY);
  scAdd_External_Symbol("GetHotspotProperty",(void *)GetHotspotProperty);
  scAdd_External_Symbol("GetHotspotPropertyText",(void *)GetHotspotPropertyText);
  scAdd_External_Symbol("GetInvAt",(void *)GetInvAt);
  scAdd_External_Symbol("GetInvGraphic",(void *)GetInvGraphic);
  scAdd_External_Symbol("GetInvName",(void *)GetInvName);
  scAdd_External_Symbol("GetInvProperty",(void *)GetInvProperty);
  scAdd_External_Symbol("GetInvPropertyText",(void *)GetInvPropertyText);
  //scAdd_External_Symbol("GetLanguageString",(void *)GetLanguageString);
  scAdd_External_Symbol("GetLocationName",(void *)GetLocationName);
  scAdd_External_Symbol("GetLocationType",(void *)GetLocationType);
  scAdd_External_Symbol("GetMessageText", (void *)GetMessageText);
  scAdd_External_Symbol("GetMIDIPosition", (void *)GetMIDIPosition);
  scAdd_External_Symbol("GetMP3PosMillis", (void *)GetMP3PosMillis);
  scAdd_External_Symbol("GetObjectAt",(void *)GetObjectAt);
  scAdd_External_Symbol("GetObjectBaseline",(void *)GetObjectBaseline);
  scAdd_External_Symbol("GetObjectGraphic",(void *)GetObjectGraphic);
  scAdd_External_Symbol("GetObjectName",(void *)GetObjectName);
  scAdd_External_Symbol("GetObjectProperty",(void *)GetObjectProperty);
  scAdd_External_Symbol("GetObjectPropertyText",(void *)GetObjectPropertyText);
  scAdd_External_Symbol("GetObjectX",(void *)GetObjectX);
  scAdd_External_Symbol("GetObjectY",(void *)GetObjectY);
//  scAdd_External_Symbol("GetPalette",(void *)scGetPal);
  scAdd_External_Symbol("GetPlayerCharacter",(void *)GetPlayerCharacter);
  scAdd_External_Symbol("GetRawTime",(void *)GetRawTime);
  scAdd_External_Symbol("GetRegionAt",(void *)GetRegionAt);
  scAdd_External_Symbol("GetRoomProperty",(void *)GetRoomProperty);
  scAdd_External_Symbol("GetRoomPropertyText",(void *)GetRoomPropertyText);
  scAdd_External_Symbol("GetSaveSlotDescription",(void *)GetSaveSlotDescription);
  scAdd_External_Symbol("GetScalingAt",(void *)GetScalingAt);
  scAdd_External_Symbol("GetSliderValue",(void *)GetSliderValue);
  scAdd_External_Symbol("GetTextBoxText",(void *)GetTextBoxText);
  scAdd_External_Symbol("GetTextHeight",(void *)GetTextHeight);
  scAdd_External_Symbol("GetTextWidth",(void *)GetTextWidth);
  scAdd_External_Symbol("GetTime",(void *)sc_GetTime);
  scAdd_External_Symbol("GetTranslation", (void *)get_translation);
  scAdd_External_Symbol("GetTranslationName", (void *)GetTranslationName);
  scAdd_External_Symbol("GetViewportX",(void *)GetViewportX);
  scAdd_External_Symbol("GetViewportY",(void *)GetViewportY);
  scAdd_External_Symbol("GetWalkableAreaAt",(void *)GetWalkableAreaAt);
  scAdd_External_Symbol("GiveScore",(void *)GiveScore);
  scAdd_External_Symbol("HasPlayerBeenInRoom",(void *)HasPlayerBeenInRoom);
  scAdd_External_Symbol("HideMouseCursor",(void *)HideMouseCursor);
  scAdd_External_Symbol("InputBox",(void *)sc_inputbox);
  scAdd_External_Symbol("InterfaceOff",(void *)InterfaceOff);
  scAdd_External_Symbol("InterfaceOn",(void *)InterfaceOn);
  scAdd_External_Symbol("IntToFloat",(void *)IntToFloat);
  scAdd_External_Symbol("InventoryScreen",(void *)sc_invscreen);
  scAdd_External_Symbol("IsButtonDown",(void *)IsButtonDown);
  scAdd_External_Symbol("IsChannelPlaying",(void *)IsChannelPlaying);
  scAdd_External_Symbol("IsGamePaused",(void *)IsGamePaused);
  scAdd_External_Symbol("IsGUIOn", (void *)IsGUIOn);
  scAdd_External_Symbol("IsInteractionAvailable", (void *)IsInteractionAvailable);
  scAdd_External_Symbol("IsInventoryInteractionAvailable", (void *)IsInventoryInteractionAvailable);
  scAdd_External_Symbol("IsInterfaceEnabled", (void *)IsInterfaceEnabled);
  scAdd_External_Symbol("IsKeyPressed",(void *)IsKeyPressed);
  scAdd_External_Symbol("IsMusicPlaying",(void *)IsMusicPlaying);
  scAdd_External_Symbol("IsMusicVoxAvailable",(void *)IsMusicVoxAvailable);
  scAdd_External_Symbol("IsObjectAnimating",(void *)IsObjectAnimating);
  scAdd_External_Symbol("IsObjectMoving",(void *)IsObjectMoving);
  scAdd_External_Symbol("IsObjectOn",(void *)IsObjectOn);
  scAdd_External_Symbol("IsOverlayValid",(void *)IsOverlayValid);
  scAdd_External_Symbol("IsSoundPlaying",(void *)IsSoundPlaying);
  scAdd_External_Symbol("IsTimerExpired",(void *)IsTimerExpired);
  scAdd_External_Symbol("IsTranslationAvailable", (void *)IsTranslationAvailable);
  scAdd_External_Symbol("IsVoxAvailable",(void *)IsVoxAvailable);
  scAdd_External_Symbol("ListBoxAdd", (void *)ListBoxAdd);
  scAdd_External_Symbol("ListBoxClear", (void *)ListBoxClear);
  scAdd_External_Symbol("ListBoxDirList", (void *)ListBoxDirList);
  scAdd_External_Symbol("ListBoxGetItemText", (void *)ListBoxGetItemText);
  scAdd_External_Symbol("ListBoxGetNumItems", (void *)ListBoxGetNumItems);
  scAdd_External_Symbol("ListBoxGetSelected", (void *)ListBoxGetSelected);
  scAdd_External_Symbol("ListBoxRemove", (void *)ListBoxRemove);
  scAdd_External_Symbol("ListBoxSaveGameList", (void *)ListBoxSaveGameList);
  scAdd_External_Symbol("ListBoxSetSelected", (void *)ListBoxSetSelected);
  scAdd_External_Symbol("ListBoxSetTopItem", (void *)ListBoxSetTopItem);
  scAdd_External_Symbol("LoadImageFile",(void *)LoadImageFile);
  scAdd_External_Symbol("LoadSaveSlotScreenshot",(void *)LoadSaveSlotScreenshot);
  scAdd_External_Symbol("LoseInventory",(void *)lose_inventory);
  scAdd_External_Symbol("LoseInventoryFromCharacter",(void *)LoseInventoryFromCharacter);
  scAdd_External_Symbol("MergeObject",(void *)MergeObject);
  scAdd_External_Symbol("MoveCharacter",(void *)MoveCharacter);
  scAdd_External_Symbol("MoveCharacterBlocking",(void *)MoveCharacterBlocking);
  scAdd_External_Symbol("MoveCharacterDirect",(void *)MoveCharacterDirect);
  scAdd_External_Symbol("MoveCharacterPath",(void *)MoveCharacterPath);
  scAdd_External_Symbol("MoveCharacterStraight",(void *)MoveCharacterStraight);
  scAdd_External_Symbol("MoveCharacterToHotspot",(void *)MoveCharacterToHotspot);
  scAdd_External_Symbol("MoveCharacterToObject",(void *)MoveCharacterToObject);
  scAdd_External_Symbol("MoveObject",(void *)MoveObject);
  scAdd_External_Symbol("MoveObjectDirect",(void *)MoveObjectDirect);
  scAdd_External_Symbol("MoveOverlay",(void *)MoveOverlay);
  scAdd_External_Symbol("MoveToWalkableArea", (void *)MoveToWalkableArea);
  scAdd_External_Symbol("NewRoom",(void *)NewRoom);
  scAdd_External_Symbol("NewRoomEx",(void *)NewRoomEx);
  scAdd_External_Symbol("NewRoomNPC",(void *)NewRoomNPC);
  scAdd_External_Symbol("ObjectOff",(void *)ObjectOff);
  scAdd_External_Symbol("ObjectOn",(void *)ObjectOn);
  scAdd_External_Symbol("ParseText",(void *)ParseText);
  scAdd_External_Symbol("PauseGame",(void *)PauseGame);
  scAdd_External_Symbol("PlayAmbientSound",(void *)PlayAmbientSound);
  scAdd_External_Symbol("PlayFlic",(void *)play_flc_file);
  scAdd_External_Symbol("PlayMP3File",(void *)PlayMP3File);
  scAdd_External_Symbol("PlayMusic",(void *)PlayMusicResetQueue);
  scAdd_External_Symbol("PlayMusicQueued",(void *)PlayMusicQueued);
  scAdd_External_Symbol("PlaySilentMIDI",(void *)PlaySilentMIDI);
  scAdd_External_Symbol("PlaySound",(void *)play_sound);
  scAdd_External_Symbol("PlaySoundEx",(void *)PlaySoundEx);
  scAdd_External_Symbol("PlaySpeech",(void *)__scr_play_speech);
  scAdd_External_Symbol("PlayVideo",(void *)scrPlayVideo);
  scAdd_External_Symbol("ProcessClick",(void *)ProcessClick);
  scAdd_External_Symbol("QuitGame",(void *)QuitGame);
  scAdd_External_Symbol("Random",(void *)__Rand);
  scAdd_External_Symbol("RawClearScreen", (void *)RawClear);
  scAdd_External_Symbol("RawDrawCircle",(void *)RawDrawCircle);
  scAdd_External_Symbol("RawDrawFrameTransparent",(void *)RawDrawFrameTransparent);
  scAdd_External_Symbol("RawDrawImage", (void *)RawDrawImage);
  scAdd_External_Symbol("RawDrawImageOffset", (void *)RawDrawImageOffset);
  scAdd_External_Symbol("RawDrawImageResized", (void *)RawDrawImageResized);
  scAdd_External_Symbol("RawDrawImageTransparent", (void *)RawDrawImageTransparent);
  scAdd_External_Symbol("RawDrawLine", (void *)RawDrawLine);
  scAdd_External_Symbol("RawDrawRectangle", (void *)RawDrawRectangle);
  scAdd_External_Symbol("RawDrawTriangle", (void *)RawDrawTriangle);
  scAdd_External_Symbol("RawPrint", (void *)RawPrint);
  scAdd_External_Symbol("RawPrintMessageWrapped", (void *)RawPrintMessageWrapped);
  scAdd_External_Symbol("RawRestoreScreen", (void *)RawRestoreScreen);
  scAdd_External_Symbol("RawRestoreScreenTinted", (void *)RawRestoreScreenTinted);
  scAdd_External_Symbol("RawSaveScreen", (void *)RawSaveScreen);
  scAdd_External_Symbol("RawSetColor", (void *)RawSetColor);
  scAdd_External_Symbol("RawSetColorRGB", (void *)RawSetColorRGB);
  scAdd_External_Symbol("RefreshMouse",(void *)RefreshMouse);
  scAdd_External_Symbol("ReleaseCharacterView",(void *)ReleaseCharacterView);
  scAdd_External_Symbol("ReleaseViewport",(void *)ReleaseViewport);
  scAdd_External_Symbol("RemoveObjectTint",(void *)RemoveObjectTint);
  scAdd_External_Symbol("RemoveOverlay",(void *)RemoveOverlay);
  scAdd_External_Symbol("RemoveWalkableArea",(void *)RemoveWalkableArea);
  scAdd_External_Symbol("ResetRoom",(void *)ResetRoom);
  scAdd_External_Symbol("RestartGame",(void *)restart_game);
  scAdd_External_Symbol("RestoreGameDialog",(void *)restore_game_dialog);
  scAdd_External_Symbol("RestoreGameSlot",(void *)RestoreGameSlot);
  scAdd_External_Symbol("RestoreWalkableArea",(void *)RestoreWalkableArea);
  scAdd_External_Symbol("RunAGSGame", (void *)RunAGSGame);
  scAdd_External_Symbol("RunCharacterInteraction",(void *)RunCharacterInteraction);
  scAdd_External_Symbol("RunDialog",(void *)RunDialog);
  scAdd_External_Symbol("RunHotspotInteraction", (void *)RunHotspotInteraction);
  scAdd_External_Symbol("RunInventoryInteraction", (void *)RunInventoryInteraction);
  scAdd_External_Symbol("RunObjectInteraction", (void *)RunObjectInteraction);
  scAdd_External_Symbol("RunRegionInteraction", (void *)RunRegionInteraction);
  scAdd_External_Symbol("Said",(void *)Said);
  scAdd_External_Symbol("SaidUnknownWord",(void *)SaidUnknownWord);
  scAdd_External_Symbol("SaveCursorForLocationChange",(void *)SaveCursorForLocationChange);
  scAdd_External_Symbol("SaveGameDialog",(void *)save_game_dialog);
  scAdd_External_Symbol("SaveGameSlot",(void *)save_game);
  scAdd_External_Symbol("SaveScreenShot",(void *)SaveScreenShot);
  scAdd_External_Symbol("SeekMIDIPosition", (void *)SeekMIDIPosition);
  scAdd_External_Symbol("SeekMODPattern",(void *)SeekMODPattern);
  scAdd_External_Symbol("SeekMP3PosMillis", (void *)SeekMP3PosMillis);
  scAdd_External_Symbol("SetActiveInventory",(void *)SetActiveInventory);
  scAdd_External_Symbol("SetAmbientTint",(void *)SetAmbientTint);
  scAdd_External_Symbol("SetAreaLightLevel",(void *)SetAreaLightLevel);
  scAdd_External_Symbol("SetAreaScaling",(void *)SetAreaScaling);
  scAdd_External_Symbol("SetBackgroundFrame",(void *)SetBackgroundFrame);
  scAdd_External_Symbol("SetButtonPic",(void *)SetButtonPic);
  scAdd_External_Symbol("SetButtonText",(void *)SetButtonText);
  scAdd_External_Symbol("SetChannelVolume",(void *)SetChannelVolume);
  scAdd_External_Symbol("SetCharacterBaseline",(void *)SetCharacterBaseline);
  scAdd_External_Symbol("SetCharacterClickable",(void *)SetCharacterClickable);
  scAdd_External_Symbol("SetCharacterFrame",(void *)SetCharacterFrame);
  scAdd_External_Symbol("SetCharacterIdle",(void *)SetCharacterIdle);
  scAdd_External_Symbol("SetCharacterIgnoreLight",(void *)SetCharacterIgnoreLight);
  scAdd_External_Symbol("SetCharacterIgnoreWalkbehinds",(void *)SetCharacterIgnoreWalkbehinds);
  scAdd_External_Symbol("SetCharacterProperty",(void *)SetCharacterProperty);
  scAdd_External_Symbol("SetCharacterBlinkView",(void *)SetCharacterBlinkView);
  scAdd_External_Symbol("SetCharacterSpeechView",(void *)SetCharacterSpeechView);
  scAdd_External_Symbol("SetCharacterSpeed",(void *)SetCharacterSpeed);
  scAdd_External_Symbol("SetCharacterSpeedEx",(void *)SetCharacterSpeedEx);
  scAdd_External_Symbol("SetCharacterTransparency",(void *)SetCharacterTransparency);
  scAdd_External_Symbol("SetCharacterView",(void *)SetCharacterView);
  scAdd_External_Symbol("SetCharacterViewEx",(void *)SetCharacterViewEx);
  scAdd_External_Symbol("SetCharacterViewOffset",(void *)SetCharacterViewOffset);
  scAdd_External_Symbol("SetCursorMode",(void *)set_cursor_mode);
  scAdd_External_Symbol("SetDefaultCursor",(void *)set_default_cursor);
  scAdd_External_Symbol("SetDialogOption",(void *)SetDialogOption);
  scAdd_External_Symbol("SetDigitalMasterVolume",(void *)SetDigitalMasterVolume);
  scAdd_External_Symbol("SetFadeColor",(void *)SetFadeColor);
  scAdd_External_Symbol("SetFrameSound",(void *)SetFrameSound);
  scAdd_External_Symbol("SetGameOption",(void *)SetGameOption);
  scAdd_External_Symbol("SetGameSpeed",(void *)SetGameSpeed);
  scAdd_External_Symbol("SetGlobalInt",(void *)SetGlobalInt);
  scAdd_External_Symbol("SetGlobalString",(void *)SetGlobalString);
  scAdd_External_Symbol("SetGraphicalVariable",(void *)SetGraphicalVariable);
  scAdd_External_Symbol("SetGUIBackgroundPic", (void *)SetGUIBackgroundPic);
  scAdd_External_Symbol("SetGUIClickable", (void *)SetGUIClickable);
  scAdd_External_Symbol("SetGUIObjectEnabled",(void *)SetGUIObjectEnabled);
  scAdd_External_Symbol("SetGUIObjectPosition",(void *)SetGUIObjectPosition);
  scAdd_External_Symbol("SetGUIObjectSize",(void *)SetGUIObjectSize);
  scAdd_External_Symbol("SetGUIPosition",(void *)SetGUIPosition);
  scAdd_External_Symbol("SetGUISize",(void *)SetGUISize);
  scAdd_External_Symbol("SetGUITransparency", (void *)SetGUITransparency);
  scAdd_External_Symbol("SetGUIZOrder", (void *)SetGUIZOrder);
  scAdd_External_Symbol("SetInvItemName",(void *)SetInvItemName);
  scAdd_External_Symbol("SetInvItemPic",(void *)set_inv_item_pic);
  scAdd_External_Symbol("SetInvDimensions",(void *)SetInvDimensions);
  scAdd_External_Symbol("SetLabelColor",(void *)SetLabelColor);
  scAdd_External_Symbol("SetLabelFont",(void *)SetLabelFont);
  scAdd_External_Symbol("SetLabelText",(void *)SetLabelText);
  scAdd_External_Symbol("SetMouseBounds",(void *)SetMouseBounds);
  scAdd_External_Symbol("SetMouseCursor",(void *)set_mouse_cursor);
  scAdd_External_Symbol("SetMousePosition",(void *)SetMousePosition);
  scAdd_External_Symbol("SetMultitaskingMode",(void *)SetMultitasking);
  scAdd_External_Symbol("SetMusicMasterVolume",(void *)SetMusicMasterVolume);
  scAdd_External_Symbol("SetMusicRepeat",(void *)SetMusicRepeat);
  scAdd_External_Symbol("SetMusicVolume",(void *)SetMusicVolume);
  scAdd_External_Symbol("SetNextCursorMode", (void *)SetNextCursor);
  scAdd_External_Symbol("SetNextScreenTransition",(void *)SetNextScreenTransition);
  scAdd_External_Symbol("SetNormalFont", (void *)SetNormalFont);
  scAdd_External_Symbol("SetObjectBaseline",(void *)SetObjectBaseline);
  scAdd_External_Symbol("SetObjectClickable",(void *)SetObjectClickable);
  scAdd_External_Symbol("SetObjectFrame",(void *)SetObjectFrame);
  scAdd_External_Symbol("SetObjectGraphic",(void *)SetObjectGraphic);
  scAdd_External_Symbol("SetObjectIgnoreWalkbehinds",(void *)SetObjectIgnoreWalkbehinds);
  scAdd_External_Symbol("SetObjectPosition",(void *)SetObjectPosition);
  scAdd_External_Symbol("SetObjectTint",(void *)SetObjectTint);
  scAdd_External_Symbol("SetObjectTransparency",(void *)SetObjectTransparency);
  scAdd_External_Symbol("SetObjectView",(void *)SetObjectView);
//  scAdd_External_Symbol("SetPalette",scSetPal);
  scAdd_External_Symbol("SetPalRGB",(void *)SetPalRGB);
  scAdd_External_Symbol("SetPlayerCharacter",(void *)SetPlayerCharacter);
  scAdd_External_Symbol("SetRegionTint",(void *)SetRegionTint);
  scAdd_External_Symbol("SetRestartPoint",(void *)SetRestartPoint);
  scAdd_External_Symbol("SetScreenTransition",(void *)SetScreenTransition);
  scAdd_External_Symbol("SetSkipSpeech",(void *)SetSkipSpeech);
  scAdd_External_Symbol("SetSliderValue",(void *)SetSliderValue);
  scAdd_External_Symbol("SetSoundVolume",(void *)SetSoundVolume);
  scAdd_External_Symbol("SetSpeechFont", (void *)SetSpeechFont);
  scAdd_External_Symbol("SetSpeechStyle", (void *)SetSpeechStyle);
  scAdd_External_Symbol("SetSpeechVolume",(void *)SetSpeechVolume);
  scAdd_External_Symbol("SetTalkingColor",(void *)SetTalkingColor);
  scAdd_External_Symbol("SetTextBoxFont",(void *)SetTextBoxFont);
  scAdd_External_Symbol("SetTextBoxText",(void *)SetTextBoxText);
  scAdd_External_Symbol("SetTextOverlay",(void *)SetTextOverlay);
  scAdd_External_Symbol("SetTextWindowGUI",(void *)SetTextWindowGUI);
  scAdd_External_Symbol("SetTimer",(void *)script_SetTimer);
  scAdd_External_Symbol("SetViewport",(void *)SetViewport);
  scAdd_External_Symbol("SetVoiceMode",(void *)SetVoiceMode);
  scAdd_External_Symbol("SetWalkBehindBase",(void *)SetWalkBehindBase);
  scAdd_External_Symbol("ShakeScreen",(void *)ShakeScreen);
  scAdd_External_Symbol("ShakeScreenBackground",(void *)ShakeScreenBackground);
  scAdd_External_Symbol("ShowMouseCursor",(void *)ShowMouseCursor);
  scAdd_External_Symbol("SkipUntilCharacterStops",(void *)SkipUntilCharacterStops);
  scAdd_External_Symbol("StartCutscene", (void *)StartCutscene);
  scAdd_External_Symbol("StartRecording", (void *)scStartRecording);
  scAdd_External_Symbol("StopAmbientSound",(void *)StopAmbientSound);
  scAdd_External_Symbol("StopChannel",(void *)stop_and_destroy_channel);
  scAdd_External_Symbol("StopDialog",(void *)StopDialog);
  scAdd_External_Symbol("StopMoving",(void *)StopMoving);
  scAdd_External_Symbol("StopMusic", (void *)scr_StopMusic);
  scAdd_External_Symbol("StopObjectMoving",(void *)StopObjectMoving);
  scAdd_External_Symbol("StrCat",(void *)_sc_strcat);
  scAdd_External_Symbol("StrCaseComp",(void *)stricmp);
  scAdd_External_Symbol("StrComp",(void *)strcmp);
  scAdd_External_Symbol("StrContains",(void *)StrContains);
  scAdd_External_Symbol("StrCopy",(void *)_sc_strcpy);
  scAdd_External_Symbol("StrFormat",(void *)_sc_sprintf);
  scAdd_External_Symbol("StrGetCharAt", (void *)StrGetCharAt);
  scAdd_External_Symbol("StringToInt",(void *)StringToInt);
  scAdd_External_Symbol("StrLen",(void *)strlen);
  scAdd_External_Symbol("StrSetCharAt", (void *)StrSetCharAt);
  scAdd_External_Symbol("StrToLowerCase", (void *)_sc_strlower);
  scAdd_External_Symbol("StrToUpperCase", (void *)_sc_strupper);
  scAdd_External_Symbol("TintScreen",(void *)TintScreen);
  scAdd_External_Symbol("UnPauseGame",(void *)UnPauseGame);
  scAdd_External_Symbol("UpdateInventory", (void *)update_invorder);
  scAdd_External_Symbol("UpdatePalette",(void *)UpdatePalette);
  scAdd_External_Symbol("Wait",(void *)scrWait);
  scAdd_External_Symbol("WaitKey",(void *)WaitKey);
  scAdd_External_Symbol("WaitMouseKey",(void *)WaitMouseKey);
  scAdd_External_Symbol("game",&play);
  scAdd_External_Symbol("gs_globals",&play.globalvars[0]);
  scAdd_External_Symbol("mouse",&scmouse);
  scAdd_External_Symbol("palette",&palette[0]);
  scAdd_External_Symbol("system",&scsystem);
  scAdd_External_Symbol("savegameindex",&play.filenumbers[0]);


  // Stubs for plugin functions.

  // ags_shell.dll
  scAdd_External_Symbol("ShellExecute", (void*)ScriptStub_ShellExecute);

  // ags_snowrain.dll
  scAdd_External_Symbol("srSetSnowDriftRange",(void *)srSetSnowDriftRange);
  scAdd_External_Symbol("srSetSnowDriftSpeed",(void *)srSetSnowDriftSpeed);
  scAdd_External_Symbol("srSetSnowFallSpeed",(void *)srSetSnowFallSpeed);
  scAdd_External_Symbol("srChangeSnowAmount",(void *)srChangeSnowAmount);
  scAdd_External_Symbol("srSetSnowBaseline",(void *)srSetSnowBaseline);
  scAdd_External_Symbol("srSetSnowTransparency",(void *)srSetSnowTransparency);
  scAdd_External_Symbol("srSetSnowDefaultView",(void *)srSetSnowDefaultView);
  scAdd_External_Symbol("srSetSnowWindSpeed",(void *)srSetSnowWindSpeed);
  scAdd_External_Symbol("srSetSnowAmount",(void *)srSetSnowAmount);
  scAdd_External_Symbol("srSetSnowView",(void *)srSetSnowView);
  scAdd_External_Symbol("srChangeRainAmount",(void *)srChangeRainAmount);
  scAdd_External_Symbol("srSetRainView",(void *)srSetRainView);
  scAdd_External_Symbol("srSetRainDefaultView",(void *)srSetRainDefaultView);
  scAdd_External_Symbol("srSetRainTransparency",(void *)srSetRainTransparency);
  scAdd_External_Symbol("srSetRainWindSpeed",(void *)srSetRainWindSpeed);
  scAdd_External_Symbol("srSetRainBaseline",(void *)srSetRainBaseline);
  scAdd_External_Symbol("srSetRainAmount",(void *)srSetRainAmount);
  scAdd_External_Symbol("srSetRainFallSpeed",(void *)srSetRainFallSpeed);
  scAdd_External_Symbol("srSetWindSpeed",(void *)srSetWindSpeed);
  scAdd_External_Symbol("srSetBaseline",(void *)srSetBaseline);

  // agsjoy.dll
  scAdd_External_Symbol("JoystickCount",(void *)JoystickCount);
  scAdd_External_Symbol("Joystick::Open^1",(void *)Joystick_Open);
  scAdd_External_Symbol("Joystick::IsButtonDown^1",(void *)Joystick_IsButtonDown);
  scAdd_External_Symbol("Joystick::EnableEvents^1",(void *)Joystick_EnableEvents);
  scAdd_External_Symbol("Joystick::DisableEvents^0",(void *)Joystick_DisableEvents);
  scAdd_External_Symbol("Joystick::Click^1",(void *)Joystick_Click);
  scAdd_External_Symbol("Joystick::Valid^0",(void *)Joystick_Valid);
  scAdd_External_Symbol("Joystick::Unplugged^0",(void *)Joystick_Unplugged);

  // agsblend.dll
  scAdd_External_Symbol("DrawAlpha",(void *)DrawAlpha);
  scAdd_External_Symbol("GetAlpha",(void *)GetAlpha);
  scAdd_External_Symbol("PutAlpha",(void *)PutAlpha);
  scAdd_External_Symbol("Blur",(void *)Blur);
  scAdd_External_Symbol("HighPass",(void *)HighPass);
  scAdd_External_Symbol("DrawAdd",(void *)DrawAdd);
}


extern const char* ccGetSectionNameAtOffs(ccScript *scri, long offs);

void break_into_debugger() 
{
#ifdef WINDOWS_VERSION

  if (editor_window_handle != NULL)
    SetForegroundWindow(editor_window_handle);

  send_message_to_editor("BREAK");
  game_paused_in_debugger = 1;

  while (game_paused_in_debugger) 
  {
    update_polled_stuff();
    platform->YieldCPU();
  }

#endif
}

int scrDebugWait = 0;
// allow LShift to single-step,  RShift to pause flow
void scriptDebugHook (ccInstance *ccinst, int linenum) {

  if (pluginsWantingDebugHooks > 0) {
    // a plugin is handling the debugging
    char scname[40];
    get_script_name(ccinst, scname);
    platform->RunPluginDebugHooks(scname, linenum);
    return;
  }

  // no plugin, use built-in debugger

  if (ccinst == NULL) 
  {
    // come out of script
    return;
  }

  if (break_on_next_script_step) 
  {
    break_on_next_script_step = 0;
    break_into_debugger();
    return;
  }

  const char *scriptName = ccGetSectionNameAtOffs(ccinst->runningInst->instanceof, ccinst->pc);

  for (int i = 0; i < numBreakpoints; i++)
  {
    if ((breakpoints[i].lineNumber == linenum) &&
        (strcmp(breakpoints[i].scriptName, scriptName) == 0))
    {
      break_into_debugger();
      break;
    }
  }
}

void check_debug_keys() {
    if (play.debug_mode) {
      // do the run-time script debugging

      if ((!key[KEY_SCRLOCK]) && (scrlockWasDown))
        scrlockWasDown = 0;
      else if ((key[KEY_SCRLOCK]) && (!scrlockWasDown)) {

        break_on_next_script_step = 1;
        scrlockWasDown = 1;
      }

    }

}

void check_new_room() {
  // if they're in a new room, run Player Enters Screen and on_event(ENTER_ROOM)
  if ((in_new_room>0) & (in_new_room!=3)) {
    EventHappened evh;
    evh.type = EV_RUNEVBLOCK;
    evh.data1 = EVB_ROOM;
    evh.data2 = 0;
    evh.data3 = 5;
    evh.player=game.playercharacter;
    // make sure that any script calls don't re-call enters screen
    int newroom_was = in_new_room;
    in_new_room = 0;
    play.disabled_user_interface ++;
    process_event(&evh);
    play.disabled_user_interface --;
    in_new_room = newroom_was;
//    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,5);
  }
}

void construct_virtual_screen(bool fullRedraw) 
{
  gfxDriver->ClearDrawList();

  if (play.fast_forward)
    return;

  our_eip=3;

  gfxDriver->UseSmoothScaling(IS_ANTIALIAS_SPRITES);

  platform->RunPluginHooks(AGSE_PRERENDER, 0);

  if (displayed_room >= 0) {
    
    if (fullRedraw)
      invalidate_screen();

    draw_screen_background();
  }
  else if (!gfxDriver->RequiresFullRedrawEachFrame()) 
  {
    // if the driver is not going to redraw the screen,
    // black it out so we don't get cursor trails
    clear(abuf);
  }

  // reset the Baselines Changed flag now that we've drawn stuff
  walk_behind_baselines_changed = 0;

  // make sure that the mp3 is always playing smoothly
  UPDATE_MP3
  our_eip=4;
  draw_screen_overlay();

  if (fullRedraw)
  {
    // ensure the virtual screen is reconstructed
    // in case we want to take any screenshots before
    // the next game loop
    if (gfxDriver->UsesMemoryBackBuffer())
      gfxDriver->RenderToBackBuffer();
  }
}

// Draw everything 
void render_graphics(IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {

  construct_virtual_screen(false);
  our_eip=5;

  if (extraBitmap != NULL)
    gfxDriver->DrawSprite(extraX, extraY, extraBitmap);

  update_screen();
}

void mainloop(bool checkControls, IDriverDependantBitmap *extraBitmap, int extraX, int extraY) {
  UPDATE_MP3

  int numEventsAtStartOfFunction = numevents;

  if (want_exit) {
    want_exit = 0;
    proper_exit = 1;
    quit("||exit!");
/*#ifdef WINDOWS_VERSION
    // the Quit thread is running now, so exit this main one.
    ExitThread (1);
#endif*/
  }
  ccNotifyScriptStillAlive ();
  our_eip=1;
  timerloop=0;
  if (want_quit) {
    want_quit = 0;
    QuitGame(1);
    }
  if ((in_enters_screen != 0) & (displayed_room == starting_room))
    quit("!A text script run in the Player Enters Screen event caused the\n"
      "screen to be updated. If you need to use Wait(), do so in After Fadein");
  if ((in_enters_screen != 0) && (done_es_error == 0)) {
    debug_log("Wait() was used in Player Enters Screen - use Enters Screen After Fadein instead");
    done_es_error = 1;
  }
  if (no_blocking_functions)
    quit("!A blocking function was called from within a non-blocking event such as " REP_EXEC_ALWAYS_NAME);

  // if we're not fading in, don't count the fadeouts
  if ((play.no_hicolor_fadein) && (game.options[OPT_FADETYPE] == FADE_NORMAL))
    play.screen_is_faded_out = 0;

  our_eip = 1014;
  update_gui_disabled_status();

  our_eip = 1004;
  if (in_new_room == 0) {
    // Run the room and game script repeatedly_execute
    run_function_on_non_blocking_thread(&repExecAlways);
    setevent(EV_TEXTSCRIPT,TS_REPEAT);
    setevent(EV_RUNEVBLOCK,EVB_ROOM,0,6);  
  }
  // run this immediately to make sure it gets done before fade-in
  // (player enters screen)
  check_new_room ();
  
  our_eip = 1005;

  if ((play.ground_level_areas_disabled & GLED_INTERACTION) == 0) {
    // check if he's standing on a hotspot
    int hotspotThere = get_hotspot_at(playerchar->x, playerchar->y);
    // run Stands on Hotspot event
    setevent(EV_RUNEVBLOCK, EVB_HOTSPOT, hotspotThere, 0);

    // check current region
    int onRegion = GetRegionAt (playerchar->x, playerchar->y);
    int inRoom = displayed_room;

    if (onRegion != play.player_on_region) {
      // we need to save this and set play.player_on_region
      // now, so it's correct going into RunRegionInteraction
      int oldRegion = play.player_on_region;
    
      play.player_on_region = onRegion;
      // Walks Off last region
      if (oldRegion > 0)
        RunRegionInteraction (oldRegion, 2);
      // Walks Onto new region
      if (onRegion > 0)
        RunRegionInteraction (onRegion, 1);
    }
    if (play.player_on_region > 0)   // player stands on region
      RunRegionInteraction (play.player_on_region, 0);

    // one of the region interactions sent us to another room
    if (inRoom != displayed_room) {
      check_new_room();
    }

    // if in a Wait loop which is no longer valid (probably
    // because the Region interaction did a NewRoom), abort
    // the rest of the loop
    if ((restrict_until) && (!wait_loop_still_valid())) {
      // cancel the Rep Exec and Stands on Hotspot events that
      // we just added -- otherwise the event queue gets huge
      numevents = numEventsAtStartOfFunction;
      return;
    }
  } // end if checking ground level interactions

  mouse_on_iface=-1;

  check_debug_keys();

  // don't let the player do anything before the screen fades in
  if ((in_new_room == 0) && (checkControls)) {
    int inRoom = displayed_room;
    check_controls();
    // If an inventory interaction changed the room
    if (inRoom != displayed_room)
      check_new_room();
  }
  our_eip=2;
  if (debug_flags & DBG_NOUPDATE) ;
  else if (game_paused==0) update_stuff();

  // update animating GUI buttons
  // this bit isn't in update_stuff because it always needs to
  // happen, even when the game is paused
  for (int aa = 0; aa < numAnimButs; aa++) {
    if (UpdateAnimatingButton(aa)) {
      StopButtonAnimation(aa);
      aa--;
    } 
  }

  update_polled_stuff_and_crossfade();

  if (!play.fast_forward) {
    int mwasatx=mousex,mwasaty=mousey;

    // Only do this if we are not skipping a cutscene
    render_graphics(extraBitmap, extraX, extraY);

    // Check Mouse Moves Over Hotspot event
    static int offsetxWas = -100, offsetyWas = -100;

    if (((mwasatx!=mousex) || (mwasaty!=mousey) ||
        (offsetxWas != offsetx) || (offsetyWas != offsety)) &&
        (displayed_room >= 0)) 
    {
      // mouse moves over hotspot
      if (__GetLocationType(divide_down_coordinate(mousex), divide_down_coordinate(mousey), 1) == LOCTYPE_HOTSPOT) {
        int onhs = getloctype_index;
        
        setevent(EV_RUNEVBLOCK,EVB_HOTSPOT,onhs,6); 
      }
    }

    offsetxWas = offsetx;
    offsetyWas = offsety;

#ifdef MAC_VERSION
    // take a breather after the heavy work
    // cuts down on CPU usage and reduces the fan noise
    rest(2);
#endif
  }
  our_eip=6;
  new_room_was = in_new_room;
  if (in_new_room>0)
    setevent(EV_FADEIN,0,0,0);
  in_new_room=0;
  update_events();
  if ((new_room_was > 0) && (in_new_room == 0)) {
    // if in a new room, and the room wasn't just changed again in update_events,
    // then queue the Enters Screen scripts
    // run these next time round, when it's faded in
    if (new_room_was==2)  // first time enters screen
      setevent(EV_RUNEVBLOCK,EVB_ROOM,0,4);
    if (new_room_was!=3)   // enters screen after fadein
      setevent(EV_RUNEVBLOCK,EVB_ROOM,0,7);
  }
  our_eip=7;
//    if (mgetbutton()>NONE) break;
  update_polled_stuff();
  if (play.bg_anim_delay > 0) play.bg_anim_delay--;
  else if (play.bg_frame_locked) ;
  else {
    play.bg_anim_delay = play.anim_background_speed;
    play.bg_frame++;
    if (play.bg_frame >= thisroom.num_bscenes)
      play.bg_frame=0;
    if (thisroom.num_bscenes >= 2) {
      // get the new frame's palette
      on_background_frame_change();
    }
  }
  loopcounter++;

  if (play.wait_counter > 0) play.wait_counter--;
  if (play.shakesc_length > 0) play.shakesc_length--;

  if (loopcounter % 5 == 0)
  {
    update_ambient_sound_vol();
    update_directional_sound_vol();
  }

  if (replay_start_this_time) {
    replay_start_this_time = 0;
    start_replay_record();
  }

  if (play.fast_forward)
    return;

  our_eip=72;
  if (time(NULL) != t1) {
    t1 = time(NULL);
    fps = loopcounter - lastcounter;
    lastcounter = loopcounter;
  }

  // make sure we poll, cos a low framerate (eg 5 fps) could stutter
  // mp3 music
  while (timerloop == 0) {
    update_polled_stuff();
    platform->YieldCPU();
  }
}


int check_write_access() {

  if (platform->GetDiskFreeSpaceMB() < 2)
    return 0;

  our_eip = -1895;

  // The Save Game Dir is the only place that we should write to
  char tempPath[MAX_PATH];
  sprintf(tempPath, "%s""tmptest.tmp", saveGameDirectory);
  FILE *yy = fopen(tempPath, "wb");
  if (yy == NULL)
    return 0;

  our_eip = -1896;

  fwrite("just to test the drive free space", 30, 1, yy);
  fclose(yy);

  our_eip = -1897;

  if (unlink(tempPath))
    return 0;

  return 1;
}

//char datname[80]="ac.clb";
char ac_conf_file_defname[MAX_PATH] = "acsetup.cfg";
char *ac_config_file = &ac_conf_file_defname[0];
char conffilebuf[512];


int wait_loop_still_valid() {
  if (restrict_until == 0)
    quit("end_wait_loop called but game not in loop_until state");
  int retval = restrict_until;

  if (restrict_until==UNTIL_MOVEEND) {
    short*wkptr=(short*)user_disabled_data;
    if (wkptr[0]<1) retval=0;
  }
  else if (restrict_until==UNTIL_CHARIS0) {
    char*chptr=(char*)user_disabled_data;
    if (chptr[0]==0) retval=0;
  }
  else if (restrict_until==UNTIL_NEGATIVE) {
    short*wkptr=(short*)user_disabled_data;
    if (wkptr[0]<0) retval=0;
  }
  else if (restrict_until==UNTIL_INTISNEG) {
    int*wkptr=(int*)user_disabled_data;
    if (wkptr[0]<0) retval=0;
  }
  else if (restrict_until==UNTIL_NOOVERLAY) {
    if (is_text_overlay < 1) retval=0;
  }
  else if (restrict_until==UNTIL_INTIS0) {
    int*wkptr=(int*)user_disabled_data;
    if (wkptr[0]<0) retval=0;
  }
  else if (restrict_until==UNTIL_SHORTIS0) {
    short*wkptr=(short*)user_disabled_data;
    if (wkptr[0]==0) retval=0;
  }
  else quit("loop_until: unknown until event");

  return retval;
}

int main_game_loop() {
  if (displayed_room < 0)
    quit("!A blocking function was called before the first room has been loaded");

  mainloop(true);
  // Call GetLocationName - it will internally force a GUI refresh
  // if the result it returns has changed from last time
  char tempo[STD_BUFFER_SIZE];
  GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);

  if ((play.get_loc_name_save_cursor >= 0) &&
      (play.get_loc_name_save_cursor != play.get_loc_name_last_time) &&
      (mouse_on_iface < 0) && (ifacepopped < 0)) {
    // we have saved the cursor, but the mouse location has changed
    // and it's time to restore it
    play.get_loc_name_save_cursor = -1;
    set_cursor_mode(play.restore_cursor_mode_to);

    if (cur_mode == play.restore_cursor_mode_to)
    {
      // make sure it changed -- the new mode might have been disabled
      // in which case don't change the image
      set_mouse_cursor(play.restore_cursor_image_to);
    }
    DEBUG_CONSOLE("Restore mouse to mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
  }

  our_eip=76;
  if (restrict_until==0) ;
  else {
    restrict_until = wait_loop_still_valid();
    our_eip = 77;

    if (restrict_until==0) {
      set_default_cursor();
      guis_need_update = 1;
      play.disabled_user_interface--;
/*      if (user_disabled_for==FOR_ANIMATION)
        run_animation((FullAnimation*)user_disabled_data2,user_disabled_data3);
      else*/ if (user_disabled_for==FOR_EXITLOOP) {
        user_disabled_for=0; return -1; }
      else if (user_disabled_for==FOR_SCRIPT) {
        quit("err: for_script obsolete (v2.1 and earlier only)");
      }
      else
        quit("Unknown user_disabled_for in end restrict_until");

      user_disabled_for=0;
    }
  }
  our_eip = 78;
  return 0;
}

void do_main_cycle(int untilwhat,int daaa) {
  // blocking cutscene - end skipping
  EndSkippingUntilCharStops();

  // this function can get called in a nested context, so
  // remember the state of these vars in case a higher level
  // call needs them
  int cached_restrict_until = restrict_until;
  int cached_user_disabled_data = user_disabled_data;
  int cached_user_disabled_for = user_disabled_for;

  main_loop_until(untilwhat,daaa,0);
  user_disabled_for=FOR_EXITLOOP;
  while (main_game_loop()==0) ;

  restrict_until = cached_restrict_until;
  user_disabled_data = cached_user_disabled_data;
  user_disabled_for = cached_user_disabled_for;
}

//char*ac_default_header=NULL,*temphdr=NULL;
char ac_default_header[15000],temphdr[10000];

void setup_exports(char*expfrom) {
  char namof[30]="\0"; temphdr[0]=0;
  while (expfrom[0]!=0) {
    expfrom=strstr(expfrom,"function ");
    if (expfrom==NULL) break;
    if (expfrom[-1]!=10) { expfrom++; continue; }
    expfrom+=9;
    int iid=0;
    while (expfrom[iid]!='(') { namof[iid]=expfrom[iid]; iid++; }
    namof[iid]=0;
    strcat(temphdr,"export ");
    strcat(temphdr,namof);
    strcat(temphdr,";\r\n");
    }
  int aa;
  for (aa=0;aa<game.numcharacters-1;aa++) {
    if (game.chars[aa].scrname[0]==0) continue;
    strcat(temphdr,"#define ");
    strcat(temphdr,game.chars[aa].scrname);
    strcat(temphdr," ");
    char*ptro=&temphdr[strlen(temphdr)];
    sprintf(ptro,"%d\r\n",aa);
    }
  }


void compile_room_script() {
  ccError = 0;

  roominst = ccCreateInstance(thisroom.compiled_script);

  if ((ccError!=0) || (roominst==NULL)) {
   char thiserror[400];
   sprintf(thiserror, "Unable to create local script: %s", ccErrorString);
   quit(thiserror);
  }

  roominstFork = ccForkInstance(roominst);
  if (roominstFork == NULL)
    quitprintf("Unable to create forked room instance: %s", ccErrorString);

  repExecAlways.roomHasFunction = true;
  getDialogOptionsDimensionsFunc.roomHasFunction = true;
}

void get_new_size_for_sprite (int ee, int ww, int hh, int &newwid, int &newhit) {
  newwid = ww * current_screen_resolution_multiplier;
  newhit = hh * current_screen_resolution_multiplier;
  if (game.spriteflags[ee] & SPF_640x400) 
  {
    if (current_screen_resolution_multiplier == 2) {
      newwid = ww;
      newhit = hh;
    }
    else {
      newwid=(ww/2) * current_screen_resolution_multiplier;
      newhit=(hh/2) * current_screen_resolution_multiplier;
      // just make sure - could crash if wid or hit is 0
      if (newwid < 1)
        newwid = 1;
      if (newhit < 1)
        newhit = 1;
    }
  }
}

// set any alpha-transparent pixels in the image to the appropriate
// RGB mask value so that the draw_sprite calls work correctly
void set_rgb_mask_using_alpha_channel(block image)
{
  int x, y;

  for (y=0; y < image->h; y++) 
  {
    unsigned long*psrc = (unsigned long *)image->line[y];

    for (x=0; x < image->w; x++) 
    {
      if ((psrc[x] & 0xff000000) == 0x00000000)
        psrc[x] = MASK_COLOR_32;
		}
  }
}

// from is a 32-bit RGBA image, to is a 15/16/24-bit destination image
block remove_alpha_channel(block from) {
  int depth = final_col_dep;

  block to = create_bitmap_ex(depth, from->w, from->h);
  int maskcol = bitmap_mask_color(to);
  int y,x;
  unsigned long c,b,g,r;

  if (depth == 24) {
    // 32-to-24
    for (y=0; y < from->h; y++) {
      unsigned long*psrc = (unsigned long *)from->line[y];
      unsigned char*pdest = (unsigned char*)to->line[y];

      for (x=0; x < from->w; x++) {
			  c = psrc[x];
        // less than 50% opaque, remove the pixel
        if (((c >> 24) & 0x00ff) < 128)
          c = maskcol;

        // copy the RGB values across
        memcpy(&pdest[x * 3], &c, 3);
		  }
    }
  }
  else {  // 32 to 15 or 16

    for (y=0; y < from->h; y++) {
      unsigned long*psrc = (unsigned long *)from->line[y];
      unsigned short*pdest = (unsigned short *)to->line[y];

      for (x=0; x < from->w; x++) {
			  c = psrc[x];
        // less than 50% opaque, remove the pixel
        if (((c >> 24) & 0x00ff) < 128)
          pdest[x] = maskcol;
        else {
          // otherwise, copy it across
			    r = (c >> 16) & 0x00ff;
          g = (c >> 8) & 0x00ff;
          b = c & 0x00ff;
			    pdest[x] = makecol_depth(depth, r, g, b);
        }
		  }
    }
  }

  return to;
}

void pre_save_sprite(int ee) {
  // not used, we don't save
}

// these vars are global to help with debugging
block tmpdbl, curspr;
int newwid, newhit;
void initialize_sprite (int ee) {

  if ((ee < 0) || (ee > spriteset.elements))
    quit("initialize_sprite: invalid sprite number");

  if ((spriteset[ee] == NULL) && (ee > 0)) {
    // replace empty sprites with blue cups, to avoid crashes
    //spriteset[ee] = spriteset[0];
    spriteset.set (ee, spriteset[0]);
    spritewidth[ee] = spritewidth[0];
    spriteheight[ee] = spriteheight[0];
  }
  else if (spriteset[ee]==NULL) {
    spritewidth[ee]=0;
    spriteheight[ee]=0;
  }
  else {
    // stretch sprites to correct resolution
    int oldeip = our_eip;
    our_eip = 4300;

    if (game.spriteflags[ee] & SPF_HADALPHACHANNEL) {
      // we stripped the alpha channel out last time, put
      // it back so that we can remove it properly again
      game.spriteflags[ee] |= SPF_ALPHACHANNEL;
    }

    curspr = spriteset[ee];
    get_new_size_for_sprite (ee, curspr->w, curspr->h, newwid, newhit);

    eip_guinum = ee;
    eip_guiobj = newwid;

    if ((newwid != curspr->w) || (newhit != curspr->h)) {
      tmpdbl = create_bitmap_ex(bitmap_color_depth(curspr),newwid,newhit);
      if (tmpdbl == NULL)
        quit("Not enough memory to load sprite graphics");
      acquire_bitmap (tmpdbl);
      acquire_bitmap (curspr);
      clear_to_color(tmpdbl,bitmap_mask_color(tmpdbl));
/*#ifdef USE_CUSTOM_EXCEPTION_HANDLER
      __try {
#endif*/
        stretch_sprite(tmpdbl,curspr,0,0,tmpdbl->w,tmpdbl->h);
/*#ifdef USE_CUSTOM_EXCEPTION_HANDLER
      } __except (1) {
        // I can't trace this fault, but occasionally stretch_sprite
        // crashes, even with valid source and dest bitmaps. So,
        // for now, just ignore the exception, since the stretch
        // looks successful
      //MessageBox (allegro_wnd, "ERROR", "FATAL ERROR", MB_OK);
      }
#endif*/
      release_bitmap (curspr);
      release_bitmap (tmpdbl);
      wfreeblock(curspr);
      spriteset.set (ee, tmpdbl);
    }

    spritewidth[ee]=wgetblockwidth(spriteset[ee]);
    spriteheight[ee]=wgetblockheight(spriteset[ee]);

    int spcoldep = bitmap_color_depth(spriteset[ee]);

    if (((spcoldep > 16) && (final_col_dep <= 16)) ||
        ((spcoldep == 16) && (final_col_dep > 16))) {
      // 16-bit sprite in 32-bit game or vice versa - convert
      // so that scaling and draw_sprite calls work properly
      block oldSprite = spriteset[ee];
      block newSprite;
      
      if (game.spriteflags[ee] & SPF_ALPHACHANNEL)
        newSprite = remove_alpha_channel(oldSprite);
      else {
        newSprite = create_bitmap_ex(final_col_dep, spritewidth[ee], spriteheight[ee]);
        blit(oldSprite, newSprite, 0, 0, 0, 0, spritewidth[ee], spriteheight[ee]);
      }
      spriteset.set(ee, newSprite);
      destroy_bitmap(oldSprite);
      spcoldep = final_col_dep;
    }
    // PSP: Convert to BGR color order.
    else if ((spcoldep == 32) && (final_col_dep == 32))
    {
      spriteset.set(ee, convert_32_to_32bgr(spriteset[ee]));

      if ((game.spriteflags[ee] & SPF_ALPHACHANNEL) != 0)
      {
        set_rgb_mask_using_alpha_channel(spriteset[ee]);
      }
    }

#ifdef USE_15BIT_FIX
    else if ((final_col_dep != game.color_depth*8) && (spcoldep == game.color_depth*8)) {
      // running in 15-bit mode with a 16-bit game, convert sprites
      block oldsprite = spriteset[ee];

      if (game.spriteflags[ee] & SPF_ALPHACHANNEL)
        // 32-to-24 with alpha channel
        spriteset.set (ee, remove_alpha_channel(oldsprite));
      else
        spriteset.set (ee, convert_16_to_15(oldsprite));

      destroy_bitmap(oldsprite);
    }
    if ((convert_16bit_bgr == 1) && (bitmap_color_depth(spriteset[ee]) == 16))
      spriteset.set (ee, convert_16_to_16bgr (spriteset[ee]));
#endif

    if ((spcoldep == 8) && (final_col_dep > 8))
      select_palette(palette);

    spriteset.set(ee, gfxDriver->ConvertBitmapToSupportedColourDepth(spriteset[ee]));

    if ((spcoldep == 8) && (final_col_dep > 8))
      unselect_palette();

    if (final_col_dep < 32) {
      game.spriteflags[ee] &= ~SPF_ALPHACHANNEL;
      // save the fact that it had one for the next time this
      // is re-loaded from disk
      game.spriteflags[ee] |= SPF_HADALPHACHANNEL;
    }

    platform->RunPluginHooks(AGSE_SPRITELOAD, ee);
    update_polled_stuff();

    our_eip = oldeip;
  }
}

#ifdef WINDOWS_VERSION
CONTEXT cpustate;
EXCEPTION_RECORD excinfo;
int miniDumpResultCode = 0;

typedef enum _MINIDUMP_TYPE {
    MiniDumpNormal                         = 0x0000,
    MiniDumpWithDataSegs                   = 0x0001,
    MiniDumpWithFullMemory                 = 0x0002,
    MiniDumpWithHandleData                 = 0x0004,
    MiniDumpFilterMemory                   = 0x0008,
    MiniDumpScanMemory                     = 0x0010,
    MiniDumpWithUnloadedModules            = 0x0020,
    MiniDumpWithIndirectlyReferencedMemory = 0x0040,
    MiniDumpFilterModulePaths              = 0x0080,
    MiniDumpWithProcessThreadData          = 0x0100,
    MiniDumpWithPrivateReadWriteMemory     = 0x0200,
    MiniDumpWithoutOptionalData            = 0x0400,
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef BOOL (WINAPI * MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, 
  HANDLE hFile, MINIDUMP_TYPE DumpType, 
  CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
  CONST void* UserStreamParam, 
  CONST void* CallbackParam); 

MINIDUMPWRITEDUMP _MiniDumpWriteDump;


void CreateMiniDump( EXCEPTION_POINTERS* pep ) 
{
  HMODULE dllHandle = LoadLibrary(L"dbghelp.dll");
  if (dllHandle == NULL)
  {
    miniDumpResultCode = 1;
    return;
  }

  _MiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(dllHandle, "MiniDumpWriteDump");
  if (_MiniDumpWriteDump == NULL)
  {
    FreeLibrary(dllHandle);
    miniDumpResultCode = 2;
    return;
  }

  char fileName[80];
  sprintf(fileName, "CrashInfo.%s.dmp", ACI_VERSION_TEXT);
  HANDLE hFile = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

  if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
  {
    MINIDUMP_EXCEPTION_INFORMATION mdei; 

    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = pep;
    mdei.ClientPointers = FALSE;

    MINIDUMP_TYPE mdt = MiniDumpNormal; //MiniDumpWithPrivateReadWriteMemory;

    BOOL rv = _MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
      hFile, mdt, (pep != 0) ? &mdei : 0, NULL, NULL); 

    if (!rv)
      miniDumpResultCode = 4;

    CloseHandle(hFile); 
  }
  else
    miniDumpResultCode = 3;

  FreeLibrary(dllHandle);
}

int CustomExceptionHandler (LPEXCEPTION_POINTERS exinfo) {
  cpustate = exinfo->ContextRecord[0];
  excinfo = exinfo->ExceptionRecord[0];
  CreateMiniDump(exinfo);
  
  return EXCEPTION_EXECUTE_HANDLER;
}

FILE *logfile;
int OurReportingFunction( int reportType, char *userMessage, int *retVal ) {

  fprintf(logfile,"%s: %s\n",(reportType == _CRT_ASSERT) ? "Assertion failed" : "Warning",userMessage);
  fflush (logfile);
  return 0;
}
#endif

#if defined(WINDOWS_VERSION)
#include <new.h>
char tempmsg[100];
char*printfworkingspace;
int malloc_fail_handler(size_t amountwanted) {
  free(printfworkingspace);
  sprintf(tempmsg,"Out of memory: failed to allocate %ld bytes (at PP=%d)",amountwanted, our_eip);
  quit(tempmsg);
  return 0;
}
#endif

int init_gfx_mode(int wid,int hit,int cdep) {

  // a mode has already been initialized, so abort
  if (working_gfx_mode_status == 0) return 0;

  if (debug_15bit_mode)
    cdep = 15;
  else if (debug_24bit_mode)
    cdep = 24;

  platform->WriteDebugString("Attempt to switch gfx mode to %d x %d (%d-bit)", wid, hit, cdep);

  if (usetup.refresh >= 50)
    request_refresh_rate(usetup.refresh);

  final_scrn_wid = wid;
  final_scrn_hit = hit;
  final_col_dep = cdep;

  if (game.color_depth == 1) {
    final_col_dep = 8;
  }
  else {
    set_color_depth(cdep);
  }

  working_gfx_mode_status = (gfxDriver->Init(wid, hit, final_col_dep, usetup.windowed > 0, &timerloop) ? 0 : -1);

  if (working_gfx_mode_status == 0) 
    platform->WriteDebugString("Succeeded. Using gfx mode %d x %d (%d-bit)", wid, hit, final_col_dep);
  else
    platform->WriteDebugString("Failed, resolution not supported");

  if ((working_gfx_mode_status < 0) && (usetup.windowed > 0) && (editor_debugging_enabled == 0)) {
    usetup.windowed ++;
    if (usetup.windowed > 2) usetup.windowed = 0;
    return init_gfx_mode(wid,hit,cdep);
  }
  return working_gfx_mode_status;    
}

void winclosehook() {
  want_exit = 1;
  abort_engine = 1;
  check_dynamic_sprites_at_exit = 0;
/*  while (want_exit == 1)
    yield_timeslice();
  / *if (want_quit == 0)
    want_quit = 1;
  else* / quit("|game aborted");
*/
}

void init_game_settings() {
  int ee;

  for (ee=0;ee<256;ee++) {
    if (game.paluses[ee]!=PAL_BACKGROUND)
      palette[ee]=game.defpal[ee];
  }

  if (game.options[OPT_NOSCALEFNT]) wtext_multiply=1;

  for (ee = 0; ee < game.numcursors; ee++) 
  {
    // The cursor graphics are assigned to mousecurs[] and so cannot
    // be removed from memory
    if (game.mcurs[ee].pic >= 0)
      spriteset.precache (game.mcurs[ee].pic);

    // just in case they typed an invalid view number in the editor
    if (game.mcurs[ee].view >= game.numviews)
      game.mcurs[ee].view = -1;

    if (game.mcurs[ee].view >= 0)
      precache_view (game.mcurs[ee].view);
  }
  // may as well preload the character gfx
  if (playerchar->view >= 0)
    precache_view (playerchar->view);

  for (ee = 0; ee < MAX_INIT_SPR; ee++)
    objcache[ee].image = NULL;

/*  dummygui.guiId = -1;
  dummyguicontrol.guin = -1;
  dummyguicontrol.objn = -1;*/

  our_eip=-6;
//  game.chars[0].talkview=4;
  //init_language_text(game.langcodes[0]);

  for (ee = 0; ee < MAX_INIT_SPR; ee++) {
    scrObj[ee].id = ee;
    scrObj[ee].obj = NULL;
  }

  for (ee=0;ee<game.numcharacters;ee++) {
    memset(&game.chars[ee].inv[0],0,MAX_INV*sizeof(short));
    game.chars[ee].activeinv=-1;
    game.chars[ee].following=-1;
    game.chars[ee].followinfo=97 | (10 << 8);
    game.chars[ee].idletime=20;  // can be overridden later with SetIdle or summink
    game.chars[ee].idleleft=game.chars[ee].idletime;
    game.chars[ee].transparency = 0;
    game.chars[ee].baseline = -1;
    game.chars[ee].walkwaitcounter = 0;
    game.chars[ee].z = 0;
    charextra[ee].xwas = INVALID_X;
    charextra[ee].zoom = 100;
    if (game.chars[ee].view >= 0) {
      // set initial loop to 0
      game.chars[ee].loop = 0;
      // or to 1 if they don't have up/down frames
      if (views[game.chars[ee].view].loops[0].numFrames < 1)
        game.chars[ee].loop = 1;
    }
    charextra[ee].process_idle_this_time = 0;
    charextra[ee].invorder_count = 0;
    charextra[ee].slow_move_counter = 0;
    charextra[ee].animwait = 0;
  }
  // multiply up gui positions
  guibg = (block*)malloc(sizeof(block) * game.numgui);
  guibgbmp = (IDriverDependantBitmap**)malloc(sizeof(IDriverDependantBitmap*) * game.numgui);
  for (ee=0;ee<game.numgui;ee++) {
    guibgbmp[ee] = NULL;
    GUIMain*cgp=&guis[ee];
    guibg[ee] = create_bitmap_ex (final_col_dep, cgp->wid, cgp->hit);
    guibg[ee] = gfxDriver->ConvertBitmapToSupportedColourDepth(guibg[ee]);
  }

  our_eip=-5;
  for (ee=0;ee<game.numinvitems;ee++) {
    if (game.invinfo[ee].flags & IFLG_STARTWITH) playerchar->inv[ee]=1;
    else playerchar->inv[ee]=0;
  }
  play.score=0;
  play.sierra_inv_color=7;
  play.talkanim_speed = 5;
  play.inv_item_wid = 40;
  play.inv_item_hit = 22;
  play.messagetime=-1;
  play.disabled_user_interface=0;
  play.gscript_timer=-1;
  play.debug_mode=game.options[OPT_DEBUGMODE];
  play.inv_top=0;
  play.inv_numdisp=0;
  play.obsolete_inv_numorder=0;
  play.text_speed=15;
  play.text_min_display_time_ms = 1000;
  play.ignore_user_input_after_text_timeout_ms = 500;
  play.ignore_user_input_until_time = 0;
  play.lipsync_speed = 15;
  play.close_mouth_speech_time = 10;
  play.disable_antialiasing = 0;
  play.rtint_level = 0;
  play.rtint_light = 255;
  play.text_speed_modifier = 0;
  play.text_align = SCALIGN_LEFT;
  // Make the default alignment to the right with right-to-left text
  if (game.options[OPT_RIGHTLEFTWRITE])
    play.text_align = SCALIGN_RIGHT;

  play.speech_bubble_width = get_fixed_pixel_size(100);
  play.bg_frame=0;
  play.bg_frame_locked=0;
  play.bg_anim_delay=0;
  play.anim_background_speed = 0;
  play.silent_midi = 0;
  play.current_music_repeating = 0;
  play.skip_until_char_stops = -1;
  play.get_loc_name_last_time = -1;
  play.get_loc_name_save_cursor = -1;
  play.restore_cursor_mode_to = -1;
  play.restore_cursor_image_to = -1;
  play.ground_level_areas_disabled = 0;
  play.next_screen_transition = -1;
  play.temporarily_turned_off_character = -1;
  play.inv_backwards_compatibility = 0;
  play.gamma_adjustment = 100;
  play.num_do_once_tokens = 0;
  play.do_once_tokens = NULL;
  play.music_queue_size = 0;
  play.shakesc_length = 0;
  play.wait_counter=0;
  play.key_skip_wait = 0;
  play.cur_music_number=-1;
  play.music_repeat=1;
  play.music_master_volume=160;
  play.digital_master_volume = 100;
  play.screen_flipped=0;
  play.offsets_locked=0;
  play.cant_skip_speech = user_to_internal_skip_speech(game.options[OPT_NOSKIPTEXT]);
  play.sound_volume = 255;
  play.speech_volume = 255;
  play.normal_font = 0;
  play.speech_font = 1;
  play.speech_text_shadow = 16;
  play.screen_tint = -1;
  play.bad_parsed_word[0] = 0;
  play.swap_portrait_side = 0;
  play.swap_portrait_lastchar = -1;
  play.in_conversation = 0;
  play.skip_display = 3;
  play.no_multiloop_repeat = 0;
  play.in_cutscene = 0;
  play.fast_forward = 0;
  play.totalscore = game.totalscore;
  play.roomscript_finished = 0;
  play.no_textbg_when_voice = 0;
  play.max_dialogoption_width = get_fixed_pixel_size(180);
  play.no_hicolor_fadein = 0;
  play.bgspeech_game_speed = 0;
  play.bgspeech_stay_on_display = 0;
  play.unfactor_speech_from_textlength = 0;
  play.mp3_loop_before_end = 70;
  play.speech_music_drop = 60;
  play.room_changes = 0;
  play.check_interaction_only = 0;
  play.replay_hotkey = 318;  // Alt+R
  play.dialog_options_x = 0;
  play.dialog_options_y = 0;
  play.min_dialogoption_width = 0;
  play.disable_dialog_parser = 0;
  play.ambient_sounds_persist = 0;
  play.screen_is_faded_out = 0;
  play.player_on_region = 0;
  play.top_bar_backcolor = 8;
  play.top_bar_textcolor = 16;
  play.top_bar_bordercolor = 8;
  play.top_bar_borderwidth = 1;
  play.top_bar_ypos = 25;
  play.top_bar_font = -1;
  play.screenshot_width = 160;
  play.screenshot_height = 100;
  play.speech_text_align = SCALIGN_CENTRE;
  play.auto_use_walkto_points = 1;
  play.inventory_greys_out = 0;
  play.skip_speech_specific_key = 0;
  play.abort_key = 324;  // Alt+X
  play.fade_to_red = 0;
  play.fade_to_green = 0;
  play.fade_to_blue = 0;
  play.show_single_dialog_option = 0;
  play.keep_screen_during_instant_transition = 0;
  play.read_dialog_option_colour = -1;
  play.narrator_speech = game.playercharacter;
  play.crossfading_out_channel = 0;
  play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
  if (play.speech_textwindow_gui == 0)
    play.speech_textwindow_gui = -1;
  strcpy(play.game_name, game.gamename);
  play.lastParserEntry[0] = 0;
  play.follow_change_room_timer = 150;
  for (ee = 0; ee < MAX_BSCENE; ee++) 
    play.raw_modified[ee] = 0;
  play.game_speed_modifier = 0;
  if (debug_flags & DBG_DEBUGMODE)
    play.debug_mode = 1;
  gui_disabled_style = convert_gui_disabled_style(game.options[OPT_DISABLEOFF]);

  memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);
  memset(&play.script_timers[0],0,MAX_TIMERS * sizeof(int));
  memset(&play.default_audio_type_volumes[0], -1, MAX_AUDIO_TYPES * sizeof(int));

  // reset graphical script vars (they're still used by some games)
  for (ee = 0; ee < MAXGLOBALVARS; ee++) 
    play.globalvars[ee] = 0;

  for (ee = 0; ee < MAXGLOBALSTRINGS; ee++)
    play.globalstrings[ee][0] = 0;

  for (ee = 0; ee < MAX_SOUND_CHANNELS; ee++)
    last_sound_played[ee] = -1;

  if (usetup.translation)
    init_translation (usetup.translation);

  update_invorder();
  displayed_room = -10;
}

char filetouse[MAX_PATH] = "nofile";

// Replace the filename part of complete path WASGV with INIFIL
void INIgetdirec(char *wasgv, char *inifil) {
  int u = strlen(wasgv) - 1;

  for (u = strlen(wasgv) - 1; u >= 0; u--) {
    if ((wasgv[u] == '\\') || (wasgv[u] == '/')) {
      memcpy(&wasgv[u + 1], inifil, strlen(inifil) + 1);
      break;
    }
  }

  if (u <= 0) {
    // no slashes - either the path is just "f:acwin.exe"
    if (strchr(wasgv, ':') != NULL)
      memcpy(strchr(wasgv, ':') + 1, inifil, strlen(inifil) + 1);
    // or it's just "acwin.exe" (unlikely)
    else
      strcpy(wasgv, inifil);
  }

}

char *INIreaditem(const char *sectn, const char *entry) {
  FILE *fin = fopen(filetouse, "rt");
  if (fin == NULL)
    return NULL;

  char templine[200];
  char wantsect[100];
  sprintf (wantsect, "[%s]", sectn);

  while (!feof(fin)) {
    fgets (templine, 199, fin);
    // find the section
    if (strnicmp (wantsect, templine, strlen(wantsect)) == 0) {
      while (!feof(fin)) {
        // we're in the right section, find the entry
        fgets (templine, 199, fin);
        if (templine[0] == '[')
          break;
        if (feof(fin))
          break;
        // Strip CRLF
        char *lastchar = &templine[strlen(templine) -1];
        while(*lastchar == '\r' || *lastchar == '\n') {
          *lastchar = 0;
          lastchar--;
        }
        // Have we found the entry?
        if (strnicmp (templine, entry, strlen(entry)) == 0) {
          char *pptr = &templine[strlen(entry)];
          while ((pptr[0] == ' ') || (pptr[0] == '\t'))
            pptr++;
          if (pptr[0] == '=') {
            pptr++;
            while ((pptr[0] == ' ') || (pptr[0] == '\t'))
              pptr++;
            char *toret = (char*)malloc (strlen(pptr) + 5);
            strcpy (toret, pptr);
            fclose (fin);
            return toret;
          }
        }
      }
    }
  }
  fclose (fin);
  return NULL;
}

int INIreadint (const char *sectn, const char *item, int errornosect = 1) {
  char *tempstr = INIreaditem (sectn, item);
  if (tempstr == NULL)
    return -1;

  int toret = atoi(tempstr);
  free (tempstr);
  return toret;
}


void read_config_file(char *argv0) {

  // Try current directory for config first; else try exe dir
  strcpy (ac_conf_file_defname, "acsetup.cfg");
  ac_config_file = &ac_conf_file_defname[0];
  FILE *ppp = fopen(ac_config_file, "rb");
  if (ppp == NULL) {

    strcpy(conffilebuf,argv0);
    
/*    for (int ee=0;ee<(int)strlen(conffilebuf);ee++) {
      if (conffilebuf[ee]=='/') conffilebuf[ee]='\\';
    }*/
    fix_filename_case(conffilebuf);
    fix_filename_slashes(conffilebuf);
    
    INIgetdirec(conffilebuf,ac_config_file);
//    printf("Using config: '%s'\n",conffilebuf);
    ac_config_file=&conffilebuf[0];
  }
  else {
    fclose(ppp);
    // put the full path, or it gets written back to the Windows folder
    _getcwd (ac_config_file, 255);
    strcat (ac_config_file, "\\acsetup.cfg");
    fix_filename_case(ac_config_file);
    fix_filename_slashes(ac_config_file);
  }

  // set default dir if no config file
  usetup.data_files_dir = ".";
  usetup.translation = NULL;
  usetup.main_data_filename = "ac2game.dat";
#ifdef WINDOWS_VERSION
  usetup.digicard = DIGI_DIRECTAMX(0);
#endif

  ppp=fopen(ac_config_file,"rt");
  if (ppp!=NULL) {
    strcpy(filetouse,ac_config_file);
    fclose(ppp);
#ifndef WINDOWS_VERSION
    usetup.digicard=INIreadint("sound","digiid");
    usetup.midicard=INIreadint("sound","midiid");
#else
    int idx = INIreadint("sound","digiwinindx", 0);
    if (idx == 0)
      idx = DIGI_DIRECTAMX(0);
    else if (idx == 1)
      idx = DIGI_WAVOUTID(0);
    else if (idx == 2)
      idx = DIGI_NONE;
    else if (idx == 3) 
      idx = DIGI_DIRECTX(0);
    else 
      idx = DIGI_AUTODETECT;
    usetup.digicard = idx;

    idx = INIreadint("sound","midiwinindx", 0);
    if (idx == 1)
      idx = MIDI_NONE;
    else if (idx == 2)
      idx = MIDI_WIN32MAPPER;
    else
      idx = MIDI_AUTODETECT;
    usetup.midicard = idx;

    if (usetup.digicard < 0)
      usetup.digicard = DIGI_AUTODETECT;
    if (usetup.midicard < 0)
      usetup.midicard = MIDI_AUTODETECT;
#endif

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
    usetup.windowed = INIreadint("misc","windowed");
    if (usetup.windowed < 0)
      usetup.windowed = 0;
#endif

    usetup.refresh = INIreadint ("misc", "refresh", 0);
    usetup.enable_antialiasing = INIreadint ("misc", "antialias", 0);
    usetup.force_hicolor_mode = INIreadint("misc", "notruecolor", 0);
    usetup.enable_side_borders = INIreadint("misc", "sideborders", 0);
    force_letterbox = INIreadint ("misc", "forceletterbox", 0);

    if (usetup.enable_antialiasing < 0)
      usetup.enable_antialiasing = 0;
    if (usetup.force_hicolor_mode < 0)
      usetup.force_hicolor_mode = 0;
    if (usetup.enable_side_borders < 0)
      usetup.enable_side_borders = 1;

    // This option is backwards (usevox is 0 if no_speech_pack)
    usetup.no_speech_pack = INIreadint ("sound", "usespeech", 0);
    if (usetup.no_speech_pack == 0)
      usetup.no_speech_pack = 1;
    else
      usetup.no_speech_pack = 0;

    usetup.data_files_dir = INIreaditem("misc","datadir");
    if (usetup.data_files_dir == NULL)
      usetup.data_files_dir = ".";
    // strip any trailing slash
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
    if (usetup.data_files_dir[strlen(usetup.data_files_dir)-1] == '/')
      usetup.data_files_dir[strlen(usetup.data_files_dir)-1] = 0;
#else
    if ((strlen(usetup.data_files_dir) < 4) && (usetup.data_files_dir[1] == ':'))
    { }  // if the path is just  d:\  don't strip the slash
    else if (usetup.data_files_dir[strlen(usetup.data_files_dir)-1] == '\\')
      usetup.data_files_dir[strlen(usetup.data_files_dir)-1] = 0;
#endif

    usetup.main_data_filename = INIreaditem ("misc", "datafile");
    if (usetup.main_data_filename == NULL)
      usetup.main_data_filename = "ac2game.dat";

    usetup.gfxFilterID = INIreaditem("misc", "gfxfilter");

    usetup.gfxDriverID = INIreaditem("misc", "gfxdriver");

    usetup.translation = INIreaditem ("language", "translation");
    int tempint = INIreadint ("misc", "cachemax");
    if (tempint > 0)
      spriteset.maxCacheSize = tempint * 1024;

    char *repfile = INIreaditem ("misc", "replay");
    if (repfile != NULL) {
      strcpy (replayfile, repfile);
      free (repfile);
      play.playback = 1;
    }
    else
      play.playback = 0;

  }

  if (usetup.gfxDriverID == NULL)
    usetup.gfxDriverID = "DX5";

}

void start_game() {
  set_cursor_mode(MODE_WALK);
  filter->SetMousePosition(160,100);
  newmusic(0);

  our_eip = -42;

  for (int kk = 0; kk < numScriptModules; kk++)
    run_text_script(moduleInst[kk], "game_start");

  run_text_script(gameinst,"game_start");

  our_eip = -43;

  SetRestartPoint();

  our_eip=-3;

  if (displayed_room < 0) {
    current_fade_out_effect();
    load_new_room(playerchar->room,playerchar);
    // load_new_room updates it, but it should be -1 in the first room
    playerchar->prevroom = -1;
  }

  first_room_initialization();
}

void initialize_start_and_play_game(int override_start_room, const char *loadSaveGameOnStartup)
{
  try { // BEGIN try for ALI3DEXception
  
  set_cursor_mode (MODE_WALK);

  if (convert_16bit_bgr) {
    // Disable text as speech while displaying the warning message
    // This happens if the user's graphics card does BGR order 16-bit colour
    int oldalways = game.options[OPT_ALWAYSSPCH];
    game.options[OPT_ALWAYSSPCH] = 0;
    // PSP: This is normal. Don't show a warning.
    //Display ("WARNING: AGS has detected that you have an incompatible graphics card for this game. You may experience colour problems during the game. Try running the game with \"--15bit\" command line parameter and see if that helps.[[Click the mouse to continue.");
    game.options[OPT_ALWAYSSPCH] = oldalways;
  }

  srand (play.randseed);
  play.gamestep = 0;
  if (override_start_room)
    playerchar->room = override_start_room;

  write_log_debug("Checking replay status");

  if (play.recording) {
    start_recording();
  }
  else if (play.playback) {
    FILE *in = fopen(replayfile, "rb");
    if (in != NULL) {
      char buffer [100];
      fread (buffer, 12, 1, in);
      buffer[12] = 0;
      if (strcmp (buffer, "AGSRecording") != 0) {
        Display("ERROR: Invalid recorded data file");
        play.playback = 0;
      }
      else {
        fgetstring_limit (buffer, in, 12);
        if (buffer[0] != '2') 
          quit("!Replay file is from an old version of AGS");
        if (strcmp (buffer, "2.55.553") < 0)
          quit("!Replay file was recorded with an older incompatible version");

        if (strcmp (buffer, ACI_VERSION_TEXT)) {
          // Disable text as speech while displaying the warning message
          // This happens if the user's graphics card does BGR order 16-bit colour
          int oldalways = game.options[OPT_ALWAYSSPCH];
          game.options[OPT_ALWAYSSPCH] = 0;
          play.playback = 0;
          Display("Warning! replay is from a different version of AGS (%s) - it may not work properly.", buffer);
          play.playback = 1;
          srand (play.randseed);
          play.gamestep = 0;
          game.options[OPT_ALWAYSSPCH] = oldalways;
        }

        int replayver = getw(in);

        if ((replayver < 1) || (replayver > 3))
          quit("!Unsupported Replay file version");

        if (replayver >= 2) {
          fgetstring_limit (buffer, in, 99);
          int uid = getw (in);
          if ((strcmp (buffer, game.gamename) != 0) || (uid != game.uniqueid)) {
            char msg[150];
            sprintf (msg, "!This replay is meant for the game '%s' and will not work correctly with this game.", buffer);
            quit (msg);
          }
          // skip the total time
          getw (in);
          // replay description, maybe we'll use this later
          fgetstring_limit (buffer, in, 99);
        }

        play.randseed = getw(in);
        int flen = filelength(fileno(in)) - ftell (in);
        if (replayver >= 3) {
          flen = getw(in) * sizeof(short);
        }
        recordbuffer = (short*)malloc (flen);
        fread (recordbuffer, flen, 1, in);
        srand (play.randseed);
        recbuffersize = flen / sizeof(short);
        recsize = 0;
        disable_mgetgraphpos = 1;
        replay_time = 0;
        replay_last_second = loopcounter;
        if (replayver >= 3) {
          int issave = getw(in);
          if (issave) {
            if (restore_game_data (in, replayfile))
              quit("!Error running replay... could be incorrect game version");
            replay_last_second = loopcounter;
          }
        }
        fclose (in);
      }
    }
    else // file not found
      play.playback = 0;
  }

  write_log_debug("Engine initialization complete");
  write_log_debug("Starting game");
  
  if (editor_debugging_enabled)
  {
    SetMultitasking(1);
    if (init_editor_debugging())
    {
      timerloop = 0;
      while (timerloop < 20)
      {
        // pick up any breakpoints in game_start
        check_for_messages_from_editor();
      }

      ccSetDebugHook(scriptDebugHook);
    }
  }

  if (loadSaveGameOnStartup != NULL)
  {
    int saveGameNumber = 1000;
    const char *sgName = strstr(loadSaveGameOnStartup, "agssave.");
    if (sgName != NULL)
    {
      sscanf(sgName, "agssave.%03d", &saveGameNumber);
    }
    current_fade_out_effect();
    int loadGameErrorCode = do_game_load(loadSaveGameOnStartup, saveGameNumber, NULL, NULL);
    if (loadGameErrorCode)
    {
      quitprintf("Unable to resume the save game. Try starting the game over. (Error: %s)", load_game_errors[-loadGameErrorCode]);
    }
  }

  // only start if replay playback hasn't loaded a game
  if (displayed_room < 0)
    start_game();

  while (!abort_engine) {
    main_game_loop();

    if (load_new_game) {
      RunAGSGame (NULL, load_new_game, 0);
      load_new_game = 0;
    }
  }

  } catch (Ali3DException gfxException)
  {
    quit((char*)gfxException._message);
  }

}

int initialize_graphics_filter(const char *filterID, int width, int height, int colDepth)
{
  int idx = 0;
  GFXFilter **filterList;

  if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
  {
    filterList = get_d3d_gfx_filter_list(false);
  }
  else
  {
    filterList = get_allegro_gfx_filter_list(false);
  }
  
  // by default, select No Filter
  filter = filterList[0];

  GFXFilter *thisFilter = filterList[idx];
  while (thisFilter != NULL) {

    if ((filterID != NULL) &&
        (strcmp(thisFilter->GetFilterID(), filterID) == 0))
      filter = thisFilter;
    else if (idx > 0)
      delete thisFilter;

    idx++;
    thisFilter = filterList[idx];
  }

  const char *filterError = filter->Initialize(width, height, colDepth);
  if (filterError != NULL) {
    proper_exit = 1;
    platform->DisplayAlert("Unable to initialize the graphics filter. It returned the following error:\n'%s'\n\nTry running Setup and selecting a different graphics filter.", filterError);
    return -1;
  }

  return 0;
}

int try_widescreen_bordered_graphics_mode_if_appropriate(int initasx, int initasy, int firstDepth)
{
  if (working_gfx_mode_status == 0) return 0;
  if (usetup.enable_side_borders == 0)
  {
    platform->WriteDebugString("Widescreen side borders: disabled in Setup");
    return 1;
  }
  if (usetup.windowed > 0)
  {
    platform->WriteDebugString("Widescreen side borders: disabled (windowed mode)");
    return 1;
  }

  int failed = 1;
  int desktopWidth, desktopHeight;
  if (get_desktop_resolution(&desktopWidth, &desktopHeight) == 0)
  {
    int gameHeight = initasy;

    int screenRatio = (desktopWidth * 1000) / desktopHeight;
    int gameRatio = (initasx * 1000) / gameHeight;
    // 1250 = 1280x1024 
    // 1333 = 640x480, 800x600, 1024x768, 1152x864, 1280x960
    // 1600 = 640x400, 960x600, 1280x800, 1680x1050
    // 1666 = 1280x768

    platform->WriteDebugString("Widescreen side borders: game resolution: %d x %d; desktop resolution: %d x %d", initasx, gameHeight, desktopWidth, desktopHeight);

    if ((screenRatio > 1500) && (gameRatio < 1500))
    {
      int tryWidth = (initasx * screenRatio) / gameRatio;
      int supportedRes = gfxDriver->FindSupportedResolutionWidth(tryWidth, gameHeight, firstDepth, 110);
      if (supportedRes > 0)
      {
        tryWidth = supportedRes;
        platform->WriteDebugString("Widescreen side borders: enabled, attempting resolution %d x %d", tryWidth, gameHeight);
      }
      else
      {
        platform->WriteDebugString("Widescreen side borders: gfx card does not support suitable resolution. will attempt %d x %d anyway", tryWidth, gameHeight);
      }
      failed = init_gfx_mode(tryWidth, gameHeight, firstDepth);
    }
    else
    {
      platform->WriteDebugString("Widescreen side borders: disabled (not necessary, game and desktop aspect ratios match)", initasx, gameHeight, desktopWidth, desktopHeight);
    }
  }
  else 
  {
    platform->WriteDebugString("Widescreen side borders: disabled (unable to obtain desktop resolution)");
  }
  return failed;
}

int switch_to_graphics_mode(int initasx, int initasy, int scrnwid, int scrnhit, int firstDepth, int secondDepth) 
{
  int failed;
  int initasyLetterbox = (initasy * 12) / 10;

  // first of all, try 16-bit normal then letterboxed
  if (game.options[OPT_LETTERBOX] == 0) 
  {
    failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, firstDepth);
    failed = init_gfx_mode(initasx,initasy, firstDepth);
  }
  failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, firstDepth);
  failed = init_gfx_mode(initasx, initasyLetterbox, firstDepth);

  if (secondDepth != firstDepth) {
    // now, try 15-bit normal then letterboxed
    if (game.options[OPT_LETTERBOX] == 0) 
    {
      failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasy, secondDepth);
      failed = init_gfx_mode(initasx,initasy, secondDepth);
    }
    failed = try_widescreen_bordered_graphics_mode_if_appropriate(initasx, initasyLetterbox, secondDepth);
    failed = init_gfx_mode(initasx, initasyLetterbox, secondDepth);
  }

  if ((scrnwid != initasx) || (scrnhit != initasy))
  {
    // now, try the original resolution at 16 then 15 bit
    failed = init_gfx_mode(scrnwid,scrnhit,firstDepth);
    failed = init_gfx_mode(scrnwid,scrnhit, secondDepth);
  }
  
  if (failed)
    return -1;

  return 0;
}

void CreateBlankImage()
{
  // this is the first time that we try to use the graphics driver,
  // so it's the most likey place for a crash
  try
  {
    BITMAP *blank = create_bitmap_ex(final_col_dep, 16, 16);
    blank = gfxDriver->ConvertBitmapToSupportedColourDepth(blank);
    clear(blank);
    blankImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
    blankSidebarImage = gfxDriver->CreateDDBFromBitmap(blank, false, true);
    destroy_bitmap(blank);
  }
  catch (Ali3DException gfxException)
  {
    quit((char*)gfxException._message);
  }

}

void show_preload () {
  // ** Do the preload graphic if available
  color temppal[256];
  block splashsc = load_pcx("preload.pcx",temppal);
  if (splashsc != NULL) {
    if (bitmap_color_depth(splashsc) == 8)
      wsetpalette(0,255,temppal);
    block tsc = create_bitmap_ex(bitmap_color_depth(screen),splashsc->w,splashsc->h);
    blit(splashsc,tsc,0,0,0,0,tsc->w,tsc->h);
    clear(screen);
    stretch_sprite(screen, tsc, 0, 0, scrnwid,scrnhit);

    gfxDriver->ClearDrawList();

    if (!gfxDriver->UsesMemoryBackBuffer())
    {
      IDriverDependantBitmap *ddb = gfxDriver->CreateDDBFromBitmap(screen, false, true);
      gfxDriver->DrawSprite(0, 0, ddb);
      render_to_screen(screen, 0, 0);
      gfxDriver->DestroyDDB(ddb);
    }
    else
      render_to_screen(screen, 0, 0);

    wfreeblock(splashsc);
    wfreeblock(tsc);
    platform->Delay(500);
  }
}

void change_to_directory_of_file(LPCWSTR fileName)
{
  WCHAR wcbuffer[MAX_PATH];
  StrCpyW(wcbuffer, fileName);

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
    if (strrchr(wcbuffer, '/') != NULL) {
      strrchr(wcbuffer, '/')[0] = 0;
      chdir(wcbuffer);
    }
#else
  LPWSTR backSlashAt = StrRChrW(wcbuffer, NULL, L'\\');
  if (backSlashAt != NULL) {
      wcbuffer[wcslen(wcbuffer) - wcslen(backSlashAt)] = L'\0';
      SetCurrentDirectoryW(wcbuffer);
    }
#endif
}

// Startup flags, set from parameters to engine
int datafile_argv=0, change_to_game_dir = 0, force_window = 0;
int override_start_room = 0, force_16bit = 0;
bool justRegisterGame = false;
bool justUnRegisterGame = false;
const char *loadSaveGameOnStartup = NULL;
#ifndef WINDOWS_VERSION
char **global_argv = 0;
#endif

void initialise_game_file_name()
{
#ifdef WINDOWS_VERSION
  WCHAR buffer[MAX_PATH];
  LPCWSTR dataFilePath = wArgv[datafile_argv];
  // Hack for Windows in case there are unicode chars in the path.
  // The normal argv[] array has ????? instead of the unicode chars
  // and fails, so instead we manually get the short file name, which
  // is always using ANSI chars.
  if (wcschr(dataFilePath, '\\') == NULL)
  {
    GetCurrentDirectoryW(MAX_PATH, buffer);
    wcscat(buffer, L"\\");
    wcscat(buffer, dataFilePath);
    dataFilePath = &buffer[0];
  }
  if (GetShortPathNameW(dataFilePath, directoryPathBuffer, MAX_PATH) == 0)
  {
    platform->DisplayAlert("Unable to determine startup path: GetShortPathNameW failed. The specified game file might be missing.");
    game_file_name = NULL;
    return;
  }
  game_file_name = (char*)malloc(MAX_PATH);
  WideCharToMultiByte(CP_ACP, 0, directoryPathBuffer, -1, game_file_name, MAX_PATH, NULL, NULL);
#else
  game_file_name = global_argv[datafile_argv];
#endif
}

int main(int argc,char*argv[]) { 
  our_eip = -999;
  cfopenpriority=2;
  int ee;
  play.recording = 0;
  play.playback = 0;
  play.takeover_data = 0;

  platform = AGSPlatformDriver::GetDriver();

#ifdef WINDOWS_VERSION
  wArgv = CommandLineToArgvW(GetCommandLineW(), &wArgc);
  if (wArgv == NULL)
  {
    platform->DisplayAlert("CommandLineToArgvW failed, unable to start the game.");
    return 9;
  }
#else
  global_argv = argv;
#endif

  print_welcome_text(AC_VERSION_TEXT,ACI_VERSION_TEXT);
  if ((argc>1) && (argv[1][1]=='?'))
    return 0;

  write_log_debug("***** ENGINE STARTUP");

#if defined(WINDOWS_VERSION)
  _set_new_handler(malloc_fail_handler);
  _set_new_mode(1);
  printfworkingspace=(char*)malloc(7000);
#endif
  debug_flags=0;

  for (ee=1;ee<argc;ee++) {
    if (argv[ee][1]=='?') return 0;
    if (stricmp(argv[ee],"-shelllaunch") == 0)
      change_to_game_dir = 1;
    else if (stricmp(argv[ee],"-updatereg") == 0)
      debug_flags |= DBG_REGONLY;
    else if (stricmp(argv[ee],"-windowed") == 0)
      force_window = 1;
    else if (stricmp(argv[ee],"-fullscreen") == 0)
      force_window = 2;
    else if (stricmp(argv[ee],"-hicolor") == 0)
      force_16bit = 1;
    else if (stricmp(argv[ee],"-letterbox") == 0)
      force_letterbox = 1;
    else if (stricmp(argv[ee],"-record") == 0)
      play.recording = 1;
    else if (stricmp(argv[ee],"-playback") == 0)
      play.playback = 1;
#ifdef _DEBUG
    else if ((stricmp(argv[ee],"--startr") == 0) && (ee < argc-1)) {
      override_start_room = atoi(argv[ee+1]);
      ee++;
    }
#endif
    else if ((stricmp(argv[ee],"--testre") == 0) && (ee < argc-2)) {
      strcpy(return_to_roomedit, argv[ee+1]);
      strcpy(return_to_room, argv[ee+2]);
      ee+=2;
    }
    else if (stricmp(argv[ee],"--15bit")==0) debug_15bit_mode = 1;
    else if (stricmp(argv[ee],"--24bit")==0) debug_24bit_mode = 1;
    else if (stricmp(argv[ee],"--fps")==0) display_fps = 2;
    else if (stricmp(argv[ee],"--test")==0) debug_flags|=DBG_DEBUGMODE;
    else if (stricmp(argv[ee],"-noiface")==0) debug_flags|=DBG_NOIFACE;
    else if (stricmp(argv[ee],"-nosprdisp")==0) debug_flags|=DBG_NODRAWSPRITES;
    else if (stricmp(argv[ee],"-nospr")==0) debug_flags|=DBG_NOOBJECTS;
    else if (stricmp(argv[ee],"-noupdate")==0) debug_flags|=DBG_NOUPDATE;
    else if (stricmp(argv[ee],"-nosound")==0) debug_flags|=DBG_NOSFX;
    else if (stricmp(argv[ee],"-nomusic")==0) debug_flags|=DBG_NOMUSIC;
    else if (stricmp(argv[ee],"-noscript")==0) debug_flags|=DBG_NOSCRIPT;
    else if (stricmp(argv[ee],"-novideo")==0) debug_flags|=DBG_NOVIDEO;
    else if (stricmp(argv[ee],"-noexceptionhandler")==0) usetup.disable_exception_handling = 1;
    else if (stricmp(argv[ee],"-dbgscript")==0) debug_flags|=DBG_DBGSCRIPT;
    else if (stricmp(argv[ee],"-registergame") == 0)
    {
      justRegisterGame = true;
    }
    else if (stricmp(argv[ee],"-unregistergame") == 0)
    {
      justUnRegisterGame = true;
    }
    else if ((stricmp(argv[ee],"-loadsavedgame") == 0) && (argc > ee + 1))
    {
      loadSaveGameOnStartup = argv[ee + 1];
      ee++;
    }
    else if ((stricmp(argv[ee],"--enabledebugger") == 0) && (argc > ee + 1))
    {
      strcpy(editor_debugger_instance_token, argv[ee + 1]);
      editor_debugging_enabled = 1;
      force_window = 1;
      ee++;
    }
    else if (stricmp(argv[ee],"--takeover")==0) {
      if (argc < ee+2)
        break;
      play.takeover_data = atoi (argv[ee + 1]);
      strncpy (play.takeover_from, argv[ee + 2], 49);
      play.takeover_from[49] = 0;
      ee += 2;
    }
    else if (argv[ee][0]!='-') datafile_argv=ee;
  }
  

#ifdef _DEBUG
  /* logfile=fopen("g:\\ags.log","at");
   //_CrtSetReportHook( OurReportingFunction );
    int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF;

    tmpDbgFlag = (tmpDbgFlag & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF | _CRTDBG_DELAY_FREE_MEM_DF;

    _CrtSetDbgFlag(tmpDbgFlag);

/*
//  _CrtMemState memstart,memnow;
  _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
  _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_WNDW );
  _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_WNDW );
/*
//   _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
//   _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
//   _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );

//  _CrtMemCheckpoint(&memstart);
//  _CrtMemDumpStatistics( &memstart );*/
#endif

  if ((loadSaveGameOnStartup != NULL) && (argv[0] != NULL))
  {
    // When launched by double-clicking a save game file, the curdir will
    // be the save game folder unless we correct it
    change_to_directory_of_file(wArgv[0]);
  }
 
#ifdef MAC_VERSION
  getcwd(appDirectory, 512);
#endif
 
  //if (change_to_game_dir == 1)  {
  if (datafile_argv > 0) {
    // If launched by double-clicking .AGS file, change to that
    // folder; else change to this exe's folder
    change_to_directory_of_file(wArgv[datafile_argv]);
  }

#ifdef MAC_VERSION
  getcwd(dataDirectory, 512);
#endif
 
  // Update shell associations and exit
  if (debug_flags & DBG_REGONLY)
    exit(0);

#ifndef USE_CUSTOM_EXCEPTION_HANDLER
  usetup.disable_exception_handling = 1;
#endif

  if (usetup.disable_exception_handling)
  {
    return initialize_engine(argc, argv);
  }
  else
  {
    return initialize_engine_with_exception_handling(argc, argv);
  }
}

void create_gfx_driver() 
{
#ifdef WINDOWS_VERSION
  if (stricmp(usetup.gfxDriverID, "D3D9") == 0)
    gfxDriver = GetD3DGraphicsDriver(filter);
  else
#endif
    gfxDriver = GetSoftwareGraphicsDriver(filter);

  gfxDriver->SetCallbackOnInit(GfxDriverOnInitCallback);
  gfxDriver->SetTintMethod(TintReColourise);
}

int initialize_engine(int argc,char*argv[])
{
  FILE*ppp;
  int ee;

  write_log_debug("Reading config file");

  our_eip = -200;
  read_config_file(argv[0]);

  set_uformat(U_ASCII);

  write_log_debug("Initializing allegro");

  our_eip = -199;
  // Initialize allegro
#ifdef WINDOWS_VERSION
  if (install_allegro(SYSTEM_AUTODETECT,&myerrno,atexit)) {
    platform->DisplayAlert("Unable to initialize graphics subsystem. Make sure you have DirectX 5 or above installed.");
#else
  if (allegro_init()) {
    platform->DisplayAlert("Unknown error initializing graphics subsystem.");
#endif
    return EXIT_NORMAL;
  }

  write_log_debug("Setting up window");

  our_eip = -198;
#if (ALLEGRO_DATE > 19990103)
  set_window_title("Adventure Game Studio");
#if (ALLEGRO_DATE > 20021115)
  set_close_button_callback (winclosehook);
#else
  set_window_close_hook (winclosehook);
#endif

  our_eip = -197;
#endif

  platform->SetGameWindowIcon();

  our_eip = -196;

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
  // check if Setup needs to be run instead
  if (argc>1) {
    if (stricmp(argv[1],"--setup")==0) { 
      write_log_debug("Running Setup");

      if (!platform->RunSetup())
        return EXIT_NORMAL;

#ifndef WINDOWS_VERSION
#define _spawnl spawnl
#define _P_OVERLAY P_OVERLAY
#endif
      // Just re-reading the config file seems to cause a caching
      // problem on Win9x, so let's restart the process.
      allegro_exit();
      char quotedpath[255];
      sprintf (quotedpath, "\"%s\"", argv[0]);
      _spawnl (_P_OVERLAY, argv[0], quotedpath, NULL);
      //read_config_file(argv[0]);
    }
  }
#endif

  // Force to run in a window, override the config file
  if (force_window == 1)
    usetup.windowed = 1;
  else if (force_window == 2)
    usetup.windowed = 0;
  
  our_eip = -195;

  write_log_debug("Initializing game data");

  // initialize the data file
  initialise_game_file_name();
  if (game_file_name == NULL) return EXIT_NORMAL;

  int errcod = csetlib(game_file_name,"");  // assume it's appended to exe

  our_eip = -194;
//  char gamefilenamebuf[200];
  if ((errcod!=0) && (change_to_game_dir == 0)) {
    // it's not, so look for the file
    
    game_file_name = ci_find_file(usetup.data_files_dir, usetup.main_data_filename);

    errcod=csetlib(game_file_name,"");
    if (errcod) {
      //sprintf(gamefilenamebuf,"%s\\ac2game.ags",usetup.data_files_dir);
      free(game_file_name);
      game_file_name = ci_find_file(usetup.data_files_dir, "ac2game.ags");
      
      errcod = csetlib(game_file_name,"");
    }
  }
  else {
    // set the data filename to the EXE name
    
    usetup.main_data_filename = get_filename(game_file_name);

    if (((strchr(game_file_name, '/') != NULL) ||
         (strchr(game_file_name, '\\') != NULL)) &&
         (stricmp(usetup.data_files_dir, ".") == 0)) {
      // there is a path in the game file name (and the user
      // has not specified another one)
      // save the path, so that it can load the VOX files, etc
      usetup.data_files_dir = (char*)malloc(strlen(game_file_name) + 1);
      strcpy(usetup.data_files_dir, game_file_name);
    
      if (strrchr(usetup.data_files_dir, '/') != NULL)
        strrchr(usetup.data_files_dir, '/')[0] = 0;
      else if (strrchr(usetup.data_files_dir, '\\') != NULL)
        strrchr(usetup.data_files_dir, '\\')[0] = 0;
      else {
        platform->DisplayAlert("Error processing game file name: slash but no slash");
        return EXIT_NORMAL;
      }
    }

  }

  our_eip = -193;

  if (errcod!=0) {  // there's a problem
    if (errcod==-1) {  // file not found
      char emsg[STD_BUFFER_SIZE];
      sprintf (emsg,
        "You must create and save a game first in the AGS Editor before you can use "
        "this engine.\n\n"
        "If you have just downloaded AGS, you are probably running the wrong executable.\n"
        "Run AGSEditor.exe to launch the editor.\n\n"
        "(Unable to find '%s')\n", argv[datafile_argv]);
      platform->DisplayAlert(emsg);
      }
    else if (errcod==-4)
      platform->DisplayAlert("ERROR: Too many files in data file.");
    else platform->DisplayAlert("ERROR: The file is corrupt. Make sure you have the correct version of the\n"
        "editor, and that this really is an AGS game.\n");
    return EXIT_NORMAL;
  }

  our_eip = -192;

  write_log_debug("Initializing TTF renderer");

  init_font_renderer();

  our_eip = -188;

  write_log_debug("Initializing mouse");

#ifdef _DEBUG
  // Quantify fails with the mouse for some reason
  minstalled();
#else
  if (minstalled()==0) {
    platform->DisplayAlert(platform->GetNoMouseErrorString());
    return EXIT_NORMAL;
  }
#endif // DEBUG
  our_eip = -187;

  write_log_debug("Checking memory");

  char*memcheck=(char*)malloc(4000000);
  if (memcheck==NULL) {
    platform->DisplayAlert("There is not enough memory available to run this game. You need 4 Mb free\n"
      "extended memory to run the game.\n"
      "If you are running from Windows, check the 'DPMI memory' setting on the DOS box\n"
      "properties.\n");
    return EXIT_NORMAL;
    }
  free(memcheck);
  unlink (replayTempFile);

  write_log_debug("Initializing rooms");

  roomstats=(RoomStatus*)calloc(sizeof(RoomStatus),MAX_ROOMS);
  for (ee=0;ee<MAX_ROOMS;ee++) {
    roomstats[ee].beenhere=0;
    roomstats[ee].numobj=0;
    roomstats[ee].tsdatasize=0;
    roomstats[ee].tsdata=NULL;
    }
  play.want_speech=-2;

  our_eip = -186;
  if (usetup.no_speech_pack == 0) {
    /* Can't just use fopen here, since we need to change the filename
        so that pack functions, etc. will have the right case later */
    speech_file = ci_find_file(usetup.data_files_dir, "speech.vox");
    
    ppp = fopen(speech_file, "rb");

    if (ppp == NULL)
    {
      // In case they're running in debug, check Compiled folder
      free(speech_file);
      speech_file = ci_find_file("Compiled", "speech.vox");
      ppp = fopen(speech_file, "rb");
    }
    
    if (ppp!=NULL) {
      fclose(ppp);

      write_log_debug("Initializing speech vox");

      //if (csetlib(useloc,"")!=0) {
      if (csetlib(speech_file,"")!=0) {
        platform->DisplayAlert("Unable to initialize speech sample file - check for corruption and that\nit belongs to this game.\n");
        return EXIT_NORMAL;
      }
      FILE *speechsync = clibfopen("syncdata.dat", "rb");
      if (speechsync != NULL) {
        // this game has voice lip sync
        if (getw(speechsync) != 4)
          platform->DisplayAlert("Unknown speech lip sync format (might be from older or newer version); lip sync disabled");
        else {
          numLipLines = getw(speechsync);
          splipsync = (SpeechLipSyncLine*)malloc (sizeof(SpeechLipSyncLine) * numLipLines);
          for (ee = 0; ee < numLipLines; ee++)
          {
            splipsync[ee].numPhenomes = getshort(speechsync);
            fread(splipsync[ee].filename, 1, 14, speechsync);
            splipsync[ee].endtimeoffs = (int*)malloc(splipsync[ee].numPhenomes * sizeof(int));
            fread(splipsync[ee].endtimeoffs, sizeof(int), splipsync[ee].numPhenomes, speechsync);
            splipsync[ee].frame = (short*)malloc(splipsync[ee].numPhenomes * sizeof(short));
            fread(splipsync[ee].frame, sizeof(short), splipsync[ee].numPhenomes, speechsync);
          }
        }
        fclose (speechsync);
      }
      csetlib(game_file_name,"");
      platform->WriteConsole("Speech sample file found and initialized.\n");
      play.want_speech=1;
    }
  }

  our_eip = -185;
  play.seperate_music_lib = 0;

  /* Can't just use fopen here, since we need to change the filename
      so that pack functions, etc. will have the right case later */
  music_file = ci_find_file(usetup.data_files_dir, "audio.vox");

  /* Don't need to use ci_fopen here, because we've used ci_find_file to get
      the case insensitive matched filename already */
  ppp = fopen(music_file, "rb");
  
  if (ppp == NULL)
  {
    // In case they're running in debug, check Compiled folder
    free(music_file);
    music_file = ci_find_file("Compiled", "audio.vox");
    ppp = fopen(music_file, "rb");
  }

  if (ppp!=NULL) {
    fclose(ppp);

    write_log_debug("Initializing audio vox");

    //if (csetlib(useloc,"")!=0) {
    if (csetlib(music_file,"")!=0) {
      platform->DisplayAlert("Unable to initialize music library - check for corruption and that\nit belongs to this game.\n");
      return EXIT_NORMAL;
    }
    csetlib(game_file_name,"");
    platform->WriteConsole("Audio vox found and initialized.\n");
    play.seperate_music_lib = 1;
  }

  our_eip = -184;

#ifdef ALLEGRO_KEYBOARD_HANDLER
  write_log_debug("Initializing keyboard");

  install_keyboard();
#endif

  our_eip = -183;

  write_log_debug("Install timer");

  platform->WriteConsole("Checking sound inits.\n");
  if (opts.mod_player) reserve_voices(16,-1);
  // maybe this line will solve the sound volume?
  install_timer();
#if ALLEGRO_DATE > 19991010
  set_volume_per_voice(1);
#endif

  our_eip = -182;

#ifdef WINDOWS_VERSION
  // don't let it use the hardware mixer verion, crashes some systems
  //if ((usetup.digicard == DIGI_AUTODETECT) || (usetup.digicard == DIGI_DIRECTX(0)))
//    usetup.digicard = DIGI_DIRECTAMX(0);

  if (usetup.digicard == DIGI_DIRECTX(0)) {
    // DirectX mixer seems to buffer an extra sample itself
    use_extra_sound_offset = 1;
  }

  // if the user clicked away to another app while we were
  // loading, DirectSound will fail to initialize. There doesn't
  // seem to be a solution to force us back to the foreground,
  // because we have no actual visible window at this time

#endif

  write_log_debug("Initialize sound drivers");

  if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
    reserve_voices(-1,-1);
    opts.mod_player=0;
    opts.mp3_player=0;
    if (install_sound(usetup.digicard,usetup.midicard,NULL)!=0) {
      if ((usetup.digicard != DIGI_NONE) && (usetup.midicard != MIDI_NONE)) {
        // only flag an error if they wanted a sound card
        platform->DisplayAlert("\nUnable to initialize your audio hardware.\n"
          "[Problem: %s]\n",allegro_error);
      }
      reserve_voices(0,0);
      install_sound(DIGI_NONE, MIDI_NONE, NULL);
      usetup.digicard = DIGI_NONE;
      usetup.midicard = MIDI_NONE;
    }
  }

  our_eip = -181;

  if (usetup.digicard == DIGI_NONE) {
    // disable speech and music if no digital sound
    // therefore the MIDI soundtrack will be used if present,
    // and the voice mode should not go to Voice Only
    play.want_speech = -2;
    play.seperate_music_lib = 0;
  }

  //set_volume(255,-1);
  if ((debug_flags & (~DBG_DEBUGMODE)) >0) {
    platform->DisplayAlert("Engine debugging enabled.\n"
     "\nNOTE: You have selected to enable one or more engine debugging options.\n"
     "These options cause many parts of the game to behave abnormally, and you\n"
     "may not see the game as you are used to it. The point is to test whether\n"
     "the engine passes a point where it is crashing on you normally.\n"
     "[Debug flags enabled: 0x%02X]\n"
     "Press a key to continue.\n",debug_flags);
    }

  our_eip = -10;

  write_log_debug("Install exit handler");

  atexit(atexit_handler);
  unlink("warnings.log");
  play.randseed = time(NULL);
  srand (play.randseed);

  write_log_debug("Initialize path finder library");

  init_pathfinder();

  write_log_debug("Initialize gfx");

  platform->InitialiseAbufAtStartup();

  LOCK_VARIABLE(timerloop);
  LOCK_FUNCTION(dj_timer_handler);
  set_game_speed(40);

  our_eip=-20;
  //thisroom.allocall();
  our_eip=-19;
  //setup_sierra_interface();   // take this out later

  write_log_debug("Load game data");

  our_eip=-17;
  if ((ee=load_game_file())!=0) {
    proper_exit=1;
    platform->FinishedUsingGraphicsMode();

    if (ee==-1)
      platform->DisplayAlert("Main game file not found. This may be from a different AGS version, or the file may have got corrupted.\n");
    else if (ee==-2)
      platform->DisplayAlert("Invalid file format. The file may be corrupt, or from a different\n"
        "version of AGS.\nThis engine can only run games made with AGS 3.2 or later.\n");
    else if (ee==-3)
      platform->DisplayAlert("Script link failed: %s\n",ccErrorString);
    return EXIT_NORMAL;
  }

  if (justRegisterGame) 
  {
    platform->RegisterGameWithGameExplorer();
    proper_exit = 1;
    return EXIT_NORMAL;
  }

  if (justUnRegisterGame) 
  {
    platform->UnRegisterGameWithGameExplorer();
    proper_exit = 1;
    return EXIT_NORMAL;
  }

  //platform->DisplayAlert("loaded game");
  our_eip=-91;
#if (ALLEGRO_DATE > 19990103)
  set_window_title(game.gamename);
#endif

  write_log_debug(game.gamename);

  our_eip = -189;

  if (file_exists("Compiled", FA_ARCH | FA_DIREC, NULL))
  {
    // running in debugger
    use_compiled_folder_as_current_dir = 1;
    // don't redirect to the game exe folder (_Debug)
    usetup.data_files_dir = ".";
  }

  if (game.saveGameFolderName[0] != 0)
  {
    char newDirBuffer[MAX_PATH];
    sprintf(newDirBuffer, "$MYDOCS$/%s", game.saveGameFolderName);
    Game_SetSaveGameDirectory(newDirBuffer);
  }
  else if (use_compiled_folder_as_current_dir)
  {
    Game_SetSaveGameDirectory("Compiled");
  }
  our_eip = -178;

  write_log_debug("Checking for disk space");

  //init_language_text("en");
  if (check_write_access()==0) {
    platform->DisplayAlert("Unable to write to the current directory. Do not run this game off a\n"
    "network or CD-ROM drive. Also check drive free space (you need 1 Mb free).\n");
    proper_exit = 1;
    return EXIT_NORMAL; 
  }

  if (fontRenderers[0] == NULL) 
  {
    platform->DisplayAlert("No fonts found. If you're trying to run the game from the Debug directory, this is not supported. Use the Build EXE command to create an executable in the Compiled folder.");
    proper_exit = 1;
    return EXIT_NORMAL;
  }

  our_eip = -179;

  if (game.options[OPT_NOMODMUSIC])
    opts.mod_player = 0;

  if (opts.mod_player) {
    write_log_debug("Initializing MOD/XM player");

    if (init_mod_player(NUM_MOD_DIGI_VOICES) < 0) {
      platform->DisplayAlert("Warning: install_mod: MOD player failed to initialize.");
      opts.mod_player=0;
    }
  }

  write_log_debug("Initializing screen settings");

  // default shifts for how we store the sprite data

  // PSP: Switch b<>r for 15/16 bit.
  _rgb_r_shift_32 = 16;
  _rgb_g_shift_32 = 8;
  _rgb_b_shift_32 = 0;
  _rgb_b_shift_16 = 11;
  _rgb_g_shift_16 = 5;
  _rgb_r_shift_16 = 0;
  _rgb_b_shift_15 = 10;
  _rgb_g_shift_15 = 5;
  _rgb_r_shift_15 = 0;

  usetup.base_width = 320;
  usetup.base_height = 200;
  
  if (game.default_resolution >= 5)
  {
    if (game.default_resolution >= 6)
    {
      // 1024x768
      usetup.base_width = 512;
      usetup.base_height = 384;
    }
    else
    {
      // 800x600
      usetup.base_width = 400;
      usetup.base_height = 300;
    }
    // don't allow letterbox mode
    game.options[OPT_LETTERBOX] = 0;
    force_letterbox = 0;
    scrnwid = usetup.base_width * 2;
    scrnhit = usetup.base_height * 2;
    wtext_multiply = 2;
  }
  else if ((game.default_resolution == 4) ||
           (game.default_resolution == 3))
  {
    scrnwid = 640;
    scrnhit = 400;
    wtext_multiply = 2;
  }
  else if ((game.default_resolution == 2) ||
           (game.default_resolution == 1))
  {
    scrnwid = 320;
    scrnhit = 200;
    wtext_multiply = 1;
  }
  else
  {
    scrnwid = usetup.base_width;
    scrnhit = usetup.base_height;
    wtext_multiply = 1;
  }

  usetup.textheight = wgetfontheight(0) + 1;

  vesa_xres=scrnwid; vesa_yres=scrnhit;
  //scrnwto=scrnwid-1; scrnhto=scrnhit-1;
  current_screen_resolution_multiplier = scrnwid / BASEWIDTH;

  if ((game.default_resolution > 2) &&
      (game.options[OPT_NATIVECOORDINATES]))
  {
    usetup.base_width *= 2;
    usetup.base_height *= 2;
  }

  int initasx=scrnwid,initasy=scrnhit;
  if (scrnwid==960) { initasx=1024; initasy=768; }

  // save this setting so we only do 640x480 full-screen if they want it
  usetup.want_letterbox = game.options[OPT_LETTERBOX];

  if (force_letterbox > 0)
    game.options[OPT_LETTERBOX] = 1;

  // don't allow them to force a 256-col game to hi-color
  if (game.color_depth < 2)
    usetup.force_hicolor_mode = 0;

  int firstDepth = 8, secondDepth = 8;
  if ((game.color_depth == 2) || (force_16bit) || (usetup.force_hicolor_mode)) {
    firstDepth = 16;
    secondDepth = 15;
  }
  else if (game.color_depth > 2) {
    firstDepth = 32;
    secondDepth = 24;
  }
  
#if 1
#ifdef MAC_VERSION
  if (game.color_depth > 1)
  {
    // force true color - bug in hi color
    // only noticed in KQ2VGA, and haven't tracked down yet
    // (some gfx are corrupt)
    firstDepth = 32;
    secondDepth = 24;
  }
#endif
#endif

  adjust_sizes_for_resolution(loaded_game_file_version);

  write_log_debug("Init gfx filters");

  if (initialize_graphics_filter(usetup.gfxFilterID, initasx, initasy, firstDepth))
  {
    return EXIT_NORMAL;
  }
  
  write_log_debug("Init gfx driver");

  create_gfx_driver();

  write_log_debug("Switching to graphics mode");

  if (switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
  {
    bool errorAndExit = true;

    if (((usetup.gfxFilterID == NULL) || 
      (stricmp(usetup.gfxFilterID, "None") == 0)) &&
      (scrnwid == 320))
    {
      // If the game is 320x200 and no filter is being used, try using a 2x
      // filter automatically since many gfx drivers don't suport 320x200.
      write_log_debug("320x200 not supported, trying with 2x filter");
      delete filter;

      if (initialize_graphics_filter("StdScale2", initasx, initasy, firstDepth)) 
      {
        return EXIT_NORMAL;
      }

      create_gfx_driver();

      if (!switch_to_graphics_mode(initasx, initasy, scrnwid, scrnhit, firstDepth, secondDepth))
      {
        errorAndExit = false;
      }

    }
    
    if (errorAndExit)
    {
      proper_exit=1;
      platform->FinishedUsingGraphicsMode();
  
      // make sure the error message displays the true resolution
      if (game.options[OPT_LETTERBOX])
        initasy = (initasy * 12) / 10;

      if (filter != NULL)
        filter->GetRealResolution(&initasx, &initasy);

      platform->DisplayAlert("There was a problem initializing graphics mode %d x %d (%d-bit).\n"
       "(Problem: '%s')\n"
       "Try to correct the problem, or seek help from the AGS homepage.\n"
       "\nPossible causes:\n* your graphics card drivers do not support this resolution. "
       "Run the game setup program and try the other resolution.\n"
       "* the graphics driver you have selected does not work. Try switching between Direct3D and DirectDraw.\n"
       "* the graphics filter you have selected does not work. Try another filter.",
       initasx, initasy, firstDepth, allegro_error);
      return EXIT_NORMAL;
    }
  }
  
  //screen = _filter->ScreenInitialized(screen, final_scrn_wid, final_scrn_hit);
  _old_screen = screen;

  if (gfxDriver->HasAcceleratedStretchAndFlip()) 
  {
    walkBehindMethod = DrawAsSeparateSprite;

    CreateBlankImage();
  }

  write_log_debug("Preparing graphics mode screen");

  if ((final_scrn_hit != scrnhit) || (final_scrn_wid != scrnwid)) {
    initasx = final_scrn_wid;
    initasy = final_scrn_hit;
    clear(_old_screen);
    screen = create_sub_bitmap(_old_screen, initasx / 2 - scrnwid / 2, initasy/2-scrnhit/2, scrnwid, scrnhit);
    _sub_screen=screen;

    scrnhit = screen->h;
    vesa_yres = screen->h;
    scrnwid = screen->w;
    vesa_xres = screen->w;
    gfxDriver->SetMemoryBackBuffer(screen);

    platform->WriteDebugString("Screen resolution: %d x %d; game resolution %d x %d", _old_screen->w, _old_screen->h, scrnwid, scrnhit);
  }


  // Most cards do 5-6-5 RGB, which is the format the files are saved in
  // Some do 5-6-5 BGR, or  6-5-5 RGB, in which case convert the gfx
  if ((final_col_dep == 16) && ((_rgb_b_shift_16 != 0) || (_rgb_r_shift_16 != 11))) {
    convert_16bit_bgr = 1;
    if (_rgb_r_shift_16 == 10) {
      // some very old graphics cards lie about being 16-bit when they
      // are in fact 15-bit ... get around this
      _places_r = 3;
      _places_g = 3;
    }
  }
  if (final_col_dep > 16) {
    // when we're using 32-bit colour, it converts hi-color images
    // the wrong way round - so fix that
    _rgb_b_shift_16 = 0;
    _rgb_g_shift_16 = 5;
    _rgb_r_shift_16 = 11;

    _rgb_b_shift_15 = 0;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 10;

    _rgb_r_shift_32 = 0;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 16;
  }
  else if (final_col_dep <= 16) {
    // ensure that any 32-bit graphics displayed are converted
    // properly to the current depth
    _rgb_r_shift_32 = 0;
    _rgb_g_shift_32 = 8;
    _rgb_b_shift_32 = 16;

    _rgb_b_shift_15 = 0;
    _rgb_g_shift_15 = 5;
    _rgb_r_shift_15 = 10;
  }

  platform->PostAllegroInit((usetup.windowed > 0) ? true : false);

  gfxDriver->SetCallbackForPolling(update_polled_stuff);
  gfxDriver->SetCallbackToDrawScreen(draw_screen_callback);
  gfxDriver->SetCallbackForNullSprite(GfxDriverNullSpriteCallback);

  write_log_debug("Initializing colour conversion");

  set_color_conversion(COLORCONV_MOST | COLORCONV_EXPAND_256 | COLORCONV_REDUCE_16_TO_15);

  SetMultitasking(0);

  write_log_debug("Check for preload image");

  show_preload ();

  write_log_debug("Initialize sprites");

  if (spriteset.initFile ("acsprset.spr")) 
  {
    platform->FinishedUsingGraphicsMode();
    allegro_exit();
    proper_exit=1;
    platform->DisplayAlert("Could not load sprite set file ACSPRSET.SPR\n"
      "This means that the file is missing or there is not enough free\n"
      "system memory to load the file.\n");
    return EXIT_NORMAL;
  }

  write_log_debug("Set up screen");

  virtual_screen=create_bitmap_ex(final_col_dep,scrnwid,scrnhit);
  clear(virtual_screen);
  gfxDriver->SetMemoryBackBuffer(virtual_screen);
//  ignore_mouseoff_bitmap = virtual_screen;
  abuf=screen;
  our_eip=-7;

  for (ee = 0; ee < MAX_INIT_SPR + game.numcharacters; ee++)
    actsps[ee] = NULL;

  write_log_debug("Initialize game settings");

  init_game_settings();

  write_log_debug("Prepare to start game");

  scsystem.width = final_scrn_wid;
  scsystem.height = final_scrn_hit;
  scsystem.coldepth = final_col_dep;
  scsystem.windowed = 0;
  scsystem.vsync = 0;
  scsystem.viewport_width = divide_down_coordinate(scrnwid);
  scsystem.viewport_height = divide_down_coordinate(scrnhit);
  strcpy(scsystem.aci_version, ACI_VERSION_TEXT);
  scsystem.os = platform->GetSystemOSID();

  if (usetup.windowed)
    scsystem.windowed = 1;

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
  filter->SetMouseArea(0, 0, scrnwid-1, scrnhit-1);
#else
  filter->SetMouseArea(0,0,BASEWIDTH-1,BASEHEIGHT-1);
#endif
//  mloadwcursor("mouse.spr");
  //mousecurs[0]=spriteset[2054];
  currentcursor=0;
  our_eip=-4;
  mousey=100;  // stop icon bar popping up
  init_invalid_regions(final_scrn_hit);
  wsetscreen(virtual_screen);
  our_eip = -41;

  gfxDriver->SetRenderOffset(get_screen_x_adjustment(virtual_screen), get_screen_y_adjustment(virtual_screen));

  initialize_start_and_play_game(override_start_room, loadSaveGameOnStartup);

  quit("|bye!");
  return 0;
}

#if defined(WINDOWS_VERSION) || defined(LINUX_VERSION) || defined(MAC_VERSION)
END_OF_MAIN()
#endif

int initialize_engine_with_exception_handling(int argc,char*argv[])
{
  write_log_debug("Installing exception handler");

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
  __try 
  {
#endif

    return initialize_engine(argc, argv);

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
  }
  __except (CustomExceptionHandler ( GetExceptionInformation() )) 
  {
    strcpy (tempmsg, "");
    sprintf (printfworkingspace, "An exception 0x%X occurred in ACWIN.EXE at EIP = 0x%08X %s; program pointer is %+d, ACI version " ACI_VERSION_TEXT ", gtags (%d,%d)\n\n"
      "AGS cannot continue, this exception was fatal. Please note down the numbers above, remember what you were doing at the time and post the details on the AGS Technical Forum.\n\n%s\n\n"
      "Most versions of Windows allow you to press Ctrl+C now to copy this entire message to the clipboard for easy reporting.\n\n%s (code %d)",
      excinfo.ExceptionCode, excinfo.ExceptionAddress, tempmsg, our_eip, eip_guinum, eip_guiobj, get_cur_script(5),
      (miniDumpResultCode == 0) ? "An error file CrashInfo.dmp has been created. You may be asked to upload this file when reporting this problem on the AGS Forums." : 
      "Unable to create an error dump file.", miniDumpResultCode);
    MessageBoxA(allegro_wnd, printfworkingspace, "Illegal exception", MB_ICONSTOP | MB_OK);
    proper_exit = 1;
  }
  return EXIT_CRASH;
#endif
}

