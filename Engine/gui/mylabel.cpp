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
#include "gui/mylabel.h"
#include <string.h>
#include "ac/display.h"
#include "ac/gamesetup.h"
#include "ac/string.h"
#include "font/fonts.h"
#include "gui/guidialogdefines.h"

using namespace Common;

extern int acdialog_font;

MyLabel::MyLabel(int xx, int yy, int wii, const char *tee)
{
    snprintf(text, sizeof(text), "%s", tee);
    x = xx;
    y = yy;
    wid = wii;
    hit = TEXT_HT;
}

void MyLabel::draw(Bitmap *ds)
{
    int cyp = y;
    color_t text_color = ds->GetCompatibleColor(0);

    const char *draw_text = skip_voiceover_token(text);
    if (break_up_text_into_lines(draw_text, Lines, wid, acdialog_font) == 0)
        return;
    for (size_t ee = 0; ee < Lines.Count(); ee++) {
        wouttext_outline(ds, x, cyp, acdialog_font, text_color, Lines[ee].GetCStr());
        cyp += TEXT_HT;
    }
}

int MyLabel::pressedon(int /*mx*/, int /*my*/)
{
    return 0;
}

int MyLabel::processmessage(int /*mcode*/, int /*wParam*/, intptr_t /*lParam*/)
{
    return -1;                  // doesn't support messages
}
