//
// Implementation from acgui.h and acgui.cpp specific to Engine runtime
//

// Headers, as they are in acgui.cpp
#pragma unmanaged
#include "wgt2allg.h"
#include "acruntim.h"
#include "acgui/ac_guimain.h"
#include "acgui/ac_guibutton.h"
#include "acgui/ac_guilabel.h"
#include "acgui/ac_guilistbox.h"
#include "acgui/ac_guitextbox.h"
#include <ctype.h>

#include "bigend.h"

// For engine these are defined in ac.cpp
extern int eip_guiobj;
extern void replace_macro_tokens(char*,char*);

// For engine these are defined in acfonts.cpp
extern void ensure_text_valid_for_font(char *, int);
//

//=============================================================================
// Engine-specific implementation split out of acgui.h
//=============================================================================

int GUIObject::IsClickable()
{
  return !(flags & GUIF_NOCLICKS);
}

void check_font(int *fontnum)
{
    // do nothing
}

//=============================================================================
// Engine-specific implementation split out of acgui.cpp
//=============================================================================

int get_adjusted_spritewidth(int spr)
{
  return wgetblockwidth(spriteset[spr]);
}

int get_adjusted_spriteheight(int spr)
{
  return wgetblockheight(spriteset[spr]);
}

bool is_sprite_alpha(int spr)
{
  return ((game.spriteflags[spr] & SPF_ALPHACHANNEL) != 0);
}

void set_eip_guiobj(int eip)
{
  eip_guiobj = eip;
}

int get_eip_guiobj()
{
  return eip_guiobj;
}

bool outlineGuiObjects = false;

void GUILabel::Draw_replace_macro_tokens(char *oritext, char *text)
{
  replace_macro_tokens(get_translation(text), oritext);
  ensure_text_valid_for_font(oritext, font);
}

void GUILabel::Draw_split_lines(char *teptr, int wid, int font, int &numlines)
{
  // Use the engine's word wrap tool, to have hebrew-style writing
  // and other features

  break_up_text_into_lines (wid, font, teptr);

  // [IKM] numlines not used in engine's implementation
}

void GUITextBox::Draw_text_box_contents()
{
  int startx, starty;

  wouttext_outline(x + 1 + get_fixed_pixel_size(1), y + 1 + get_fixed_pixel_size(1), font, text);
  
  if (!IsDisabled()) {
    // draw a cursor
    startx = wgettextwidth(text, font) + x + 3;
    starty = y + 1 + wgettextheight("BigyjTEXT", font);
    wrectangle(startx, starty, startx + get_fixed_pixel_size(5), starty + (get_fixed_pixel_size(1) - 1));
  }
}

void GUIListBox::Draw_items_fix()
{
  // do nothing
}

void GUIListBox::Draw_items_unfix()
{
  // do nothing
}

void GUIButton::Draw_set_oritext(char *oritext, const char *text)
{
  // Allow it to change the string to unicode if it's TTF
  strcpy(oritext, get_translation(text));
  ensure_text_valid_for_font(oritext, font);

  // oritext is assumed to be made long enough by caller function
}
