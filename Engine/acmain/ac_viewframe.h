#ifndef __AC_VIEWFRAME_H
#define __AC_VIEWFRAME_H

#include "ac/dynobj/scriptviewframe.h"
#include "ac/ac_characterinfo.h"
#include "ac/ac_view.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/ac_audioclip.h"

void SetFrameSound (int vii, int loop, int frame, int sound);
void CheckViewFrame (int view, int loop, int frame);
void CheckViewFrameForCharacter(CharacterInfo *chi);
// draws a view frame, flipped if appropriate
void DrawViewFrame(block target, ViewFrame *vframe, int x, int y);

void allocate_memory_for_views(int viewCount);
int ViewFrame_GetFlipped(ScriptViewFrame *svf);
int ViewFrame_GetGraphic(ScriptViewFrame *svf);
void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic);
ScriptAudioClip* ViewFrame_GetLinkedAudio(ScriptViewFrame *svf);
void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip* clip);
int ViewFrame_GetSound(ScriptViewFrame *svf);
void ViewFrame_SetSound(ScriptViewFrame *svf, int newSound);
int ViewFrame_GetSpeed(ScriptViewFrame *svf);
int ViewFrame_GetView(ScriptViewFrame *svf);
int ViewFrame_GetLoop(ScriptViewFrame *svf);
int ViewFrame_GetFrame(ScriptViewFrame *svf);

#endif // __AC_VIEWFRAME_H
