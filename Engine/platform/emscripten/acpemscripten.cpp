//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"

#if AGS_PLATFORM_OS_EMSCRIPTEN

// ********* EMSCRIPTEN PLACEHOLDER DRIVER *********

#include <emscripten.h>
#include <stdio.h>
#include <chrono>
#include <allegro.h>
#include "SDL.h"
#include "main/graphics_mode.h"
#include "main/engine.h"
#include "ac/system.h"
#include "gfx/graphicsdriver.h"
#include "platform/base/agsplatformdriver.h"
#include "util/filestream.h"
#include "util/time_util.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern IGraphicsDriver *gfxDriver;

FSLocation CommonDataDirectory;
FSLocation UserDataDirectory;
FSLocation SavedGamesDirectory;

const auto MaximumDelayBetweenPolling = std::chrono::milliseconds(16);
static bool ags_syncfs_running = false;

// We need this to export for Emscripten due to C++ name mangling
extern "C" 
{
  EMSCRIPTEN_KEEPALIVE int ext_gfxmode_get_width()
  {
      DisplayMode rdm = gfxDriver->GetDisplayMode();
      return rdm.Width;
  }

  EMSCRIPTEN_KEEPALIVE int ext_gfxmode_get_height()
  {
      DisplayMode rdm = gfxDriver->GetDisplayMode();
      return rdm.Height;
  }

  EMSCRIPTEN_KEEPALIVE void ext_syncfs_done(void)
  {
    ags_syncfs_running = false;
  }

  EMSCRIPTEN_KEEPALIVE int ext_get_windowed(void)
  {
    return System_GetWindowed();
  }

  EMSCRIPTEN_KEEPALIVE int ext_toggle_fullscreen(void)
  {
    return engine_try_switch_windowed_gfxmode() ? 1 : 0;
  }

  EMSCRIPTEN_KEEPALIVE int ext_fullscreen_supported(void)
  {
    // for now, this returns false if it's Safari, because Safari is lying on IsFullscreenEnabled even on iPhone.
    // returns true for any other browser. This means we will manually handle FS in Safari in the html launcher.
    return EM_ASM_INT(return !(/^((?!chrome|android).)*safari/i.test(navigator.userAgent)) ? 1 : 0) == 1;
  }
} // END of Extern "C"

struct AGSEmscripten : AGSPlatformDriver {

  int  CDPlayerCommand(int cmdd, int datt) override;
  void Delay(int millis) override;
  void YieldCPU() override;
  FSLocation GetAllUsersDataDirectory() override;
  FSLocation GetUserSavedgamesDirectory() override;
  FSLocation GetUserConfigDirectory() override;
  FSLocation GetUserGlobalConfigDirectory() override;
  FSLocation GetAppOutputDirectory() override;
  uint64_t GetDiskFreeSpaceMB(const String &path) override;
  const char* GetBackendFailUserHint() override;
  eScriptSystemOSID GetSystemOSID() override;
  int  InitializeCDPlayer() override;
  void ShutdownCDPlayer() override;
  void MainInit() override;
  bool FullscreenSupported() override;

  // new members
  void SyncEmscriptenFS();
  void ScheduleSyncFS();
  void CheckNeedToSyncFS();
  int NeededSyncFS;
};


int AGSEmscripten::CDPlayerCommand(int cmdd, int datt) 
{
    return 0;
}

void AGSEmscripten::SyncEmscriptenFS()
{
    if (ags_syncfs_running)
    {
        EM_ASM(
            console.log("INFO: FS.syncfs needed but already running");
        );
        return;
    }

    ags_syncfs_running = true;

    // Sync files
    EM_ASM(
        FS.syncfs(false, function (err) {
            if (err) {
                console.error("ERROR: IDBFS syncfs failed with" + err + "(errno:" + err.errno + ")");
            } else {
                console.log("INFO: IDBFS synced game files.");
            }
            _ext_syncfs_done();
        });
    );
}

void AGSEmscripten::ScheduleSyncFS()
{
    NeededSyncFS++;
}

void AGSEmscripten::CheckNeedToSyncFS()
{
    if(NeededSyncFS == 0) return;

    NeededSyncFS = 0;
    SyncEmscriptenFS();
}

void AGSEmscripten::MainInit()
{
    SavedGamesDirectory = FSLocation("/home/web_user").Concat("saved_games");
    UserDataDirectory = FSLocation("/home/web_user").Concat("ags");
    CommonDataDirectory = FSLocation("/home/web_user").Concat("common");

    NeededSyncFS = 0;
    ags_syncfs_running = false;

    FileStream::FileCloseNotify = [this](const FileStream::CloseNotifyArgs &args){
        // Only sync if file stream was in write mode
        if ((args.WorkMode & StreamMode::kStream_ReadWrite) == StreamMode::kStream_Read)
            return;
        if (!Path::IsSameOrSubDir(SavedGamesDirectory.FullDir,args.Filepath))
            return;
        ScheduleSyncFS();
    };
}

void AGSEmscripten::YieldCPU()
{
    this->Delay(1);
}

void AGSEmscripten::Delay(int millis)
{
    CheckNeedToSyncFS();
    if(millis < 0) {
        // if negative we just want a regular SDL_delay in Emscripten
        millis = -millis;
        SDL_Delay(millis);
    }

    auto now = Clock::now();
    auto delayUntil = now + std::chrono::milliseconds(millis);

    for (;;) {
        if (now >= delayUntil) { break; }

        auto duration = std::min<std::chrono::nanoseconds>(delayUntil - now, MaximumDelayBetweenPolling);
        SDL_Delay(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        now = Clock::now(); // update now

        if (now >= delayUntil) { break; }

        // don't allow it to check for debug messages, since this Delay()
        // call might be from within a debugger polling loop
        now = Clock::now(); // update now
    }
}

FSLocation AGSEmscripten::GetAllUsersDataDirectory()
{
    return CommonDataDirectory;
}

FSLocation AGSEmscripten::GetUserSavedgamesDirectory()
{
    return SavedGamesDirectory;
}

FSLocation AGSEmscripten::GetUserConfigDirectory()
{
    return GetUserSavedgamesDirectory();
}

FSLocation AGSEmscripten::GetUserGlobalConfigDirectory()
{
    return GetUserSavedgamesDirectory();
}

FSLocation AGSEmscripten::GetAppOutputDirectory()
{
    return UserDataDirectory;
}

uint64_t AGSEmscripten::GetDiskFreeSpaceMB(const String &path) 
{
    // placeholder
    return 100;
}

const char* AGSEmscripten::GetBackendFailUserHint()
{
  return "Make sure your browser is compatible with SDL2 Emscripten port.";
}

eScriptSystemOSID AGSEmscripten::GetSystemOSID() 
{
    return eOS_Web;
}

int AGSEmscripten::InitializeCDPlayer() 
{
    return 0;// cd_player_init();
}

void AGSEmscripten::ShutdownCDPlayer() 
{
    //cd_exit();
}

bool AGSEmscripten::FullscreenSupported() {
    return ext_fullscreen_supported();
}

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSEmscripten();
}

#endif
