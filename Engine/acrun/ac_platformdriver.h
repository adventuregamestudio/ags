#ifndef __AC_PLATFORMDRIVER_H
#define __AC_PLATFORMDRIVER_H

#include "acrun/ac_datetime.h"

enum eScriptSystemOSID {
    eOS_DOS = 1,
    eOS_Win = 2,
    eOS_Linux = 3,
    eOS_Mac = 4
};

struct AGSPlatformDriver {
    virtual void AboutToQuitGame();
    virtual void Delay(int millis) = 0;
    virtual void DisplayAlert(const char*, ...) = 0;
    virtual const char *GetAllUsersDataDirectory() { return NULL; }
    virtual unsigned long GetDiskFreeSpaceMB() = 0;
    virtual const char* GetNoMouseErrorString() = 0;
    virtual eScriptSystemOSID GetSystemOSID() = 0;
    virtual void GetSystemTime(ScriptDateTime*) = 0;
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
    virtual void YieldCPU() = 0;
    virtual void DisplaySwitchOut();
    virtual void DisplaySwitchIn();
    virtual void RegisterGameWithGameExplorer();
    virtual void UnRegisterGameWithGameExplorer();
    virtual int  ConvertKeycodeToScanCode(int keyCode);

    virtual int  InitializeCDPlayer() = 0;  // return 0 on success
    virtual int  CDPlayerCommand(int cmdd, int datt) = 0;
    virtual void ShutdownCDPlayer() = 0;

    virtual void ReadPluginsFromDisk(FILE *);
    virtual void StartPlugins();
    virtual int  RunPluginHooks(int event, int data);
    virtual void RunPluginInitGfxHooks(const char *driverName, void *data);
    virtual int  RunPluginDebugHooks(const char *scriptfile, int linenum);
    virtual void ShutdownPlugins();

    static AGSPlatformDriver *GetDriver();

private:
    static AGSPlatformDriver *instance;
};

extern AGSPlatformDriver *platform;

#endif // __AC_PLATFORMDRIVER_H