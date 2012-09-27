#if !defined(LINUX_VERSION)
#error This file should only be included on the Linux or BSD build
#endif

// ********* LINUX PLACEHOLDER DRIVER *********

#include "util/wgt2allg.h"
#include "gfx/ali3d.h"
#include "ac/runtime_defines.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include <libcda.h>

#include <pwd.h>
#include <sys/stat.h>

struct AGSLinux : AGSPlatformDriver {

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
  virtual void WriteDebugString(const char* texx, ...);
  virtual void ReplaceSpecialPaths(const char*, char*);
  virtual void ReadPluginsFromDisk(AGS::Common::DataStream *iii);
  virtual void StartPlugins();
  virtual void ShutdownPlugins();
  virtual int RunPluginHooks(int event, long data);
  virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
  virtual int RunPluginDebugHooks(const char *scriptfile, int linenum);
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

void AGSLinux::ReadPluginsFromDisk(AGS::Common::DataStream *iii) {
  pl_read_plugins_from_disk(iii);
}

void AGSLinux::StartPlugins() {
  pl_startup_plugins();
}

void AGSLinux::ShutdownPlugins() {
  pl_stop_plugins();
}

int AGSLinux::RunPluginHooks(int event, long data) {
  return pl_run_plugin_hooks(event, data);
}

void AGSLinux::RunPluginInitGfxHooks(const char *driverName, void *data) {
  pl_run_plugin_init_gfx_hooks(driverName, data);
}

int AGSLinux::RunPluginDebugHooks(const char *scriptfile, int linenum) {
  return pl_run_plugin_debug_hooks(scriptfile, linenum);
}