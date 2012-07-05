
#include "ac/global_walkbehind.h"
#include "ac/ac_common.h"
#include "ac/ac_defines.h"
#include "ac/roomstatus.h"
#include "ac/walkbehind.h"
#include "wgt2allg.h"
#include "acmain/ac_draw.h"
#include "debug/debug.h"

extern RoomStatus*croom;
extern int walk_behind_baselines_changed;

void SetWalkBehindBase(int wa,int bl) {
  if ((wa < 1) || (wa >= MAX_OBJ))
    quit("!SetWalkBehindBase: invalid walk-behind area specified");

  if (bl != croom->walkbehind_base[wa]) {
    walk_behind_baselines_changed = 1;
    invalidate_cached_walkbehinds();
    croom->walkbehind_base[wa] = bl;
    DEBUG_CONSOLE("Walk-behind %d baseline changed to %d", wa, bl);
  }
}
