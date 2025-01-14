//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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


extern GameSetupStruct game;
extern GameState play;


namespace AGS
{
namespace Common
{

bool GUIInvWindow::HasAlphaChannel() const
{
    // We would have to test every inventory item's graphic to tell precisely,
    // so just test game color depth instead:
    return game.GetColorDepth() == 32;
}

int GUIInvWindow::GetCharacterId() const
{
    if (CharId < 0)
        return game.playercharacter;

    return CharId;
}

void GUIInvWindow::Draw(Bitmap *ds, int x, int y)
{
    const bool enabled = IsGUIEnabled(this);
    if (!enabled && (GUI::Options.DisabledStyle == kGuiDis_Blackout))
        return;

    // backwards compatibility
    // TODO: find a way to not have this inside GUIInvWindow::Draw!
    play.inv_numinline = ColCount;
    play.inv_numdisp = RowCount * ColCount;
    play.inv_numorder = charextra[game.playercharacter].invorder_count;
    // if the user changes top_inv_item, switch into backwards compat mode
    if (play.inv_top != 0)
        play.inv_backwards_compatibility = 1;
    if (play.inv_backwards_compatibility)
        TopItem = play.inv_top;

    // draw the items
    const int leftmost_x = x;
    int at_x = x;
    int at_y = y;
    int lastItem = TopItem + (ColCount * RowCount);
    if (lastItem > charextra[GetCharacterId()].invorder_count)
        lastItem = charextra[GetCharacterId()].invorder_count;

    for (int item = TopItem; item < lastItem; ++item)
    {
        // draw inv graphic
        draw_gui_sprite(ds, game.invinfo[charextra[GetCharacterId()].invorder[item]].pic, at_x, at_y, true);
        at_x += data_to_game_coord(ItemWidth);

        // go to next row when appropriate
        if ((item - TopItem) % ColCount == (ColCount - 1))
        {
            at_x = leftmost_x;
            at_y += data_to_game_coord(ItemHeight);
        }
    }

    if (!enabled &&
        GUI::Options.DisabledStyle == kGuiDis_Greyout &&
        play.inventory_greys_out == 1)
    {
        // darken the inventory when disabled
        GUI::DrawDisabledEffect(ds, RectWH(x, y, _width, _height));
    }
}

} // namespace Common
} // namespace AGS
