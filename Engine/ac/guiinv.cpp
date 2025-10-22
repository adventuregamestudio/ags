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


namespace AGS
{
namespace Common
{

void GUIInvWindow::SetTopItem(int item)
{
    // NOTE: unfortunately, we do not have an access to real inventory here,
    // so cannot clamp to the upper item limit
    item = std::max(0, item);
    if (_topItem != item)
    {
        _topItem = item;
        MarkChanged();
    }
}

int GUIInvWindow::GetCharacterID() const
{
    if (_charID < 0)
        return game.playercharacter;

    return _charID;
}

void GUIInvWindow::Draw(Bitmap *ds, int x, int y)
{
    const bool draw_disabled = GUI::ShouldDrawDisabled(this);

    // draw the items
    const int leftmost_x = x;
    int at_x = x;
    int at_y = y;
    int lastItem = _topItem + (_colCount * _rowCount);
    if (lastItem > charextra[GetCharacterID()].invorder_count)
        lastItem = charextra[GetCharacterID()].invorder_count;

    for (int item = _topItem; item < lastItem; ++item)
    {
        // draw inv graphic
        draw_gui_sprite(ds, game.invinfo[charextra[GetCharacterID()].invorder[item]].pic, at_x, at_y);
        at_x += _itemWidth;

        // go to next row when appropriate
        if ((item - _topItem) % _colCount == (_colCount - 1))
        {
            at_x = leftmost_x;
            at_y += _itemHeight;
        }
    }

    if (draw_disabled)
    {
        // darken the inventory when disabled
        GUI::DrawDisabledEffect(ds, RectWH(x, y, _width, _height));
    }
}

} // namespace Common
} // namespace AGS
