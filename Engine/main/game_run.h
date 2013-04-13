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

#include "gfx/ali3d.h"

namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS::Engine; // FIXME later

// Run single game iteration; calls UpdateGameOnce() internally
int GameTick();
// Update everything game related
void UpdateGameOnce(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
// Loops GameTick() until certain event takes place (for blocking actions)
void GameLoopUntilEvent(int untilwhat,long daaa);
// Checks if user interface should remain disabled for now
int  ShouldStayInWaitMode();
// Increases game frame count; used for recording/replay only
void NextIteration();
// Run the actual game until it ends, or aborted by player/error; loops GameTick() internally
void RunGameUntilAborted();

#endif // __AGS_EE_MAIN__GAMERUN_H
