
//=============================================================================
//
// Exporting DynamicSprite script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_dynamicsprite_script_functions()
{
	scAdd_External_Symbol("DynamicSprite::ChangeCanvasSize^4", (void*)DynamicSprite_ChangeCanvasSize);
	scAdd_External_Symbol("DynamicSprite::CopyTransparencyMask^1", (void*)DynamicSprite_CopyTransparencyMask);
	scAdd_External_Symbol("DynamicSprite::Crop^4", (void*)DynamicSprite_Crop);
	scAdd_External_Symbol("DynamicSprite::Delete", (void*)DynamicSprite_Delete);
	scAdd_External_Symbol("DynamicSprite::Flip^1", (void*)DynamicSprite_Flip);
	scAdd_External_Symbol("DynamicSprite::GetDrawingSurface^0", (void*)DynamicSprite_GetDrawingSurface);
	scAdd_External_Symbol("DynamicSprite::Resize^2", (void*)DynamicSprite_Resize);
	scAdd_External_Symbol("DynamicSprite::Rotate^3", (void*)DynamicSprite_Rotate);
	scAdd_External_Symbol("DynamicSprite::SaveToFile^1", (void*)DynamicSprite_SaveToFile);
	scAdd_External_Symbol("DynamicSprite::Tint^5", (void*)DynamicSprite_Tint);
	scAdd_External_Symbol("DynamicSprite::get_ColorDepth", (void*)DynamicSprite_GetColorDepth);
	scAdd_External_Symbol("DynamicSprite::get_Graphic", (void*)DynamicSprite_GetGraphic);
	scAdd_External_Symbol("DynamicSprite::get_Height", (void*)DynamicSprite_GetHeight);
	scAdd_External_Symbol("DynamicSprite::get_Width", (void*)DynamicSprite_GetWidth);

	scAdd_External_Symbol("DynamicSprite::Create^3", (void*)DynamicSprite_Create);
	scAdd_External_Symbol("DynamicSprite::CreateFromBackground", (void*)DynamicSprite_CreateFromBackground);
	scAdd_External_Symbol("DynamicSprite::CreateFromDrawingSurface^5", (void*)DynamicSprite_CreateFromDrawingSurface);
	scAdd_External_Symbol("DynamicSprite::CreateFromExistingSprite^1", (void*)DynamicSprite_CreateFromExistingSprite_Old);
	scAdd_External_Symbol("DynamicSprite::CreateFromExistingSprite^2", (void*)DynamicSprite_CreateFromExistingSprite);
	scAdd_External_Symbol("DynamicSprite::CreateFromFile", (void*)DynamicSprite_CreateFromFile);
	scAdd_External_Symbol("DynamicSprite::CreateFromSaveGame", (void*)DynamicSprite_CreateFromSaveGame);
	scAdd_External_Symbol("DynamicSprite::CreateFromScreenShot", (void*)DynamicSprite_CreateFromScreenShot);
}
