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
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/game_version.h"
#include "ac/viewframe.h"
#include "ac/spritecache.h"
#include "ac/dynobj/cc_audioclip.h"
#include "debug/debug_log.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "media/audio/audio_system.h"
#include "util/math.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern std::vector<ViewStruct> views;
extern CCAudioClip ccDynamicAudioClip;


int ViewFrame_GetFlipped(ScriptViewFrame *svf) {
  // We can return GraphicFlip here, because old boolean value matches horizontal flip
  return GfxDef::GetFlipFromFlags(views[svf->view].loops[svf->loop].frames[svf->frame].flags);
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
    return nullptr;

  return &game.audioClips[soundIndex];
}

void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip* clip) 
{
  int newSoundIndex = -1;
  if (clip != nullptr)
    newSoundIndex = clip->id;

  views[svf->view].loops[svf->loop].frames[svf->frame].sound = newSoundIndex;
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

int CalcFrameSoundVolume(int obj_vol, int anim_vol, int scale)
{
    // We view the audio property relation as the relation of the entities:
    // system -> audio type -> audio emitter (object, character) -> animation's audio
    // therefore the sound volume is a multiplication of factors.
    int frame_vol = 100; // default to full volume
    // Object's animation volume property
    frame_vol = frame_vol * obj_vol / 100;
    // Active animation volume
    frame_vol = frame_vol * anim_vol / 100;
    // Zoom volume scaling (optional)
    // NOTE: historically scales only in 0-100 range :/
    scale = Math::Clamp(scale, 0, 100);
    frame_vol = frame_vol * scale / 100;
    return frame_vol;
}

// Handle the new animation frame (play linked sounds, etc)
void CheckViewFrame(int view, int loop, int frame, int sound_volume)
{
    ScriptAudioChannel *channel = nullptr;

    if (views[view].loops[loop].frames[frame].sound >= 0) {
        // play this sound (eg. footstep)
        channel = play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound);
    }

    if (channel)
    {
        sound_volume = Math::Clamp(sound_volume, 0, 100);
        auto* ch = AudioChans::GetChannel(channel->id);
        if (ch)
            ch->set_volume100(ch->get_volume100() * sound_volume / 100);
    }
    
}

// Note: the following function is only used for speech views in update_sierra_speech() and _displayspeech()
// draws a view frame, flipped if appropriate
void DrawViewFrame(Bitmap *ds, const ViewFrame *vframe, int x, int y)
{
    Bitmap *vf_bmp = spriteset[vframe->pic];
    if ((ds->GetColorDepth() == 32) && (vf_bmp->GetColorDepth() == 32))
    {
        Bitmap *src = vf_bmp;
        if (GfxDef::FlagsHaveFlip(vframe->flags))
        {
            src = new Bitmap(vf_bmp->GetWidth(), vf_bmp->GetHeight(), vf_bmp->GetColorDepth());
            src->FlipBlt(vf_bmp, 0, 0, GfxDef::GetFlipFromFlags(vframe->flags));
        }
        draw_sprite_support_alpha(ds, x, y, src);
        if (src != vf_bmp)
            delete src;
    }
    else
    {
        if (GfxDef::FlagsHaveFlip(vframe->flags))
            ds->FlipBlt(vf_bmp, x, y, GfxDef::GetFlipFromFlags(vframe->flags));
        else
            ds->Blit(vf_bmp, x, y, Common::kBitmap_Transparency);
    }
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetFlipped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetFlipped);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetFrame);
}
// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetGraphic);
}

// void (ScriptViewFrame *svf, int newPic)
RuntimeScriptValue Sc_ViewFrame_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetGraphic);
}

// ScriptAudioClip* (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetLinkedAudio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptViewFrame, ScriptAudioClip, ccDynamicAudioClip, ViewFrame_GetLinkedAudio);
}

// void (ScriptViewFrame *svf, ScriptAudioClip* clip)
RuntimeScriptValue Sc_ViewFrame_SetLinkedAudio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewFrame, ViewFrame_SetLinkedAudio, ScriptAudioClip);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetLoop);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetSpeed);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetView);
}


void RegisterViewFrameAPI()
{
    ScFnRegister viewframe_api[] = {
        { "ViewFrame::get_Flipped",       API_FN_PAIR(ViewFrame_GetFlipped) },
        { "ViewFrame::get_Frame",         API_FN_PAIR(ViewFrame_GetFrame) },
        { "ViewFrame::get_Graphic",       API_FN_PAIR(ViewFrame_GetGraphic) },
        { "ViewFrame::set_Graphic",       API_FN_PAIR(ViewFrame_SetGraphic) },
        { "ViewFrame::get_LinkedAudio",   API_FN_PAIR(ViewFrame_GetLinkedAudio) },
        { "ViewFrame::set_LinkedAudio",   API_FN_PAIR(ViewFrame_SetLinkedAudio) },
        { "ViewFrame::get_Loop",          API_FN_PAIR(ViewFrame_GetLoop) },
        { "ViewFrame::get_Speed",         API_FN_PAIR(ViewFrame_GetSpeed) },
        { "ViewFrame::get_View",          API_FN_PAIR(ViewFrame_GetView) },
    };

    ccAddExternalFunctions(viewframe_api);
}
