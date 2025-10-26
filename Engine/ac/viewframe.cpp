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
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/game_version.h"
#include "ac/viewframe.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/dynobj/cc_audioclip.h"
#include "ac/dynobj/scriptstring.h"
#include "debug/debug_log.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"
#include "media/audio/audio_system.h"
#include "util/math.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern std::vector<ViewStruct> views;
extern CCAudioClip ccDynamicAudioClip;


const char *ViewFrame_GetEventName(ScriptViewFrame *svf)
{
    return CreateNewScriptString(views[svf->view].loops[svf->loop].frames[svf->frame].event_name);
}

void ViewFrame_SetEventName(ScriptViewFrame *svf, const char *name)
{
    views[svf->view].loops[svf->loop].frames[svf->frame].event_name = name;
}

int ViewFrame_GetFlipped(ScriptViewFrame *svf)
{
    // We can return GraphicFlip here, because old boolean value matches horizontal flip
    return GfxDef::GetFlipFromFlags(views[svf->view].loops[svf->loop].frames[svf->frame].flags);
}

void ViewFrame_SetFlipped(ScriptViewFrame *svf, int flip)
{
    views[svf->view].loops[svf->loop].frames[svf->frame].flags = GfxDef::GetFlagsFromFlip((GraphicFlip)flip);
}

int ViewFrame_GetGraphic(ScriptViewFrame *svf)
{
    return views[svf->view].loops[svf->loop].frames[svf->frame].pic;
}

void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic)
{
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

int ViewFrame_GetSpeed(ScriptViewFrame *svf)
{
    return views[svf->view].loops[svf->loop].frames[svf->frame].speed;
}

void ViewFrame_SetSpeed(ScriptViewFrame *svf, int speed)
{
    views[svf->view].loops[svf->loop].frames[svf->frame].speed = speed;
}

int ViewFrame_GetXOffset(ScriptViewFrame *svf)
{
    return views[svf->view].loops[svf->loop].frames[svf->frame].xoffs;
}

void ViewFrame_SetXOffset(ScriptViewFrame *svf, int xoff)
{
    views[svf->view].loops[svf->loop].frames[svf->frame].xoffs = xoff;
}

int ViewFrame_GetYOffset(ScriptViewFrame *svf)
{
    return views[svf->view].loops[svf->loop].frames[svf->frame].xoffs;
}

void ViewFrame_SetYOffset(ScriptViewFrame *svf, int yoff)
{
    views[svf->view].loops[svf->loop].frames[svf->frame].yoffs = yoff;
}

int ViewFrame_GetView(ScriptViewFrame *svf)
{
    return svf->view + 1;
}

int ViewFrame_GetLoop(ScriptViewFrame *svf)
{
    return svf->loop;
}

int ViewFrame_GetFrame(ScriptViewFrame *svf)
{
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

bool PlayViewFrameSound(int view, int loop, int frame, int sound_volume)
{
    if (views[view].loops[loop].frames[frame].sound < 0)
        return false;

    // play this sound (eg. footstep)
    ScriptAudioChannel *channel = play_audio_clip_by_index(views[view].loops[loop].frames[frame].sound);
    if (channel)
    {
        sound_volume = Math::Clamp(sound_volume, 0, 100);
        auto *ch = AudioChans::GetChannel(channel->id);
        if (ch)
            ch->set_volume100(ch->get_volume100() * sound_volume / 100);
        return true; // return positive if sound is linked, regardless of the playback success
    }
    return false;
}

bool RunViewFrameEvent(int view, int loop, int frame,
    const ObjectEvent &obj_evt, Common::ScriptEventsBase *handlers, int evnt)
{
    if (!views[view].loops[loop].frames[frame].event_name.IsEmpty()
        && obj_evt && handlers->HasHandler(evnt))
    {
        ObjectEvent evt_ext = obj_evt;
        evt_ext.Params[1] = RuntimeScriptValue().SetInt32(view);
        evt_ext.Params[2] = RuntimeScriptValue().SetInt32(loop);
        evt_ext.Params[3] = RuntimeScriptValue().SetInt32(frame);
        auto dyn_str = ScriptString::Create(views[view].loops[loop].frames[frame].event_name.GetCStr());
        evt_ext.Params[4] = RuntimeScriptValue().SetScriptObject(dyn_str.Obj, dyn_str.Mgr);
        evt_ext.ParamCount = 5;
        // This event is run on either blocking or non-blocking thread
        run_event_script_always(evt_ext, handlers, evnt);
        return true;
    }

    return false;
}

void CheckViewFrame(int view, int loop, int frame, int sound_volume)
{
    CheckViewFrame(view, loop, frame, sound_volume, ObjectEvent(), nullptr, 0);
}

// Handle the new animation frame (play linked sounds, etc)
void CheckViewFrame(int view, int loop, int frame, int sound_volume,
                    const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int evnt)
{
    PlayViewFrameSound(view, loop, frame, sound_volume);
    RunViewFrameEvent(view, loop, frame, obj_evt, handlers, evnt);
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

RuntimeScriptValue Sc_ViewFrame_GetEventName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptViewFrame, const char, myScriptStringImpl, ViewFrame_GetEventName);
}

RuntimeScriptValue Sc_ViewFrame_SetEventName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewFrame, ViewFrame_SetEventName, const char);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetFlipped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetFlipped);
}

RuntimeScriptValue Sc_ViewFrame_SetFlipped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetFlipped);
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
RuntimeScriptValue Sc_ViewFrame_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetSpeed);
}

RuntimeScriptValue Sc_ViewFrame_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetSpeed);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetLoop);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetView);
}

RuntimeScriptValue Sc_ViewFrame_GetXOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetXOffset);
}

RuntimeScriptValue Sc_ViewFrame_SetXOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetXOffset);
}

RuntimeScriptValue Sc_ViewFrame_GetYOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetYOffset);
}

RuntimeScriptValue Sc_ViewFrame_SetYOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetYOffset);
}


void RegisterViewFrameAPI()
{
    ScFnRegister viewframe_api[] = {
        { "ViewFrame::get_EventName",     API_FN_PAIR(ViewFrame_GetEventName) },
        { "ViewFrame::set_EventName",     API_FN_PAIR(ViewFrame_SetEventName) },
        { "ViewFrame::get_Flipped",       API_FN_PAIR(ViewFrame_GetFlipped) },
        { "ViewFrame::set_Flipped",       API_FN_PAIR(ViewFrame_SetFlipped) },
        { "ViewFrame::get_Frame",         API_FN_PAIR(ViewFrame_GetFrame) },
        { "ViewFrame::get_Graphic",       API_FN_PAIR(ViewFrame_GetGraphic) },
        { "ViewFrame::set_Graphic",       API_FN_PAIR(ViewFrame_SetGraphic) },
        { "ViewFrame::get_LinkedAudio",   API_FN_PAIR(ViewFrame_GetLinkedAudio) },
        { "ViewFrame::set_LinkedAudio",   API_FN_PAIR(ViewFrame_SetLinkedAudio) },
        { "ViewFrame::get_Loop",          API_FN_PAIR(ViewFrame_GetLoop) },
        { "ViewFrame::get_Speed",         API_FN_PAIR(ViewFrame_GetSpeed) },
        { "ViewFrame::set_Speed",         API_FN_PAIR(ViewFrame_SetSpeed) },
        { "ViewFrame::get_View",          API_FN_PAIR(ViewFrame_GetView) },
        { "ViewFrame::get_XOffset",       API_FN_PAIR(ViewFrame_GetXOffset) },
        { "ViewFrame::set_XOffset",       API_FN_PAIR(ViewFrame_SetXOffset) },
        { "ViewFrame::get_YOffset",       API_FN_PAIR(ViewFrame_GetYOffset) },
        { "ViewFrame::set_YOffset",       API_FN_PAIR(ViewFrame_SetYOffset) },
    };

    ccAddExternalFunctions(viewframe_api);
}
