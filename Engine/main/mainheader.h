
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__MAINHEADER_H
#define __AGS_EE_MAIN__MAINHEADER_H

#include "main/maindefines_ex.h"

#define USE_CLIB
#include <stdio.h>
#include "wgt2allg.h"
#include "ali3d.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_commonheaders.h"
#include "acmain/ac_controls.h"
#include "acmain/ac_lipsync.h"
#include "ac/math.h"
#include "acmain/ac_cdplayer.h"
#include "script/script_runtime.h"
#include "gui/animatingguibutton.h"
#include "gui/guibutton.h"
#include "gfx/gfxfilter.h"
#include "acmain/ac_transition.h"
#include "cs/cs_utils.h"
#include "mousew32.h"
#include "acmain/ac_display.h"
#include "ac/file.h"
#include "routefnd.h"
#include "misc.h"
#include "cs/cc_error.h"
#include "acfont/ac_agsfontrenderer.h"
#include "acmain/ac_display.h"
#include "media/audio/audio.h"
#include "media/audio/soundcache.h"

#ifdef LINUX_VERSION
#include "../PSP/launcher/pe.h"
#endif

#ifdef WINDOWS_VERSION
#include <shlwapi.h>
//#include <crtdbg.h>
//#include "winalleg.h"
#else
// ???
#endif

#if !defined(LINUX_VERSION) && !defined(MAC_VERSION)
#include <process.h>
#endif

#if defined(ANDROID_VERSION)
#include <sys/stat.h>
#include <android/log.h>

extern "C" void android_render();
extern "C" void selectLatestSavegame();
extern bool psp_load_latest_savegame;
#endif

extern "C" int csetlib(char *namm, char *passw);

#endif // __AGS_EE_MAIN__MAINHEADER_H
