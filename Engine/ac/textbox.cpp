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
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "ac/textbox.h"

extern GameSetupStruct game;


// ** TEXT BOX FUNCTIONS

const char* TextBox_GetText_New(GUITextBox *texbox) {
    return CreateNewScriptString(texbox->Text);
}

void TextBox_GetText(GUITextBox *texbox, char *buffer) {
    snprintf(buffer, MAX_MAXSTRLEN, "%s", texbox->Text.GetCStr());
}

void TextBox_SetText(GUITextBox *texbox, const char *newtex) {
    if (texbox->Text != newtex) {
        texbox->Text = newtex;
        texbox->MarkChanged();
    }
}

int TextBox_GetTextColor(GUITextBox *guit) {
    return guit->TextColor;
}

void TextBox_SetTextColor(GUITextBox *guit, int colr)
{
    if (guit->TextColor != colr) 
    {
        guit->TextColor = colr;
        guit->MarkChanged();
    }
}

int TextBox_GetFont(GUITextBox *guit) {
    return guit->Font;
}

void TextBox_SetFont(GUITextBox *guit, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetTextBoxFont: invalid font number.");

    if (guit->Font != fontnum) {
        guit->Font = fontnum;
        guit->MarkChanged();
    }
}

bool TextBox_GetShowBorder(GUITextBox *guit) {
    return guit->IsBorderShown();
}

void TextBox_SetShowBorder(GUITextBox *guit, bool on)
{
    if (guit->IsBorderShown() != on)
    {
        guit->SetShowBorder(on);
        guit->MarkChanged();
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

// void (GUITextBox *texbox, char *buffer)
RuntimeScriptValue Sc_TextBox_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUITextBox, TextBox_GetText, char);
}

// void (GUITextBox *texbox, const char *newtex)
RuntimeScriptValue Sc_TextBox_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUITextBox, TextBox_SetText, const char);
}

// int (GUITextBox *guit)
RuntimeScriptValue Sc_TextBox_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUITextBox, TextBox_GetFont);
}

// void (GUITextBox *guit, int fontnum)
RuntimeScriptValue Sc_TextBox_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUITextBox, TextBox_SetFont);
}

RuntimeScriptValue Sc_TextBox_GetShowBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUITextBox, TextBox_GetShowBorder);
}

// void (GUITextBox *guit, int fontnum)
RuntimeScriptValue Sc_TextBox_SetShowBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUITextBox, TextBox_SetShowBorder);
}

// const char* (GUITextBox *texbox)
RuntimeScriptValue Sc_TextBox_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUITextBox, const char *, myScriptStringImpl, TextBox_GetText_New);
}

// int (GUITextBox *guit)
RuntimeScriptValue Sc_TextBox_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUITextBox, TextBox_GetTextColor);
}

// void (GUITextBox *guit, int colr)
RuntimeScriptValue Sc_TextBox_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUITextBox, TextBox_SetTextColor);
}


void RegisterTextBoxAPI()
{
    ScFnRegister textbox_api[] = {
        { "TextBox::GetText^1",       API_FN_PAIR(TextBox_GetText) },
        { "TextBox::SetText^1",       API_FN_PAIR(TextBox_SetText) },
        { "TextBox::get_Font",        API_FN_PAIR(TextBox_GetFont) },
        { "TextBox::set_Font",        API_FN_PAIR(TextBox_SetFont) },
        { "TextBox::get_ShowBorder",  API_FN_PAIR(TextBox_GetShowBorder) },
        { "TextBox::set_ShowBorder",  API_FN_PAIR(TextBox_SetShowBorder) },
        { "TextBox::get_Text",        API_FN_PAIR(TextBox_GetText_New) },
        { "TextBox::set_Text",        API_FN_PAIR(TextBox_SetText) },
        { "TextBox::get_TextColor",   API_FN_PAIR(TextBox_GetTextColor) },
        { "TextBox::set_TextColor",   API_FN_PAIR(TextBox_SetTextColor) },
    };

    ccAddExternalFunctions(textbox_api);
}
