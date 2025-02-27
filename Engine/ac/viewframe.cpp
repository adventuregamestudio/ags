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
#include "ac/gamesetupstruct.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_audioclip.h"
#include "ac/draw.h"
#include "ac/game_version.h"
#include "media/audio/audio_system.h"
#include "util/math.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;
extern CCAudioClip ccDynamicAudioClip;


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

int ViewFrame_GetSound(ScriptViewFrame *svf) {
  // convert audio clip to old-style sound number
  return get_old_style_number_for_sound(views[svf->view].loops[svf->loop].frames[svf->frame].sound);
}

void ViewFrame_SetSound(ScriptViewFrame *svf, int newSound) 
{
  if (newSound < 1)
  {
    views[svf->view].loops[svf->loop].frames[svf->frame].sound = -1;
    views[svf->view].loops[svf->loop].frames[svf->frame].audioclip = -1;
  }
  else
  {
    // convert sound number to audio clip
    ScriptAudioClip* clip = GetAudioClipForOldStyleNumber(game, false, newSound);
    if (clip == nullptr)
      quitprintf("!SetFrameSound: audio clip aSound%d not found", newSound);

    views[svf->view].loops[svf->loop].frames[svf->frame].sound =
        game.IsLegacyAudioSystem() ? newSound : clip->id;
    views[svf->view].loops[svf->loop].frames[svf->frame].audioclip = clip->id;
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

void precache_view(int view) 
{
    if (view < 0) 
        return;

    for (int i = 0; i < views[view].numLoops; i++) {
        for (int j = 0; j < views[view].loops[i].numFrames; j++)
            spriteset.Precache(views[view].loops[i].frames[j].pic);
    }
}

// Handle the new animation frame (play linked sounds, etc)
void CheckViewFrame(int view, int loop, int frame, int sound_volume)
{
    ScriptAudioChannel *channel = nullptr;
    // Play a sound, if one is linked to this frame
    if (game.IsLegacyAudioSystem())
    {
        // sound field contains legacy sound num, so we also need an actual clip index
        const int sound = views[view].loops[loop].frames[frame].sound;
        if (sound > 0)
        {
            int &clip_id = views[view].loops[loop].frames[frame].audioclip;
            if (clip_id < 0)
            {
                ScriptAudioClip* clip = GetAudioClipForOldStyleNumber(game, false, sound);
                if (!clip)
                    return;
                clip_id = clip->id;
            }
            channel = play_audio_clip_by_index(clip_id);
        }
    }
    else
    {
        if (views[view].loops[loop].frames[frame].sound >= 0) {
            channel = play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound);
        }
    }
    if (channel && (sound_volume >= 0))
    {
        sound_volume = Math::Clamp(sound_volume, 0, 100);
        auto* ch = AudioChans::GetChannel(channel->id);
        if (ch)
            ch->set_volume100(ch->get_volume100() * sound_volume / 100);
    }
    
}

// draws a view frame, flipped if appropriate
void DrawViewFrame(Bitmap *ds, const ViewFrame *vframe, int x, int y, bool alpha_blend)
{
    // NOTE: DrawViewFrame supports alpha blending only since OPT_SPRITEALPHA;
    // this is why there's no sense in blending if it's not set (will do no good anyway).
    if (alpha_blend && game.options[OPT_SPRITEALPHA] == kSpriteAlphaRender_Proper)
    {
        Bitmap *vf_bmp = spriteset[vframe->pic];
        Bitmap *src = vf_bmp;
        if (vframe->flags & VFLG_FLIPSPRITE)
        {
            src = new Bitmap(vf_bmp->GetWidth(), vf_bmp->GetHeight(), vf_bmp->GetColorDepth());
            src->FlipBlt(vf_bmp, 0, 0, Common::kFlip_Horizontal);
        }
        draw_sprite_support_alpha(ds, true, x, y, src, (game.SpriteInfos[vframe->pic].Flags & SPF_ALPHACHANNEL) != 0);
        if (src != vf_bmp)
            delete src;
    }
    else
    {
        if (vframe->flags & VFLG_FLIPSPRITE)
            ds->FlipBlt(spriteset[vframe->pic], x, y, Common::kFlip_Horizontal);
        else
            ds->Blit(spriteset[vframe->pic], x, y, Common::kBitmap_Transparency);
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
RuntimeScriptValue Sc_ViewFrame_GetSound(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetSound);
}

// void (ScriptViewFrame *svf, int newSound)
RuntimeScriptValue Sc_ViewFrame_SetSound(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetSound);
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
    ccAddExternalObjectFunction("ViewFrame::get_Flipped",       Sc_ViewFrame_GetFlipped);
    ccAddExternalObjectFunction("ViewFrame::get_Frame",         Sc_ViewFrame_GetFrame);
    ccAddExternalObjectFunction("ViewFrame::get_Graphic",       Sc_ViewFrame_GetGraphic);
    ccAddExternalObjectFunction("ViewFrame::set_Graphic",       Sc_ViewFrame_SetGraphic);
    ccAddExternalObjectFunction("ViewFrame::get_LinkedAudio",   Sc_ViewFrame_GetLinkedAudio);
    ccAddExternalObjectFunction("ViewFrame::set_LinkedAudio",   Sc_ViewFrame_SetLinkedAudio);
    ccAddExternalObjectFunction("ViewFrame::get_Loop",          Sc_ViewFrame_GetLoop);
    ccAddExternalObjectFunction("ViewFrame::get_Sound",         Sc_ViewFrame_GetSound);
    ccAddExternalObjectFunction("ViewFrame::set_Sound",         Sc_ViewFrame_SetSound);
    ccAddExternalObjectFunction("ViewFrame::get_Speed",         Sc_ViewFrame_GetSpeed);
    ccAddExternalObjectFunction("ViewFrame::get_View",          Sc_ViewFrame_GetView);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("ViewFrame::get_Flipped",       (void*)ViewFrame_GetFlipped);
    ccAddExternalFunctionForPlugin("ViewFrame::get_Frame",         (void*)ViewFrame_GetFrame);
    ccAddExternalFunctionForPlugin("ViewFrame::get_Graphic",       (void*)ViewFrame_GetGraphic);
    ccAddExternalFunctionForPlugin("ViewFrame::set_Graphic",       (void*)ViewFrame_SetGraphic);
    ccAddExternalFunctionForPlugin("ViewFrame::get_LinkedAudio",   (void*)ViewFrame_GetLinkedAudio);
    ccAddExternalFunctionForPlugin("ViewFrame::set_LinkedAudio",   (void*)ViewFrame_SetLinkedAudio);
    ccAddExternalFunctionForPlugin("ViewFrame::get_Loop",          (void*)ViewFrame_GetLoop);
    ccAddExternalFunctionForPlugin("ViewFrame::get_Sound",         (void*)ViewFrame_GetSound);
    ccAddExternalFunctionForPlugin("ViewFrame::set_Sound",         (void*)ViewFrame_SetSound);
    ccAddExternalFunctionForPlugin("ViewFrame::get_Speed",         (void*)ViewFrame_GetSpeed);
    ccAddExternalFunctionForPlugin("ViewFrame::get_View",          (void*)ViewFrame_GetView);
}
