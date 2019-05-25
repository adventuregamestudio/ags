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
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__GAMERUN_H
#define __AGS_EE_MAIN__GAMERUN_H

namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS::Engine; // FIXME later

// Loops game frames until certain event takes place (for blocking actions)
void GameLoopUntilValueIsZero(const char *value);
void GameLoopUntilValueIsZero(const short *value);
void GameLoopUntilValueIsZero(const int *value);
void GameLoopUntilValueIsZeroOrLess(const short *move);
void GameLoopUntilValueIsNegative(const short *value);
void GameLoopUntilValueIsNegative(const int *value);
void GameLoopUntilNotMoving(const short *move);
void GameLoopUntilNoOverlay();

// Run the actual game until it ends, or aborted by player/error; loops GameTick() internally
void RunGameUntilAborted();
// Update everything game related
void UpdateGameOnce(bool checkControls = false, IDriverDependantBitmap *extraBitmap = nullptr, int extraX = 0, int extraY = 0);

float get_current_fps();
// Runs service key controls, returns false if service key combinations were handled
// and no more processing required, otherwise returns true and provides current keycode and key shifts.
bool run_service_key_controls(int &kgn);

#endif // __AGS_EE_MAIN__GAMERUN_H
