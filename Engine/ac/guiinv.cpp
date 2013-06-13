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

#include "gui/guiinv.h"
#include "gui/guimain.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/characterextras.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;


extern GameSetupStruct game;
extern int gui_disabled_style;
extern GameState play;
extern CharacterExtras *charextra;
extern SpriteCache spriteset;


int GUIInv::CharToDisplay() {
    if (this->charId < 0)
        return game.playercharacter;

    return this->charId;
}

void GUIInv::Draw(Bitmap *ds) {
    if ((IsDisabled()) && (gui_disabled_style == GUIDIS_BLACKOUT))
        return;

    // backwards compatibility
    play.inv_numinline = this->itemsPerLine;
    play.inv_numdisp = this->numLines * this->itemsPerLine;
    play.obsolete_inv_numorder = charextra[game.playercharacter].invorder_count;
    // if the user changes top_inv_item, switch into backwards
    // compatibiltiy mode
    if (play.inv_top) {
        play.inv_backwards_compatibility = 1;
    }

    if (play.inv_backwards_compatibility) {
        this->topIndex = play.inv_top;
    }

    // draw the items
    int xxx = x;
    int uu, cxp = x, cyp = y;
    int lastItem = this->topIndex + (this->itemsPerLine * this->numLines);
    if (lastItem > charextra[this->CharToDisplay()].invorder_count)
        lastItem = charextra[this->CharToDisplay()].invorder_count;

    for (uu = this->topIndex; uu < lastItem; uu++) {
        // draw inv graphic
        draw_sprite_compensate(ds, game.invinfo[charextra[this->CharToDisplay()].invorder[uu]].pic, cxp, cyp, 1);
        cxp += multiply_up_coordinate(this->itemWidth);

        // go to next row when appropriate
        if ((uu - this->topIndex) % this->itemsPerLine == (this->itemsPerLine - 1)) {
            cxp = xxx;
            cyp += multiply_up_coordinate(this->itemHeight);
        }
    }

    if ((IsDisabled()) &&
        (gui_disabled_style == GUIDIS_GREYOUT) && 
        (play.inventory_greys_out == 1)) {
            color_t draw_color = ds->GetCompatibleColor(8);
            int jj, kk;   // darken the inventory when disabled
            for (jj = 0; jj < wid; jj++) {
                for (kk = jj % 2; kk < hit; kk += 2)
                    ds->PutPixel(x + jj, y + kk, draw_color);
            }
    }

}
