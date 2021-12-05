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

#include "core/platform.h"

#if AGS_PLATFORM_OS_LINUX

// ********* LINUX PLACEHOLDER DRIVER *********

#include <stdio.h>
#include <allegro.h>
#include <xalleg.h>
#include "ac/runtime_defines.h"
#include "gfx/gfxdefines.h"
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
FSLocation CommonDataDirectory;
FSLocation UserDataDirectory;

struct AGSLinux : AGSPlatformDriver {

  int  CDPlayerCommand(int cmdd, int datt) override;
  void DisplayAlert(const char*, ...) override;
  FSLocation GetAllUsersDataDirectory() override;
  FSLocation GetUserSavedgamesDirectory() override;
  FSLocation GetUserConfigDirectory() override;
  FSLocation GetUserGlobalConfigDirectory() override;
  FSLocation GetAppOutputDirectory() override;
  unsigned long GetDiskFreeSpaceMB() override;
  const char* GetNoMouseErrorString() override;
  const char* GetAllegroFailUserHint() override;
  eScriptSystemOSID GetSystemOSID() override;
  int  InitializeCDPlayer() override;
  void PostAllegroExit() override;
  void SetGameWindowIcon() override;
  void ShutdownCDPlayer() override;
  bool LockMouseToWindow() override;
  void UnlockMouse() override;
  void GetSystemDisplayModes(std::vector<Engine::DisplayMode> &dms) override;
};


int AGSLinux::CDPlayerCommand(int cmdd, int datt) {
  return cd_player_control(cmdd, datt);
}

void AGSLinux::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  if (_logToStdErr)
    fprintf(stderr, "%s\n", displbuf);
  else
    fprintf(stdout, "%s\n", displbuf);
}

static FSLocation BuildXDGPath()
{
  // Check to see if XDG_DATA_HOME is set in the enviroment
  const char* home_dir = getenv("XDG_DATA_HOME");
  if (home_dir)
    return FSLocation(home_dir);
  // No evironment variable, so we fall back to home dir in /etc/passwd
  struct passwd *p = getpwuid(getuid());
  if (p)
    return FSLocation(p->pw_dir).Concat(".local/share");
  return FSLocation();
}

void DetermineDataDirectories()
{
  if (UserDataDirectory.IsValid())
    return;
  FSLocation fsloc = BuildXDGPath();
  if (!fsloc.IsValid())
    fsloc = FSLocation("/tmp");
  UserDataDirectory = fsloc.Concat("ags");
  CommonDataDirectory = fsloc.Concat("ags-common");
}

FSLocation AGSLinux::GetAllUsersDataDirectory()
{
  DetermineDataDirectories();
  return CommonDataDirectory;
}

FSLocation AGSLinux::GetUserSavedgamesDirectory()
{
  DetermineDataDirectories();
  return UserDataDirectory;
}

FSLocation AGSLinux::GetUserConfigDirectory()
{
  return GetUserSavedgamesDirectory();
}

FSLocation AGSLinux::GetUserGlobalConfigDirectory()
{
  return GetUserSavedgamesDirectory();
}

FSLocation AGSLinux::GetAppOutputDirectory()
{
  DetermineDataDirectories();
  return UserDataDirectory;
}

unsigned long AGSLinux::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSLinux::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

const char* AGSLinux::GetAllegroFailUserHint()
{
  return "Make sure you have latest version of Allegro 4 libraries installed, and X server is running.";
}

eScriptSystemOSID AGSLinux::GetSystemOSID() {
  return eOS_Linux;
}

int AGSLinux::InitializeCDPlayer() {
  return cd_player_init();
}

void AGSLinux::PostAllegroExit() {
  // do nothing
}

void AGSLinux::SetGameWindowIcon() {
  // do nothing
}

void AGSLinux::ShutdownCDPlayer() {
  cd_exit();
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == nullptr)
    instance = new AGSLinux();
  return instance;
}

bool AGSLinux::LockMouseToWindow()
{
    return XGrabPointer(_xwin.display, _xwin.window, False,
        PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
        GrabModeAsync, GrabModeAsync, _xwin.window, None, CurrentTime) == GrabSuccess;
}

void AGSLinux::UnlockMouse()
{
    XUngrabPointer(_xwin.display, CurrentTime);
}

void AGSLinux::GetSystemDisplayModes(std::vector<Engine::DisplayMode> &dms)
{
    dms.clear();
    GFX_MODE_LIST *gmlist = get_gfx_mode_list(GFX_XWINDOWS_FULLSCREEN);
    for (int i = 0; i < gmlist->num_modes; ++i)
    {
        const GFX_MODE &m = gmlist->mode[i];
        dms.push_back(Engine::DisplayMode(Engine::GraphicResolution(m.width, m.height, m.bpp)));
    }
    destroy_gfx_mode_list(gmlist);
}

#endif
