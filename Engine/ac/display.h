//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DISPLAY_H
#define __AGS_EE_AC__DISPLAY_H

#include "gui/guimain.h"

using AGS::Common::GUIMain;

// options for 'disp_type' parameter
// blocking speech
#define DISPLAYTEXT_SPEECH        0
// super-blocking message box
#define DISPLAYTEXT_MESSAGEBOX    1
// regular non-blocking overlay
#define DISPLAYTEXT_NORMALOVERLAY 2
// also accepts explicit overlay ID >= OVER_CUSTOM

struct ScreenOverlay;
// Generates a textual image from the given text and parameters;
// see _display_main's comment below for parameters description.
// NOTE: this function treats text as-is, not doing any processing over it.
Common::Bitmap *create_textual_image(const char *text, int asspch, int isThought,
    int &xx, int &yy, int &adjustedXX, int &adjustedYY, int wii, int usingfont, int allowShrink,
    bool &alphaChannel);
// Creates a textual overlay using the given parameters;
// Pass yy = -1 to find Y co-ord automatically
// allowShrink = 0 for none, 1 for leftwards, 2 for rightwards
// pass blocking=2 to create permanent overlay
// asspch has several meanings, which affect how the message is positioned
//   == 0 - standard display box
//   != 0 - text color for a speech or a regular textual overlay, where
//     < 0 - use text window if applicable
//     > 0 - suppose it's a classic LA-style speech above character's head
// NOTE: this function treats the text as-is; it assumes that any processing
// (translation, parsing voice token) was done prior to its call.
ScreenOverlay *display_main(int xx, int yy, int wii, const char *text, int disp_type, int usingfont,
    int asspch, int isThought, int allowShrink, bool overlayPositionFixed, bool roomlayer = false);
// Displays a standard blocking message box at a given position
void display_at(int xx, int yy, int wii, const char *text);
// Cleans up display message state
void post_display_cleanup();
// Tests the given string for the voice-over tags and plays cue clip for the given character;
// will assign replacement string, which will be blank string if game is in "voice-only" mode
// and clip was started, or string cleaned from voice-over tags which is safe to display on screen.
// Returns whether voice-over clip was started successfully.
bool try_auto_play_speech(const char *text, const char *&replace_text, int charid);
// Calculates meaningful length of the displayed text
int GetTextDisplayLength(const char *text);
// Calculates number of game loops for displaying a text on screen
int GetTextDisplayTime(const char *text, int canberel = 0);
// Draw an outline if requested, then draw the text on top 
void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx);
void wouttext_aligned (Common::Bitmap *ds, int usexp, int yy, int oriwid, int usingfont, color_t text_color, const char *text, HorAlignment align);
void do_corner(Common::Bitmap *ds, int sprn,int xx1,int yy1,int typx,int typy);
// Returns the image of a button control on the GUI under given child index
int get_but_pic(GUIMain*guo,int indx);
void draw_button_background(Common::Bitmap *ds, int xx1,int yy1,int xx2,int yy2,GUIMain*iep);
// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width (int twgui);
// get the hegiht of the text window's top border
int get_textwindow_top_border_height (int twgui);
// draw_text_window: draws the normal or custom text window
// create a new bitmap the size of the window before calling, and
//   point text_window_ds to it
// returns text start x & y pos in parameters
// Warning!: draw_text_window() and draw_text_window_and_bar() can create new text_window_ds
void draw_text_window(Common::Bitmap **text_window_ds, bool should_free_ds, int*xins,int*yins,int*xx,int*yy,int*wii,color_t *set_text_color,int ovrheight, int ifnum);
void draw_text_window_and_bar(Common::Bitmap **text_window_ds, bool should_free_ds,
                              int*xins,int*yins,int*xx,int*yy,int*wii,color_t *set_text_color,int ovrheight=0, int ifnum=-1);
int get_textwindow_padding(int ifnum);

// The efficient length of the last source text prepared for display
extern int source_text_length;

#endif // __AGS_EE_AC__DISPLAY_H
