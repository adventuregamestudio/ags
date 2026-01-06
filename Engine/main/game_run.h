//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_MAIN__GAMERUN_H
#define __AGS_EE_MAIN__GAMERUN_H

#include "ac/keycode.h"

namespace AGS { namespace Engine { class IDriverDependantBitmap; }}

// Loops game frames until certain event takes place (for blocking actions)
// FIXME: replace arg types with intX_t types
void GameLoopUntilValueIsZero(const char *value);
void GameLoopUntilValueIsZero(const short *value);
void GameLoopUntilValueIsZero(const int *value);
void GameLoopUntilValueIsNegative(const short *value);
void GameLoopUntilValueIsNegative(const int *value);
void GameLoopUntilNotMoving(const short *move);
void GameLoopUntilNoOverlay();
void GameLoopUntilButAnimEnd(int guin, int objn);

// Run the actual game until it ends, or aborted by player/error; loops GameTick() internally
void RunGameUntilAborted();
// Update everything game related; wait for the next frame
// * do_controls - tells whether to handle player input this time
// * extra_ddb, x and y - adds an additional arbitrary texture to the render list,
//       rendered on top of everything else (except cursor).
//       This is a leftover used only for built-in dialogs, and must be replaced eventually.
void UpdateGameOnce(bool do_controls = false, AGS::Engine::IDriverDependantBitmap *extra_ddb = nullptr, int extra_x = 0, int extra_y = 0);
// Update minimal required game state: audio, loop counter, etc; wait for the next frame
void UpdateGameAudioOnly();
// Updates everything related to object views that could have changed in the midst of a
// blocking script, cursor position and view, poll anything related to cursor position;
// this function is useful when you don't want to update whole game, but only things
// that are necessary for rendering the game screen.
void UpdateCursorAndDrawables();
// Syncs object drawable states with their logical states.
// Useful after a major game state change, such as loading new room, in case we expect
// that a render may occur before a normal game update is performed.
void SyncDrawablesState();
// Checks if currently in blocking action state (blocking action, or Wait called from script).
bool IsInBlockingAction();
// Shuts down game's waiting state, if one is running right now.
void ShutGameWaitState();

// Gets current logical game FPS, this is normally a fixed number set in script;
// in case of "maxed fps" mode this function returns real measured FPS.
float get_game_fps();
// Gets real fps, calculated based on the game performance.
float get_real_fps();
// Sets game frame index
void set_loop_counter(uint32_t new_counter);
// Increments game frame index once
void increment_loop_counter();
// Gets game frame index, counted since the game started
uint32_t get_loop_counter();
// Reset runtime counter
void set_runtime_start();
// Tells if the runtime counter was initialized
bool is_runtime_set();
// Gets real runtime duration in ms, for diagnostic purposes
uint32_t get_runtime_ms();

// Runs service key controls, returns false if no key was pressed or key input was claimed by the engine,
// otherwise returns true and provides a keycode.
bool run_service_key_controls(KeyInput &kgn);
// Runs service mouse controls, returns false if mouse input was claimed by the engine,
// otherwise returns true and provides mouse button code.
bool run_service_mb_controls(eAGSMouseButton &mbut, Point *mpos = nullptr);
// Polls few things (exit flag and debugger messages)
// TODO: refactor this
void update_polled_stuff();


#endif // __AGS_EE_MAIN__GAMERUN_H
