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
#include "ac/textbox.h"
#include "ac/common.h"
#include "ac/string.h"
#include "game/game_objects.h"

extern int guis_need_update;


// ** TEXT BOX FUNCTIONS

const char* TextBox_GetText_New(GuiTextBox *texbox) {
    return CreateNewScriptString(texbox->Text);
}

void TextBox_GetText(GuiTextBox *texbox, char *buffer) {
    strcpy(buffer, texbox->Text);
}

void TextBox_SetText(GuiTextBox *texbox, const char *newtex) {
    if (strlen(newtex) > 190)
        quit("!SetTextBoxText: text too long");

    if (strcmp(texbox->Text, newtex)) {
        texbox->Text = newtex;
        guis_need_update = 1;
    }
}

int TextBox_GetTextColor(GuiTextBox *guit) {
    return guit->TextColor;
}

void TextBox_SetTextColor(GuiTextBox *guit, int colr)
{
    if (guit->TextColor != colr) 
    {
        guit->TextColor = colr;
        guis_need_update = 1;
    }
}

int TextBox_GetFont(GuiTextBox *guit) {
    return guit->TextFont;
}

void TextBox_SetFont(GuiTextBox *guit, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.FontCount))
        quit("!SetTextBoxFont: invalid font number.");

    if (guit->TextFont != fontnum) {
        guit->TextFont = fontnum;
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

// void (GuiTextBox *texbox, char *buffer)
RuntimeScriptValue Sc_TextBox_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GuiTextBox, TextBox_GetText, char);
}

// void (GuiTextBox *texbox, const char *newtex)
RuntimeScriptValue Sc_TextBox_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GuiTextBox, TextBox_SetText, const char);
}

// int (GuiTextBox *guit)
RuntimeScriptValue Sc_TextBox_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiTextBox, TextBox_GetFont);
}

// void (GuiTextBox *guit, int fontnum)
RuntimeScriptValue Sc_TextBox_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiTextBox, TextBox_SetFont);
}

// const char* (GuiTextBox *texbox)
RuntimeScriptValue Sc_TextBox_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GuiTextBox, const char *, myScriptStringImpl, TextBox_GetText_New);
}

// int (GuiTextBox *guit)
RuntimeScriptValue Sc_TextBox_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GuiTextBox, TextBox_GetTextColor);
}

// void (GuiTextBox *guit, int colr)
RuntimeScriptValue Sc_TextBox_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GuiTextBox, TextBox_SetTextColor);
}


void RegisterTextBoxAPI()
{
    ccAddExternalObjectFunction("TextBox::GetText^1",       Sc_TextBox_GetText);
    ccAddExternalObjectFunction("TextBox::SetText^1",       Sc_TextBox_SetText);
    ccAddExternalObjectFunction("TextBox::get_Font",        Sc_TextBox_GetFont);
    ccAddExternalObjectFunction("TextBox::set_Font",        Sc_TextBox_SetFont);
    ccAddExternalObjectFunction("TextBox::get_Text",        Sc_TextBox_GetText_New);
    ccAddExternalObjectFunction("TextBox::set_Text",        Sc_TextBox_SetText);
    ccAddExternalObjectFunction("TextBox::get_TextColor",   Sc_TextBox_GetTextColor);
    ccAddExternalObjectFunction("TextBox::set_TextColor",   Sc_TextBox_SetTextColor);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("TextBox::GetText^1",       (void*)TextBox_GetText);
    ccAddExternalFunctionForPlugin("TextBox::SetText^1",       (void*)TextBox_SetText);
    ccAddExternalFunctionForPlugin("TextBox::get_Font",        (void*)TextBox_GetFont);
    ccAddExternalFunctionForPlugin("TextBox::set_Font",        (void*)TextBox_SetFont);
    ccAddExternalFunctionForPlugin("TextBox::get_Text",        (void*)TextBox_GetText_New);
    ccAddExternalFunctionForPlugin("TextBox::set_Text",        (void*)TextBox_SetText);
    ccAddExternalFunctionForPlugin("TextBox::get_TextColor",   (void*)TextBox_GetTextColor);
    ccAddExternalFunctionForPlugin("TextBox::set_TextColor",   (void*)TextBox_SetTextColor);
}
