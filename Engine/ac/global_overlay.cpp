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
#include "ac/global_overlay.h"
#include "ac/common.h" // quit
#include "ac/draw.h"
#include "ac/overlay.h"
#include "ac/runtime_defines.h"


void RemoveOverlay(int ovrid) {
    if (!get_overlay(ovrid))
        quit("!RemoveOverlay: invalid overlay id passed");
    remove_screen_overlay(ovrid);
}

int CreateGraphicOverlay(int x, int y, int slott, int trans) {
    auto *over = Overlay_CreateGraphicCore(false, x, y, slott, trans != 0, true); // always clone
    return over ? over->GetID() : 0;
}

int CreateTextOverlay(int xx, int yy, int wii, int fontid, int text_color, const char* text) {
    return CreateTextOverlay(xx, yy, wii, fontid, text_color, text, kDisplayTextStyle_TextWindow);
}

int CreateTextOverlay(int xx, int yy, int wii, int fontid, int text_color, const char* text, DisplayTextStyle style) {

    DisplayTextShrink allow_shrink = kDisplayTextShrink_None;
    if (xx != OVR_AUTOPLACE)
    {
        data_to_game_coords(&xx, &yy);
        // NOTE: this is ugly, but OVR_AUTOPLACE here suggests that width is already in game coords
        wii = data_to_game_coord(wii);
    }
    else
    {
        // allow DisplaySpeechBackground to be shrunk
        allow_shrink = kDisplayTextShrink_Left;
    }

    auto *over = Overlay_CreateTextCore(false, xx, yy, wii, fontid, text_color, text, OVER_CUSTOM, style, allow_shrink);
    return over ? over->GetID() : 0;
}

void SetTextOverlay(int ovrid, int xx, int yy, int wii, int fontid, int text_color, const char *text) {
    auto *over = get_overlay(ovrid);
    if (!over)
        quit("!SetTextOverlay: invalid overlay ID specified");

    Overlay_SetText(*over, xx, yy, wii, fontid, text_color, text);
}

void MoveOverlay(int ovrid, int newx,int newy) {
    data_to_game_coords(&newx, &newy);

    auto *over = get_overlay(ovrid);
    if (!over)
        quit("!MoveOverlay: invalid overlay ID specified");
    over->SetPosition(newx, newy);
}

int IsOverlayValid(int ovrid) {
    return (get_overlay(ovrid) != nullptr) ? 1 : 0;
}
