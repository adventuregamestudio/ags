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

#if !defined(MAC_VERSION)
#error This file should only be included on the Mac build
#endif

// ********* MacOS PLACEHOLDER DRIVER *********

//#include "util/wgt2allg.h"
//#include "gfx/ali3d.h"
//#include "ac/runtime_defines.h"
//#include "main/config.h"
//#include "plugin/agsplugin.h"
//#include <libcda.h>
//#include <pwd.h>
//#include <sys/stat.h>
#include <unistd.h>
#include "platform/base/agsplatformdriver.h"
#include "util/directory.h"
#include "ac/common.h"

void AGSMacInitPaths(char gamename[256], char appdata[PATH_MAX]);
void AGSMacGetBundleDir(char gamepath[PATH_MAX]);
//bool PlayMovie(char const *name, int skipType);

// PSP variables
int psp_ignore_acsetup_cfg_file = 0;
int psp_clear_cache_on_room_change = 0;

char psp_translation[100];

//unsigned int psp_audio_samplerate = 44100;
int psp_audio_enabled = 1;
int psp_audio_cachesize = 10;
int psp_midi_enabled = 1;
int psp_midi_preload_patches = 0;

int psp_video_framedrop = 0;
int psp_gfx_smooth_sprites = 0;

char psp_game_file_name[256];

int psp_gfx_renderer = 0;
int psp_gfx_scaling = 1;
int psp_gfx_smoothing = 0;
int psp_gfx_super_sampling = 1;

static char libraryApplicationSupport[PATH_MAX];
static char commonDataPath[PATH_MAX];

struct AGSMac : AGSPlatformDriver
{
  AGSMac();
  virtual void DisplayAlert(const char*, ...) override;
  virtual unsigned long GetDiskFreeSpaceMB() override;
  virtual const char* GetNoMouseErrorString() override;
  virtual eScriptSystemOSID GetSystemOSID() override;
  virtual void PlayVideo(const char* name, int skip, int flags) override;
  virtual void PostAllegroExit() override;
  virtual void SetGameWindowIcon() override;
    
  virtual const char *GetUserSavedgamesDirectory() override;
  virtual const char *GetAllUsersDataDirectory() override;
  virtual const char *GetUserConfigDirectory() override;
  virtual const char *GetAppOutputDirectory() override;
  virtual const char *GetIllegalFileChars() override;
};

AGSMac::AGSMac()
{
  AGSMacInitPaths(psp_game_file_name, libraryApplicationSupport);
  
  snprintf(commonDataPath, PATH_MAX, "%s/uk.co.adventuregamestudio", libraryApplicationSupport);
  AGS::Common::Directory::CreateDirectory(commonDataPath);

  strcpy(psp_translation, "default");
}

void AGSMac::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s\n", displbuf);
}

unsigned long AGSMac::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSMac::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSMac::GetSystemOSID() {
  // override performed if `override.os` is set in config.
  return eOS_Mac;
}

void AGSMac::PlayVideo(const char *name, int skip, int flags) {
/*
  if (!PlayMovie(name, skip))
  {
    char useloc[512];
    sprintf(useloc, "%s/%s", get_filename(usetup.data_files_dir), name);
    PlayMovie(useloc, skip);
  }
*/
}

void AGSMac::PostAllegroExit() {
  // do nothing
}

void AGSMac::SetGameWindowIcon() {
  // do nothing
}

const char* AGSMac::GetAllUsersDataDirectory()
{
  return commonDataPath;
}

const char *AGSMac::GetUserSavedgamesDirectory()
{
  return libraryApplicationSupport;
}

const char *AGSMac::GetUserConfigDirectory()
{
  return libraryApplicationSupport;
}

const char *AGSMac::GetAppOutputDirectory()
{
  return commonDataPath;
}

const char *AGSMac::GetIllegalFileChars()
{
  return "\\/:?\"<>|*"; // keep same as Windows so we can sync.
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSMac();
  return instance;
}
