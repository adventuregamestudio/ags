//
// Implementation from acgui.h and acgui.cpp specific to AGS.Native library
//

#pragma unmanaged

#include "ac/gamesetupstruct.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
#include "util/string_utils.h"

extern GameSetupStruct thisgame;

bool AGS::Common::GUIMain::HasAlphaChannel() const
{
    if (this->BgImage > 0)
    {
        // alpha state depends on background image
        return is_sprite_alpha(this->BgImage);
    }
    if (this->BgColor > 0)
    {
        // not alpha transparent if there is a background color
        return false;
    }
    // transparent background, enable alpha blending
    return thisgame.color_depth * 8 >= 24;
}

//=============================================================================
// AGS.Native-specific implementation split out of acgui.h
//=============================================================================

void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx)
{
  wouttextxy(ds, xxp, yyp, usingfont, text_color, texx);
}

//=============================================================================
// AGS.Native-specific implementation split out of acgui.cpp
//=============================================================================

int final_col_dep = 32;

bool is_sprite_alpha(int spr)
{
  return false;
}

void set_eip_guiobj(int eip)
{
  // do nothing
}

int get_eip_guiobj()
{
  return 0;
}

int wgettextwidth_compensate(const char *tex, int font)
{
  return wgettextwidth(tex, font);
}

namespace AGS
{
namespace Common
{

bool GUIObject::IsClickable() const
{
    // make sure the button can be selected in the editor
    return true;
}

void GUILabel::PrepareTextToDraw()
{
    _textToDraw = Text;
}

int GUILabel::SplitLinesForDrawing()
{
    numlines = 0;
    split_lines(_textToDraw, Width, Font);
    return numlines;
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, color_t text_color)
{
    // print something fake so we can see what it looks like
    wouttext_outline(ds, X + 2, Y + 2, Font, text_color, "Text Box Contents");
}

static int num_items_temp;
void GUIListBox::DrawItemsFix()
{
    num_items_temp = ItemCount;
    ItemCount = 2;
    Items.push_back("Sample selected");
    Items.push_back("Sample item");
}

void GUIListBox::DrawItemsUnfix()
{
    Items.clear();
    ItemCount = num_items_temp;
}

void GUIListBox::PrepareTextToDraw(const String &text)
{
    _textToDraw = text;
}

void GUIInvWindow::Draw(Bitmap *ds)
{
    color_t draw_color = ds->GetCompatibleColor(15);
    ds->DrawRect(Rect(X, Y, X + Width, Y + Height), draw_color);
}

void GUIButton::PrepareTextToDraw()
{
    _textToDraw = _text;
}

} // namespace Common
} // namespace AGS
