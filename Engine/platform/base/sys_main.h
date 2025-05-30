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
//
// The main backend interface.
//
// TODO: split up later if it gets filled with functions in all categories.
//
//=============================================================================
#include "core/platform.h"
#include <vector>
#include "gfx/gfxdefines.h"
#include "util/string.h"

using AGS::Common::String;

// Initializes main backend system;
// should be called before anything else backend related.
// Returns 0 on success, non-0 on failure.
int  sys_main_init(/*config*/);
// Shutdown main backend system;
// should be called last, after everything else backend related is shutdown.
void sys_main_shutdown();
// Sets whether the engine wants to update while the window has no focus.
// TODO: this is a placeholder at the moment, check later if we need any implementation
void sys_set_background_mode(bool on);

// Display utilities.
//
// Gets number of available displays.
int sys_get_display_count();
// Tells if the given display index valid (exists).
bool sys_is_display_valid(int display_index);
// Queries the display index on which the window is currently positioned.
// Returns default display index in case window does not exist yet, or on any error.
int sys_get_window_display_index();
// Queries current desktop resolution from the display that hosts game window.
bool sys_get_desktop_resolution(int &width, int &height);
// Queries current desktop resolution from the requested display.
bool sys_get_desktop_resolution(int display_index, int &width, int &height);
// Queries supported desktop modes.
void sys_get_desktop_modes(int display_index, std::vector<AGS::Engine::DisplayMode> &dms, int color_depth = 0);
// Sets output driver for the backend's renderer
// TODO: consider make part of the SDLRendererGraphicsDriver later
void sys_renderer_set_output(const String &name);

// Audio utilities.
//
// Tries to init the audio backend; optionally requests particular driver
bool sys_audio_init(const String &driver_name = "");
// Shutdown audio backend
void sys_audio_shutdown();

// Window utilities.
//
struct SDL_Window;
// Create a new single game window.
SDL_Window *sys_window_create(const char *window_title, int display_index, int w, int h, AGS::Engine::WindowMode mode, int ex_flags = 0);
// Returns current game window, if one exists, or null.
SDL_Window *sys_get_window();
// Changes the window style for the given mode (fullscreen / windowed).
void sys_window_set_style(AGS::Engine::WindowMode mode, Size size = Size());
// Brings the game window to front
void sys_window_bring_to_front();
// Set new window size; optionally center new window on screen
bool sys_window_set_size(int w, int h, bool center);
// Centers the window on screen, optionally choose the display to position on
void sys_window_center(int display_index = -1);
// Reduces window's size to fit into the said display bounds, and repositions to the display's center
void sys_window_fit_in_display(int display_index);
// Shows or hides system cursor when it's in the game window
void sys_window_show_cursor(bool on);
// Locks on unlocks mouse inside the window.
// Returns new state of the mouse lock.
bool sys_window_lock_mouse(bool on);
bool sys_window_lock_mouse(bool on, const Rect &bounds);
// Sets mouse position within the game window
void sys_window_set_mouse(int x, int y);
// Destroy current game window, if one exists.
void sys_window_destroy();
// Set window title text.
void sys_window_set_title(const char *title);
// Set window icon.
// TODO: this is a placeholder, until we figure out the best way to set icon with SDL on wanted systems.
void sys_window_set_icon();

#if AGS_PLATFORM_OS_WINDOWS
// Returns game window's handle.
void* sys_win_get_window();
#endif
