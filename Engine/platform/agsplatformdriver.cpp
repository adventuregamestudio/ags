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
#include "platform/agsplatformdriver.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "util/string_utils.h"

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
