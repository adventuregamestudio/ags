
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__VIEWFRAME_H
#define __AGS_EE_AC__VIEWFRAME_H

#include "ac/view.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/dynobj/scriptviewframe.h"

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

void allocate_memory_for_views(int viewCount);
void precache_view(int view);
void CheckViewFrame (int view, int loop, int frame);
// draws a view frame, flipped if appropriate
void DrawViewFrame(Common::Bitmap *target, ViewFrame *vframe, int x, int y);

#endif // __AGS_EE_AC__VIEWFRAME_H
