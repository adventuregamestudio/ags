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

#include <string.h>
#include "ac/label.h"
#include "ac/common.h"
#include "ac/global_translation.h"
#include "ac/string.h"
#include "game/game_objects.h"

extern int guis_need_update;

// ** LABEL FUNCTIONS

const char* Label_GetText_New(GuiLabel *labl) {
    return CreateNewScriptString(labl->GetText());
}

void Label_GetText(GuiLabel *labl, char *buffer) {
    strcpy(buffer, labl->GetText());
}

void Label_SetText(GuiLabel *labl, const char *newtx) {
    newtx = get_translation(newtx);

    if (strcmp(labl->GetText(), newtx)) {
        guis_need_update = 1;
        labl->SetText(newtx);
    }
}

int Label_GetColor(GuiLabel *labl) {
    return labl->TextColor;
}

void Label_SetColor(GuiLabel *labl, int colr) {
    if (labl->TextColor != colr) {
        labl->TextColor = colr;
        guis_need_update = 1;
    }
}

int Label_GetFont(GuiLabel *labl) {
    return labl->TextFont;
}

void Label_SetFont(GuiLabel *guil, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.FontCount))
        quit("!SetLabelFont: invalid TextFont number.");

    if (fontnum != guil->TextFont) {
        guil->TextFont = fontnum;
        guis_need_update = 1;
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

extern ScriptString myScriptStringImpl;

// void (GuiLabel *labl, char *buffer)
RuntimeScriptValue Sc_Label_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GuiLabel, Label_GetText, char);
}

// void (GuiLabel *labl, const char *newtx)
RuntimeScriptValue Sc_Label_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GuiLabel, Label_SetText, const char);
}

// int (GuiLabel *labl)
RuntimeScriptValue Sc_Label_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiLabel, Label_GetFont);
}

// void (GuiLabel *guil, int fontnum)
RuntimeScriptValue Sc_Label_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiLabel, Label_SetFont);
}

// const char* (GuiLabel *labl)
RuntimeScriptValue Sc_Label_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiLabel, const char, myScriptStringImpl, Label_GetText_New);
}

// int (GuiLabel *labl)
RuntimeScriptValue Sc_Label_GetColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiLabel, Label_GetColor);
}

// void (GuiLabel *labl, int colr)
RuntimeScriptValue Sc_Label_SetColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiLabel, Label_SetColor);
}



void RegisterLabelBoxAPI()
{
    ccAddExternalObjectFunction("Label::GetText^1",     Sc_Label_GetText);
    ccAddExternalObjectFunction("Label::SetText^1",     Sc_Label_SetText);
    ccAddExternalObjectFunction("Label::get_Font",      Sc_Label_GetFont);
    ccAddExternalObjectFunction("Label::set_Font",      Sc_Label_SetFont);
    ccAddExternalObjectFunction("Label::get_Text",      Sc_Label_GetText_New);
    ccAddExternalObjectFunction("Label::set_Text",      Sc_Label_SetText);
    ccAddExternalObjectFunction("Label::get_TextColor", Sc_Label_GetColor);
    ccAddExternalObjectFunction("Label::set_TextColor", Sc_Label_SetColor);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Label::GetText^1",     (void*)Label_GetText);
    ccAddExternalFunctionForPlugin("Label::SetText^1",     (void*)Label_SetText);
    ccAddExternalFunctionForPlugin("Label::get_Font",      (void*)Label_GetFont);
    ccAddExternalFunctionForPlugin("Label::set_Font",      (void*)Label_SetFont);
    ccAddExternalFunctionForPlugin("Label::get_Text",      (void*)Label_GetText_New);
    ccAddExternalFunctionForPlugin("Label::set_Text",      (void*)Label_SetText);
    ccAddExternalFunctionForPlugin("Label::get_TextColor", (void*)Label_GetColor);
    ccAddExternalFunctionForPlugin("Label::set_TextColor", (void*)Label_SetColor);
}
