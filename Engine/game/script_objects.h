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
// A collection of script objects
//
//=============================================================================
#ifndef __AGS_EE_GAME__SCRIPT_OBJECTS_H
#define __AGS_EE_GAME__SCRIPT_OBJECTS_H

#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "util/array.h"

extern CCGUIObject ccDynamicGUIObject;
extern CCCharacter ccDynamicCharacter;
extern CCHotspot   ccDynamicHotspot;
extern CCRegion    ccDynamicRegion;
extern CCInventory ccDynamicInv;
extern CCGUI       ccDynamicGUI;
extern CCObject    ccDynamicObject;
extern CCDialog    ccDynamicDialog;
extern CCAudioClip ccDynamicAudioClip;
extern CCAudioChannel ccDynamicAudio;
extern ScriptString myScriptStringImpl;
extern AGS::Common::Array<ScriptObject> scrObj; // [MAX_INIT_SPR]
extern AGS::Common::Array<ScriptGUI> scrGui;
extern AGS::Common::Array<ScriptHotspot> scrHotspot; //[MAX_HOTSPOTS]
extern AGS::Common::Array<ScriptRegion> scrRegion; //[MAX_REGIONS]
extern AGS::Common::Array<ScriptInvItem> scrInv; //[MAX_INV]
extern AGS::Common::Array<ScriptDialog> scrDialog; //[MAX_DIALOG]
extern AGS::Common::Array<ScriptAudioChannel> scrAudioChannel; // [MAX_SOUND_CHANNELS + 1]

#endif // __AGS_EE_GAME__SCRIPT_OBJECTS_H
