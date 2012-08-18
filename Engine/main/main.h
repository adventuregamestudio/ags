
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__MAIN_H
#define __AGS_EE_MAIN__MAIN_H

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

#if defined(WINDOWS_VERSION) && !defined(_DEBUG)
#define USE_CUSTOM_EXCEPTION_HANDLER
#endif

//=============================================================================

#ifdef WINDOWS_VERSION

extern int wArgc;
extern LPWSTR *wArgv;

#else

#define wArgc argc
#define wArgv argv
#define LPWSTR char*
#define LPCWSTR const char*
#define WCHAR char
#define StrCpyW strcpy

#endif

// Startup flags, set from parameters to engine
extern int datafile_argv, change_to_game_dir, force_window;
extern int override_start_room, force_16bit;
extern bool justRegisterGame;
extern bool justUnRegisterGame;
extern const char *loadSaveGameOnStartup;

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

#endif // __AGS_EE_MAIN__MAIN_H
