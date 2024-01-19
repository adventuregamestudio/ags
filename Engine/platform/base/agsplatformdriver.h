//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS Cross-Platform Header
//
//=============================================================================
#ifndef __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
#define __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H

#include <errno.h>
#include <memory>
#include <vector>
#include "ac/datetime.h"
#include "ac/path_helper.h"
#include "debug/outputhandler.h"
#include "util/geometry.h"
#include "util/ini_util.h"

struct SDL_Surface;

namespace AGS
{
    namespace Common { class Stream; }
    namespace Engine { struct DisplayMode; }
}
using namespace AGS; // FIXME later

enum eScriptSystemOSID
{
    eOS_Unknown = 0,
    eOS_DOS,
    eOS_Win,
    eOS_Linux,
    eOS_Mac,
    eOS_Android,
    eOS_iOS,
    eOS_PSP,
    eOS_Web,
    eOS_FreeBSD,
    eNumOS
};

enum SetupReturnValue
{
    kSetup_Cancel,
    kSetup_Done,
    kSetup_RunGame
};

class AGSPlatformDriver
    // be used as a output target for logging system
    : public AGS::Common::IOutputHandler
{
public:
    AGSPlatformDriver();
    virtual ~AGSPlatformDriver() = default;

    // Called at the creation of the platform driver
    virtual void MainInit() { };
    // Called right before the formal backend init
    virtual void PreBackendInit() { };
    // Called right after the formal backend init
    virtual void PostBackendInit() { };
    // Called right before the backend is deinitialized
    virtual void PreBackendExit() { };
    // Called right after the backend is deinitialized
    virtual void PostBackendExit() { };

    virtual void Delay(int millis);
    virtual void DisplayAlert(const char *text, ...);
    virtual void AttachToParentConsole();
    virtual int  GetLastSystemError() { return errno; }
    // platform specific data file
    virtual const char * GetGameDataFile() {return nullptr; }
    // Optionally fill in config tree from the platform-specific config source
    virtual void ReadConfiguration(Common::ConfigTree & /*cfg*/) { };
    // Get root directory for storing per-game shared data
    virtual FSLocation GetAllUsersDataDirectory() { return FSLocation("."); }
    // Get root directory for storing per-game saved games
    virtual FSLocation GetUserSavedgamesDirectory() { return FSLocation("."); }
    // Get root directory for storing per-game user configuration files
    virtual FSLocation GetUserConfigDirectory() { return FSLocation("."); }
    // Get directory for storing all-games user configuration files
    virtual FSLocation GetUserGlobalConfigDirectory()  { return FSLocation("."); }
    // Get default directory for program output (logs)
    virtual FSLocation GetAppOutputDirectory() { return FSLocation("."); }
    // Returns array of characters illegal to use in file names
    virtual const char *GetIllegalFileChars() { return "\\/"; }
    virtual const char *GetDiskWriteAccessTroubleshootingText();
    virtual const char *GetGraphicsTroubleshootingText() { return ""; }
    virtual uint64_t GetDiskFreeSpaceMB() = 0;
    virtual const char* GetBackendFailUserHint();
    virtual eScriptSystemOSID GetSystemOSID() = 0;
    virtual void GetSystemTime(ScriptDateTime*);
    virtual SetupReturnValue RunSetup(const Common::ConfigTree &cfg_in, Common::ConfigTree &cfg_out);
    // Formats message and writes to standard platform's output;
    // Always adds trailing '\n' after formatted string
    virtual void WriteStdOut(const char *fmt, ...);
    // Formats message and writes to platform's error output;
    // Always adds trailing '\n' after formatted string
    virtual void WriteStdErr(const char *fmt, ...);
    // Display a text in a message box with a "warning" icon.
    // Platforms which do not support this should do nothing.
    virtual void DisplayMessageBox(const char *text);
    virtual void YieldCPU();
    // Called when the application is being paused completely (e.g. when player alt+tabbed from it).
    // This function should suspend any platform-specific realtime processing.
    virtual void PauseApplication();
    // Called when the application is being resumed.
    virtual void ResumeApplication();
    // Adjust window's * client size * to ensure it is in the supported limits
    virtual Size ValidateWindowSize(const Size &sz, bool borderless) const;
    // Either set window icon using system API directly, or create a SDL_Surface
    // for the SDL backend to set an icon instead.
    virtual SDL_Surface *CreateWindowIcon() { return nullptr; }

    virtual int  InitializeCDPlayer() = 0;  // return 0 on success
    virtual int  CDPlayerCommand(int cmdd, int datt) = 0;
    virtual void ShutdownCDPlayer() = 0;
    virtual bool FullscreenSupported() { return true; }

    // Returns command line argument in a UTF-8 format
    virtual Common::String GetCommandArg(size_t arg_index);

    // Gets the only platform driver instance, creates one if necessary
    static AGSPlatformDriver *GetDriver();
    // Shuts down and deletes the platform driver
    static void Shutdown();

    // Store command line arguments for the future use
    void SetCommandArgs(const char *const argv[], size_t argc);
    // Set whether PrintMessage should output to stdout or stderr
    void SetOutputToErr(bool on);
    // Set whether DisplayAlert is allowed to show modal GUIs on some systems;
    // it will print to either stdout or stderr otherwise, depending on above flag
    void SetGUIMode(bool on) { _guiMode = on; }

    //-----------------------------------------------
    // IOutputHandler implementation
    //-----------------------------------------------
    // Writes to the standard platform's output, prepending "AGS: " prefix to the message
    void PrintMessage(const AGS::Common::DebugMessage &msg) override;

protected:
    // TODO: this is a quick solution for IOutputHandler implementation
    // logging either to stdout or stderr. Normally there should be
    // separate implementation, one for each kind of output, but
    // with both going through PlatformDriver need to figure a better
    // design first.
    bool _logToStdErr = false;
    // A function pointer for stdout write;
    // this is used when printing log, and may be set to null disabling an output
    void (AGSPlatformDriver::*_writeStdOut)(const char *fmt, ...) = nullptr;
    // Defines whether engine is allowed to display important warnings
    // and errors by showing a message box kind of GUI.
    bool _guiMode = false;

    const char * const *_cmdArgs = nullptr;
    size_t _cmdArgCount = 0u;

private:
    static AGSPlatformDriver *CreateDriver();

    static std::unique_ptr<AGSPlatformDriver> _instance;
};

#if defined (AGS_HAS_CD_AUDIO)
int cd_player_init();
int cd_player_control(int cmdd, int datt);
#endif

// [IKM] What is a need to have this global var if you can get AGSPlatformDriver
// instance by calling AGSPlatformDriver::GetDriver()?
extern AGSPlatformDriver *platform;

#endif // __AGS_EE_PLATFORM__AGSPLATFORMDRIVER_H
