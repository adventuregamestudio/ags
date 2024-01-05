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
#include <stdio.h>
#include "ac/common.h" // quit
#include "ac/gamesetupstruct.h"
#include "ac/global_translation.h"
#include "ac/label.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"

using namespace AGS::Common;

extern GameSetupStruct game;

// ** LABEL FUNCTIONS

const char* Label_GetText_New(GUILabel *labl) {
    return CreateNewScriptString(labl->GetText());
}

void Label_GetText(GUILabel *labl, char *buffer) {
    snprintf(buffer, MAX_MAXSTRLEN, "%s", labl->GetText().GetCStr());
}

void Label_SetText(GUILabel *labl, const char *newtx) {
    newtx = get_translation(newtx);

    if (labl->GetText() != newtx) {
        labl->SetText(newtx);
    }
}

int Label_GetTextAlignment(GUILabel *labl)
{
    return (loaded_game_file_version >= kGameVersion_350) ?
        labl->TextAlignment :
        GetLegacyGUIAlignment(labl->TextAlignment);
}

void Label_SetTextAlignment(GUILabel *labl, int align)
{
    // NOTE: some custom engines supported Label.TextAlignment
    // before 3.5.0 got this added officially
    HorAlignment use_align =
        (loaded_game_file_version >= kGameVersion_350) ?
        (HorAlignment)align :
        ConvertLegacyGUIAlignment((LegacyGUIAlignment)align);
    if (labl->TextAlignment != use_align)
    {
        labl->TextAlignment = use_align;
        labl->MarkChanged();
    }
}

int Label_GetColor(GUILabel *labl) {
    return labl->TextColor;
}

void Label_SetColor(GUILabel *labl, int colr) {
    if (labl->TextColor != colr) {
        labl->TextColor = colr;
        labl->MarkChanged();
    }
}

int Label_GetFont(GUILabel *labl) {
    return labl->Font;
}

void Label_SetFont(GUILabel *guil, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetLabelFont: invalid font number.");

    if (fontnum != guil->Font) {
        guil->Font = fontnum;
        guil->MarkChanged();
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
#include "ac/dynobj/scriptstring.h"

// void (GUILabel *labl, char *buffer)
RuntimeScriptValue Sc_Label_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUILabel, Label_GetText, char);
}

// void (GUILabel *labl, const char *newtx)
RuntimeScriptValue Sc_Label_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUILabel, Label_SetText, const char);
}

RuntimeScriptValue Sc_Label_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetTextAlignment);
}

RuntimeScriptValue Sc_Label_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetTextAlignment);
}


// int (GUILabel *labl)
RuntimeScriptValue Sc_Label_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetFont);
}

// void (GUILabel *guil, int fontnum)
RuntimeScriptValue Sc_Label_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetFont);
}

// const char* (GUILabel *labl)
RuntimeScriptValue Sc_Label_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUILabel, const char, myScriptStringImpl, Label_GetText_New);
}

// int (GUILabel *labl)
RuntimeScriptValue Sc_Label_GetColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetColor);
}

// void (GUILabel *labl, int colr)
RuntimeScriptValue Sc_Label_SetColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetColor);
}



void RegisterLabelAPI()
{
    ScFnRegister label_api[] = {
        { "Label::GetText^1",     API_FN_PAIR(Label_GetText) },
        { "Label::SetText^1",     API_FN_PAIR(Label_SetText) },
        { "Label::get_TextAlignment", API_FN_PAIR(Label_GetTextAlignment) },
        { "Label::set_TextAlignment", API_FN_PAIR(Label_SetTextAlignment) },
        { "Label::get_Font",      API_FN_PAIR(Label_GetFont) },
        { "Label::set_Font",      API_FN_PAIR(Label_SetFont) },
        { "Label::get_Text",      API_FN_PAIR(Label_GetText_New) },
        { "Label::set_Text",      API_FN_PAIR(Label_SetText) },
        { "Label::get_TextColor", API_FN_PAIR(Label_GetColor) },
        { "Label::set_TextColor", API_FN_PAIR(Label_SetColor) },
    };

    ccAddExternalFunctions(label_api);
}
