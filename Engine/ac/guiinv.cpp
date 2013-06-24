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
#include "ac/characterextras.h"
#include "ac/spritecache.h"
#include "game/game_objects.h"
#include "gfx/bitmap.h"

using AGS::Common::Bitmap;
using AGS::Common::GuiInvWindow;


extern int gui_disabled_style;
extern CharacterExtras *charextra;
extern SpriteCache spriteset;


int GuiInvWindow::GetCharacterId()
{
    if (CharacterId < 0)
    {
        return game.PlayerCharacterIndex;
    }
    return CharacterId;
}

void GuiInvWindow::Draw(Bitmap *ds)
{
    if (IsDisabled() && gui_disabled_style == kGuiDisabled_HideControls)
    {
        return;
    }

    // backwards compatibility
    play.InventoryColCount = ColumnCount;
    play.InventoryDisplayedCount = RowCount * ColumnCount;
    play.obsolete_inv_numorder = charextra[game.PlayerCharacterIndex].invorder_count;
    // if the user changes top_inv_item, switch into backwards
    // compatibiltiy mode
    if (play.TopInvItemIndex)
    {
        play.InventoryBackwardsCompatible = 1;
    }
    if (play.InventoryBackwardsCompatible)
    {
        TopItem = play.TopInvItemIndex;
    }

    // draw the items
    int leftmost_x = Frame.Left;
    Point draw_at(Frame.Left, Frame.Top);
    int last_item = TopItem + (ColumnCount * RowCount);
    if (last_item > charextra[GetCharacterId()].invorder_count)
    {
        last_item = charextra[GetCharacterId()].invorder_count;
    }

    for (int item = TopItem; item < last_item; ++item)
    {
        // draw inv graphic
        draw_sprite_compensate(ds, game.InventoryItems[charextra[GetCharacterId()].invorder[item]].pic, draw_at.X, draw_at.Y, 1);
        draw_at.X += multiply_up_coordinate(ItemWidth);

        // go to next row when appropriate
        if ((item - TopItem) % ColumnCount == (ColumnCount - 1))
        {
            draw_at.X = leftmost_x;
            draw_at.Y += multiply_up_coordinate(ItemHeight);
        }
    }

    if (IsDisabled() &&
        gui_disabled_style == kGuiDisabled_GreyOut && 
        play.InventoryGreysOutWhenDisabled == 1)
    {
        // darken the inventory when disabled
        color_t draw_color = ds->GetCompatibleColor(8);
        int width = Frame.GetWidth();
        int height = Frame.GetHeight();
        for (int x = 0; x < width; ++x)
        {
            for (int y = x % 2; y < height; y += 2)
            {
                ds->PutPixel(Frame.Left + x, Frame.Top + y, draw_color);
            }
        }
    }
}
