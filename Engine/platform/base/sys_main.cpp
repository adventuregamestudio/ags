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

// ----------------------------------------------------------------------------
// INIT / SHUTDOWN
// ----------------------------------------------------------------------------

int sys_main_init(/*config*/) {
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE); // TODO: backend log verbosity from config

    // TODO: setup these subsystems in config rather than keep hardcoded?
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        Debug::Printf(kDbgMsg_Error, "Unable to initialize SDL: %s", SDL_GetError());
        return -1;
    }
    return 0;
}

void sys_main_shutdown() {
    sys_window_destroy();
    SDL_Quit();
}

void sys_set_background_mode(bool on) {
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

void sys_get_desktop_modes(std::vector<AGS::Engine::DisplayMode> &dms) {
    SDL_DisplayMode mode;
    const int display_id = DEFAULT_DISPLAY_INDEX;
    const int count = SDL_GetNumDisplayModes(display_id);
    dms.clear();
    for (int i = 0; i < count; ++i) {
        if (SDL_GetDisplayMode(display_id, i, &mode) != 0) {
            Debug::Printf(kDbgMsg_Error, "SDL_GetDisplayMode failed: %s", SDL_GetError());
            continue;
        }
        AGS::Engine::DisplayMode dm;
        dm.Width = mode.w;
        dm.Height = mode.h;
        dm.ColorDepth = SDL_BITSPERPIXEL(mode.format);
        dm.RefreshRate = mode.refresh_rate;
        dms.push_back(dm);
    }
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
    window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_CENTERED_DISPLAY(DEFAULT_DISPLAY_INDEX),
        SDL_WINDOWPOS_CENTERED_DISPLAY(DEFAULT_DISPLAY_INDEX),
        w,
        h,
        flags
    );
    return window;
}

SDL_Window *sys_get_window() {
    return window;
}

void sys_window_set_style(WindowMode mode, int ex_flags) {
    if (!window) return;
    Uint32 flags = 0;
    switch (mode)
    {
    case kWnd_Windowed: flags |= SDL_WINDOW_RESIZABLE; break;
    case kWnd_Fullscreen: flags |= SDL_WINDOW_FULLSCREEN; break;
    case kWnd_FullDesktop: flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; break;
    }
    flags |= ex_flags;
    SDL_SetWindowFullscreen(window, flags);
    SDL_SetWindowResizable(window, (flags & SDL_WINDOW_RESIZABLE) ? SDL_TRUE : SDL_FALSE);
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
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        int new_w, new_h;
        SDL_GetWindowSize(window, &new_w, &new_h);
        return new_w == w && new_h == h;
    }
    return false;
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
