#if !defined(IOS_VERSION)
#error This file should only be included on the iOS build
#endif

#include "acplatfm.h"

#include <allegro.h>
#include "bigend.h"


//int psp_return_to_menu = 1;
int psp_ignore_acsetup_cfg_file = 1;
int psp_clear_cache_on_room_change = 0;
int psp_rotation = 0;
int psp_config_enabled = 0;
char psp_translation[100];
char* psp_translations[100];

// Mouse option from Allegro.
extern int config_mouse_control_mode;


// Graphic options from the Allegro library.
extern int psp_gfx_scaling;
extern int psp_gfx_smoothing;


// Audio options from the Allegro library.
unsigned int psp_audio_samplerate = 44100;
int psp_audio_enabled = 1;
volatile int psp_audio_multithreaded = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;

int psp_gfx_renderer = 0;
int psp_gfx_super_sampling = 0;
int psp_gfx_smooth_sprites = 0;

int psp_debug_write_to_logcat = 0;

int config_mouse_longclick = 0;

extern int display_fps;
extern int want_exit;
extern void PauseGame();
extern void UnPauseGame();
extern int main(int argc,char*argv[]);

char psp_game_file_name[256];
char* psp_game_file_name_pointer = psp_game_file_name;

bool psp_load_latest_savegame = false;
extern char saveGameDirectory[260];
extern const char *loadSaveGameOnStartup;
char lastSaveGameName[200];




struct AGSIOS : AGS32BitOSDriver {

  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual unsigned long GetDiskFreeSpaceMB();
  virtual const char* GetNoMouseErrorString();
  virtual eScriptSystemOSID GetSystemOSID();
  virtual int  InitializeCDPlayer();
  virtual void PlayVideo(const char* name, int skip, int flags);
  virtual void PostAllegroExit();
  virtual int  RunSetup();
  virtual void SetGameWindowIcon();
  virtual void ShutdownCDPlayer();
  virtual void WriteConsole(const char*, ...);
  virtual void ReplaceSpecialPaths(const char*, char*);
  virtual void WriteDebugString(const char* texx, ...);
  
};


void AGSIOS::WriteDebugString(const char* texx, ...)
{
  {
    char displbuf[2000] = "AGS: ";
    va_list ap;
    va_start(ap,texx);
    vsprintf(&displbuf[5],texx,ap);
    va_end(ap);
    printf("%s\n", displbuf);
  }
}


void AGSIOS::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s\n", displbuf);
}


void startEngine(char* filename, char* directory, int loadLastSave)
{
  strcpy(psp_game_file_name, filename);

  // Get the base directory (usually "/sdcard/ags").
  chdir(directory);

  // Reset configuration.
//  ResetConfiguration();

  // Read general configuration.
//  ReadConfiguration(ANDROID_CONFIG_FILENAME, true);

  // Get the games path.
  char path[256];
  strcpy(path, psp_game_file_name);
  int lastindex = strlen(path) - 1;
  while (path[lastindex] != '/')
  {
    path[lastindex] = 0;
    lastindex--;
  }
  chdir(path);
  
  setenv("ULTRADIR", "..", 1);

  // Read game specific configuration.
//  ReadConfiguration(ANDROID_CONFIG_FILENAME, false);

  psp_load_latest_savegame = loadLastSave;

  // Start the engine main function.
  main(1, &psp_game_file_name_pointer);
  
  // Explicitly quit here, otherwise the app will hang forever.
  exit(0);
}




int AGSIOS::CDPlayerCommand(int cmdd, int datt) {
  return 0;//cd_player_control(cmdd, datt);
}



void AGSIOS::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
  
}

void AGSIOS::Delay(int millis) {
  usleep(millis);
}

unsigned long AGSIOS::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSIOS::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSIOS::GetSystemOSID() {
  return eOS_Win;
}

int AGSIOS::InitializeCDPlayer() {
  return 0;//cd_player_init();
}

void AGSIOS::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSIOS::PostAllegroExit() {
  // do nothing
}

int AGSIOS::RunSetup() {
  return 0;
}

void AGSIOS::SetGameWindowIcon() {
  // do nothing
}



void AGSIOS::ShutdownCDPlayer() {
  //cd_exit();
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSIOS();
  return instance;
}

void AGSIOS::ReplaceSpecialPaths(const char *sourcePath, char *destPath) {
  if (strnicmp(sourcePath, "$MYDOCS$", 8) == 0) 
  {
    strcpy(destPath, ".");
    strcat(destPath, &sourcePath[8]);
  }
  else if (strnicmp(sourcePath, "$APPDATADIR$", 12) == 0) 
  {
    strcpy(destPath, ".");
    strcat(destPath, &sourcePath[12]);
  }
  else {
    strcpy(destPath, sourcePath);
  }
}
