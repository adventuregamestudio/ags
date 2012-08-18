
#include "ac/gamestate.h"
#include "ac/global_invwindow.h"
#include "ac/global_translation.h"
#include "ac/properties.h"
#include "gui/guiinv.h"
#include "script/executingscript.h"

extern ExecutingScript*curscript;
extern GameState play;
extern DynamicArray<GUIInv> guiinv;
extern int numguiinv;
extern int guis_need_update;

void sc_invscreen() {
    curscript->queue_action(ePSAInvScreen, 0, "InventoryScreen");
}

void SetInvDimensions(int ww,int hh) {
    play.inv_item_wid = ww;
    play.inv_item_hit = hh;
    play.inv_numdisp = 0;
    // backwards compatibility
    for (int i = 0; i < numguiinv; i++) {
        guiinv[i].itemWidth = ww;
        guiinv[i].itemHeight = hh;
        guiinv[i].Resized();
    }
    guis_need_update = 1;
}
