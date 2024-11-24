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
// AGS Platform-specific functions
//
//=============================================================================
#include "platform/base/agsplatformdriver.h"
#include <stdio.h>
#include <thread>
#include <SDL.h>
#include "ac/common.h"
#include "ac/runtime_defines.h"
#include "ac/timer.h"
#include "gfx/bitmap.h"
#include "media/audio/audio_system.h"
#include "platform/base/sys_main.h"
#include "util/memory_compat.h"
#include "util/string_utils.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// We don't have many places where we delay longer than a frame, but where we
// do, we should give the audio layer a chance to update.
// 16 milliseconds is rough period for 60fps
const auto MaximumDelayBetweenPolling = std::chrono::milliseconds(16);

std::unique_ptr<AGSPlatformDriver> AGSPlatformDriver::_instance;
AGSPlatformDriver *platform = nullptr;

// ******** DEFAULT IMPLEMENTATIONS *******

AGSPlatformDriver::AGSPlatformDriver()
{
    _writeStdOut = &AGSPlatformDriver::WriteStdOut;
}

void AGSPlatformDriver::AttachToParentConsole() { }
void AGSPlatformDriver::PauseApplication() { }
void AGSPlatformDriver::ResumeApplication() { }

Size AGSPlatformDriver::ValidateWindowSize(const Size &sz, bool borderless) const
{
    // TODO: Ideally we should also test for the minimal / maximal
    // allowed size of the window in current system here;
    // and more importantly: subtract window's border size.
    // SDL2 currently does not allow to get window border size
    // without creating a window first. But this potentially may be
    // acquired, at least on some platforms (e.g. Windows).
    SDL_Rect rc;
    SDL_GetDisplayUsableBounds(sys_get_window_display_index(), &rc);
    return Size::Clamp(sz, Size(1, 1), Size(rc.w, rc.h));
}

const char* AGSPlatformDriver::GetBackendFailUserHint()
{
    return "Make sure you have latest version of SDL2 libraries installed, and your system is running in graphical mode.";
}

const char *AGSPlatformDriver::GetDiskWriteAccessTroubleshootingText()
{
    return "Make sure you have write permissions, and also check the disk's free space.";
}

void AGSPlatformDriver::DisplayAlert(const char *text, ...)
{
    char displbuf[2048];
    va_list ap;
    va_start(ap, text);
    vsnprintf(displbuf, sizeof(displbuf), text, ap);
    va_end(ap);

    // Print alert to the log system, to let other outputs receive it;
    // but use a dirty method to avoid duplicate message in stdout/stderr
    auto old_stdout = _writeStdOut;
    _writeStdOut = nullptr;
    Debug::Printf(kDbgMsg_Alert, "%s", displbuf);
    _writeStdOut = old_stdout;

    // Always write to either stderr or stdout, even if message boxes are enabled.
    (this->*_writeStdOut)("%s", displbuf);

    if (_guiMode)
        DisplayMessageBox(displbuf);
}

void AGSPlatformDriver::WriteStdOut(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
    fflush(stdout);
}

void AGSPlatformDriver::WriteStdErr(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void AGSPlatformDriver::DisplayMessageBox(const char *text)
{
    if (_guiMode)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Adventure Game Studio", text, sys_get_window());
}

void AGSPlatformDriver::YieldCPU() {
    // NOTE: this is called yield, but if we actually yield instead of delay,
    // we get a massive increase in CPU usage.
    this->Delay(1);
    //std::this_thread::yield();
}

SetupReturnValue AGSPlatformDriver::RunSetup(const ConfigTree &/*cfg_in*/, const ConfigTree &/*def_cfg_in*/, ConfigTree &/*cfg_out*/)
{
    return kSetup_Cancel;
}

void AGSPlatformDriver::SetCommandArgs(const char *const argv[], size_t argc)
{
    _cmdArgs = argv;
    _cmdArgCount = argc;
}

void AGSPlatformDriver::SetOutputToErr(bool on)
{
    _logToStdErr = on;
    _writeStdOut = _logToStdErr ? &AGSPlatformDriver::WriteStdErr : &AGSPlatformDriver::WriteStdOut;
}

String AGSPlatformDriver::GetCommandArg(size_t arg_index)
{
    return arg_index < _cmdArgCount ? _cmdArgs[arg_index] : nullptr;
}

//-----------------------------------------------------------------------------
// IOutputHandler implementation
// Writes to the standard platform's output using provided method(s).
//-----------------------------------------------------------------------------
class StdOutLogger : public IOutputHandler
{
public:
    typedef void (AGSPlatformDriver::*PfnWriteStdOut)(const char *fmt, ...);

    StdOutLogger(AGSPlatformDriver *platform, PfnWriteStdOut write_stdout)
        : _platform(platform)
        , _writeStdOut(write_stdout)
    {}

    void OnRegister() override
    {
        // do nothing
    }

    void PrintMessage(const Common::DebugMessage &msg) override
    {
        if (_writeStdOut)
        {
            if (msg.GroupName.IsEmpty())
                (_platform->*_writeStdOut)("%s", msg.Text.GetCStr());
            else
                (_platform->*_writeStdOut)("%s : %s", msg.GroupName.GetCStr(), msg.Text.GetCStr());
        }
    }

private:
    AGSPlatformDriver * const _platform = nullptr;
    PfnWriteStdOut const _writeStdOut = nullptr;
};

std::unique_ptr<IOutputHandler> AGSPlatformDriver::GetStdOut()
{
    std::unique_ptr<IOutputHandler> out(new StdOutLogger(this, _writeStdOut));
    return out;
}

AGSPlatformDriver* AGSPlatformDriver::GetDriver()
{
    if (!_instance)
        _instance.reset(CreateDriver());
    return _instance.get();
}

void AGSPlatformDriver::Shutdown()
{
    _instance.reset();
}


void AGSPlatformDriver::Delay(int millis) {
  auto now = AGS_Clock::now();
  auto delayUntil = now + std::chrono::milliseconds(millis);

  for (;;) {
    if (now >= delayUntil) { break; }

    auto duration = std::min<std::chrono::nanoseconds>(delayUntil - now, MaximumDelayBetweenPolling);
    std::this_thread::sleep_for(duration);
    now = AGS_Clock::now(); // update now

    if (now >= delayUntil) { break; }

    // don't allow it to check for debug messages, since this Delay()
    // call might be from within a debugger polling loop
    now = AGS_Clock::now(); // update now
  }
}
