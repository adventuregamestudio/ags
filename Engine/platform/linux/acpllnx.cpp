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

#if !defined(LINUX_VERSION)
#error This file should only be included on the Linux or BSD build
#endif

// ********* LINUX PLACEHOLDER DRIVER *********

#include <stdio.h>
#include <allegro.h>
#include "gfx/ali3d.h"
#include "ac/runtime_defines.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "util/string.h"
#include <libcda.h>

#include <pwd.h>
#include <sys/stat.h>

using AGS::Common::String;


// Replace the default Allegro icon. The original defintion is in the
// Allegro 4.4 source under "src/x/xwin.c".
#include "icon.xpm"
void* allegro_icon = icon_xpm;
String LinuxOutputDirectory;

#include "binreloc.h"

// PSP variables
int psp_video_framedrop = 1;
int psp_audio_enabled = 1;
int psp_midi_enabled = 1;
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;

int psp_midi_preload_patches = 0;
int psp_audio_cachesize = 10;
char psp_game_file_name[256];
int psp_gfx_smooth_sprites = 1;
char psp_translation[100];

char userAppDataRoot[PATH_MAX];

struct AGSLinux : AGSPlatformDriver {
  AGSLinux();

  virtual int  CDPlayerCommand(int cmdd, int datt);
  virtual void Delay(int millis);
  virtual void DisplayAlert(const char*, ...);
  virtual const char *GetAppOutputDirectory();
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
  virtual void WriteDebugString(const char* texx, ...);
  virtual void ReplaceSpecialPaths(const char*, char*);
};

AGSLinux::AGSLinux() {
  strcpy(psp_game_file_name, "agsgame.dat");
  strcpy(psp_translation, "default");

  BrInitError e = BR_INIT_ERROR_DISABLED;
  if (br_init(&e)) {
    char *exedir = br_find_exe_dir(NULL);
    if (exedir) {
      chdir(exedir);
      free(exedir);
    }
  }

  // force private modules
  setenv("ALLEGRO_MODULES", ".", 1);
}

int AGSLinux::CDPlayerCommand(int cmdd, int datt) {
  return cd_player_control(cmdd, datt);
}

void AGSLinux::WriteDebugString(const char* texx, ...) {
  char displbuf[STD_BUFFER_SIZE] = "AGS: ";
  va_list ap;
  va_start(ap,texx);
  vsprintf(&displbuf[5],texx,ap);
  va_end(ap);
  strcat(displbuf, "\n");

  printf(displbuf);
}

void AGSLinux::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
}

void DetermineAppOutputDirectory()
{
  if (!LinuxOutputDirectory.IsEmpty())
  {
    return;
  }

  bool log_to_home_dir = false;
  const char* home_dir = getenv("HOME");
  if (home_dir)
  {
    LinuxOutputDirectory = home_dir;
    LinuxOutputDirectory.Append("/.ags");
    log_to_home_dir = mkdir(LinuxOutputDirectory, 0755) == 0 || errno == EEXIST;
  }

  if (!log_to_home_dir)
  {
    LinuxOutputDirectory = "/tmp";
  }
}

const char *AGSLinux::GetAppOutputDirectory()
{
  DetermineAppOutputDirectory();
  return LinuxOutputDirectory;
}

void AGSLinux::Delay(int millis) {
  usleep(millis);
}

unsigned long AGSLinux::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSLinux::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSLinux::GetSystemOSID() {
  return eOS_Linux;
}

int AGSLinux::InitializeCDPlayer() {
  return cd_player_init();
}

void AGSLinux::PlayVideo(const char *name, int skip, int flags) {
  // do nothing
}

void AGSLinux::PostAllegroExit() {
  // do nothing
}

int AGSLinux::RunSetup() {
  return 0;
}

void AGSLinux::SetGameWindowIcon() {
  // do nothing
}

void AGSLinux::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
}

void AGSLinux::ShutdownCDPlayer() {
  cd_exit();
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSLinux();
  return instance;
}

void AGSLinux::ReplaceSpecialPaths(const char *sourcePath, char *destPath) {
  // MYDOCS is what is used in acplwin.cpp
  if(strncasecmp(sourcePath, "$MYDOCS$", 8) == 0) {
    struct passwd *p = getpwuid(getuid());
    strcpy(destPath, p->pw_dir);
    strcpy(destPath, "/.ags");
    mkdir(destPath, 0755);
    strcpy(destPath, "/SavedGames");
    mkdir(destPath, 0755);
    strcat(destPath, &sourcePath[8]);
    mkdir(destPath, 0755);
  // SAVEGAMEDIR is what is actually used in ac.cpp
  } else if(strncasecmp(sourcePath, "$SAVEGAMEDIR$", 13) == 0) {
    struct passwd *p = getpwuid(getuid());
    strcpy(destPath, p->pw_dir);
    strcpy(destPath, "/.ags");
    mkdir(destPath, 0755);
    strcpy(destPath, "/SavedGames");
    mkdir(destPath, 0755);
    strcat(destPath, &sourcePath[8]);
    mkdir(destPath, 0755);
  } else if(strncasecmp(sourcePath, "$APPDATADIR$", 12) == 0) {
    struct passwd *p = getpwuid(getuid());
    strcpy(destPath, p->pw_dir);
    strcpy(destPath, "/.ags");
    mkdir(destPath, 0755);
    strcat(destPath, &sourcePath[12]);
    mkdir(destPath, 0755);
  } else {
    strcpy(destPath, sourcePath);
  }
}
