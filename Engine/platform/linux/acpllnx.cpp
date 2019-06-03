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
String LinuxOutputDirectory;

struct AGSLinux : AGSPlatformDriver
{
  virtual void Delay(int millis) override;
  virtual void DisplayAlert(const char*, ...) override;
  virtual const char *GetUserSavedgamesDirectory() override;
  virtual const char *GetUserConfigDirectory() override;
  virtual const char *GetUserGlobalConfigDirectory() override;
  virtual const char *GetAppOutputDirectory() override;
  virtual unsigned long GetDiskFreeSpaceMB() override;
  virtual const char* GetNoMouseErrorString() override;
  virtual bool IsMouseControlSupported(bool windowed) override;
  virtual const char* GetAllegroFailUserHint() override;
  virtual eScriptSystemOSID GetSystemOSID() override;
  virtual void PlayVideo(const char* name, int skip, int flags) override;
  virtual void PostAllegroExit() override;
  virtual void SetGameWindowIcon() override;
  virtual bool LockMouseToWindow() override;
  virtual void UnlockMouse() override;
};


void AGSLinux::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s\n", displbuf);
}

size_t BuildXDGPath(char *destPath, size_t destSize)
{
  // Check to see if XDG_DATA_HOME is set in the enviroment
  const char* home_dir = getenv("XDG_DATA_HOME");
  size_t l = 0;
  if (home_dir)
  {
    l = snprintf(destPath, destSize, "%s", home_dir);
  }
  else
  {
    // No evironment variable, so we fall back to home dir in /etc/passwd
    struct passwd *p = getpwuid(getuid());
    l = snprintf(destPath, destSize, "%s/.local", p->pw_dir);
    if (mkdir(destPath, 0755) != 0 && errno != EEXIST)
      return 0;
    l += snprintf(destPath + l, destSize - l, "/share");
    if (mkdir(destPath, 0755) != 0 && errno != EEXIST)
      return 0;
  }
  l += snprintf(destPath + l, destSize - l, "/ags");
  if (mkdir(destPath, 0755) != 0 && errno != EEXIST)
    return 0;
  return l;
}

void DetermineAppOutputDirectory()
{
  if (!LinuxOutputDirectory.IsEmpty())
  {
    return;
  }

  char xdg_path[256];
  if (BuildXDGPath(xdg_path, sizeof(xdg_path)) > 0)
    LinuxOutputDirectory = xdg_path;
  else
    LinuxOutputDirectory = "/tmp";
}

const char *AGSLinux::GetUserSavedgamesDirectory()
{
  DetermineAppOutputDirectory();
  return LinuxOutputDirectory;
}

const char *AGSLinux::GetUserConfigDirectory()
{
  return GetUserSavedgamesDirectory();
}

const char *AGSLinux::GetUserGlobalConfigDirectory()
{
  return GetUserSavedgamesDirectory();
}

const char *AGSLinux::GetAppOutputDirectory()
{
  DetermineAppOutputDirectory();
  return LinuxOutputDirectory;
}

unsigned long AGSLinux::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSLinux::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

bool AGSLinux::IsMouseControlSupported(bool windowed)
{
  return true; // supported for both fullscreen and windowed modes
}

const char* AGSLinux::GetAllegroFailUserHint()
{
  return "Make sure you have latest version of Allegro 4 libraries installed, and X server is running.";
}

eScriptSystemOSID AGSLinux::GetSystemOSID() {
  return eOS_Linux;
}

void AGSLinux::PostAllegroExit() {
  // do nothing
}

void AGSLinux::SetGameWindowIcon() {
  // do nothing
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
