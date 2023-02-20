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

#include "core/platform.h"

#if AGS_PLATFORM_OS_EMSCRIPTEN

// ********* EMSCRIPTEN PLACEHOLDER DRIVER *********

#include <stdio.h>
#include <chrono>
#include <allegro.h>
#include "SDL.h"
#include "ac/runtime_defines.h"
#include "ac/timer.h"
#include "gfx/gfxdefines.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/agsplugin.h"
#include "util/string.h"

#include <pwd.h>
#include <sys/stat.h>

using AGS::Common::String;

FSLocation CommonDataDirectory;
FSLocation UserDataDirectory;

const auto MaximumDelayBetweenPolling = std::chrono::milliseconds(16);

struct AGSEmscripten : AGSPlatformDriver {

  int  CDPlayerCommand(int cmdd, int datt) override;
  void DisplayAlert(const char*, ...) override;
  void Delay(int millis) override;
  void YieldCPU() override;
  FSLocation GetAllUsersDataDirectory() override;
  FSLocation GetUserSavedgamesDirectory() override;
  FSLocation GetUserConfigDirectory() override;
  FSLocation GetUserGlobalConfigDirectory() override;
  FSLocation GetAppOutputDirectory() override;
  unsigned long GetDiskFreeSpaceMB() override;
  const char* GetBackendFailUserHint() override;
  eScriptSystemOSID GetSystemOSID() override;
  int  InitializeCDPlayer() override;
  void ShutdownCDPlayer() override;
  void MainInit() override;
};


int AGSEmscripten::CDPlayerCommand(int cmdd, int datt) 
{
    return 0;
}

void AGSEmscripten::DisplayAlert(const char *text, ...) 
{
    char displbuf[2000];
    va_list ap;
    va_start(ap, text);
    vsprintf(displbuf, text, ap);
    va_end(ap);
    if (_logToStdErr)
    {
        fprintf(stderr, "%s\n", displbuf);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "AGS Error", displbuf, nullptr);
    }
    else
    {
        fprintf(stdout, "%s\n", displbuf);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "AGS Alert", displbuf, nullptr);
    }
}

void AGSEmscripten::MainInit() {
    UserDataDirectory = FSLocation("/home/web_user").Concat("ags");
    CommonDataDirectory = FSLocation("/home/web_user").Concat("common");
}

void AGSEmscripten::YieldCPU() {
    this->Delay(1);
}

void AGSEmscripten::Delay(int millis) {
    if(millis < 0) {
        // if negative we just want a regular SDL_delay in Emscripten
        millis = -millis;
        SDL_Delay(millis);
    }

    auto now = AGS_Clock::now();
    auto delayUntil = now + std::chrono::milliseconds(millis);

    for (;;) {
        if (now >= delayUntil) { break; }

        auto duration = std::min<std::chrono::nanoseconds>(delayUntil - now, MaximumDelayBetweenPolling);
        SDL_Delay(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        now = AGS_Clock::now(); // update now

        if (now >= delayUntil) { break; }

        // don't allow it to check for debug messages, since this Delay()
        // call might be from within a debugger polling loop
        now = AGS_Clock::now(); // update now
    }
}

FSLocation AGSEmscripten::GetAllUsersDataDirectory()
{
    return CommonDataDirectory;
}

FSLocation AGSEmscripten::GetUserSavedgamesDirectory()
{
    return UserDataDirectory;
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

unsigned long AGSEmscripten::GetDiskFreeSpaceMB() 
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

AGSPlatformDriver* AGSPlatformDriver::CreateDriver()
{
    return new AGSEmscripten();
}

#endif
