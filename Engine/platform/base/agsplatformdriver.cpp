/*
  AGS Platform-specific functions

  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#include <stdio.h>
#include <string.h>
#include "platform/base/agsplatformdriver.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "util/string_utils.h"

#if !defined(BSD_VERSION) && (defined(LINUX_VERSION) || defined(WINDOWS_VERSION))
#include "libcda.h"
#endif

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#define strnicmp strncasecmp
#endif

extern block abuf; // in wgt2allg

AGSPlatformDriver* AGSPlatformDriver::instance = NULL;
AGSPlatformDriver *platform = NULL;

// ******** DEFAULT IMPLEMENTATIONS *******

int  AGSPlatformDriver::RunPluginDebugHooks(const char *scriptfile, int linenum) { return 0; }
void AGSPlatformDriver::RunPluginInitGfxHooks(const char *driverName, void *data) { }
void AGSPlatformDriver::ShutdownPlugins() { }
void AGSPlatformDriver::StartPlugins() { }
int  AGSPlatformDriver::RunPluginHooks(int event, int data) { return 0; }
void AGSPlatformDriver::WriteDebugString(const char*, ...) { }
void AGSPlatformDriver::AboutToQuitGame() { }
void AGSPlatformDriver::PostAllegroInit(bool windowed) { }
void AGSPlatformDriver::DisplaySwitchOut() { }
void AGSPlatformDriver::DisplaySwitchIn() { }
void AGSPlatformDriver::RegisterGameWithGameExplorer() { }
void AGSPlatformDriver::UnRegisterGameWithGameExplorer() { }


void AGSPlatformDriver::GetSystemTime(ScriptDateTime *sdt) {
    struct tm *newtime;
    time_t long_time;

    time( &long_time );
    newtime = localtime( &long_time );

    sdt->hour = newtime->tm_hour;
    sdt->minute = newtime->tm_min;
    sdt->second = newtime->tm_sec;
    sdt->day = newtime->tm_mday;
    sdt->month = newtime->tm_mon + 1;
    sdt->year = newtime->tm_year + 1900;
}

void AGSPlatformDriver::YieldCPU() {
    this->Delay(1);
}

void AGSPlatformDriver::ReplaceSpecialPaths(const char *sourcePath, char *destPath) {

    if (strnicmp(sourcePath, "$MYDOCS$", 8) == 0) {
        // For platforms with no My Documents folder, just
        // redirect it back to current folder
        strcpy(destPath, ".");
        strcat(destPath, &sourcePath[8]);
    }
    else {
        strcpy(destPath, sourcePath);
    }

}

void AGSPlatformDriver::ReadPluginsFromDisk(FILE *iii) {
    if (getw(iii) != 1)
        quit("ERROR: unable to load game, invalid version of plugin data");

    int numPlug = getw(iii), a, datasize;
    char buffer[80];

    for (a = 0; a < numPlug; a++) {
        // read the plugin name
        fgetstring (buffer, iii);
        datasize = getw(iii);
        fseek (iii, datasize, SEEK_CUR);
    }

}

void AGSPlatformDriver::InitialiseAbufAtStartup()
{
    // because loading the game file accesses abuf, it must exist
    abuf = create_bitmap_ex(8,10,10);
}

void AGSPlatformDriver::FinishedUsingGraphicsMode()
{
    // don't need to do anything on any OS except DOS
}

void AGSPlatformDriver::SetGameWindowIcon() {
    // do nothing
}

int AGSPlatformDriver::ConvertKeycodeToScanCode(int keycode)
{
    keycode -= ('A' - KEY_A);
    return keycode;
}

//-----------------------------------------------
// IOutputTarget implementation
//-----------------------------------------------
void AGSPlatformDriver::out(const char *sz_fullmsg) {
    // do nothing
}

// ********** CD Player Functions common to Win and Linux ********

#if !defined(ANDROID_VERSION) && !defined(PSP_VERSION) && !defined(DOS_VERSION) && !defined(BSD_VERSION) && !defined(MAC_VERSION)

// from ac_cdplayer
extern int use_cdplayer;
extern int need_to_stop_cd;

int numcddrives=0;

int cd_player_init() {
    int erro = cd_init();
    if (erro) return -1;
    numcddrives=1;
    use_cdplayer=1;
    return 0;
}

int cd_player_control(int cmdd, int datt) {
    // WINDOWS & LINUX VERSION
    if (cmdd==1) {
        if (cd_current_track() > 0) return 1;
        return 0;
    }
    else if (cmdd==2) {
        cd_play_from(datt);
        need_to_stop_cd=1;
    }
    else if (cmdd==3) 
        cd_pause();
    else if (cmdd==4) 
        cd_resume();
    else if (cmdd==5) {
        int first,last;
        if (cd_get_tracks(&first,&last)==0)
            return (last-first)+1;
        else return 0;
    }
    else if (cmdd==6)
        cd_eject();
    else if (cmdd==7)
        cd_close();
    else if (cmdd==8)
        return numcddrives;
    else if (cmdd==9) ;
    else quit("!CDAudio: Unknown command code");

    return 0;
}

#endif // #if !defined(ANDROID_VERSION) && !defined(PSP_VERSION) && !defined(DOS_VERSION) && !defined(BSD_VERSION) && !defined(MAC_VERSION)
