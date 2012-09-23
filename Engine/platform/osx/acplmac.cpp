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
  virtual void WriteConsole(const char*, ...);
  virtual void ReplaceSpecialPaths(const char*, char*);  
  virtual void ReadPluginsFromDisk(AGS::Common::DataStream *iii);
  virtual void StartPlugins();
  virtual void ShutdownPlugins();
  virtual int RunPluginHooks(int event, int data);
  virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
  virtual int RunPluginDebugHooks(const char *scriptfile, int linenum);
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

void AGSMac::WriteConsole(const char *text, ...) {
  char displbuf[2000];
  va_list ap;
  va_start(ap, text);
  vsprintf(displbuf, text, ap);
  va_end(ap);
  printf("%s", displbuf);
}

void AGSMac::ShutdownCDPlayer() {
  //cd_exit();
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver() {
  if (instance == NULL)
    instance = new AGSMac();
  return instance;
}

void AGSMac::ReadPluginsFromDisk(AGS::Common::DataStream *iii) {
  pl_read_plugins_from_disk(iii);
}

void AGSMac::StartPlugins() {
  pl_startup_plugins();
}

void AGSMac::ShutdownPlugins() {
  pl_stop_plugins();
}

int AGSMac::RunPluginHooks(int event, int data) {
  return pl_run_plugin_hooks(event, data);
}

void AGSMac::RunPluginInitGfxHooks(const char *driverName, void *data) {
  pl_run_plugin_init_gfx_hooks(driverName, data);
}

int AGSMac::RunPluginDebugHooks(const char *scriptfile, int linenum) {
  return pl_run_plugin_debug_hooks(scriptfile, linenum);
}