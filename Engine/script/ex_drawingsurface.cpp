
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
	scAdd_External_Symbol("DrawingSurface::Clear^1", (void *)DrawingSurface_Clear);
	scAdd_External_Symbol("DrawingSurface::CreateCopy^0", (void *)DrawingSurface_CreateCopy);
	scAdd_External_Symbol("DrawingSurface::DrawCircle^3", (void *)DrawingSurface_DrawCircle);
	scAdd_External_Symbol("DrawingSurface::DrawImage^6", (void *)DrawingSurface_DrawImage);
	scAdd_External_Symbol("DrawingSurface::DrawLine^5", (void *)DrawingSurface_DrawLine);
	scAdd_External_Symbol("DrawingSurface::DrawMessageWrapped^5", (void *)DrawingSurface_DrawMessageWrapped);
	scAdd_External_Symbol("DrawingSurface::DrawPixel^2", (void *)DrawingSurface_DrawPixel);
	scAdd_External_Symbol("DrawingSurface::DrawRectangle^4", (void *)DrawingSurface_DrawRectangle);
	scAdd_External_Symbol("DrawingSurface::DrawString^104", (void *)DrawingSurface_DrawString);
	scAdd_External_Symbol("DrawingSurface::DrawStringWrapped^6", (void *)DrawingSurface_DrawStringWrapped);
	scAdd_External_Symbol("DrawingSurface::DrawSurface^2", (void *)DrawingSurface_DrawSurface);
	scAdd_External_Symbol("DrawingSurface::DrawTriangle^6", (void *)DrawingSurface_DrawTriangle);
	scAdd_External_Symbol("DrawingSurface::GetPixel^2", (void *)DrawingSurface_GetPixel);
	scAdd_External_Symbol("DrawingSurface::Release^0", (void *)DrawingSurface_Release);
	scAdd_External_Symbol("DrawingSurface::get_DrawingColor", (void *)DrawingSurface_GetDrawingColor);
	scAdd_External_Symbol("DrawingSurface::set_DrawingColor", (void *)DrawingSurface_SetDrawingColor);
	scAdd_External_Symbol("DrawingSurface::get_Height", (void *)DrawingSurface_GetHeight);
	scAdd_External_Symbol("DrawingSurface::get_UseHighResCoordinates", (void *)DrawingSurface_GetUseHighResCoordinates);
	scAdd_External_Symbol("DrawingSurface::set_UseHighResCoordinates", (void *)DrawingSurface_SetUseHighResCoordinates);
	scAdd_External_Symbol("DrawingSurface::get_Width", (void *)DrawingSurface_GetWidth);
}
