
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_viewframe.h"
#include "acmain/ac_commonheaders.h"
#include "acaudio/ac_audio.h"

// PSP specific variables:
extern int psp_is_old_datafile; // Set for 3.1.1 and 3.1.2 datafiles // in ac_game

void SetFrameSound (int vii, int loop, int frame, int sound) {
    if ((vii < 1) || (vii > game.numviews))
        quit("!SetFrameSound: invalid view number");
    vii--;

    if (loop >= views[vii].numLoops)
        quit("!SetFrameSound: invalid loop number");

    if (frame >= views[vii].loops[loop].numFrames)
        quit("!SetFrameSound: invalid frame number");

    if (sound < 1)
    {
        views[vii].loops[loop].frames[frame].sound = -1;
    }
    else
    {
        ScriptAudioClip* clip = get_audio_clip_for_old_style_number(false, sound);
        if (clip == NULL)
            quitprintf("!SetFrameSound: audio clip aSound%d not found", sound);

        views[vii].loops[loop].frames[frame].sound = clip->id + (psp_is_old_datafile ? 0x10000000 : 0);
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

void CheckViewFrameForCharacter(CharacterInfo *chi) {

    int soundVolumeWas = play.sound_volume;

    if (chi->flags & CHF_SCALEVOLUME) {
        // adjust the sound volume using the character's zoom level
        int zoom_level = charextra[chi->index_id].zoom;
        if (zoom_level == 0)
            zoom_level = 100;

        play.sound_volume = (play.sound_volume * zoom_level) / 100;

        if (play.sound_volume < 0)
            play.sound_volume = 0;
        if (play.sound_volume > 255)
            play.sound_volume = 255;
    }

    CheckViewFrame(chi->view, chi->loop, chi->frame);

    play.sound_volume = soundVolumeWas;
}



// draws a view frame, flipped if appropriate
void DrawViewFrame(block target, ViewFrame *vframe, int x, int y) {
    if (vframe->flags & VFLG_FLIPSPRITE)
        draw_sprite_h_flip(target, spriteset[vframe->pic], x, y);
    else
        draw_sprite(target, spriteset[vframe->pic], x, y);
}


// ** SCRIPT VIEW FRAME OBJECT

int ScriptViewFrame::Dispose(const char *address, bool force) {
    // always dispose a ViewFrame
    delete this;
    return 1;
}

const char *ScriptViewFrame::GetType() {
    return "ViewFrame";
}

int ScriptViewFrame::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(view);
    SerializeInt(loop);
    SerializeInt(frame);
    return EndSerialize();
}

void ScriptViewFrame::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    view = UnserializeInt();
    loop = UnserializeInt();
    frame = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptViewFrame::ScriptViewFrame(int p_view, int p_loop, int p_frame) {
    view = p_view;
    loop = p_loop;
    frame = p_frame;
}

ScriptViewFrame::ScriptViewFrame() {
    view = -1;
    loop = -1;
    frame = -1;
}


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
