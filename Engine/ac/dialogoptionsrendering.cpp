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

#include "util/wgt2allg.h"
#include "ac/dialogoptionsrendering.h"
#include "ac/dialogtopic.h"
#include "ac/gamestructdefines.h"
#include "debug/debug_log.h"
#include "script/runtimescriptvalue.h"
#include "ac/dynobj/cc_dialog.h"

extern ScriptDialog scrDialog[MAX_DIALOG];
extern DialogTopic *dialog;
extern CCDialog ccDynamicDialog;

// ** SCRIPT DIALOGOPTIONSRENDERING OBJECT

int DialogOptionsRendering_GetX(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->x;
}

void DialogOptionsRendering_SetX(ScriptDialogOptionsRendering *dlgOptRender, int newX)
{
    dlgOptRender->x = newX;
}

int DialogOptionsRendering_GetY(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->y;
}

void DialogOptionsRendering_SetY(ScriptDialogOptionsRendering *dlgOptRender, int newY)
{
    dlgOptRender->y = newY;
}

int DialogOptionsRendering_GetWidth(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->width;
}

void DialogOptionsRendering_SetWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
{
    dlgOptRender->width = newWidth;
}

int DialogOptionsRendering_GetHeight(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->height;
}

void DialogOptionsRendering_SetHeight(ScriptDialogOptionsRendering *dlgOptRender, int newHeight)
{
    dlgOptRender->height = newHeight;
}

int DialogOptionsRendering_GetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->parserTextboxX;
}

void DialogOptionsRendering_SetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender, int newX)
{
    dlgOptRender->parserTextboxX = newX;
}

int DialogOptionsRendering_GetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->parserTextboxY;
}

void DialogOptionsRendering_SetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender, int newY)
{
    dlgOptRender->parserTextboxY = newY;
}

int DialogOptionsRendering_GetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->parserTextboxWidth;
}

void DialogOptionsRendering_SetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
{
    dlgOptRender->parserTextboxWidth = newWidth;
}

ScriptDialog* DialogOptionsRendering_GetDialogToRender(ScriptDialogOptionsRendering *dlgOptRender)
{
    return &scrDialog[dlgOptRender->dialogID];
}

ScriptDrawingSurface* DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender)
{
    dlgOptRender->surfaceAccessed = true;
    return dlgOptRender->surfaceToRenderTo;
}

int DialogOptionsRendering_GetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->activeOptionID + 1;
}

void DialogOptionsRendering_SetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID)
{
    int optionCount = dialog[scrDialog[dlgOptRender->dialogID].id].numoptions;
    if ((activeOptionID < 0) || (activeOptionID > optionCount))
        quitprintf("DialogOptionsRenderingInfo.ActiveOptionID: invalid ID specified for this dialog (specified %d, valid range: 1..%d)", activeOptionID, optionCount);

    dlgOptRender->activeOptionID = activeOptionID - 1;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetActiveOptionID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetActiveOptionID);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID)
RuntimeScriptValue Sc_DialogOptionsRendering_SetActiveOptionID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetActiveOptionID);
}

// ScriptDialog* (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetDialogToRender(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptDialogOptionsRendering, ScriptDialog, ccDynamicDialog, DialogOptionsRendering_GetDialogToRender);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetHeight);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newHeight)
RuntimeScriptValue Sc_DialogOptionsRendering_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetHeight);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetParserTextboxX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetParserTextboxX);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newX)
RuntimeScriptValue Sc_DialogOptionsRendering_SetParserTextboxX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetParserTextboxX);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetParserTextboxY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetParserTextboxY);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newY)
RuntimeScriptValue Sc_DialogOptionsRendering_SetParserTextboxY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetParserTextboxY);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetParserTextboxWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetParserTextboxWidth);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
RuntimeScriptValue Sc_DialogOptionsRendering_SetParserTextboxWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetParserTextboxWidth);
}

// ScriptDrawingSurface* (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDialogOptionsRendering, ScriptDrawingSurface, DialogOptionsRendering_GetSurface);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetWidth);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
RuntimeScriptValue Sc_DialogOptionsRendering_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetWidth);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetX);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newX)
RuntimeScriptValue Sc_DialogOptionsRendering_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetX);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetY);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newY)
RuntimeScriptValue Sc_DialogOptionsRendering_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetY);
}

void RegisterDialogOptionsRenderingAPI()
{
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ActiveOptionID",   Sc_DialogOptionsRendering_GetActiveOptionID);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ActiveOptionID",   Sc_DialogOptionsRendering_SetActiveOptionID);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_DialogToRender",   Sc_DialogOptionsRendering_GetDialogToRender);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Height",           Sc_DialogOptionsRendering_GetHeight);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_Height",           Sc_DialogOptionsRendering_SetHeight);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ParserTextBoxX",   Sc_DialogOptionsRendering_GetParserTextboxX);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ParserTextBoxX",   Sc_DialogOptionsRendering_SetParserTextboxX);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ParserTextBoxY",   Sc_DialogOptionsRendering_GetParserTextboxY);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ParserTextBoxY",   Sc_DialogOptionsRendering_SetParserTextboxY);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_ParserTextBoxWidth", Sc_DialogOptionsRendering_GetParserTextboxWidth);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_ParserTextBoxWidth", Sc_DialogOptionsRendering_SetParserTextboxWidth);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Surface",          Sc_DialogOptionsRendering_GetSurface);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Width",            Sc_DialogOptionsRendering_GetWidth);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_Width",            Sc_DialogOptionsRendering_SetWidth);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_X",                Sc_DialogOptionsRendering_GetX);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_X",                Sc_DialogOptionsRendering_SetX);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::get_Y",                Sc_DialogOptionsRendering_GetY);
    ccAddExternalObjectFunction("DialogOptionsRenderingInfo::set_Y",                Sc_DialogOptionsRendering_SetY);
}
