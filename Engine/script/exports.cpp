//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Registering symbols for the script system
//
//=============================================================================

#include "ac/gamestructdefines.h"

extern void RegisterAudioChannelAPI();
extern void RegisterAudioClipAPI();
extern void RegisterButtonAPI();
extern void RegisterCharacterAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
extern void RegisterContainerAPI();
extern void RegisterDateTimeAPI();
extern void RegisterDialogAPI();
extern void RegisterDialogOptionsRenderingAPI();
extern void RegisterDrawingSurfaceAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
extern void RegisterDynamicArrayAPI();
extern void RegisterDynamicSpriteAPI();
extern void RegisterFileAPI();
extern void RegisterGameAPI();
extern void RegisterGlobalAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
extern void RegisterGUIAPI();
extern void RegisterGUIControlAPI();
extern void RegisterHotspotAPI();
extern void RegisterInventoryItemAPI();
extern void RegisterInventoryWindowAPI();
extern void RegisterJoystickAPI();
extern void RegisterLabelAPI();
extern void RegisterListBoxAPI();
extern void RegisterMathAPI();
extern void RegisterMouseAPI();
extern void RegisterObjectAPI();
extern void RegisterOverlayAPI();
extern void RegisterParserAPI();
extern void RegisterPathfinderAPI();
extern void RegisterRegionAPI();
extern void RegisterRoomAPI();
extern void RegisterSaveInfoAPI();
extern void RegisterScreenAPI();
extern void RegisterSliderAPI();
extern void RegisterSpeechAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
extern void RegisterStringAPI();
extern void RegisterSystemAPI();
extern void RegisterTextBoxAPI();
extern void RegisterTouchAPI();
extern void RegisterViewFrameAPI();
extern void RegisterViewportAPI();
extern void RegisterVideoAPI();
extern void RegisterWalkareaAPI();
extern void RegisterWalkbehindAPI();

extern void RegisterShaderAPI();

extern void RegisterStaticObjects();

void setup_script_exports(ScriptAPIVersion base_api, ScriptAPIVersion compat_api)
{
    RegisterAudioChannelAPI();
    RegisterAudioClipAPI();
    RegisterButtonAPI();
    RegisterCharacterAPI(base_api, compat_api);
    RegisterContainerAPI();
    RegisterDateTimeAPI();
    RegisterDialogAPI();
    RegisterDialogOptionsRenderingAPI();
    RegisterDrawingSurfaceAPI(base_api, compat_api);
    RegisterDynamicArrayAPI();
    RegisterDynamicSpriteAPI();
    RegisterFileAPI();
    RegisterGameAPI();
    RegisterGlobalAPI(base_api, compat_api);
    RegisterGUIAPI();
    RegisterGUIControlAPI();
    RegisterHotspotAPI();
    RegisterInventoryItemAPI();
    RegisterInventoryWindowAPI();
    RegisterJoystickAPI();
    RegisterLabelAPI();
    RegisterListBoxAPI();
    RegisterMathAPI();
    RegisterMouseAPI();
    RegisterObjectAPI();
    RegisterOverlayAPI();
    RegisterParserAPI();
    RegisterPathfinderAPI();
    RegisterRegionAPI();
    RegisterRoomAPI();
    RegisterSaveInfoAPI();
    RegisterScreenAPI();
    RegisterSliderAPI();
    RegisterSpeechAPI(base_api, compat_api);
    RegisterStringAPI();
    RegisterSystemAPI();
    RegisterTextBoxAPI();
    RegisterTouchAPI();
    RegisterViewFrameAPI();
    RegisterViewportAPI();
    RegisterVideoAPI();
    RegisterWalkareaAPI();
    RegisterWalkbehindAPI();

    RegisterShaderAPI();

    RegisterStaticObjects();
}
