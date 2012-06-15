
#include "acmain/ac_maindefines.h"


int ViewFrame_GetFlipped(ScriptViewFrame *svf) {
  if (views[svf->view].loops[svf->loop].frames[svf->frame].flags & VFLG_FLIPSPRITE)
    return 1;
  return 0;
}

int ViewFrame_GetGraphic(ScriptViewFrame *svf) {
  return views[svf->view].loops[svf->loop].frames[svf->frame].pic;
}

void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic) {
  views[svf->view].loops[svf->loop].frames[svf->frame].pic = newPic;
}

ScriptAudioClip* ViewFrame_GetLinkedAudio(ScriptViewFrame *svf) 
{
  int soundIndex = views[svf->view].loops[svf->loop].frames[svf->frame].sound;
  if (soundIndex < 0)
    return NULL;

  return &game.audioClips[soundIndex];
}

void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip* clip) 
{
  int newSoundIndex = -1;
  if (clip != NULL)
    newSoundIndex = clip->id;

  views[svf->view].loops[svf->loop].frames[svf->frame].sound = newSoundIndex;
}

int ViewFrame_GetSound(ScriptViewFrame *svf) {
  // convert audio clip to old-style sound number
  return get_old_style_number_for_sound(views[svf->view].loops[svf->loop].frames[svf->frame].sound);
}

void ViewFrame_SetSound(ScriptViewFrame *svf, int newSound) 
{
  if (newSound < 1)
  {
    views[svf->view].loops[svf->loop].frames[svf->frame].sound = -1;
  }
  else
  {
    // convert sound number to audio clip
    ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, newSound);
    if (clip == NULL)
      quitprintf("!SetFrameSound: audio clip aSound%d not found", newSound);

    views[svf->view].loops[svf->loop].frames[svf->frame].sound = clip->id + (psp_is_old_datafile ? 0x10000000 : 0);
  }
}

int ViewFrame_GetSpeed(ScriptViewFrame *svf) {
  return views[svf->view].loops[svf->loop].frames[svf->frame].speed;
}

int ViewFrame_GetView(ScriptViewFrame *svf) {
  return svf->view + 1;
}

int ViewFrame_GetLoop(ScriptViewFrame *svf) {
  return svf->loop;
}

int ViewFrame_GetFrame(ScriptViewFrame *svf) {
  return svf->frame;
}
