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

struct AGSLinux : AGSPlatformDriver {

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
  virtual void ReplaceSpecialPaths(const char *sourcePath, char *destPath, size_t destSize);
};


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

size_t BuildXDGPath(char *destPath, size_t destSize)
{
  // Check to see if XDG_DATA_HOME is set in the enviroment
  const char* home_dir = getenv("XDG_DATA_HOME");
  if (home_dir)
  {
    return snprintf(destPath, destSize, "%s", home_dir);
  }
  else
  {
    // No evironment variable, so we fall back to home dir in /etc/passwd
    struct passwd *p = getpwuid(getuid());
    size_t l = snprintf(destPath, destSize, "%s/.local", p->pw_dir);
    mkdir(destPath, 0755);
    l += snprintf(destPath + l, destSize - l, "/share");
    mkdir(destPath, 0755);
    return l;
  }
}

void DetermineAppOutputDirectory()
{
  if (!LinuxOutputDirectory.IsEmpty())
  {
    return;
  }

  bool log_to_home_dir = false;
  char xdg_path[256];
  if (BuildXDGPath(xdg_path, sizeof(xdg_path)) > 0)
  {
    LinuxOutputDirectory = xdg_path;
    LinuxOutputDirectory.Append("/ags");
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

void AGSLinux::ReplaceSpecialPaths(const char *sourcePath, char *destPath, size_t destSize) {
  
  static const char *special_paths[3] = {"$MYDOCS$", "$SAVEGAMEDIR$", "$APPDATADIR$"};
  static const size_t sp_path_len[3] = {8, 13, 12};
  int use_sp_path = -1;
  for (int i = 0; i < 3; ++i)
  {
    if (strncasecmp(sourcePath, special_paths[i], sp_path_len[i]) == 0)
    {
      use_sp_path = i;
      break;
    }
  }
  
  if (use_sp_path >= 0)
  {
    size_t l = BuildXDGPath(destPath, destSize);
    snprintf(destPath + l, destSize - l, "%s", sourcePath + sp_path_len[use_sp_path]);
    mkdir(destPath, 0755);
  }
  else
  {
    snprintf(destPath, destSize, "%s", sourcePath);
  }
}
