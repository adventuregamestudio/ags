
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
void	DrawingSurface_DrawSurface(ScriptDrawingSurface* target, ScriptDrawingSurface* source, int translev);
void	DrawingSurface_DrawImage(ScriptDrawingSurface* sds, int xx, int yy, int slot, int trans, int width, int height);
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
void	DrawingSurface_DrawString(ScriptDrawingSurface *sds, int xx, int yy, int font, const char* texx, ...);
void	DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int alignment, const char *msg);
void	DrawingSurface_DrawMessageWrapped(ScriptDrawingSurface *sds, int xx, int yy, int wid, int font, int msgm);
void	DrawingSurface_DrawLine(ScriptDrawingSurface *sds, int fromx, int fromy, int tox, int toy, int thickness);
void	DrawingSurface_DrawPixel(ScriptDrawingSurface *sds, int x, int y);
int		DrawingSurface_GetPixel(ScriptDrawingSurface *sds, int x, int y);

#endif // __AGS_EE_AC__DRAWINGSURFACE_H
