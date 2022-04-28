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
#ifndef __AGS_EE_AC__OVERLAY_H
#define __AGS_EE_AC__OVERLAY_H
#include <vector>
#include "ac/screenoverlay.h"
#include "ac/dynobj/scriptoverlay.h"
#include "util/geometry.h"

namespace AGS { namespace Common { class Bitmap; } }
using namespace AGS; // FIXME later

void Overlay_Remove(ScriptOverlay *sco);
void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, const char*text);
int  Overlay_GetX(ScriptOverlay *scover);
void Overlay_SetX(ScriptOverlay *scover, int newx);
int  Overlay_GetY(ScriptOverlay *scover);
void Overlay_SetY(ScriptOverlay *scover, int newy);
int  Overlay_GetValid(ScriptOverlay *scover);
ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent);
ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text);

int  find_overlay_of_type(int type);
void remove_screen_overlay(int type);
// Calculates overlay position in screen coordinates
Point get_overlay_position(const ScreenOverlay &over);
size_t add_screen_overlay(int x,int y,int type,Common::Bitmap *piccy, bool alphaChannel = false);
size_t add_screen_overlay(int x, int y, int type, Common::Bitmap *piccy, int pic_offx, int pic_offy, bool alphaChannel = false);
void remove_screen_overlay_index(size_t over_idx);
// Creates and registers a managed script object for existing overlay object
ScriptOverlay* create_scriptobj_for_overlay(ScreenOverlay &over);
void recreate_overlay_ddbs();


extern std::vector<ScreenOverlay> screenover;
#endif // __AGS_EE_AC__OVERLAY_H
