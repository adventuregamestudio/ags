//
// Implementation from acgui.h and acgui.cpp specific to AGS.Native library
//

// Headers, as they are in acgui.cpp
#pragma unmanaged
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#define CROOM_NOFUNCTIONS
#include "acroom.h"
#include "acruntim.h"
#include "acgui.h"
#include <ctype.h>

#include "bigend.h"

#undef CROOM_NOFUNCTIONS

//=============================================================================
// AGS.Native-specific implementation split out of acgui.h
//=============================================================================

int GUIObject::IsClickable()
{
  // make sure the button can be selected in the editor
  return 1;
}

void wouttext_outline(int xxp, int yyp, int usingfont, char *texx)
{
  wouttextxy(xxp, yyp, usingfont, texx);
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

void GUILabel::Draw_replace_macro_tokens(char *oritext, char *text)
{
  strcpy(oritext, text);
}

void GUILabel::Draw_split_lines(char *teptr, int wid, int font, int &numlines)
{
  numlines=0;
  split_lines_leftright(teptr, wid, font);
}

void GUITextBox::Draw_text_box_contents()
{
  // print something fake so we can see what it looks like
  wouttext_outline(x + 2, y + 2, font, "Text Box Contents");
}

void GUIListBox::Draw_items_fix()
{
  numItemsTemp = numItems;
  numItems = 2;
  items[0] = "Sample selected";
  items[1] = "Sample item";
  //
  // [IKM] 2012-06-08: a-a-a-and just to test this thing is working....
  numItems = 3;
  items[2] = "This item means HACK!! >:]";
  // ^_^
  //
}

void GUIListBox::Draw_items_unfix()
{
  numItems = numItemsTemp;
}

void GUIButton::Draw_set_oritext(char *oritext, const char *text)
{
  strcpy(oritext, text);

  // original code was:
  //      oritext = text; 
  // it seems though the 'text' variable is assumed to be a null-terminated string
  // oritext is assumed to be made long enough by caller function
}
