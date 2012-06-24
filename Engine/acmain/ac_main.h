#ifndef __AC_MAIN_H
#define __AC_MAIN_H

#include "ac/characterinfo.h"

void do_main_cycle(int untilwhat,int daaa);
void can_run_delayed_command();
void mainloop(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
void show_preload () ;
void init_game_settings();
void start_game();

int do_movelist_move(short*mlnum,int*xx,int*yy);
int wait_loop_still_valid();

int initialize_engine(int argc,char*argv[]);
int initialize_engine_with_exception_handling(int argc,char*argv[]);
const char *get_engine_version();

void precache_view(int view);

void read_config_file(char *argv0);
void winclosehook();
void initialise_game_file_name();
void atexit_handler();
int check_write_access();
int main_game_loop();
int initialize_graphics_filter(const char *filterID, int width, int height, int colDepth);
void create_gfx_driver();
int switch_to_graphics_mode(int initasx, int initasy, int scrnwid, int scrnhit, int firstDepth, int secondDepth) ;


#if defined(PSP_VERSION)
// PSP: Workaround for sound stuttering. Do sound updates in its own thread.
int update_mp3_thread(SceSize args, void *argp);
#elif (defined(LINUX_VERSION) && !defined(PSP_VERSION)) || defined(MAC_VERSION)
void* update_mp3_thread(void* arg);
#elif defined(WINDOWS_VERSION)
DWORD WINAPI update_mp3_thread(LPVOID lpParam);
#endif

void CreateBlankImage();
void initialize_start_and_play_game(int override_start_room, const char *loadSaveGameOnStartup);
void change_to_directory_of_file(LPCWSTR fileName);

#if defined(WINDOWS_VERSION)
#include <new.h>
extern char tempmsg[100];
extern char*printfworkingspace;
int malloc_fail_handler(size_t amountwanted);
#endif

extern int convert_16bit_bgr;

extern char *music_file;
extern char *speech_file;

extern int face_talking,facetalkview,facetalkwait,facetalkframe;
extern int facetalkloop, facetalkrepeat, facetalkAllowBlink;
extern int facetalkBlinkLoop;
extern CharacterInfo *facetalkchar;

// Startup flags, set from parameters to engine
extern int datafile_argv, change_to_game_dir, force_window;
extern int override_start_room, force_16bit;
extern bool justRegisterGame;
extern bool justUnRegisterGame;
extern const char *loadSaveGameOnStartup;
#ifndef WINDOWS_VERSION
extern char **global_argv;
#endif

extern int use_extra_sound_offset;

#if !defined(IOS_VERSION) && !defined(PSP_VERSION) && !defined(ANDROID_VERSION)
extern int psp_video_framedrop;
extern int psp_audio_enabled;
extern int psp_midi_enabled;
extern int psp_ignore_acsetup_cfg_file;
extern int psp_clear_cache_on_room_change;

extern int psp_midi_preload_patches;
extern int psp_audio_cachesize;
extern char psp_game_file_name[];
extern int psp_gfx_smooth_sprites;
extern char psp_translation[];
#endif

// set to 0 once successful
extern int working_gfx_mode_status;
extern int debug_15bit_mode, debug_24bit_mode;
extern int convert_16bit_bgr;

#ifdef WINDOWS_VERSION
extern int wArgc;
extern LPWSTR *wArgv;
#endif

#endif // __AC_MAIN_H