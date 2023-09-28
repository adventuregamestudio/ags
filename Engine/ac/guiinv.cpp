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


extern GameSetupStruct game;
extern GameState play;


namespace AGS
{
namespace Common
{

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
        draw_gui_sprite(ds, game.invinfo[charextra[GetCharacterId()].invorder[item]].pic, at_x, at_y);
        at_x += ItemWidth;

        // go to next row when appropriate
        if ((item - TopItem) % ColCount == (ColCount - 1))
        {
            at_x = leftmost_x;
            at_y += ItemHeight;
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
