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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DISPLAY_H
#define __AGS_EE_AC__DISPLAY_H

#include "gui/guimain.h"

int  _display_main(int xx,int yy,int wii,char*todis,int blocking,int usingfont,int asspch, int isThought, int allowShrink, bool overlayPositionFixed);
void _display_at(int xx,int yy,int wii,char*todis,int blocking,int asspch, int isThought, int allowShrink, bool overlayPositionFixed);
bool ShouldAntiAliasText();
int GetTextDisplayTime (const char *text, int canberel=0);
void wouttext_outline(Common::Graphics *g, int xxp, int yyp, int usingfont, char*texx);
void wouttext_aligned (Common::Graphics *g, int usexp, int yy, int oriwid, int usingfont, const char *text, int align);
int wgetfontheight(int font);
int wgettextwidth_compensate(const char *tex, int font);
void do_corner(Common::Graphics *g, int sprn,int xx1,int yy1,int typx,int typy);
int get_but_pic(GUIMain*guo,int indx);
void draw_button_background(Common::Graphics *g, int xx1,int yy1,int xx2,int yy2,GUIMain*iep);
// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width (int twgui);
// get the hegiht of the text window's top border
int get_textwindow_top_border_height (int twgui);
void draw_text_window(Common::Graphics *g, int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight, int ifnum);
void draw_text_window_and_bar(Common::Graphics *g, int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight=0, int ifnum=-1);

#endif // __AGS_EE_AC__DISPLAY_H
