//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
    return CreateNewScriptString(texbox->GetText());
}

void TextBox_GetText(GUITextBox *texbox, char *buffer) {
    snprintf(buffer, MAX_MAXSTRLEN, "%s", texbox->GetText().GetCStr());
}

void TextBox_SetText(GUITextBox *texbox, const char *newtex) {
    texbox->SetText(newtex);
}

int TextBox_GetTextAlignment(GUITextBox *guit)
{
    return guit->GetTextAlignment();
}

void TextBox_SetTextAlignment(GUITextBox *guit, int align)
{
    guit->SetTextAlignment(static_cast<FrameAlignment>(align));
}

int TextBox_GetTextColor(GUITextBox *guit) {
    return guit->GetTextColor();
}

void TextBox_SetTextColor(GUITextBox *guit, int color)
{
    guit->SetTextColor(color);
    // Prior to 3.6.3 text color was also used for border
    if (loaded_game_file_version < kGameVersion_363_04)
        guit->SetBorderColor(color);
}

int TextBox_GetTextOutlineColor(GUITextBox *guit)
{
    return guit->GetTextOutlineColor();
}

void TextBox_SetTextOutlineColor(GUITextBox *guit, int color)
{
    guit->SetTextOutlineColor(color);
}

int TextBox_GetFont(GUITextBox *guit) {
    return guit->GetFont();
}

void TextBox_SetFont(GUITextBox *guit, int fontnum) {
    fontnum = ValidateFontNumber("TextBox.Font", fontnum);
    guit->SetFont(fontnum);
}

bool TextBox_GetShowBorder(GUITextBox *guit) {
    return guit->IsShowBorder();
}

void TextBox_SetShowBorder(GUITextBox *guit, bool on)
{
    if (guit->IsShowBorder() != on)
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

RuntimeScriptValue Sc_TextBox_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUITextBox, TextBox_GetTextAlignment);
}

RuntimeScriptValue Sc_TextBox_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUITextBox, TextBox_SetTextAlignment);
}

RuntimeScriptValue Sc_TextBox_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUITextBox, TextBox_GetTextColor);
}

RuntimeScriptValue Sc_TextBox_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUITextBox, TextBox_SetTextColor);
}

RuntimeScriptValue Sc_TextBox_GetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUITextBox, TextBox_GetTextOutlineColor);
}

RuntimeScriptValue Sc_TextBox_SetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUITextBox, TextBox_SetTextOutlineColor);
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
        { "TextBox::get_TextAlignment", API_FN_PAIR(TextBox_GetTextAlignment) },
        { "TextBox::set_TextAlignment", API_FN_PAIR(TextBox_SetTextAlignment) },
        { "TextBox::get_TextColor",   API_FN_PAIR(TextBox_GetTextColor) },
        { "TextBox::set_TextColor",   API_FN_PAIR(TextBox_SetTextColor) },
        { "TextBox::get_TextOutlineColor", API_FN_PAIR(TextBox_GetTextOutlineColor) },
        { "TextBox::set_TextOutlineColor", API_FN_PAIR(TextBox_SetTextOutlineColor) },
    };

    ccAddExternalFunctions(textbox_api);
}
