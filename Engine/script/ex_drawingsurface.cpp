
//=============================================================================
//
// Exporting DrawingSurface script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_drawingsurface_script_functions()
{
	ccAddExternalObjectFunction("DrawingSurface::Clear^1", (void *)DrawingSurface_Clear);
	ccAddExternalObjectFunction("DrawingSurface::CreateCopy^0", (void *)DrawingSurface_CreateCopy);
	ccAddExternalObjectFunction("DrawingSurface::DrawCircle^3", (void *)DrawingSurface_DrawCircle);
	ccAddExternalObjectFunction("DrawingSurface::DrawImage^6", (void *)DrawingSurface_DrawImage);
	ccAddExternalObjectFunction("DrawingSurface::DrawLine^5", (void *)DrawingSurface_DrawLine);
	ccAddExternalObjectFunction("DrawingSurface::DrawMessageWrapped^5", (void *)DrawingSurface_DrawMessageWrapped);
	ccAddExternalObjectFunction("DrawingSurface::DrawPixel^2", (void *)DrawingSurface_DrawPixel);
	ccAddExternalObjectFunction("DrawingSurface::DrawRectangle^4", (void *)DrawingSurface_DrawRectangle);
	ccAddExternalObjectFunction("DrawingSurface::DrawString^104", (void *)DrawingSurface_DrawString);
	ccAddExternalObjectFunction("DrawingSurface::DrawStringWrapped^6", (void *)DrawingSurface_DrawStringWrapped);
	ccAddExternalObjectFunction("DrawingSurface::DrawSurface^2", (void *)DrawingSurface_DrawSurface);
	ccAddExternalObjectFunction("DrawingSurface::DrawTriangle^6", (void *)DrawingSurface_DrawTriangle);
	ccAddExternalObjectFunction("DrawingSurface::GetPixel^2", (void *)DrawingSurface_GetPixel);
	ccAddExternalObjectFunction("DrawingSurface::Release^0", (void *)DrawingSurface_Release);
	ccAddExternalObjectFunction("DrawingSurface::get_DrawingColor", (void *)DrawingSurface_GetDrawingColor);
	ccAddExternalObjectFunction("DrawingSurface::set_DrawingColor", (void *)DrawingSurface_SetDrawingColor);
	ccAddExternalObjectFunction("DrawingSurface::get_Height", (void *)DrawingSurface_GetHeight);
	ccAddExternalObjectFunction("DrawingSurface::get_UseHighResCoordinates", (void *)DrawingSurface_GetUseHighResCoordinates);
	ccAddExternalObjectFunction("DrawingSurface::set_UseHighResCoordinates", (void *)DrawingSurface_SetUseHighResCoordinates);
	ccAddExternalObjectFunction("DrawingSurface::get_Width", (void *)DrawingSurface_GetWidth);
}
