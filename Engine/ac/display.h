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
//
// Functions for displaying various standard textual overlays.
//
//=============================================================================
#ifndef __AGS_EE_AC__DISPLAY_H
#define __AGS_EE_AC__DISPLAY_H

#include "gui/guimain.h"
#include "gfx/bitmap.h"
#include "util/string.h"

using namespace AGS; // FIXME later

// The general type and behavior of the displayed text
enum DisplayTextType
{
    kDisplayText_MessageBox,    // a super-blocking message box
    kDisplayText_Speech,        // a blocking character speech
    kDisplayText_NormalOverlay  // a custom text overlay
};

// More specific visual look of the displayed text
enum DisplayTextStyle
{
    kDisplayTextStyle_MessageBox,   // standard message box
    kDisplayTextStyle_TextWindow,   // use text window GUI, if applicable
    kDisplayTextStyle_Overchar,     // display text above a character
};

struct TopBarSettings
{
    Common::String Text;
    int Font = 0;
    int Height = 0;

    TopBarSettings() = default;
    TopBarSettings(const AGS::Common::String &text, int font, int height)
        : Text(text), Font(font), Height(height) {}
};

// TODO: this struct seems bit redundant now, but may be merged with other display params
struct DisplayVars
{
    int Linespacing = 0;   // font's line spacing
    int FullTextHeight = 0; // total height of all the text

    DisplayVars() = default;
    DisplayVars(int linespacing, int fulltxheight)
        : Linespacing(linespacing), FullTextHeight(fulltxheight) {}
};

struct ScreenOverlay;

// Generates a textual image from the given text and parameters;
// see _display_main's comment below for parameters description.
// NOTE: this function treats text as-is, not doing any processing over it.
// TODO: refactor this collection of args into 1-2 structs with params.
Common::Bitmap *create_textual_image(const char *text, DisplayTextStyle style, color_t text_color, int isThought,
    int &xx, int &yy, int &adjustedXX, int &adjustedYY, int wii, int usingfont, int allowShrink,
    bool &alphaChannel, const TopBarSettings *topbar);
// Creates a textual overlay using the given parameters;
// * disp_type tells the wanted type of overlay and behavior (blocking, etc);
// * over_id - OVER_CUSTOM for auto overlay, but allows to pass actual overlay id
//    to use, valid only if kDisplayText_NormalOverlay;
//    FIXME: find a way to not pass over_id at all, this logic is bad
// * style - more specific desired look (use text window, etc);
// * pass yy = -1 to find Y co-ord automatically;
// * allowShrink = 0 for none, 1 for leftwards, 2 for rightwards.
// NOTE: this function treats the text as-is; it assumes that any processing
// (translation, parsing voice token) was done prior to its call.
// TODO: refactor this collection of args into few args + 1-2 structs with extended params.
ScreenOverlay *display_main(int xx, int yy, int wii, const char *text,
    const TopBarSettings *topbar, DisplayTextType disp_type, int over_id,
    DisplayTextStyle style, int usingfont, color_t text_color, int isThought,
    int allowShrink, bool overlayPositionFixed, bool roomlayer = false);
// Displays a standard blocking message box at a given position
void display_at(int xx, int yy, int wii, const char *text, const TopBarSettings *topbar);
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
int get_but_pic(Common::GUIMain*guo,int indx);
void draw_button_background(Common::Bitmap *ds, int xx1,int yy1,int xx2,int yy2,Common::GUIMain*iep);
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
void draw_text_window(Common::Bitmap **text_window_ds, bool should_free_ds, int*xins,int*yins,int*xx,int*yy,int*wii,
    color_t *set_text_color,int ovrheight, int ifnum, const DisplayVars &disp);
void draw_text_window_and_bar(Common::Bitmap **text_window_ds, bool should_free_ds,
                              const TopBarSettings *topbar, const DisplayVars &disp,
                              int*xins,int*yins,int*xx,int*yy,int*wii,color_t *set_text_color,int ovrheight=0, int ifnum=-1);
int get_textwindow_padding(int ifnum);

// The efficient length of the last source text prepared for display
extern int source_text_length;

#endif // __AGS_EE_AC__DISPLAY_H
