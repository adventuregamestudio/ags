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
#include "ac/speech.h"
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptoverlay.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "main/engine.h"
#include "util/path.h"

using namespace AGS::Common;

// identifier (username) of the voice pak
static String VoicePakName;
// parent part to use when making voice asset names
static String VoiceAssetPath;


int user_to_internal_skip_speech(SkipSpeechStyle userval)
{
    switch (userval)
    {
    case kSkipSpeechNone:
        return SKIP_NONE;
    case kSkipSpeechKeyMouseTime:
        return SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_MOUSECLICK;
    case kSkipSpeechKeyTime:
        return SKIP_AUTOTIMER | SKIP_KEYPRESS;
    case kSkipSpeechTime:
        return SKIP_AUTOTIMER;
    case kSkipSpeechKeyMouse:
        return SKIP_KEYPRESS | SKIP_MOUSECLICK;
    case kSkipSpeechMouseTime:
        return SKIP_AUTOTIMER | SKIP_MOUSECLICK;
    case kSkipSpeechKey:
        return SKIP_KEYPRESS;
    case kSkipSpeechMouse:
        return SKIP_MOUSECLICK;
    default:
        quit("user_to_internal_skip_speech: unknown userval");
        return SKIP_NONE;
    }
}

SkipSpeechStyle internal_skip_speech_to_user(int internal_val)
{
    if (internal_val & SKIP_AUTOTIMER)
    {
        internal_val &= ~SKIP_AUTOTIMER;
        if (internal_val == (SKIP_KEYPRESS | SKIP_MOUSECLICK))
        {
            return kSkipSpeechKeyMouseTime;
        }
        else if (internal_val == SKIP_KEYPRESS)
        {
            return kSkipSpeechKeyTime;
        }
        else if (internal_val == SKIP_MOUSECLICK)
        {
            return kSkipSpeechMouseTime;
        }
        return kSkipSpeechTime;
    }
    else
    {
        if (internal_val == (SKIP_KEYPRESS | SKIP_MOUSECLICK))
        {
            return kSkipSpeechKeyMouse;
        }
        else if (internal_val == SKIP_KEYPRESS)
        {
            return kSkipSpeechKey;
        }
        else if (internal_val == SKIP_MOUSECLICK)
        {
            return kSkipSpeechMouse;
        }
    }
    return kSkipSpeechNone;
}

bool init_voicepak(const String &name)
{
    if (usetup.no_speech_pack) return false; // voice-over disabled

    String speech_file = name.IsEmpty() ? "speech.vox" : String::FromFormat("sp_%s.vox", name.GetCStr());
    if (ResPaths.SpeechPak.Name.CompareNoCase(speech_file) == 0)
        return true; // same pak already assigned

    // First remove existing voice packs
    play.voice_avail = false;
    AssetMgr->RemoveLibrary(ResPaths.SpeechPak.Path);
    AssetMgr->RemoveLibrary(ResPaths.VoiceDirSub);

    // Now check for the new packs and add if they exist
    String speech_filepath = find_assetlib(speech_file);
    if (!speech_filepath.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Info, "Voice pack found: %s", speech_file.GetCStr());
        play.voice_avail = true;
    }
    else
    {
        Debug::Printf(kDbgMsg_Error, "Unable to init voice pack '%s', file not found or of unknown format.",
            speech_file.GetCStr());
    }

    String speech_subdir = "";
    if (!ResPaths.VoiceDir2.IsEmpty() && Path::ComparePaths(ResPaths.DataDir, ResPaths.VoiceDir2) != 0)
    {
        // If we have custom voice directory set, we will enable voice-over even if speech.vox does not exist
        speech_subdir = name.IsEmpty() ? ResPaths.VoiceDir2 : Path::ConcatPaths(ResPaths.VoiceDir2, name);
        if (File::IsDirectory(speech_subdir))
        {
            Debug::Printf(kDbgMsg_Info, "Optional voice directory is defined: %s", speech_subdir.GetCStr());
            play.voice_avail = true;
        }
    }

    // Save new resource locations and register asset libraries
    VoicePakName = name;
    VoiceAssetPath = name.IsEmpty() ? "" : String::FromFormat("%s/", name.GetCStr());
    ResPaths.SpeechPak.Name = speech_file;
    ResPaths.SpeechPak.Path = speech_filepath;
    ResPaths.VoiceDirSub = speech_subdir;
    AssetMgr->AddLibrary(ResPaths.VoiceDirSub, "voice");
    AssetMgr->AddLibrary(ResPaths.SpeechPak.Path, "voice");
    return play.voice_avail;
}

String get_voicepak_name()
{
    return VoicePakName;
}

String get_voice_assetpath()
{
    return VoiceAssetPath;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "ac/global_display.h"
#include "ac/dynobj/cc_dynamicobject.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

extern GameSetupStruct game;
extern GameState play;

ScriptOverlay* Speech_GetTextOverlay()
{
    return (ScriptOverlay*)ccGetObjectAddressFromHandle(play.speech_text_schandle);
}

ScriptOverlay* Speech_GetPortraitOverlay()
{
    return (ScriptOverlay*)ccGetObjectAddressFromHandle(play.speech_face_schandle);
}

RuntimeScriptValue Sc_Speech_GetAnimationStopTimeMargin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.close_mouth_speech_time);
}

RuntimeScriptValue Sc_Speech_SetAnimationStopTimeMargin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.close_mouth_speech_time);
}

RuntimeScriptValue Sc_Speech_GetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.speech_portrait_placement);
}

RuntimeScriptValue Sc_Speech_SetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.speech_portrait_placement);
}

RuntimeScriptValue Sc_Speech_GetDisplayPostTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.speech_display_post_time_ms);
}

RuntimeScriptValue Sc_Speech_SetDisplayPostTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.speech_display_post_time_ms);
}

RuntimeScriptValue Sc_Speech_GetGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
	API_VARGET_INT(play.talkanim_speed);
}

RuntimeScriptValue Sc_Speech_SetGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    if (game.options[OPT_GLOBALTALKANIMSPD] == 0)
    {
        debug_script_warn("Speech.GlobalSpeechAnimationDelay cannot be set when global speech animation speed is not enabled; set Speech.UseGlobalSpeechAnimationDelay first!");
        return RuntimeScriptValue();
    }
	API_VARSET_PINT(play.talkanim_speed);
}

RuntimeScriptValue Sc_Speech_GetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.speech_portrait_x);
}

RuntimeScriptValue Sc_Speech_SetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.speech_portrait_x);
}

RuntimeScriptValue Sc_Speech_GetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.speech_portrait_y);
}

RuntimeScriptValue Sc_Speech_SetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.speech_portrait_y);
}

RuntimeScriptValue Sc_Speech_GetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(game.options[OPT_SPEECHTYPE]);
}

RuntimeScriptValue Sc_Speech_SetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechStyle);
}

RuntimeScriptValue Sc_Speech_GetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.skip_speech_specific_key);
}

RuntimeScriptValue Sc_Speech_SetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.skip_speech_specific_key);
}

RuntimeScriptValue Sc_Speech_GetSkipStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetSkipSpeech);
}

extern RuntimeScriptValue Sc_Speech_SetSkipStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(SetSkipSpeech, 1);
    SetSkipSpeech((SkipSpeechStyle)params[0].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Speech_GetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.speech_text_align);
}

RuntimeScriptValue Sc_Speech_SetTextAlignment_Old(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_VARIABLE_VALUE(play.speech_text_align);
    play.speech_text_align = ReadScriptAlignment(params[0].IValue);
    return RuntimeScriptValue();
}

RuntimeScriptValue Sc_Speech_SetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_VARIABLE_VALUE(play.speech_text_align);
    play.speech_text_align = (HorAlignment)params[0].IValue;
    return RuntimeScriptValue();
}

RuntimeScriptValue Sc_Speech_GetUseGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
	API_VARGET_INT(game.options[OPT_GLOBALTALKANIMSPD]);
}

RuntimeScriptValue Sc_Speech_SetUseGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
	API_VARSET_PINT(game.options[OPT_GLOBALTALKANIMSPD]);
}

RuntimeScriptValue Sc_Speech_GetVoiceMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetVoiceMode);
}

RuntimeScriptValue Sc_Speech_SetVoiceMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetVoiceMode);
}

RuntimeScriptValue Sc_Speech_GetTextOverlay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptOverlay, Speech_GetTextOverlay);
}

RuntimeScriptValue Sc_Speech_GetPortraitOverlay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptOverlay, Speech_GetPortraitOverlay);
}

void RegisterSpeechAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    ccAddExternalStaticFunction("Speech::get_AnimationStopTimeMargin", Sc_Speech_GetAnimationStopTimeMargin);
    ccAddExternalStaticFunction("Speech::set_AnimationStopTimeMargin", Sc_Speech_SetAnimationStopTimeMargin);
    ccAddExternalStaticFunction("Speech::get_CustomPortraitPlacement", Sc_Speech_GetCustomPortraitPlacement);
    ccAddExternalStaticFunction("Speech::set_CustomPortraitPlacement", Sc_Speech_SetCustomPortraitPlacement);
    ccAddExternalStaticFunction("Speech::get_DisplayPostTimeMs",      Sc_Speech_GetDisplayPostTimeMs);
    ccAddExternalStaticFunction("Speech::set_DisplayPostTimeMs",      Sc_Speech_SetDisplayPostTimeMs);
	ccAddExternalStaticFunction("Speech::get_GlobalSpeechAnimationDelay", Sc_Speech_GetGlobalSpeechAnimationDelay);
	ccAddExternalStaticFunction("Speech::set_GlobalSpeechAnimationDelay", Sc_Speech_SetGlobalSpeechAnimationDelay);
    ccAddExternalStaticFunction("Speech::get_PortraitOverlay",        Sc_Speech_GetPortraitOverlay);
    ccAddExternalStaticFunction("Speech::get_PortraitXOffset",        Sc_Speech_GetPortraitXOffset);
    ccAddExternalStaticFunction("Speech::set_PortraitXOffset",        Sc_Speech_SetPortraitXOffset);
    ccAddExternalStaticFunction("Speech::get_PortraitY",              Sc_Speech_GetPortraitY);
    ccAddExternalStaticFunction("Speech::set_PortraitY",              Sc_Speech_SetPortraitY);
    ccAddExternalStaticFunction("Speech::get_SkipKey",                Sc_Speech_GetSkipKey);
    ccAddExternalStaticFunction("Speech::set_SkipKey",                Sc_Speech_SetSkipKey);
    ccAddExternalStaticFunction("Speech::get_SkipStyle",              Sc_Speech_GetSkipStyle);
    ccAddExternalStaticFunction("Speech::set_SkipStyle",              Sc_Speech_SetSkipStyle);
    ccAddExternalStaticFunction("Speech::get_Style",                  Sc_Speech_GetStyle);
    ccAddExternalStaticFunction("Speech::set_Style",                  Sc_Speech_SetStyle);
    ccAddExternalStaticFunction("Speech::get_TextAlignment",          Sc_Speech_GetTextAlignment);
    if (base_api < kScriptAPI_v350)
        ccAddExternalStaticFunction("Speech::set_TextAlignment",      Sc_Speech_SetTextAlignment_Old);
    else
        ccAddExternalStaticFunction("Speech::set_TextAlignment",      Sc_Speech_SetTextAlignment);
    ccAddExternalStaticFunction("Speech::get_TextOverlay",            Sc_Speech_GetTextOverlay);
	ccAddExternalStaticFunction("Speech::get_UseGlobalSpeechAnimationDelay", Sc_Speech_GetUseGlobalSpeechAnimationDelay);
	ccAddExternalStaticFunction("Speech::set_UseGlobalSpeechAnimationDelay", Sc_Speech_SetUseGlobalSpeechAnimationDelay);
    ccAddExternalStaticFunction("Speech::get_VoiceMode",              Sc_Speech_GetVoiceMode);
    ccAddExternalStaticFunction("Speech::set_VoiceMode",              Sc_Speech_SetVoiceMode);

    /* -- Don't register more unsafe plugin symbols until new plugin interface is designed --*/
}
