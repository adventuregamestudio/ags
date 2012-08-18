
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DIALOGOPTIONSRENDERING_H
#define __AGS_EE_AC__DIALOGOPTIONSRENDERING_H

#include "ac/dynobj/scriptdialog.h"
#include "ac/dynobj/scriptdialogoptionsrendering.h"

int  DialogOptionsRendering_GetX(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetX(ScriptDialogOptionsRendering *dlgOptRender, int newX);
int  DialogOptionsRendering_GetY(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetY(ScriptDialogOptionsRendering *dlgOptRender, int newY);
int  DialogOptionsRendering_GetWidth(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth);
int  DialogOptionsRendering_GetHeight(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetHeight(ScriptDialogOptionsRendering *dlgOptRender, int newHeight);
int  DialogOptionsRendering_GetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender, int newX);
int  DialogOptionsRendering_GetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender, int newY);
int  DialogOptionsRendering_GetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth);
ScriptDialog* DialogOptionsRendering_GetDialogToRender(ScriptDialogOptionsRendering *dlgOptRender);
ScriptDrawingSurface* DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender);
int  DialogOptionsRendering_GetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender);
void DialogOptionsRendering_SetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID);

#endif // __AGS_EE_AC__DIALOGOPTIONSRENDERING_H
