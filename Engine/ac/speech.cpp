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

#include "ac/global_audio.h"
#include "ac/global_display.h"
#include "debug/out.h"
#include "game/game_objects.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_Speech_GetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.SpeechPortraitPlacement);
}

RuntimeScriptValue Sc_Speech_SetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.SpeechPortraitPlacement);
}

RuntimeScriptValue Sc_Speech_GetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.SpeechPortraitX);
}

RuntimeScriptValue Sc_Speech_SetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.SpeechPortraitX);
}

RuntimeScriptValue Sc_Speech_GetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.SpeechPortraitY);
}

RuntimeScriptValue Sc_Speech_SetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.SpeechPortraitY);
}

RuntimeScriptValue Sc_Speech_GetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(game.Options[OPT_SPEECHTYPE]);
}

extern RuntimeScriptValue Sc_SetSpeechStyle(const RuntimeScriptValue *params, int32_t param_count);

RuntimeScriptValue Sc_Speech_GetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.SpeechSkipKey);
}

RuntimeScriptValue Sc_Speech_SetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.SpeechSkipKey);
}

RuntimeScriptValue Sc_Speech_GetSkipType(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetSkipSpeech);
}

extern RuntimeScriptValue Sc_SetSkipSpeech(const RuntimeScriptValue *params, int32_t param_count);

RuntimeScriptValue Sc_Speech_GetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARGET_INT(play.SpeechTextAlignment);
}

RuntimeScriptValue Sc_Speech_SetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_VARSET_PINT(play.SpeechTextAlignment);
}

RuntimeScriptValue Sc_Speech_GetVoiceMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetVoiceMode);
}

extern RuntimeScriptValue Sc_SetVoiceMode(const RuntimeScriptValue *params, int32_t param_count);

void RegisterSpeechAPI()
{
    ccAddExternalStaticFunction("Speech::get_CustomPortraitPlacement", Sc_Speech_GetCustomPortraitPlacement);
    ccAddExternalStaticFunction("Speech::set_CustomPortraitPlacement", Sc_Speech_SetCustomPortraitPlacement);
    ccAddExternalStaticFunction("Speech::get_PortraitXOffset",        Sc_Speech_GetPortraitXOffset);
    ccAddExternalStaticFunction("Speech::set_PortraitXOffset",        Sc_Speech_SetPortraitXOffset);
    ccAddExternalStaticFunction("Speech::get_PortraitY",              Sc_Speech_GetPortraitY);
    ccAddExternalStaticFunction("Speech::set_PortraitY",              Sc_Speech_SetPortraitY);
    ccAddExternalStaticFunction("Speech::get_SkipKey",                Sc_Speech_GetSkipKey);
    ccAddExternalStaticFunction("Speech::set_SkipKey",                Sc_Speech_SetSkipKey);
    ccAddExternalStaticFunction("Speech::get_SkipType",               Sc_Speech_GetSkipType);
    ccAddExternalStaticFunction("Speech::set_SkipType",               Sc_SetSkipSpeech);
    ccAddExternalStaticFunction("Speech::get_Style",                  Sc_Speech_GetStyle);
    ccAddExternalStaticFunction("Speech::set_Style",                  Sc_SetSpeechStyle);
    ccAddExternalStaticFunction("Speech::get_TextAlignment",          Sc_Speech_GetTextAlignment);
    ccAddExternalStaticFunction("Speech::set_TextAlignment",          Sc_Speech_SetTextAlignment);
    ccAddExternalStaticFunction("Speech::get_VoiceMode",              Sc_Speech_GetVoiceMode);
    ccAddExternalStaticFunction("Speech::set_VoiceMode",              Sc_SetVoiceMode);

    /* -- Don't register more unsafe plugin symbols until new plugin interface is designed --*/
}
