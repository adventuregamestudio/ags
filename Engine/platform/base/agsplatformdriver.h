/*
  AGS Cross-Platform Header

  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
#define __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H

#include "ac/datetime.h"
#include "util/file.h"
#include "debug/outputtarget.h"

namespace AGS { namespace Common { class DataStream; } }
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
    virtual const char *GetAllUsersDataDirectory() { return NULL; }
    virtual unsigned long GetDiskFreeSpaceMB() = 0;
    virtual const char* GetNoMouseErrorString() = 0;
    virtual eScriptSystemOSID GetSystemOSID() = 0;
    virtual void GetSystemTime(ScriptDateTime*);
    virtual void PlayVideo(const char* name, int skip, int flags) = 0;
    virtual void InitialiseAbufAtStartup();
    virtual void PostAllegroInit(bool windowed);
    virtual void PostAllegroExit() = 0;
    virtual void FinishedUsingGraphicsMode();
    virtual void ReplaceSpecialPaths(const char *sourcePath, char *destPath) = 0;
    virtual int  RunSetup() = 0;
    virtual void SetGameWindowIcon();
    virtual void WriteConsole(const char*, ...) = 0;
    virtual void WriteDebugString(const char*, ...);
    virtual void YieldCPU();
    virtual void DisplaySwitchOut();
    virtual void DisplaySwitchIn();
    virtual void RegisterGameWithGameExplorer();
    virtual void UnRegisterGameWithGameExplorer();
    virtual int  ConvertKeycodeToScanCode(int keyCode);

    virtual int  InitializeCDPlayer() = 0;  // return 0 on success
    virtual int  CDPlayerCommand(int cmdd, int datt) = 0;
    virtual void ShutdownCDPlayer() = 0;

    virtual void ReadPluginsFromDisk(Common::DataStream *in);
    virtual void StartPlugins();
    virtual int  RunPluginHooks(int event, long data);
    virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
    virtual int  RunPluginDebugHooks(const char *scriptfile, int linenum);
    virtual void ShutdownPlugins();

    static AGSPlatformDriver *GetDriver();

    //-----------------------------------------------
    // IOutputTarget implementation
    //-----------------------------------------------
    virtual void Out(const char *sz_fullmsg);

private:
    static AGSPlatformDriver *instance;
};

#if !defined(ANDROID_VERSION) && !defined(PSP_VERSION) && !defined(DOS_VERSION) && !defined(BSD_VERSION) && !defined(MAC_VERSION)
int cd_player_init();
int cd_player_control(int cmdd, int datt);
#endif

// [IKM] What is a need to have this global var if you can get AGSPlatformDriver
// instance by calling AGSPlatformDriver::GetDriver()?
extern AGSPlatformDriver *platform;

#endif // __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
