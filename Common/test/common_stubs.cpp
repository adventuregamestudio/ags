//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
// 
// Stubs for the functions in Common.lib that expect implementations elsewhere.
// 
//=============================================================================
#include "ac/game_version.h"
#include "gui/guiobject.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"

using namespace AGS::Common;

GameDataVersion loaded_game_file_version = kGameVersion_Current;
Version game_compiled_version;

namespace AGS
{
namespace Common
{

void GUIControl::MarkChanged()
{
}

void GUIControl::MarkPositionChanged(bool, bool)
{
}

void GUIControl::MarkStateChanged(bool, bool)
{
}

void GUIControl::MarkVisualStateChanged()
{
}

void GUIButton::PrepareTextToDraw()
{
}

void GUIInvWindow::Draw(Bitmap *, int, int)
{
}

int GUILabel::PrepareTextToDraw()
{
    return 0;
}

void GUIListBox::PrepareTextToDraw(const String &text)
{
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, int x, int y)
{

}

} // namespace Common
} // namespace AGS

void quit(const char *) {}

int get_adjusted_spritewidth(int spr) { return 0; }
int get_adjusted_spriteheight(int spr) { return 0; }
bool is_sprite_alpha(int spr) { return false; }

void draw_gui_sprite(AGS::Common::Bitmap *ds, int spr, int x, int y,
    AGS::Common::BlendMode blend_mode) {}
void draw_gui_sprite(AGS::Common::Bitmap *ds, int x, int y,
    AGS::Common::Bitmap *image, AGS::Common::BlendMode blend_mode, int alpha) {}

void draw_gui_sprite_flipped(AGS::Common::Bitmap *ds, int pic, int x, int y, AGS::Common::BlendMode blend_mode, AGS::Common::GraphicFlip flip) {}
void draw_gui_sprite_flipped(AGS::Common::Bitmap *ds, int x, int y,
    AGS::Common::Bitmap *image, AGS::Common::BlendMode blend_mode, int alpha, AGS::Common::GraphicFlip flip) {}

int game_to_data_coord(int coord) { return coord; }
int data_to_game_coord(int coord) { return coord; }
void data_to_game_coords(int *x, int *y) {}
int get_fixed_pixel_size(int pixels) { return pixels; }

void wouttext_outline(AGS::Common::Bitmap *ds, int x, int y, int usingfont,
    color_t text_color, const char *text) {}
void wouttext_outline(AGS::Common::Bitmap *ds, int x, int y, int usingfont,
    color_t text_color, AGS::Common::BlendMode blend_mode, const char *text) {}
void wouttext_outline(AGS::Common::Bitmap *ds, int x, int y, int usingfont,
    color_t text_color, color_t outline_color, AGS::Common::BlendMode blend_mode, const char *texx) {}

void set_eip_guiobj(int eip) {}
int get_eip_guiobj() { return 0; }

bool ShouldAntiAliasText() { return false; }
