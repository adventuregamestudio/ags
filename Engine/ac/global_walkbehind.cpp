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

#include "ac/global_walkbehind.h"
#include "ac/common.h"
#include "ac/common_defines.h"
#include "ac/draw.h"
#include "ac/roomstatus.h"
#include "ac/walkbehind.h"
#include "debug/debug_log.h"

extern RoomStatus*croom;

void SetWalkBehindBase(int wa,int bl) {
  if ((wa < 1) || (wa >= MAX_WALK_BEHINDS))
    quit("!SetWalkBehindBase: invalid walk-behind area specified");

  if (bl != croom->walkbehind_base[wa]) {
    walk_behind_baselines_changed = 1;
    croom->walkbehind_base[wa] = bl;
    debug_script_log("Walk-behind %d baseline changed to %d", wa, bl);
  }
}
