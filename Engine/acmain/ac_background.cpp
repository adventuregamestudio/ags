
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_background.h"
#include "acmain/ac_draw.h"
#include "acrun/ac_runninggame.h"
#include "acmain/ac_event.h"
#include "ac/ac_common.h"

int bg_just_changed = 0;

void on_background_frame_change () {

  invalidate_screen();
  mark_current_background_dirty();
  invalidate_cached_walkbehinds();

  // get the new frame's palette
  memcpy (palette, thisroom.bpalettes[play.bg_frame], sizeof(color) * 256);

  // hi-colour, update the palette. It won't have an immediate effect
  // but will be drawn properly when the screen fades in
  if (game.color_depth > 1)
    setpal();

  if (in_enters_screen)
    return;

  // Don't update the palette if it hasn't changed
  if (thisroom.ebpalShared[play.bg_frame])
    return;

  // 256-colours, tell it to update the palette (will actually be done as
  // close as possible to the screen update to prevent flicker problem)
  if (game.color_depth == 1)
    bg_just_changed = 1;
}

void SetBackgroundFrame(int frnum) {
  if ((frnum<-1) | (frnum>=thisroom.num_bscenes))
    quit("!SetBackgrondFrame: invalid frame number specified");
  if (frnum<0) {
    play.bg_frame_locked=0;
    return;
  }

  play.bg_frame_locked = 1;

  if (frnum == play.bg_frame)
  {
    // already on this frame, do nothing
    return;
  }

  play.bg_frame = frnum;
  on_background_frame_change ();
}

int GetBackgroundFrame() {
  return play.bg_frame;
  }

