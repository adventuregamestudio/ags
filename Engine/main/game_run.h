
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

void do_main_cycle(int untilwhat,long daaa);
void mainloop(bool checkControls = false, IDriverDependantBitmap *extraBitmap = NULL, int extraX = 0, int extraY = 0);
int  main_game_loop();
int  wait_loop_still_valid();
void next_iteration();

#endif // __AGS_EE_MAIN__GAMERUN_H
