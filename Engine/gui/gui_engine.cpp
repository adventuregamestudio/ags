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
//
// Implementation from acgui.h and acgui.cpp specific to Engine runtime
//
//=============================================================================

// Headers, as they are in acgui.cpp
#pragma unmanaged
#include "ac/game_version.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guibutton.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
#include <ctype.h>
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "gfx/blender.h"
#include "main/graphics_mode.h"

using namespace Common;
using namespace Engine;

// For engine these are defined in ac.cpp
extern int eip_guiobj;
extern void replace_macro_tokens(const char*,char*);

// For engine these are defined in acfonts.cpp
extern void ensure_text_valid_for_font(char *, int);
//

extern SpriteCache spriteset; // in ac_runningame
extern GameSetupStruct game;

bool GUIMain::HasAlphaChannel() const
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
    return ScreenResolution.ColorDepth >= 24 &&
        // transparent background have alpha channel only since 3.2.0;
        // "classic" gui rendering mode historically had non-alpha transparent backgrounds
        // (3.2.0 broke the compatibility, now we restore it)
        loaded_game_file_version >= kGameVersion_320 && game.options[OPT_NEWGUIALPHA] != kGuiAlphaRender_Classic;
}

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
  return spriteset[spr]->GetWidth();
}

int get_adjusted_spriteheight(int spr)
{
  return spriteset[spr]->GetHeight();
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

void GUILabel::Draw_replace_macro_tokens(char *oritext, const char *text)
{
  replace_macro_tokens(flags & GUIF_TRANSLATED ? get_translation(text) : text, oritext);
  ensure_text_valid_for_font(oritext, font);
}

void GUILabel::Draw_split_lines(char *teptr, int wid, int font, int &numlines)
{
  // Use the engine's word wrap tool, to have hebrew-style writing
  // and other features

  break_up_text_into_lines (wid, font, teptr);

  // [IKM] numlines not used in engine's implementation
}

void GUITextBox::Draw_text_box_contents(Bitmap *ds, color_t text_color)
{
  int startx, starty;

  wouttext_outline(ds, x + 1 + get_fixed_pixel_size(1), y + 1 + get_fixed_pixel_size(1), font, text_color, text);
  
  if (!IsDisabled()) {
    // draw a cursor
    startx = wgettextwidth(text, font) + x + 3;
    starty = y + 1 + wgettextheight("BigyjTEXT", font);
    ds->DrawRect(Rect(startx, starty, startx + get_fixed_pixel_size(5), starty + (get_fixed_pixel_size(1) - 1)), text_color);
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

void GUIListBox::Draw_set_oritext(char *oritext, const char *text)
{
    // Allow it to change the string to unicode if it's TTF
    if (flags & GUIF_TRANSLATED)
    {
        strcpy(oritext, get_translation(text));
    }
    else
    {
        strcpy(oritext, text);
    }
    ensure_text_valid_for_font(oritext, font);

    // oritext is assumed to be made long enough by caller function
}

void GUIButton::Draw_set_oritext(char *oritext, const char *text)
{
  // Allow it to change the string to unicode if it's TTF
    if (flags & GUIF_TRANSLATED)
    {
        strcpy(oritext, get_translation(text));
    }
    else
    {
        strcpy(oritext, text);
    }
    ensure_text_valid_for_font(oritext, font);

  // oritext is assumed to be made long enough by caller function
}
