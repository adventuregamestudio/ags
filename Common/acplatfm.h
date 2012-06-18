/*
  AGS Cross-Platform Header

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#ifndef __ACPLATFM_H
#define __ACPLATFM_H

#ifdef DJGPP
#define DOS_VERSION
#endif

#include "wgt2allg.h"
//#include "acruntim.h"
//#include "acsound.h"
#include "acrun/ac_platformdriver.h"
#include "acaudio/ac_sound.h"
#include "ali3d.h"

#ifdef WINDOWS_VERSION
#include <ddraw.h>
#include <dsound.h>
#endif

#include "agsplugin.h"

#if !defined(BSD_VERSION) && (defined(LINUX_VERSION) || defined(WINDOWS_VERSION))
#include "libcda.h"
#endif

struct AGS32BitOSDriver : AGSPlatformDriver {
  virtual void GetSystemTime(ScriptDateTime*) ;
  virtual void YieldCPU();
};

void pl_stop_plugins();
void pl_startup_plugins();
int pl_run_plugin_hooks (int event, int data);
void pl_run_plugin_init_gfx_hooks(const char *driverName, void *data);
int pl_run_plugin_debug_hooks (const char *scriptfile, int linenum);
void pl_read_plugins_from_disk (FILE *iii);
int cd_player_init();
int cd_player_control(int cmdd, int datt);

extern IGraphicsDriver *gfxDriver;
extern int editor_debugging_enabled;
extern int break_on_next_script_step;
extern void get_current_dir_path(char* buffer, const char *fileName);

#endif
