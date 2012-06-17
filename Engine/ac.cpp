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
//#include <acdebug.h>
#include "acdebug/ac_debug.h"


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





int  sc_GetTime(int whatti);
void quitprintf(char*texx, ...);
void replace_macro_tokens(char*,char*);
void wouttext_reverseifnecessary(int x, int y, int font, char *text);
void SetGameSpeed(int newspd);
void SetMultitasking(int mode);

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
//#include "acsound.h"

#include "acgfx/ac_gfxfilters.h"
#include "acaudio/ac_sound.h"


// **** TYPES ****







struct TempEip {
  int oldval;
  TempEip (int newval) {
    oldval = our_eip;
    our_eip = newval;
  }
  ~TempEip () { our_eip = oldval; }
};








// **** GLOBALS ****
char *music_file;
char *speech_file;
WCHAR directoryPathBuffer[MAX_PATH];

/*extern int get_route_composition();
extern int routex1;*/
extern char*scripttempn;










int numSoundChannels = 8;










char *heightTestString = "ZHwypgfjqhkilIK";






long t1;  // timer for FPS



//int abort_all_conditions=0;




char alpha_blend_cursor = 0;




int use_extra_sound_offset = 0;








PluginObjectReader pluginReaders[MAX_PLUGIN_OBJECT_READERS];
int numPluginReaders = 0;













int num_scripts=0;

int user_disabled_for=0,user_disabled_data=0,user_disabled_data2=0;
int user_disabled_data3=0;

// Sierra-style speech settings
int face_talking=-1,facetalkview=0,facetalkwait=0,facetalkframe=0;
int facetalkloop=0, facetalkrepeat = 0, facetalkAllowBlink = 1;
int facetalkBlinkLoop = 0;
CharacterInfo *facetalkchar = NULL;




// set to 0 once successful
int working_gfx_mode_status = -1;




int gs_to_newroom=-1;



int scaddr;









int need_to_stop_cd=0;
int debug_15bit_mode = 0, debug_24bit_mode = 0;

int convert_16bit_bgr = 0;
int mouse_z_was = 0;






char ac_engine_copyright[]="Adventure Game Studio engine & tools (c) 1999-2000 by Chris Jones.";








//int current_music=0;




int in_inv_screen = 0, inv_screen_newroom = -1;

int new_room_flags=0;



int post_script_cleanup_stack = 0;




RGB_MAP rgb_table;  // for 256-col antialiasing



int scrlockWasDown = 0;




int pluginsWantingDebugHooks = 0;

const char *replayTempFile = "~replay.tmp";



//  *** FUNCTIONS ****

int  Overlay_GetValid(ScriptOverlay *scover);
void mainloop(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void set_mouse_cursor(int);
void set_default_cursor();
int  run_text_script(ccInstance*,char*);
int  run_text_script_2iparam(ccInstance*,char*,int,int);
int  run_text_script_iparam(ccInstance*,char*,int);
//void run_graph_script(int);
//void run_event_block(EventBlock*,int,int=-1, int=-1);

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


void render_graphics(IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
int  wait_loop_still_valid();
SOUNDCLIP *load_music_from_disk(int mnum, bool repeat);
void play_new_music(int mnum, SOUNDCLIP *music);
int GetGameSpeed();
int check_for_messages_from_editor();
int show_dialog_options(int dlgnum, int sayChosenOption, bool runGameLoopsInBackground);




  



//#define getch() my_readkey()
//#undef kbhit
//#define kbhit keypressed
// END KEYBOARD HANDLER









































extern const char* ccGetSectionNameAtOffs(ccScript *scri, long offs);


//char datname[80]="ac.clb";
char ac_conf_file_defname[MAX_PATH] = "acsetup.cfg";
char *ac_config_file = &ac_conf_file_defname[0];
char conffilebuf[512];


//char*ac_default_header=NULL,*temphdr=NULL;
char ac_default_header[15000],temphdr[10000];
