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
//
// AGS Platform-specific functions
//
//=============================================================================

#include <stdio.h>
#include <string.h>
#include "util/wgt2allg.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/common.h"
#include "util/string_utils.h"
#include "util/datastream.h"
#include "gfx/bitmap.h"

using AGS::Common::DataStream;
using AGS::Common::String;
using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

#if defined (AGS_HAS_CD_AUDIO)
#include "libcda.h"
#endif

extern Bitmap *abuf; // in wgt2allg

AGSPlatformDriver* AGSPlatformDriver::instance = NULL;
AGSPlatformDriver *platform = NULL;

// ******** DEFAULT IMPLEMENTATIONS *******

int  AGSPlatformDriver::RunPluginDebugHooks(const char *scriptfile, int linenum) { return 0; }
void AGSPlatformDriver::RunPluginInitGfxHooks(const char *driverName, void *data) { }
void AGSPlatformDriver::ShutdownPlugins() { }
void AGSPlatformDriver::StartPlugins() { }
int  AGSPlatformDriver::RunPluginHooks(int event, long data) { return 0; }
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

void AGSPlatformDriver::ReadPluginsFromDisk(DataStream *in) {
    if (in->ReadInt32() != 1)
        quit("ERROR: unable to load game, invalid version of plugin data");

    int numPlug = in->ReadInt32(), a, datasize;
    String buffer;
    for (a = 0; a < numPlug; a++) {
        // read the plugin name
        buffer = in->ReadString();
        datasize = in->ReadInt32();
        in->Seek (Common::kSeekCurrent, datasize);
    }

}

void AGSPlatformDriver::InitialiseAbufAtStartup()
{
    // because loading the game file accesses abuf, it must exist
    abuf = BitmapHelper::CreateBitmap(10,10,8);
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
void AGSPlatformDriver::Out(const char *sz_fullmsg) {
    // do nothing
}

// ********** CD Player Functions common to Win and Linux ********

#if defined (AGS_HAS_CD_AUDIO)

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

#endif // AGS_HAS_CD_AUDIO
