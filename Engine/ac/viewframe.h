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
#ifndef __AGS_EE_AC__VIEWFRAME_H
#define __AGS_EE_AC__VIEWFRAME_H

#include "ac/runtime_defines.h"
#include "ac/view.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptviewframe.h"
#include "gfx/bitmap.h"

namespace AGS { namespace Common { class Graphics; } }
using namespace AGS; // FIXME later

int  ViewFrame_GetFlipped(ScriptViewFrame *svf);
int  ViewFrame_GetGraphic(ScriptViewFrame *svf);
void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic);
ScriptAudioClip* ViewFrame_GetLinkedAudio(ScriptViewFrame *svf);
void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip* clip);
int  ViewFrame_GetSound(ScriptViewFrame *svf);
void ViewFrame_SetSound(ScriptViewFrame *svf, int newSound);
int  ViewFrame_GetSpeed(ScriptViewFrame *svf);
int  ViewFrame_GetView(ScriptViewFrame *svf);
int  ViewFrame_GetLoop(ScriptViewFrame *svf);
int  ViewFrame_GetFrame(ScriptViewFrame *svf);

// Calculate the frame sound volume from different factors;
// pass scale as 100 if volume scaling is disabled
// NOTE: historically scales only in 0-100 range :/
int CalcFrameSoundVolume(int obj_vol, int anim_vol, int scale = 100);
// Handle the new animation frame (play linked sounds, etc);
// sound_volume is an optional *relative* factor, 100 is default (unchanged)
void CheckViewFrame(int view, int loop, int frame, int sound_volume = 100);
// draws a view frame, flipped if appropriate
void DrawViewFrame(Common::Bitmap *ds, const ViewFrame *vframe, int x, int y, bool alpha_blend = false);

#endif // __AGS_EE_AC__VIEWFRAME_H
