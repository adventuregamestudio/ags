
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"
#include "ac/viewframe.h"
#include "debug/debug.h"
#include "media/audio/audio.h"
#include "ac/spritecache.h"

extern GameSetupStruct game;
extern ViewStruct*views;
extern int psp_is_old_datafile;
extern SpriteCache spriteset;


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

//=============================================================================

void allocate_memory_for_views(int viewCount)
{
    views = (ViewStruct*)calloc(sizeof(ViewStruct) * viewCount, 1);
    game.viewNames = (char**)malloc(sizeof(char*) * viewCount);
    game.viewNames[0] = (char*)malloc(MAXVIEWNAMELENGTH * viewCount);

    for (int i = 1; i < viewCount; i++)
    {
        game.viewNames[i] = game.viewNames[0] + (MAXVIEWNAMELENGTH * i);
    }
}

void precache_view(int view) 
{
    if (view < 0) 
        return;

    for (int i = 0; i < views[view].numLoops; i++) {
        for (int j = 0; j < views[view].loops[i].numFrames; j++)
            spriteset.precache (views[view].loops[i].frames[j].pic);
    }
}

// the specified frame has just appeared, see if we need
// to play a sound or whatever
void CheckViewFrame (int view, int loop, int frame) {
    if (psp_is_old_datafile)
    {
        if (views[view].loops[loop].frames[frame].sound > 0)
        {
            if (views[view].loops[loop].frames[frame].sound < 0x10000000)
            {
                ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, views[view].loops[loop].frames[frame].sound);
                if (clip)
                    views[view].loops[loop].frames[frame].sound = clip->id + 0x10000000;
                else
                {
                    views[view].loops[loop].frames[frame].sound = 0;
                    return;
                }
            }
            play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound - 0x10000000);
        }
    }
    else
    {
        if (views[view].loops[loop].frames[frame].sound >= 0) {
            // play this sound (eg. footstep)
            play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound);
        }
    }
}

// draws a view frame, flipped if appropriate
void DrawViewFrame(block target, ViewFrame *vframe, int x, int y) {
    if (vframe->flags & VFLG_FLIPSPRITE)
        draw_sprite_h_flip(target, spriteset[vframe->pic], x, y);
    else
        draw_sprite(target, spriteset[vframe->pic], x, y);
}
