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
#ifndef __AGS_EE_MAIN__GAMERUN_H
#define __AGS_EE_MAIN__GAMERUN_H

#include "ac/keycode.h"

namespace AGS { namespace Engine { class IDriverDependantBitmap; }}

// Loops game frames until certain event takes place (for blocking actions)
void GameLoopUntilValueIsZero(const char *value);
void GameLoopUntilValueIsZero(const short *value);
void GameLoopUntilValueIsZero(const int *value);
void GameLoopUntilValueIsZeroOrLess(const short *move);
void GameLoopUntilValueIsNegative(const short *value);
void GameLoopUntilValueIsNegative(const int *value);
void GameLoopUntilNotMoving(const short *move);
void GameLoopUntilNoOverlay();
void GameLoopUntilButAnimEnd(int guin, int objn);

// Run the actual game until it ends, or aborted by player/error; loops GameTick() internally
void RunGameUntilAborted();
// Update everything game related; wait for the next frame
void UpdateGameOnce(bool checkControls = false, AGS::Engine::IDriverDependantBitmap *extraBitmap = nullptr, int extraX = 0, int extraY = 0);
// Update minimal required game state: audio, loop counter, etc; wait for the next frame
void UpdateGameAudioOnly();
// Updates everything related to object views that could have changed in the midst of a
// blocking script, cursor position and view, poll anything related to cursor position;
// this function is useful when you don't want to update whole game, but only things
// that are necessary for rendering the game screen.
void UpdateCursorAndDrawables();
// Gets current logical game FPS, this is normally a fixed number set in script;
// in case of "maxed fps" mode this function returns real measured FPS.
float get_game_fps();
// Gets real fps, calculated based on the game performance.
float get_real_fps();
// Runs service key controls, returns false if no key was pressed or key input was claimed by the engine,
// otherwise returns true and provides a keycode.
bool run_service_key_controls(KeyInput &kgn);
// Runs service mouse controls, returns false if mouse input was claimed by the engine,
// otherwise returns true and provides mouse button code.
bool run_service_mb_controls(eAGSMouseButton &mbut);
// Polls few things (exit flag and debugger messages)
// TODO: refactor this
void update_polled_stuff();


#endif // __AGS_EE_MAIN__GAMERUN_H
