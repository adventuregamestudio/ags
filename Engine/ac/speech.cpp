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
#include "ac/speech.h"
#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/runtime_defines.h"
#include "ac/dynobj/scriptoverlay.h"
#include "core/assetmanager.h"
#include "debug/debug_log.h"
#include "main/engine.h"
#include "util/directory.h"
#include "util/file.h"
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
        return SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_MOUSECLICK | SKIP_GAMEPAD; // FIXME: remake this as `kSkipAny`
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
    ResPaths.VoiceAvail = false;
    AssetMgr->RemoveLibrary(ResPaths.SpeechPak.Path);
    AssetMgr->RemoveLibrary(ResPaths.VoiceDirSub);

    // Now check for the new packs and add if they exist
    String speech_filepath = find_assetlib(speech_file);
    if (!speech_filepath.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Info, "Voice pack found: %s", speech_file.GetCStr());
        ResPaths.VoiceAvail = true;
    }
    else
    {
        Debug::Printf(kDbgMsg_Info, "Was not able to init voice pack '%s': file not found or of unknown format.",
            speech_file.GetCStr());
    }

    String speech_subdir = "";
    // Look up in the alternative locations that include "voice" filter
    for (const auto &opt_dir : ResPaths.OptDataDirs)
    {
        if (opt_dir.second.FindSection("voice", ',') != String::NoIndex)
        {
            // If we have custom voice directory set, we will enable voice-over even if speech.vox does not exist
            speech_subdir = Path::ConcatPaths(opt_dir.first, name);
            if (File::IsDirectory(speech_subdir) && Directory::HasAnyFiles(speech_subdir))
            {
                Debug::Printf(kDbgMsg_Info, "Optional voice directory is defined: %s", speech_subdir.GetCStr());
                ResPaths.VoiceAvail = true;
            }
        }
    }

    // Save new resource locations and register asset libraries
    VoicePakName = name;
    VoiceAssetPath = name;
    ResPaths.SpeechPak.Name = speech_file;
    ResPaths.SpeechPak.Path = speech_filepath;
    ResPaths.VoiceDirSub = speech_subdir;
    AssetMgr->AddLibrary(ResPaths.VoiceDirSub, "voice");
    AssetMgr->AddLibrary(ResPaths.SpeechPak.Path, "voice");
    return ResPaths.VoiceAvail;
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
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_scriptobject.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

extern GameSetupStruct game;
extern CCCharacter ccDynamicCharacter;
extern int char_speaking;

ScriptOverlay* Speech_GetTextOverlay()
{
    return (ScriptOverlay*)ccGetObjectAddressFromHandle(play.speech_text_schandle);
}

ScriptOverlay* Speech_GetPortraitOverlay()
{
    return (ScriptOverlay*)ccGetObjectAddressFromHandle(play.speech_face_schandle);
}

CharacterInfo* Speech_GetSpeakingCharacter()
{
    return char_speaking >= 0 ? &game.chars[char_speaking] : nullptr;
}

int Speech_GetAnimationStopTimeMargin()
{
    return play.close_mouth_speech_time;
}

void Speech_SetAnimationStopTimeMargin(int time)
{
    play.close_mouth_speech_time = time;
}

int Speech_GetCustomPortraitPlacement()
{
    return play.speech_portrait_placement;
}

void Speech_SetCustomPortraitPlacement(int placement)
{
    play.speech_portrait_placement = placement;
}

int Speech_GetDisplayPostTimeMs()
{
    return play.speech_display_post_time_ms;
}

void Speech_SetDisplayPostTimeMs(int time_ms)
{
    play.speech_display_post_time_ms = time_ms;
}

int Speech_GetGlobalSpeechAnimationDelay()
{
	return play.talkanim_speed;
}

void Speech_SetGlobalSpeechAnimationDelay(int delay)
{
    if (game.options[OPT_GLOBALTALKANIMSPD] == 0)
    {
        debug_script_warn("Speech.GlobalSpeechAnimationDelay cannot be set when global speech animation speed is not enabled; set Speech.UseGlobalSpeechAnimationDelay first!");
        return;
    }
    play.talkanim_speed = delay;
}

int Speech_GetPortraitXOffset()
{
    return play.speech_portrait_x;
}

void Speech_SetPortraitXOffset(int x)
{
    play.speech_portrait_x = x;
}

int Speech_GetPortraitY()
{
    return play.speech_portrait_y;
}

void Speech_SetPortraitY(int y)
{
    play.speech_portrait_y = y;
}

int Speech_GetStyle()
{
    return game.options[OPT_SPEECHTYPE];
}

void Speech_SetStyle (int newstyle)
{
    if ((newstyle < 0) || (newstyle > 3))
        quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
    game.options[OPT_SPEECHTYPE] = newstyle;
}

SkipSpeechStyle Speech_GetSkipStyle()
{
    return internal_skip_speech_to_user(play.speech_skip_style);
}

void Speech_SetSkipStyle(SkipSpeechStyle newval)
{
    if ((newval < kSkipSpeechFirst) || (newval > kSkipSpeechLast))
        quit("!SetSkipSpeech: invalid skip mode specified");

    if (usetup.access_speechskip == kSkipSpeechNone)
    {
        debug_script_log("SkipSpeech style set to %d", newval);
        play.speech_skip_style = user_to_internal_skip_speech((SkipSpeechStyle)newval);
    }
}

int Speech_GetSkipKey()
{
    return play.skip_speech_specific_key;
}

void Speech_SetSkipKey(int key)
{
    play.skip_speech_specific_key = key;
}

int Speech_GetTextAlignment()
{
    return play.speech_text_align;
}

void Speech_SetTextAlignment(int alignment)
{
    play.speech_text_align = (HorAlignment)alignment;
}

int Speech_GetUseGlobalSpeechAnimationDelay()
{
	return game.options[OPT_GLOBALTALKANIMSPD];
}

void Speech_SetUseGlobalSpeechAnimationDelay(int delay)
{
	game.options[OPT_GLOBALTALKANIMSPD] = delay;
}

//-----------------------------------------------------------------------------

RuntimeScriptValue Sc_Speech_GetAnimationStopTimeMargin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetAnimationStopTimeMargin);
}

RuntimeScriptValue Sc_Speech_SetAnimationStopTimeMargin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetAnimationStopTimeMargin);
}

RuntimeScriptValue Sc_Speech_GetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetCustomPortraitPlacement);
}

RuntimeScriptValue Sc_Speech_SetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetCustomPortraitPlacement);
}

RuntimeScriptValue Sc_Speech_GetDisplayPostTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetDisplayPostTimeMs);
}

RuntimeScriptValue Sc_Speech_SetDisplayPostTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetDisplayPostTimeMs);
}

RuntimeScriptValue Sc_Speech_GetGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_SetGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_GetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetPortraitXOffset);
}

RuntimeScriptValue Sc_Speech_SetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetPortraitXOffset);
}

RuntimeScriptValue Sc_Speech_GetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetPortraitY);
}

RuntimeScriptValue Sc_Speech_SetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetPortraitY);
}

RuntimeScriptValue Sc_Speech_GetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetStyle);
}

RuntimeScriptValue Sc_Speech_SetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetStyle);
}

RuntimeScriptValue Sc_Speech_GetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetSkipKey);
}

RuntimeScriptValue Sc_Speech_SetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetSkipKey);
}

RuntimeScriptValue Sc_Speech_GetSkipStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetSkipStyle);
}

extern RuntimeScriptValue Sc_Speech_SetSkipStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(SetSkipSpeech, 1);
    Speech_SetSkipStyle((SkipSpeechStyle)params[0].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Speech_GetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetTextAlignment);
}

RuntimeScriptValue Sc_Speech_SetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetTextAlignment);
}

RuntimeScriptValue Sc_Speech_GetUseGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetUseGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_SetUseGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetUseGlobalSpeechAnimationDelay);
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

RuntimeScriptValue Sc_Speech_GetSpeakingCharacter(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(CharacterInfo, ccDynamicCharacter, Speech_GetSpeakingCharacter);
}


void RegisterSpeechAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister speech_api[] = {
        { "Speech::get_AnimationStopTimeMargin", API_FN_PAIR(Speech_GetAnimationStopTimeMargin) },
        { "Speech::set_AnimationStopTimeMargin", API_FN_PAIR(Speech_SetAnimationStopTimeMargin) },
        { "Speech::get_CustomPortraitPlacement", API_FN_PAIR(Speech_GetCustomPortraitPlacement) },
        { "Speech::set_CustomPortraitPlacement", API_FN_PAIR(Speech_SetCustomPortraitPlacement) },
        { "Speech::get_DisplayPostTimeMs",      API_FN_PAIR(Speech_GetDisplayPostTimeMs) },
        { "Speech::set_DisplayPostTimeMs",      API_FN_PAIR(Speech_SetDisplayPostTimeMs) },
        { "Speech::get_GlobalSpeechAnimationDelay", API_FN_PAIR(Speech_GetGlobalSpeechAnimationDelay) },
        { "Speech::set_GlobalSpeechAnimationDelay", API_FN_PAIR(Speech_SetGlobalSpeechAnimationDelay) },
        { "Speech::get_PortraitOverlay",        API_FN_PAIR(Speech_GetPortraitOverlay) },
        { "Speech::get_PortraitXOffset",        API_FN_PAIR(Speech_GetPortraitXOffset) },
        { "Speech::set_PortraitXOffset",        API_FN_PAIR(Speech_SetPortraitXOffset) },
        { "Speech::get_PortraitY",              API_FN_PAIR(Speech_GetPortraitY) },
        { "Speech::set_PortraitY",              API_FN_PAIR(Speech_SetPortraitY) },
        { "Speech::get_SkipKey",                API_FN_PAIR(Speech_GetSkipKey) },
        { "Speech::set_SkipKey",                API_FN_PAIR(Speech_SetSkipKey) },
        { "Speech::get_SkipStyle",              API_FN_PAIR(Speech_GetSkipStyle) },
        { "Speech::set_SkipStyle",              API_FN_PAIR(Speech_SetSkipStyle) },
        { "Speech::get_SpeakingCharacter",      API_FN_PAIR(Speech_GetSpeakingCharacter) },
        { "Speech::get_Style",                  API_FN_PAIR(Speech_GetStyle) },
        { "Speech::set_Style",                  API_FN_PAIR(Speech_SetStyle) },
        { "Speech::get_TextAlignment",          API_FN_PAIR(Speech_GetTextAlignment) },
        { "Speech::set_TextAlignment",          API_FN_PAIR(Speech_SetTextAlignment) },
        { "Speech::get_TextOverlay",            API_FN_PAIR(Speech_GetTextOverlay) },
        { "Speech::get_UseGlobalSpeechAnimationDelay", API_FN_PAIR(Speech_GetUseGlobalSpeechAnimationDelay) },
        { "Speech::set_UseGlobalSpeechAnimationDelay", API_FN_PAIR(Speech_SetUseGlobalSpeechAnimationDelay) },
        { "Speech::get_VoiceMode",              Sc_Speech_GetVoiceMode, GetVoiceMode },
        { "Speech::set_VoiceMode",              Sc_Speech_SetVoiceMode, SetVoiceMode },
    };

    ccAddExternalFunctions(speech_api);
}
