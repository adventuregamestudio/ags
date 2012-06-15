/* Adventure Creator v2 Run-time engine
   Started 27-May-99 (c) 1999-2011 Chris Jones

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#if (defined(MAC_VERSION) && !defined(IOS_VERSION)) || (defined(LINUX_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION))
#include <dlfcn.h>
#endif

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
int psp_video_framedrop = 1;
int psp_audio_enabled = 1;
int psp_midi_enabled = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;
volatile int psp_audio_multithreaded = 1;
int psp_midi_preload_patches = 0;
int psp_audio_cachesize = 10;
char psp_game_file_name[] = "ac2game.dat";
int psp_gfx_smooth_sprites = 1;
char psp_translation[] = "default";
#endif



#if defined(MAC_VERSION) || (defined(LINUX_VERSION) && !defined(PSP_VERSION))
#include <pthread.h>
pthread_t soundthread;
#endif

#if defined(ANDROID_VERSION)
#include <sys/stat.h>
#include <android/log.h>

extern "C" void android_render();
extern "C" void selectLatestSavegame();
extern bool psp_load_latest_savegame;
#endif

#if defined(IOS_VERSION)
extern "C" void ios_render();
#endif

// PSP specific variables:
extern int psp_video_framedrop; // Drop video frames if lagging behind audio?
extern int psp_audio_enabled; // Audio can be disabled in the config file.
extern int psp_midi_enabled; // Enable midi playback.
extern int psp_ignore_acsetup_cfg_file; // If set, the standard AGS config file is not being read.
extern int psp_clear_cache_on_room_change; // Clear the sprite cache on every room change.
extern void clear_sound_cache(); // Sound cache initialization.
extern char psp_game_file_name[]; // Game filename from the menu.
extern int psp_gfx_renderer; // Which renderer to use.
extern int psp_gfx_smooth_sprites; // usetup.enable_antialiasing
extern char psp_translation[]; // Translation file
int psp_is_old_datafile = 0; // Set for 3.1.1 and 3.1.2 datafiles


#if defined(PSP_VERSION)
// PSP header.
#include <pspsdk.h>
#include <pspdebug.h>
#include <pspthreadman.h>
#include <psputils.h>
#include <pspmath.h>
#endif

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

// Old dialog support
unsigned char** old_dialog_scripts;
char** old_speech_lines;


#ifdef MAC_VERSION
char dataDirectory[512];
char appDirectory[512];
extern "C"
{
   int osx_sys_question(const char *msg, const char *but1, const char *but2);
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





extern int our_eip;
#include "wgt2allg.h"
#include "sprcache.h"



#ifdef WINDOWS_VERSION
#include <crtdbg.h>
#include "winalleg.h"
#include <shlwapi.h>

#elif defined(LINUX_VERSION) || defined(MAC_VERSION)


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "../PSP/launcher/pe.h"

long int filelength(int fhandle)
{
	struct stat statbuf;
	fstat(fhandle, &statbuf);
	return statbuf.st_size;
}

#else   // it's DOS (DJGPP)

#include "sys/exceptn.h"

int sys_getch() {
  return getch();
}

#endif  // WINDOWS_VERSION

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

#include "ac/ac_common.h"
#include "ac/ac_compress.h"
#include "ac/ac_gamesetupstruct.h"
#include "ac/ac_lipsync.h"
#include "ac/ac_object.h"
#include "cs/cs_common.h"
#include "cs/cc_instance.h"
#include "cs/cs_runtime.h"
#include "cs/cc_error.h"
#include "cs/cc_options.h"
#include "cs/cs_utils.h"
#include "acfont/ac_agsfontrenderer.h" // fontRenderers

#include <aastr.h>
#include <acdebug.h>


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


extern "C" HWND allegro_wnd;



#ifdef WINDOWS_VERSION
int wArgc;
LPWSTR *wArgv;
#else

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


#include "acgui/ac_dynamicarray.h"
#include "acgui/ac_guimain.h"
#include "acgui/ac_guibutton.h"
#include "acgui/ac_guiinv.h"
#include "acgui/ac_guilabel.h"
#include "acgui/ac_guilistbox.h"
#include "acgui/ac_guislider.h"
#include "acgui/ac_guitextbox.h"

#include "acruntim.h"
#include "acsound.h"

#include "acgfx/ac_gfxfilters.h"


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

// PSP: Update in thread if wanted.
extern volatile int psp_audio_multithreaded;
volatile bool update_mp3_thread_running = false;
int musicPollIterator; // long name so it doesn't interfere with anything else
#define UPDATE_MP3_THREAD \
   while (switching_away_from_game) { } \
   for (musicPollIterator = 0; musicPollIterator <= MAX_SOUND_CHANNELS; musicPollIterator++) { \
     if ((channels[musicPollIterator] != NULL) && (channels[musicPollIterator]->done == 0)) \
       channels[musicPollIterator]->poll(); \
   }
#define UPDATE_MP3 \
 if (!psp_audio_multithreaded) \
  { UPDATE_MP3_THREAD }

//#define UPDATE_MP3 update_polled_stuff_if_runtime();
#if defined(PSP_VERSION)
// PSP: Workaround for sound stuttering. Do sound updates in its own thread.
int update_mp3_thread(SceSize args, void *argp)
{
  while (update_mp3_thread_running)
  {
    UPDATE_MP3_THREAD
    sceKernelDelayThread(1000 * 50);
  }
  return 0;
}
#elif (defined(LINUX_VERSION) && !defined(PSP_VERSION)) || defined(MAC_VERSION)
void* update_mp3_thread(void* arg)
{
  while (update_mp3_thread_running)
  {
    UPDATE_MP3_THREAD
    usleep(1000 * 50);
  }
  pthread_exit(NULL);
}
#elif defined(WINDOWS_VERSION)
DWORD WINAPI update_mp3_thread(LPVOID lpParam)
{
  while (update_mp3_thread_running)
  {
    UPDATE_MP3_THREAD
    Sleep(50);
  }
  return 0;
}
#endif


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

#if defined(MAC_VERSION) && !defined(IOS_VERSION)
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
/*
  FILE*ooo=fopen("ac.log","at");
  fprintf(ooo,"%s\n",msg);
  fclose(ooo);
*/
  platform->WriteDebugString(msg);
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
#if ALLEGRO_DATE > 19991010
#define PFO_PARAM const char *
#else
#define PFO_PARAM char *
#endif
#ifndef RTLD_NEXT
extern PACKFILE *__old_pack_fopen(PFO_PARAM,PFO_PARAM);
#endif

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

#ifdef RTLD_NEXT
  static PACKFILE * (*__old_pack_fopen)(PFO_PARAM, PFO_PARAM) = NULL;
  if(!__old_pack_fopen) {
    __old_pack_fopen = (PACKFILE* (*)(PFO_PARAM, PFO_PARAM))dlsym(RTLD_NEXT, "pack_fopen");
    if(!__old_pack_fopen) {
      // Looks like we're linking statically to allegro...
      // Let's see if it has been patched
      __old_pack_fopen = (PACKFILE* (*)(PFO_PARAM, PFO_PARAM))dlsym(RTLD_DEFAULT, "__allegro_pack_fopen");
      if(!__old_pack_fopen) {
        fprintf(stderr, "If you're linking statically to allegro, you need to apply this patch to allegro:\n"
        "https://sourceforge.net/tracker/?func=detail&aid=3302567&group_id=5665&atid=355665\n");
        exit(1);
      }
    }
  }
#endif

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

#if defined(ANDROID_VERSION)
      if (game.color_depth == 1)
        android_render();
#elif defined(IOS_VERSION)
      if (game.color_depth == 1)
        ios_render();
#endif

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

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
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

  update_polled_stuff_if_runtime();

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
    update_polled_stuff_if_runtime();
    
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

      update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();
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
    update_polled_stuff_if_runtime();
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

#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(PSP_VERSION)
  // PSP: Convert 32 bit backgrounds.
  if (bitmap_color_depth(thisroom.ebscene[cc]) == 32)
    thisroom.ebscene[cc] = convert_32_to_32bgr(thisroom.ebscene[cc]);
#endif

    thisroom.ebscene[cc] = gfxDriver->ConvertBitmapToSupportedColourDepth(thisroom.ebscene[cc]);
  }

  if ((bitmap_color_depth(thisroom.ebscene[0]) == 8) &&
      (final_col_dep > 8))
    unselect_palette();

  update_polled_stuff_if_runtime();

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

    update_polled_stuff_if_runtime();
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
  update_polled_stuff_if_runtime();
  redo_walkable_areas();
  // fix walk-behinds to current screen resolution
  thisroom.object = fix_bitmap_size(thisroom.object);
  update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();

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
  update_polled_stuff_if_runtime();
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
  update_polled_stuff_if_runtime();
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

  update_polled_stuff_if_runtime();

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
  update_polled_stuff_if_runtime();

  // change rooms
  unload_old_room();

  if (psp_clear_cache_on_room_change)
    spriteset.removeAll();

  update_polled_stuff_if_runtime();

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
        update_polled_stuff_if_runtime();
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
        update_polled_stuff_if_runtime();
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

    // Fall back to the inventory pic if no cursor pic is defined.
    if (cursorSprite == 0)
      cursorSprite = game.invinfo[invnum].pic;

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

    views[vii].loops[loop].frames[frame].sound = clip->id + (psp_is_old_datafile ? 0x10000000 : 0);
  }
}

// the specified frame has just appeared, see if we need
// to play a sound or whatever
void CheckViewFrame (int view, int loop, int frame) {
  if (psp_is_old_datafile)
  {
    if (views[view].loops[loop].frames[frame].sound > 0)
    {
      if (views[view].loops[loop].frames[frame].sound < 0x10000000)
      {
        ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, views[view].loops[loop].frames[frame].sound);
        if (clip)
          views[view].loops[loop].frames[frame].sound = clip->id + 0x10000000;
        else
        {
          views[view].loops[loop].frames[frame].sound = 0;
          return;
        }
      }
      play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound - 0x10000000);
    }
  }
  else
  {
    if (views[view].loops[loop].frames[frame].sound >= 0) {
      // play this sound (eg. footstep)
      play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound);
    }
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
  if ((bitmap_color_depth(piccy) < screen_depth) 
#if defined(IOS_VERSION) || defined(ANDROID_VERSION) || defined(WINDOWS_VERSION)
    || ((bmp_bpp(abuf) < screen_depth) && (psp_gfx_renderer > 0)) // Fix for corrupted speechbox outlines with the OGL driver
#endif
    ) {
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

    // Trying to make the order of equal elements reproducible across
    // different libc implementations here
#if defined WINDOWS_VERSION
    return -1;
#else
    return 1;
#endif
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
      update_polled_stuff_if_runtime();
      roomBackgroundBmp = gfxDriver->CreateDDBFromBitmap(thisroom.ebscene[play.bg_frame], false, true);

      if ((walkBehindMethod == DrawAsSeparateSprite) && (walkBehindsCachedForBgNum != play.bg_frame))
      {
        update_walk_behind_images();
      }
    }
    else if (current_background_is_dirty)
    {
      update_polled_stuff_if_runtime();
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
#ifndef PSP_NO_MOD_PLAYBACK
  if (opts.mod_player)
    remove_mod_player();
#endif

  // Quit the sound thread.
  update_mp3_thread_running = false;

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

#ifndef PSP_NO_MOD_PLAYBACK
  if (opts.mod_player)
    remove_mod_player();
#endif
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
    update_polled_stuff_if_runtime();
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

    update_polled_stuff_if_runtime();

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



// Create the missing game.audioClips data structure for 3.1.x games.
// This is done by going through the data files and adding all music*.*
// and sound*.* files to it.
extern "C" {
  extern int clibGetNumFiles();
  extern char *clibGetFileName(int index);
}

void BuildAudioClipArray()
{
  char temp_name[30];
  int temp_number;
  char temp_extension[10];

  int number_of_files = clibGetNumFiles();
  int i;
  for (i = 0; i < number_of_files; i++)
  {
    if (sscanf(clibGetFileName(i), "%5s%d.%3s", temp_name, &temp_number, temp_extension) == 3)
    {
      if (stricmp(temp_name, "music") == 0)
      {
        sprintf(game.audioClips[game.audioClipCount].scriptName, "aMusic%d", temp_number);
        sprintf(game.audioClips[game.audioClipCount].fileName, "music%d.%s", temp_number, temp_extension);
        game.audioClips[game.audioClipCount].bundlingType = (stricmp(temp_extension, "mid") == 0) ? AUCL_BUNDLE_EXE : AUCL_BUNDLE_VOX;
        game.audioClips[game.audioClipCount].type = 2;
        game.audioClips[game.audioClipCount].defaultRepeat = 1;
      }
      else if (stricmp(temp_name, "sound") == 0)
      {
        sprintf(game.audioClips[game.audioClipCount].scriptName, "aSound%d", temp_number);
        sprintf(game.audioClips[game.audioClipCount].fileName, "sound%d.%s", temp_number, temp_extension);
        game.audioClips[game.audioClipCount].bundlingType = AUCL_BUNDLE_EXE;
        game.audioClips[game.audioClipCount].type = 3;
      }
      else
      {
        continue;
      }

      game.audioClips[game.audioClipCount].defaultVolume = 100;
      game.audioClips[game.audioClipCount].defaultPriority = 50;
      game.audioClips[game.audioClipCount].id = game.audioClipCount;

      if (stricmp(temp_extension, "mp3") == 0)
        game.audioClips[game.audioClipCount].fileType = eAudioFileMP3;
      else if (stricmp(temp_extension, "wav") == 0)
        game.audioClips[game.audioClipCount].fileType = eAudioFileWAV;
      else if (stricmp(temp_extension, "voc") == 0)
        game.audioClips[game.audioClipCount].fileType = eAudioFileVOC;
      else if (stricmp(temp_extension, "mid") == 0)
        game.audioClips[game.audioClipCount].fileType = eAudioFileMIDI;
      else if ((stricmp(temp_extension, "mod") == 0) || (stricmp(temp_extension, "xm") == 0)
        || (stricmp(temp_extension, "s3m") == 0) || (stricmp(temp_extension, "it") == 0))
        game.audioClips[game.audioClipCount].fileType = eAudioFileMOD;
      else if (stricmp(temp_extension, "ogg") == 0)
        game.audioClips[game.audioClipCount].fileType = eAudioFileOGG;

      game.audioClipCount++;
    }
  }
}


/*

 Game data versions and changes:
 -------------------------------

 12 : 2.3 + 2.4

 Versions above are incompatible at the moment.

 19 : 2.5.1
 22 : 2.5.5

 Variable number of sprites.
 24 : 2.5.6
 25 : 2.6.0

 Encrypted global messages and dialogs.
 26 : 2.6.1
 27 : 2.6.2

 Script modules. Fixes bug in the inventory display.
 31 : 2.7.0
 32 : 2.7.2

 Interactions are now scripts. The number for "not set" changed from 0 to -1 for
 a lot of variables (views, sounds).
 37 : 3.0 + 3.1.0

 Dialogs are now scripts. New character animation speed.
 39 : 3.1.1
 40 : 3.1.2

 Audio clips
 41 : 3.2.0
 42 : 3.2.1

*/


int load_game_file() {
  int ee, bb;
  char teststr[31];

  game_paused = 0;  // reset the game paused flag
  ifacepopped = -1;

  FILE*iii = clibfopen("game28.dta", "rb"); // 3.x data file name
  if (iii==NULL) {
    iii = clibfopen("ac2game.dta", "rb"); // 2.x data file name
    if (iii==NULL)
      return -1;
  }

  our_eip=-18;
  setup_script_exports();

  our_eip=-16;

  teststr[30]=0;
  fread(&teststr[0],30,1,iii);
  int filever=getw(iii);

  if (filever < 42) {
    // Allow loading of 2.x+ datafiles
    if (filever < 18) // < 2.5.0
    {
      fclose(iii);
      return -2;
    }
    psp_is_old_datafile = 1;
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

#ifdef ALLEGRO_BIG_ENDIAN
  GameSetupStructBase *gameBase = (GameSetupStructBase *) &game;
  gameBase->ReadFromFile(iii);
#else
  fread(&game, sizeof (GameSetupStructBase), 1, iii);
#endif

  if (filever <= 37) // <= 3.1
  {
    // Fix animation speed for old formats
    game.options[OPT_OLDTALKANIMSPD] = 1;
  }

  if (game.numfonts > MAX_FONTS)
    quit("!This game requires a newer version of AGS. Too many fonts for this version to handle.");

  if (filever > 32) // only 3.x
  {
    fread(&game.guid[0], 1, MAX_GUID_LENGTH, iii);
    fread(&game.saveGameFileExtension[0], 1, MAX_SG_EXT_LENGTH, iii);
    fread(&game.saveGameFolderName[0], 1, MAX_SG_FOLDER_LEN, iii);

    if (game.saveGameFileExtension[0] != 0)
      sprintf(saveGameSuffix, ".%s", game.saveGameFileExtension);
    else
      saveGameSuffix[0] = 0;
  }

  fread(&game.fontflags[0], 1, game.numfonts, iii);
  fread(&game.fontoutline[0], 1, game.numfonts, iii);

#if !defined(WINDOWS_VERSION)
  // Outline fonts are misaligned on platforms other than Windows
  int i;
  for (i = 0; i < MAX_FONTS; i++)
  {
    if (game.fontoutline[i] >= 0)
      game.fontoutline[i] = FONT_OUTLINE_AUTO;
  }
#endif

  int numToRead;
  if (filever < 24)
    numToRead = 6000; // Fixed number of sprites on < 2.56
  else
    numToRead = getw(iii);

  if (numToRead > MAX_SPRITES) {
    quit("Too many sprites; need newer AGS version");
  }
  fread(&game.spriteflags[0], 1, numToRead, iii);
#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < game.numinvitems; ++iteratorCount)
  {
      game.invinfo[iteratorCount].ReadFromFile(iii);
  }
#else
  fread(&game.invinfo[0], sizeof(InventoryItemInfo), game.numinvitems, iii);
#endif

  if (game.numcursors > MAX_CURSOR)
    quit("Too many cursors: need newer AGS version");
#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < game.numcursors; ++iteratorCount)
  {
      game.mcurs[iteratorCount].ReadFromFile(iii);
  }
#else
  fread(&game.mcurs[0], sizeof(MouseCursor), game.numcursors, iii);
#endif

  if (filever <= 32) // 2.x
  {
    // Change cursor.view from 0 to -1 for non-animating cursors.
    int i;
    for (i = 0; i < game.numcursors; i++)
    {
      if (game.mcurs[i].view == 0)
        game.mcurs[i].view = -1;
    }
  }

  numGlobalVars = 0;

  if (filever > 32) // 3.x
  {
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
  }
  else // 2.x
  {
    game.intrChar = new NewInteraction*[game.numcharacters];

    for (bb = 0; bb < game.numcharacters; bb++) {
      game.intrChar[bb] = deserialize_new_interaction(iii);
    }
    for (bb = 0; bb < game.numinvitems; bb++) {
      game.intrInv[bb] = deserialize_new_interaction(iii);
    }

    numGlobalVars = getw(iii);
    fread(globalvars, sizeof(InteractionVariable), numGlobalVars, iii);
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

  if (filever > 37) // 3.1.1+ dialog script
  {
    dialogScriptsScript = fread_script(iii);
    if (dialogScriptsScript == NULL)
      quit("Dialog scripts load failed; need newer version?");
  }
  else // 2.x and < 3.1.1 dialog
  {
    dialogScriptsScript = NULL;
  }

  if (filever >= 31) // 2.7.0+ script modules
  {
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
  }
  else
  {
    numScriptModules = 0;
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
  int iteratorCount = 0;

  if (filever > 32) // 3.x views
  {
    for (int iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
    {
      views[iteratorCount].ReadFromFile(iii);
    }
  }
  else // 2.x views
  {
    ViewStruct272* oldv = (ViewStruct272*)calloc(game.numviews, sizeof(ViewStruct272));
    for (iteratorCount = 0; iteratorCount < game.numviews; ++iteratorCount)
    {
      oldv[iteratorCount].ReadFromFile(iii);
    }
    Convert272ViewsToNew(game.numviews, oldv, views);
    free(oldv);
  }

  our_eip=-14;

  if (filever <= 19) // <= 2.1 skip unknown data
  {
    int count = getw(iii);
    fseek(iii, count * 0x204, SEEK_CUR);
  }

  game.chars=(CharacterInfo*)calloc(1,sizeof(CharacterInfo)*game.numcharacters+5);
#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < game.numcharacters; ++iteratorCount)
  {
      game.chars[iteratorCount].ReadFromFile(iii);
  }
#else
  fread(&game.chars[0],sizeof(CharacterInfo),game.numcharacters,iii);  
#endif
  charcache = (CharacterCache*)calloc(1,sizeof(CharacterCache)*game.numcharacters+5);

  if (filever <= 32) // fixup charakter script names for 2.x (EGO -> cEgo)
  {
    char tempbuffer[200];
    for (int i = 0; i < game.numcharacters; i++)
    {
      memset(tempbuffer, 0, 200);
      tempbuffer[0] = 'c';
      tempbuffer[1] = game.chars[i].scrname[0];
      strcat(&tempbuffer[2], strlwr(&game.chars[i].scrname[1]));
      strcpy(game.chars[i].scrname, tempbuffer);
    }
  }

  if (filever <= 37) // fix character walk speed for < 3.1.1
  {
    for (int i = 0; i < game.numcharacters; i++)
    {
      if (game.options[OPT_ANTIGLIDE])
        game.chars[i].flags |= CHF_ANTIGLIDE;
    }
  }

  if (filever > 19) // > 2.1
    fread(&game.lipSyncFrameLetters[0][0], MAXLIPSYNCFRAMES, 50, iii);

  for (ee=0;ee<MAXGLOBALMES;ee++) {
    if (game.messages[ee]==NULL) continue;
    game.messages[ee]=(char*)malloc(500);

    if (filever < 26) // Global messages are not encrypted on < 2.61
    {
      char* nextchar = game.messages[ee];
      
      while (1)
      {
        *nextchar = fgetc(iii);
        if (*nextchar == 0)
          break;
        nextchar++;
      }
    }
    else
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

#ifdef ALLEGRO_BIG_ENDIAN
  for (int iteratorCount = 0; iteratorCount < game.numdialog; ++iteratorCount)
  {
      dialog[iteratorCount].ReadFromFile(iii);
  }
#else
  fread(&dialog[0],sizeof(DialogTopic),game.numdialog,iii);  
#endif

  if (filever <= 37) // Dialog script
  {
    old_dialog_scripts = (unsigned char**)malloc(game.numdialog * sizeof(unsigned char**));

    int i;
    for (i = 0; i < game.numdialog; i++)
    {
      old_dialog_scripts[i] = (unsigned char*)malloc(dialog[i].codesize);
      fread(old_dialog_scripts[i], dialog[i].codesize, 1, iii);
      
      // Skip encrypted text script
      unsigned int script_size = getw(iii);
      fseek(iii, script_size, SEEK_CUR);
    }

    // Read the dialog lines
    old_speech_lines = (char**)malloc(10000 * sizeof(char**));
    i = 0;

    if (filever <= 25)
    {
      // Plain text on <= 2.60
      char buffer[1000];
      bool end_reached = false;

      while (!end_reached)
      {
        char* nextchar = buffer;
        
        while (1)
        {
          *nextchar = fgetc(iii);
          if (*nextchar == 0)
            break;

          if ((unsigned char)*nextchar == 0xEF)
          {
            end_reached = true;
            fseek(iii, -1, SEEK_CUR);
            break;
          }

          nextchar++;
        }

        if (end_reached)
          break;

        old_speech_lines[i] = (char*)malloc(strlen(buffer) + 1);
        strcpy(old_speech_lines[i], buffer);
        i++;
      }
    }
    else
    {
      // Encrypted text on > 2.60
      while (1)
      {
        unsigned int newlen = getw(iii);
        if (newlen == 0xCAFEBEEF)  // GUI magic
        {
          fseek(iii, -4, SEEK_CUR);
          break;
        }
        
        old_speech_lines[i] = (char*)malloc(newlen + 1);
        fread(old_speech_lines[i], newlen, 1, iii);
        old_speech_lines[i][newlen] = 0;
        decrypt_text(old_speech_lines[i]);

        i++;
      }
    }
    old_speech_lines = (char**)realloc(old_speech_lines, i * sizeof(char**));
  }

  read_gui(iii,guis,&game, &guis);

  for (bb = 0; bb < numguilabels; bb++) {
    // labels are not clickable by default
    guilabels[bb].SetClickable(false);
  }

  play.gui_draw_order = (int*)calloc(game.numgui * sizeof(int), 1);

  if (filever >= 25) // >= 2.60
  {
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
  }

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
    // Create game.soundClips and game.audioClipTypes structures.
    game.audioClipCount = 1000;
    game.audioClipTypeCount = 4;

    game.audioClipTypes = (AudioClipType*)malloc(game.audioClipTypeCount * sizeof(AudioClipType));
    memset(game.audioClipTypes, 0, game.audioClipTypeCount * sizeof(AudioClipType));

    game.audioClips = (ScriptAudioClip*)malloc(game.audioClipCount * sizeof(ScriptAudioClip));
    memset(game.audioClips, 0, game.audioClipCount * sizeof(ScriptAudioClip));

    int i;
    for (i = 0; i < 4; i++)
    {
      game.audioClipTypes[i].reservedChannels = 1;
      game.audioClipTypes[i].id = i;
      game.audioClipTypes[i].volume_reduction_while_speech_playing = 10;
    }
    game.audioClipTypes[3].reservedChannels = 0;


    game.audioClipCount = 0;

    if (csetlib("music.vox", "") == 0)
      BuildAudioClipArray();

    csetlib(game_file_name, "");
    BuildAudioClipArray();

    game.audioClips = (ScriptAudioClip*)realloc(game.audioClips, game.audioClipCount * sizeof(ScriptAudioClip));


    play.score_sound = -1;
    if (game.options[OPT_SCORESOUND] > 0)
    {
      ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, game.options[OPT_SCORESOUND]);
      if (clip)
        play.score_sound = clip->id;
      else
        play.score_sound = -1;
    }
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

void update_polled_stuff_if_runtime()
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
  update_polled_stuff_if_runtime ();

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
    if (loaded_game_file_version < 25) // < 2.60
    {
      // Color 0 wrongly shows as transparent instead of black
      if (iep->bgcol == 0)
        iep->bgcol = 16;
    }

    if (iep->bgcol >= 0) wsetcolor(iep->bgcol);
    else wsetcolor(0); // black backrgnd behind picture

    if (iep->bgcol > 0)
      wbar(xx1,yy1,xx2,yy2);

    int leftRightWidth = spritewidth[get_but_pic(iep,4)];
    int topBottomHeight = spriteheight[get_but_pic(iep,6)];
    if (iep->bgpic>0) {
      if ((loaded_game_file_version <= 32) // 2.xx
        && (spriteset[iep->bgpic]->w == 1)
        && (spriteset[iep->bgpic]->h == 1) 
        && (*((unsigned int*)spriteset[iep->bgpic]->dat) == 0x00FF00FF))
      {
        // Don't draw fully transparent dummy GUI backgrounds
      }
      else
      {
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
            wputblock(bgoffsx, bgoffsy, spriteset[iep->bgpic], 0);
            bgoffsy += spriteheight[iep->bgpic];
          }
          bgoffsx += spritewidth[iep->bgpic];
        }
        // return to normal clipping rectangle
        set_clip(abuf, 0, 0, abuf->w - 1, abuf->h - 1);
      }
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
  // Anti-aliasing produces unreadable text on all non-Windows platforms.
#if !defined(WINDOWS_VERSION)
  return false;
#else
  return (game.options[OPT_ANTIALIASFONTS] != 0);
#endif
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

  // AGS 2.x: If the screen is faded out, fade in again when displaying a message box.
  if (!asspch && (loaded_game_file_version <= 32))
    play.screen_is_faded_out = 0;

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
        update_polled_stuff_if_runtime();
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

    // Don't reset the loop in 2.x games
    if (loaded_game_file_version > 32)
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

  update_polled_stuff_if_runtime();  // this operation can be slow so stop music skipping
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
  update_polled_stuff_if_runtime();  // this operation can be slow so stop music skipping
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

  if (loaded_game_file_version > 32) // Skip check on 2.x
  {
    if (views[viw].loops[objs[obn].loop].numFrames == 0) 
      quit("!SetObjectFrame: specified loop has no frames");
  }

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

  value = cos(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Sin(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = sin(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Tan(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = tan(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcCos(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = acos(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcSin(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = asin(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcTan(SCRIPT_FLOAT(value)) {
  INIT_SCRIPT_FLOAT(value);

  value = atan(value);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_ArcTan2(SCRIPT_FLOAT(yval), SCRIPT_FLOAT(xval)) {
  INIT_SCRIPT_FLOAT(yval);
  INIT_SCRIPT_FLOAT(xval);

  float value = atan2(yval, xval);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Log(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = log(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Log10(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = ::log10(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Exp(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = exp(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Cosh(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = cosh(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Sinh(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = sinh(num);

  RETURN_FLOAT(value);
}

FLOAT_RETURN_TYPE Math_Tanh(SCRIPT_FLOAT(num)) {
  INIT_SCRIPT_FLOAT(num);

  float value = tanh(num);

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

  // AGS 2.x: If the screen is faded out, fade in again when playing a movie.
  if (loaded_game_file_version <= 32)
    play.screen_is_faded_out = 0;

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
  // Disable framedrop because it can lead to the PSP not playing the video at all.
  apeg_enable_framedrop(psp_video_framedrop);
  update_polled_stuff_if_runtime();

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

  update_polled_stuff_if_runtime();

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

  // AGS <= 2.61 uses MoveObject with spp=-1 internally instead of SetObjectPosition
  if ((loaded_game_file_version <= 26) && (spee == -1))
  {
    objs[objj].x = tox;
    objs[objj].y = toy;
    return;
  }

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


int __Rand(int upto) {
  upto++;
  if (upto < 1)
    quit("!Random: invalid parameter passed -- must be at least 0.");
  return rand()%upto;
  }

void GiveScore(int amnt) 
{
  guis_need_update = 1;
  play.score += amnt;

  if ((amnt > 0) && (play.score_sound >= 0))
    play_audio_clip_by_index(play.score_sound);

  run_on_event (GE_GOT_SCORE, amnt);
}


char rbuffer[200];

extern const char* ccGetSectionNameAtOffs(ccScript *scri, long offs);


//char datname[80]="ac.clb";
char ac_conf_file_defname[MAX_PATH] = "acsetup.cfg";
char *ac_config_file = &ac_conf_file_defname[0];
char conffilebuf[512];


//char*ac_default_header=NULL,*temphdr=NULL;
char ac_default_header[15000],temphdr[10000];
