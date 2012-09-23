
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREEN_H
#define __AGS_EE_AC__SCREEN_H

#include "gfx/ali3d.h"

namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS::Engine; // FIXME later

void my_fade_in(PALLETE p, int speed);
void current_fade_out_effect ();
IDriverDependantBitmap* prepare_screen_for_transition_in();

#endif // __AGS_EE_AC__SCREEN_H
