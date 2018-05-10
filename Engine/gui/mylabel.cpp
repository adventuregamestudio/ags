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

#include <string.h>
#include "ac/display.h"
#include "ac/gamesetup.h"
#include "ac/string.h"
#include "font/fonts.h"
#include "gui/guidefines.h"
#include "gui/mylabel.h"
#include "gui/guidialoginternaldefs.h"

using Common::Bitmap;

extern GameSetup usetup;
// ac_guimain
extern int numlines;
extern char lines[MAXLINE][200];

extern int acdialog_font;

MyLabel::MyLabel(int xx, int yy, int wii, const char *tee)
{
    strncpy(text, tee, 150);
    text[149] = 0;
    x = xx;
    y = yy;
    wid = wii;
    hit = TEXT_HT;
}

void MyLabel::draw(Bitmap *ds)
{
    int cyp = y;
    char *teptr = &text[0];
    color_t text_color = ds->GetCompatibleColor(0);

    break_up_text_into_lines(wid, acdialog_font, teptr);
    for (int ee = 0; ee < numlines; ee++) {
        wouttext_outline(ds, x, cyp, acdialog_font, text_color, lines[ee]);
        cyp += TEXT_HT;
    }
}

int MyLabel::pressedon()
{
    return 0;
}

int MyLabel::processmessage(int mcode, int wParam, long lParam)
{
    return -1;                  // doesn't support messages
}
