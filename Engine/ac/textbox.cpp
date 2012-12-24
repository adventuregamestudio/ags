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
#include "util/wgt2allg.h"
#include "ac/textbox.h"
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/string.h"

extern int guis_need_update;
extern GameSetupStruct game;


// ** TEXT BOX FUNCTIONS

const char* TextBox_GetText_New(GUITextBox *texbox) {
    return CreateNewScriptStringAsRetVal(texbox->text);
}

void TextBox_GetText(GUITextBox *texbox, char *buffer) {
    strcpy(buffer, texbox->text);
}

void TextBox_SetText(GUITextBox *texbox, const char *newtex) {
    if (strlen(newtex) > 190)
        quit("!SetTextBoxText: text too long");

    if (strcmp(texbox->text, newtex)) {
        strcpy(texbox->text, newtex);
        guis_need_update = 1;
    }
}

int TextBox_GetTextColor(GUITextBox *guit) {
    return guit->textcol;
}

void TextBox_SetTextColor(GUITextBox *guit, int colr)
{
    if (guit->textcol != colr) 
    {
        guit->textcol = colr;
        guis_need_update = 1;
    }
}

int TextBox_GetFont(GUITextBox *guit) {
    return guit->font;
}

void TextBox_SetFont(GUITextBox *guit, int fontnum) {
    if ((fontnum < 0) || (fontnum >= game.numfonts))
        quit("!SetTextBoxFont: invalid font number.");

    if (guit->font != fontnum) {
        guit->font = fontnum;
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
    ccAddExternalObjectFunction("TextBox::GetText^1",       Sc_TextBox_GetText);
    ccAddExternalObjectFunction("TextBox::SetText^1",       Sc_TextBox_SetText);
    ccAddExternalObjectFunction("TextBox::get_Font",        Sc_TextBox_GetFont);
    ccAddExternalObjectFunction("TextBox::set_Font",        Sc_TextBox_SetFont);
    ccAddExternalObjectFunction("TextBox::get_Text",        Sc_TextBox_GetText_New);
    ccAddExternalObjectFunction("TextBox::set_Text",        Sc_TextBox_SetText);
    ccAddExternalObjectFunction("TextBox::get_TextColor",   Sc_TextBox_GetTextColor);
    ccAddExternalObjectFunction("TextBox::set_TextColor",   Sc_TextBox_SetTextColor);
}
