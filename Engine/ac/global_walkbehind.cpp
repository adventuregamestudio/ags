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

#include "ac/global_walkbehind.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/draw.h"
#include "ac/walkbehind.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"

extern int walk_behind_baselines_changed;

void SetWalkBehindBase(int wa,int bl) {
  if ((wa < 1) || (wa >= thisroom.WalkBehindCount))
    quit("!SetWalkBehindBase: invalid walk-behind area specified");

  if (bl != croom->WalkBehinds[wa].Baseline) {
    walk_behind_baselines_changed = 1;
    invalidate_cached_walkbehinds();
    croom->WalkBehinds[wa].Baseline = bl;
    DEBUG_CONSOLE("Walk-behind %d baseline changed to %d", wa, bl);
  }
}
