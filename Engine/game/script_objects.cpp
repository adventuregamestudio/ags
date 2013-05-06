
#include "game/script_objects.h"

CCGUIObject ccDynamicGUIObject;
CCCharacter ccDynamicCharacter;
CCHotspot   ccDynamicHotspot;
CCRegion    ccDynamicRegion;
CCInventory ccDynamicInv;
CCGUI       ccDynamicGUI;
CCObject    ccDynamicObject;
CCDialog    ccDynamicDialog;
CCAudioClip ccDynamicAudioClip;
CCAudioChannel ccDynamicAudio;
ScriptString myScriptStringImpl;
AGS::Common::Array<ScriptObject> scrObj;
AGS::Common::Array<ScriptGUI> scrGui;
AGS::Common::Array<ScriptHotspot> scrHotspot;
AGS::Common::Array<ScriptRegion> scrRegion;
AGS::Common::Array<ScriptInvItem> scrInv;
AGS::Common::Array<ScriptDialog> scrDialog;
AGS::Common::Array<ScriptAudioChannel> scrAudioChannel;
