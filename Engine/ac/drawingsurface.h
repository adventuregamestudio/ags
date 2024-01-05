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
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DRAWINGSURFACE_H
#define __AGS_EE_AC__DRAWINGSURFACE_H

#include "ac/dynobj/scriptdrawingsurface.h"

void	DrawingSurface_Release(ScriptDrawingSurface* sds);
// convert actual co-ordinate back to what the script is expecting
ScriptDrawingSurface* DrawingSurface_CreateCopy(ScriptDrawingSurface *sds);
void    DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int trans,
    int dst_x, int dst_y, int dst_width, int dst_height,
    int src_x, int src_y, int src_width, int src_height);
void	DrawingSurface_SetDrawingColor(ScriptDrawingSurface *sds, int newColour);
int		DrawingSurface_GetDrawingColor(ScriptDrawingSurface *sds);
void	DrawingSurface_SetUseHighResCoordinates(ScriptDrawingSurface *sds, int highRes);
int		DrawingSurface_GetUseHighResCoordinates(ScriptDrawingSurface *sds);
int		DrawingSurface_GetHeight(ScriptDrawingSurface *sds);
int		DrawingSurface_GetWidth(ScriptDrawingSurface *sds);
void	DrawingSurface_Clear(ScriptDrawingSurface *sds, int colour);
void	DrawingSurface_DrawCircle(ScriptDrawingSurface *sds, int x, int y, int radius);
void	DrawingSurface_DrawRectangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2);
void	DrawingSurface_DrawTriangle(ScriptDrawingSurface *sds, int x1, int y1, int x2, int y2, int x3, int y3);
void	DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* text);
void	DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg);
void	DrawingSurface_DrawMessageWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm);
void	DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness);
void	DrawingSurface_DrawPixel(ScriptDrawingSurface *sds, int x, int y);
int		DrawingSurface_GetPixel(ScriptDrawingSurface *sds, int x, int y);

#endif // __AGS_EE_AC__DRAWINGSURFACE_H
