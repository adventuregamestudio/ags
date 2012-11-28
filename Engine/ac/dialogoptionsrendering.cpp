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
extern RuntimeScriptValue GlobalReturnValue;
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
    GlobalReturnValue.SetDynamicObject(&scrDialog[dlgOptRender->dialogID], &ccDynamicDialog);
    return &scrDialog[dlgOptRender->dialogID];
}

ScriptDrawingSurface* DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender)
{
    dlgOptRender->surfaceAccessed = true;
    GlobalReturnValue.SetDynamicObject(dlgOptRender->surfaceToRenderTo, dlgOptRender->surfaceToRenderTo);
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
