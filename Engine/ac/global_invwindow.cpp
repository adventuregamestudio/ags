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

#include "ac/gamestate.h"
#include "ac/global_invwindow.h"
#include "ac/global_translation.h"
#include "ac/properties.h"
#include "gui/guimain.h"
#include "gui/guiinv.h"
#include "script/executingscript.h"

using namespace AGS::Common;

extern ExecutingScript*curscript;
extern GameState play;

void sc_invscreen() {
    curscript->queue_action(ePSAInvScreen, 0, "InventoryScreen");
}

void SetInvDimensions(int ww,int hh) {
    play.inv_item_wid = ww;
    play.inv_item_hit = hh;
    play.inv_numdisp = 0;
    // backwards compatibility
    for (auto &inv : guiinv)
    {
        inv.ItemWidth = ww;
        inv.ItemHeight = hh;
        inv.OnResized();
    }
}
