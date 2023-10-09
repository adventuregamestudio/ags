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
#include "platform/base/sys_main.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include "debug/out.h"
#include "platform/base/agsplatformdriver.h"
#include "util/geometry.h"
#include "util/string.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// TODO: prehaps refactor the whole sys_main into the singleton class at some point
struct SysMainInfo
{
    // Indicates which subsystems did we initialize
    uint32_t SDLSubsystems = 0u;
} static gl_SysMainInfo;

// ----------------------------------------------------------------------------
// INIT / SHUTDOWN
// ----------------------------------------------------------------------------

int sys_main_init(/*config*/) {
    SDL_version version;
    SDL_GetVersion(&version);
    Debug::Printf(kDbgMsg_Info, "SDL Version: %d.%d.%d", version.major, version.minor, version.patch);
    // Disable SDL2's own touch-to-mouse emulation:
    // we are going to use our own in the engine, and only if requested by the user config
#if defined (SDL_HINT_TOUCH_MOUSE_EVENTS) && defined (SDL_HINT_MOUSE_TOUCH_EVENTS)
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#elif defined (SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH)
    SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
    // TODO: setup these subsystems in config rather than keep hardcoded?
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        Debug::Printf(kDbgMsg_Error, "Unable to initialize SDL: %s", SDL_GetError());
        return -1;
    }
    bool controller_res = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
    if (!controller_res) {
        // In non-desktop pc platforms, there is a chance that the gamecontroller is indeed necessary
        // For now, it's better to just warn and rely on other input methods, in ags4 we can review this
        Debug::Printf(kDbgMsg_Warn, "Unable to initialize SDL Gamepad: %s", SDL_GetError());
    }
    gl_SysMainInfo.SDLSubsystems =
          SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS |
          SDL_INIT_GAMECONTROLLER * controller_res;
    return 0;
}

void sys_main_shutdown() {
    sys_window_destroy();
    sys_audio_shutdown(); // in case it's still on
    SDL_QuitSubSystem(gl_SysMainInfo.SDLSubsystems);
    SDL_Quit();
    gl_SysMainInfo.SDLSubsystems = 0u;
}

void sys_set_background_mode(bool /*on*/) {
    // TODO: consider if we want any implementation here, and what...
}


// ----------------------------------------------------------------------------
// DISPLAY UTILS
// ----------------------------------------------------------------------------
const int DEFAULT_DISPLAY_INDEX = 0; // TODO: is this always right?

int sys_get_desktop_resolution(int &width, int &height) {
    SDL_Rect r;
    if (SDL_GetDisplayBounds(DEFAULT_DISPLAY_INDEX, &r) != 0) {
        Debug::Printf(kDbgMsg_Error, "SDL_GetDisplayBounds failed: %s", SDL_GetError());
        return -1;
    }
    width = r.w;
    height = r.h;
    return 0;
}

void sys_get_desktop_modes(std::vector<AGS::Engine::DisplayMode> &dms, int color_depth) {
    SDL_DisplayMode mode;
    const int display_id = DEFAULT_DISPLAY_INDEX;
    const int count = SDL_GetNumDisplayModes(display_id);
    dms.clear();
    for (int i = 0; i < count; ++i) {
        if (SDL_GetDisplayMode(display_id, i, &mode) != 0) {
            Debug::Printf(kDbgMsg_Error, "SDL_GetDisplayMode failed: %s", SDL_GetError());
            continue;
        }
        const int bitsdepth = SDL_BITSPERPIXEL(mode.format);
        if ((color_depth == 0) || (bitsdepth != color_depth)) {
            continue;
        }
        AGS::Engine::DisplayMode dm;
        dm.Width = mode.w;
        dm.Height = mode.h;
        dm.ColorDepth = bitsdepth;
        dm.RefreshRate = mode.refresh_rate;
        dms.push_back(dm);
    }
}

void sys_renderer_set_output(const String &name)
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, name.GetCStr());
}


// ----------------------------------------------------------------------------
// AUDIO UTILS
// ----------------------------------------------------------------------------

bool sys_audio_init(const String &driver_name)
{
    if ((gl_SysMainInfo.SDLSubsystems & SDL_INIT_AUDIO) != 0)
        return true;
    // IMPORTANT: we must use a combination of SDL_setenv and SDL_InitSubSystem
    // here, and NOT use SDL_AudioInit, because SDL_AudioInit does not increment
    // subsystem's reference count. Which in turn may cause problems down the
    // way when initializing any additional SDL-based audio lib or plugin;
    // at the very least - the mojoAl (OpenAL's implementation we're using).
    bool res = false;
    // If user config contained a driver request, then apply one for a try
    if (!driver_name.IsEmpty())
        SDL_setenv("SDL_AUDIODRIVER", driver_name.GetCStr(), 1);
    const char *env_drv = SDL_getenv("SDL_AUDIODRIVER");
    Debug::Printf("Requested audio driver: %s", env_drv ? env_drv : "default");
    res = SDL_InitSubSystem(SDL_INIT_AUDIO) == 0;
    // If there have been an explicit request that failed, then try to force
    // SDL to go through a list of supported drivers and see if that succeeds.
    if (!res && env_drv)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to initialize requested audio driver '%s'; error: %s",
            env_drv, SDL_GetError());
        Debug::Printf("Attempt to initialize any audio driver from the known list");
        SDL_setenv("SDL_AUDIODRIVER", "", 1);
        res = SDL_InitSubSystem(SDL_INIT_AUDIO) == 0;
    }

    if (res)
        Debug::Printf(kDbgMsg_Info, "Audio driver: %s", SDL_GetCurrentAudioDriver());
    else
        Debug::Printf(kDbgMsg_Error, "Failed to initialize any audio driver; error: %s",
            SDL_GetError());
    gl_SysMainInfo.SDLSubsystems |= SDL_INIT_AUDIO * res;
    return res;
}

void sys_audio_shutdown()
{
    // Note: a subsystem that failed to initialize, doesn't increment ref-count
    // Additionally, we may not have init it at all, see engine_init_audio
    if ((gl_SysMainInfo.SDLSubsystems & SDL_INIT_AUDIO) != 0)
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    gl_SysMainInfo.SDLSubsystems &= ~SDL_INIT_AUDIO;
}


// ----------------------------------------------------------------------------
// WINDOW UTILS
// ----------------------------------------------------------------------------
// TODO: support multiple windows? in case we need some for diag purposes etc
static SDL_Window *window = nullptr;

SDL_Window *sys_window_create(const char *window_title, int w, int h, WindowMode mode, int ex_flags) {
    if (window) {
        sys_window_destroy();
    }
    // TODO: support display index selection (?)
    Uint32 flags = 0;
    switch (mode)
    {
    case kWnd_Windowed: flags |= SDL_WINDOW_RESIZABLE; break;
    case kWnd_Fullscreen: flags |= SDL_WINDOW_FULLSCREEN; break;
    case kWnd_FullDesktop: flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; break;
    }
    flags |= ex_flags;
#if (AGS_PLATFORM_MOBILE)
    // Resizable flag is necessary for fullscreen app rotation
    flags |= SDL_WINDOW_RESIZABLE;
#endif
    window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_CENTERED_DISPLAY(DEFAULT_DISPLAY_INDEX),
        SDL_WINDOWPOS_CENTERED_DISPLAY(DEFAULT_DISPLAY_INDEX),
        w,
        h,
        flags
    );
#if (AGS_PLATFORM_DESKTOP)
    // CHECKME: this is done because SDL2 has some bug(s) during
    // centering. See: https://github.com/libsdl-org/SDL/issues/6875
    // TODO: SDL2 docs mentioned that on some systems the window border size
    // may be known only after the window is displayed, which means that
    // this may have to be called with a short delay (but how to know when?)
    if (mode == kWnd_Windowed)
        sys_window_center();
#endif
    return window;
}

SDL_Window *sys_get_window() {
    return window;
}

void sys_window_set_style(WindowMode mode, Size size) {
    if (!window) return;
    if(mode != kWnd_Windowed && !platform->FullscreenSupported()) mode = kWnd_Windowed;
    // NOTE: depending on which mode we are switching to, the order of
    // actions may be different; e.g. if we are going windowed mode, then
    // we first should disable fullscreen and set new size only after;
    // if we are going fullscreen we first tell required display mode.
    switch (mode)
    {
    case kWnd_Windowed:
        SDL_SetWindowFullscreen(window, 0);
        if (!size.IsNull()) // resize + center
            sys_window_set_size(size.Width, size.Height, true);
        SDL_SetWindowResizable(window, SDL_TRUE);
        break;
    case kWnd_Fullscreen:
        if (!size.IsNull())
            SDL_SetWindowSize(window, size.Width, size.Height);
        SDL_SetWindowDisplayMode(window, nullptr); // use window size
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        break;
    case kWnd_FullDesktop:
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        break;
    }
}

void sys_window_show_cursor(bool on) {
    SDL_ShowCursor(on ? SDL_ENABLE : SDL_DISABLE);
}

bool sys_window_lock_mouse(bool on) {
    if (!window) return false;
    SDL_SetWindowGrab(window, static_cast<SDL_bool>(on));
    return on; // TODO: test if successful?
}

void sys_window_set_mouse(int x, int y) {
    if (!window) return;
    SDL_WarpMouseInWindow(window, x, y);
}

void sys_window_destroy() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void sys_window_set_title(const char *title) {
    if (window) {
        SDL_SetWindowTitle(window, title);
    }
}

void sys_window_set_icon() {
    if (window) {
        SDL_Surface *icon = platform->CreateWindowIcon();
        if (!icon) return; // no icon
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }
}

bool sys_window_set_size(int w, int h, bool center) {
    if (window) {
        SDL_SetWindowSize(window, w, h);
        if (center)
            sys_window_center();
        int new_w, new_h;
        SDL_GetWindowSize(window, &new_w, &new_h);
        return new_w == w && new_h == h;
    }
    return false;
}

void sys_window_center() {
    if (!window)
        return;
#if (AGS_PLATFORM_DESKTOP)
    // CHECKME:
    // There seem to be a bug in SDL2 where it either does not assume
    // the working area & taskbars when centering the window, or ignores
    // the size of the window borders.
    // Until that is fixed, try centering it ourselves
    // See: https://github.com/libsdl-org/SDL/issues/6875
    SDL_Rect bounds;
    int w, h;
    int bx1, by1, bx2, by2;
    SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex(window), &bounds);
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowBordersSize(window, &by1, &bx1, &by2, &bx2);
    // CHECKME: SDL_SetWindowPosition aligns the client rect to this pos???
    int x = bounds.x + bx1 + (bounds.w - (w + bx1 + bx2)) / 2;
    int y = bounds.y + by1 + (bounds.h - (h + by1 + by2)) / 2;
    SDL_SetWindowPosition(window, x, y);
#else // !AGS_PLATFORM_DESKTOP
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif
}

#if AGS_PLATFORM_OS_WINDOWS
void* sys_win_get_window()
{
    if (!window) return nullptr;
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;
    return hwnd;
}
#endif
