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

#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/runtime_defines.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include <libcda.h>

#include <pwd.h>
#include <sys/stat.h>

bool PlayMovie(char const *name, int skipType);


struct AGSMac : AGSPlatformDriver {

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
  virtual void WriteStdOut(const char*, ...);
  virtual void ReplaceSpecialPaths(const char*, char*);
};

void AGSMac::ReplaceSpecialPaths(const char *sourcePath, char *destPath) {
  strcpy(destPath, sourcePath);
}

int AGSMac::CDPlayerCommand(int cmdd, int datt) {
  return 0;//cd_player_control(cmdd, datt);
}

void AGSMac::DisplayAlert(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
}

void AGSMac::Delay(int millis) {
  usleep(millis);
}

unsigned long AGSMac::GetDiskFreeSpaceMB() {
  // placeholder
  return 100;
}

const char* AGSMac::GetNoMouseErrorString() {
  return "This game requires a mouse. You need to configure and setup your mouse to play this game.\n";
}

eScriptSystemOSID AGSMac::GetSystemOSID() {
  return eOS_Mac;
}

int AGSMac::InitializeCDPlayer() {
  //return cd_player_init();
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

int AGSMac::RunSetup() {
  return 0;
}

void AGSMac::SetGameWindowIcon() {
  // do nothing
}

void AGSMac::WriteStdOut(const char *text, ...) {
  char displbuf[STD_BUFFER_SIZE] = "AGS: ";
  va_list ap;
  va_start(ap,text);
  vsprintf(&displbuf[5],text,ap);
  va_end(ap);
  strcat(displbuf, "\n");

  printf(displbuf);
}

void AGSMac::ShutdownCDPlayer() {
  //cd_exit();
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSMac();
  return instance;
}
