//
// Implementation from acgui.h and acgui.cpp specific to AGS.Native library
//

#pragma unmanaged

#include "font/fonts.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
#include "util/string_utils.h"

using AGS::Common::GuiButton;
using AGS::Common::GuiInvWindow;
using AGS::Common::GuiLabel;
using AGS::Common::GuiListBox;
using AGS::Common::GuiObject;
using AGS::Common::GuiTextBox;
using AGS::Common::String;

//=============================================================================
// AGS.Native-specific implementation split out of acgui.h
//=============================================================================

bool GuiObject::IsClickable() const
{
  // make sure the button can be selected in the editor
  return true;
}

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

//-----------------------------------------------------------------------------

void GuiButton::PrepareTextToDraw()
{
    TextToDraw = Text;
}

void GuiInvWindow::Draw(Common::Bitmap *ds)
{
    color_t draw_color = ds->GetCompatibleColor(15);
    ds->DrawRect(Rect(Frame.Left, Frame.Top, Frame.Left + Frame.GetWidth(), Frame.Top + Frame.GetHeight()), draw_color);
}

void GuiLabel::PrepareTextToDraw()
{
    TextToDraw = Text;
}

extern int numlines;
int GuiLabel::SplitLinesForDrawing()
{
    numlines=0;
    split_lines_leftright(TextToDraw, Frame.GetWidth(), TextFont);
    return numlines;
}

static int num_items_temp;
void GuiListBox::DrawItemsFix()
{
    num_items_temp = ItemCount;
    ItemCount = 2;
    Items.SetLength(2);
    Items[0] = "Sample selected";
    Items[1] = "Sample item";
}

void GuiListBox::DrawItemsUnfix()
{
    ItemCount = num_items_temp;
}

void GuiListBox::PrepareTextToDraw(const String &text)
{
    TextToDraw = text;
}

void GuiTextBox::DrawTextBoxContents(Common::Bitmap *ds, color_t text_color)
{
    // print something fake so we can see what it looks like
    wouttext_outline(ds, Frame.Left + 2, Frame.Top + 2, TextFont, TextColor, "Text Box Contents");
}
