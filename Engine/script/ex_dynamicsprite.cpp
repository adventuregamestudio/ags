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
	ccAddExternalObjectFunction("DynamicSprite::ChangeCanvasSize^4", (void*)DynamicSprite_ChangeCanvasSize);
	ccAddExternalObjectFunction("DynamicSprite::CopyTransparencyMask^1", (void*)DynamicSprite_CopyTransparencyMask);
	ccAddExternalObjectFunction("DynamicSprite::Crop^4", (void*)DynamicSprite_Crop);
	ccAddExternalObjectFunction("DynamicSprite::Delete", (void*)DynamicSprite_Delete);
	ccAddExternalObjectFunction("DynamicSprite::Flip^1", (void*)DynamicSprite_Flip);
	ccAddExternalObjectFunction("DynamicSprite::GetDrawingSurface^0", (void*)DynamicSprite_GetDrawingSurface);
	ccAddExternalObjectFunction("DynamicSprite::Resize^2", (void*)DynamicSprite_Resize);
	ccAddExternalObjectFunction("DynamicSprite::Rotate^3", (void*)DynamicSprite_Rotate);
	ccAddExternalObjectFunction("DynamicSprite::SaveToFile^1", (void*)DynamicSprite_SaveToFile);
	ccAddExternalObjectFunction("DynamicSprite::Tint^5", (void*)DynamicSprite_Tint);
	ccAddExternalObjectFunction("DynamicSprite::get_ColorDepth", (void*)DynamicSprite_GetColorDepth);
	ccAddExternalObjectFunction("DynamicSprite::get_Graphic", (void*)DynamicSprite_GetGraphic);
	ccAddExternalObjectFunction("DynamicSprite::get_Height", (void*)DynamicSprite_GetHeight);
	ccAddExternalObjectFunction("DynamicSprite::get_Width", (void*)DynamicSprite_GetWidth);

	ccAddExternalStaticFunction("DynamicSprite::Create^3", (void*)DynamicSprite_Create);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromBackground", (void*)DynamicSprite_CreateFromBackground);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromDrawingSurface^5", (void*)DynamicSprite_CreateFromDrawingSurface);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromExistingSprite^1", (void*)DynamicSprite_CreateFromExistingSprite_Old);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromExistingSprite^2", (void*)DynamicSprite_CreateFromExistingSprite);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromFile", (void*)DynamicSprite_CreateFromFile);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromSaveGame", (void*)DynamicSprite_CreateFromSaveGame);
	ccAddExternalStaticFunction("DynamicSprite::CreateFromScreenShot", (void*)DynamicSprite_CreateFromScreenShot);
}
