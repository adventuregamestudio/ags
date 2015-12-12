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
// AGS Cross-Platform Header
//
//=============================================================================

#ifndef __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
#define __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H

#include "ac/datetime.h"
#include "debug/outputtarget.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#ifdef DJGPP
#define DOS_VERSION
#endif

enum eScriptSystemOSID {
    eOS_DOS = 1,
    eOS_Win = 2,
    eOS_Linux = 3,
    eOS_Mac = 4
};

struct AGSPlatformDriver
    // be used as a output target for logging system
    : public AGS::Common::Out::IOutputTarget
{
    virtual void AboutToQuitGame();
    virtual void Delay(int millis) = 0;
    virtual void DisplayAlert(const char*, ...) = 0;
    // Get directory for storing shared game data
    virtual const char *GetAllUsersDataDirectory() { return "."; }
    // Get directory for storing user's saved games
    virtual const char *GetUserSavedgamesDirectory() { return "."; }
    // Get default directory for program output (logs)
    virtual const char *GetAppOutputDirectory() { return "."; }
    // Returns array of characters illegal to use in file names
    virtual const char *GetIllegalFileChars() { return "\\/"; }
    virtual const char *GetGraphicsTroubleshootingText() { return ""; }
    virtual unsigned long GetDiskFreeSpaceMB() = 0;
    virtual const char* GetNoMouseErrorString() = 0;
    virtual const char* GetAllegroFailUserHint();
    virtual eScriptSystemOSID GetSystemOSID() = 0;
    virtual void GetSystemTime(ScriptDateTime*);
    virtual void PlayVideo(const char* name, int skip, int flags) = 0;
    virtual void InitialiseAbufAtStartup();
    virtual void PostAllegroInit(bool windowed);
    virtual void PostAllegroExit() = 0;
    virtual void FinishedUsingGraphicsMode();
    virtual int  RunSetup() = 0;
    virtual void SetGameWindowIcon();
    virtual void WriteStdOut(const char*, ...) = 0;
    virtual void YieldCPU();
    virtual void DisplaySwitchOut();
    virtual void DisplaySwitchIn();
    virtual void RegisterGameWithGameExplorer();
    virtual void UnRegisterGameWithGameExplorer();
    virtual int  ConvertKeycodeToScanCode(int keyCode);

    virtual int  InitializeCDPlayer() = 0;  // return 0 on success
    virtual int  CDPlayerCommand(int cmdd, int datt) = 0;
    virtual void ShutdownCDPlayer() = 0;

    virtual void ReadPluginsFromDisk(Common::Stream *in);
    virtual void StartPlugins();
    virtual int  RunPluginHooks(int event, long data);
    virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
    virtual int  RunPluginDebugHooks(const char *scriptfile, int linenum);
    virtual void ShutdownPlugins();

    virtual bool LockMouseToWindow();
    virtual void UnlockMouse();

    static AGSPlatformDriver *GetDriver();

    //-----------------------------------------------
    // IOutputTarget implementation
    //-----------------------------------------------
    virtual void Out(const char *sz_fullmsg);

private:
    static AGSPlatformDriver *instance;
};

#if defined (AGS_HAS_CD_AUDIO)
int cd_player_init();
int cd_player_control(int cmdd, int datt);
#endif

// [IKM] What is a need to have this global var if you can get AGSPlatformDriver
// instance by calling AGSPlatformDriver::GetDriver()?
extern AGSPlatformDriver *platform;

#endif // __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
