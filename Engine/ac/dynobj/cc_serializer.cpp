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
#include <string.h>
#include "ac/dynobj/cc_serializer.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptcontainers.h"
#include "ac/dynobj/scriptfile.h"
#include "ac/dynobj/scriptmotionpath.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/game.h"
#include "ac/gui.h"
#include "debug/debug_log.h"
#include "plugin/plugin_engine.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"
#include "scriptjoystick.h"

using namespace AGS::Common;

extern CCAudioChannel ccDynamicAudio;
extern CCAudioClip ccDynamicAudioClip;
extern CCGUIControl ccDynamicGUIControl;
extern CCGUIButton ccDynamicGUIButton;
extern CCGUIInvWindow ccDynamicGUIInvWindow;
extern CCGUILabel ccDynamicGUILabel;
extern CCGUIListBox ccDynamicGUIListBox;
extern CCGUISlider ccDynamicGUISlider;
extern CCGUITextBox ccDynamicGUITextBox;
extern CCCharacter ccDynamicCharacter;
extern CCHotspot   ccDynamicHotspot;
extern CCRegion    ccDynamicRegion;
extern CCWalkableArea ccDynamicWalkarea;
extern CCWalkbehind ccDynamicWalkbehind;
extern CCInventory ccDynamicInv;
extern CCGUI       ccDynamicGUI;
extern CCTextWindowGUI ccDynamicTextWindowGUI;
extern CCObject    ccDynamicObject;
extern CCDialog    ccDynamicDialog;
extern ScriptDrawingSurface* dialogOptionsRenderingSurface;
extern ScriptDialogOptionsRendering ccDialogOptionsRendering;
extern std::vector<PluginObjectReader> pluginReaders;
extern std::vector<std::vector<int>> StaticGUIControlsHandles;

// *** De-serialization of script objects

void AGSDeSerializer::Unserialize(int index, const char *objectType, const char *serializedData, int dataSize) {

    if (dataSize < 0)
    {
        quitprintf("Unserialise: invalid data size (%d) for object type '%s'", dataSize, objectType);
        return; // FIXME: don't quit, return error
    }
    // Note that while our builtin classes may accept Stream object,
    // classes registered by plugin cannot, because streams are not (yet)
    // part of the plugin API.
    size_t data_sz = static_cast<size_t>(dataSize);
    assert(data_sz <= INT32_MAX); // dynamic object API does not support size > int32
    Stream mems(std::make_unique<MemoryStream>(reinterpret_cast<const uint8_t*>(serializedData), dataSize));

    // TODO: consider this: there are object types that are part of the
    // script's foundation, because they are created by the bytecode ops:
    // such as DynamicArray and UserObject. *Maybe* these should be moved
    // to certain "base serializer" class which guarantees their restoration.
    //
    // TODO: should we support older save versions here (DynArray, UserObj)?
    // might have to use older class names to distinguish save formats
    if (strcmp(objectType, CCDynamicArray::TypeName) == 0) {
        globalDynamicArray.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, ScriptUserObject::TypeName) == 0) {
        ScriptUserObject *suo = new ScriptUserObject();
        suo->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "GUIObject") == 0 || // old historical name
             strcmp(objectType, "GUIControl") == 0) {
        UnserializeGUIControl(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Button") == 0) {
        ccDynamicGUIButton.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "InvWindow") == 0) {
        ccDynamicGUIInvWindow.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Label") == 0) {
        ccDynamicGUILabel.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "ListBox") == 0) {
        ccDynamicGUIListBox.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Slider") == 0) {
        ccDynamicGUISlider.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "TextBox") == 0) {
        ccDynamicGUITextBox.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Character") == 0) {
        ccDynamicCharacter.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Hotspot") == 0) {
        ccDynamicHotspot.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Region") == 0) {
        ccDynamicRegion.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "WalkableArea") == 0) {
        ccDynamicWalkarea.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Walkbehind") == 0) {
        ccDynamicWalkbehind.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Inventory") == 0) {
        ccDynamicInv.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Dialog") == 0) {
        ccDynamicDialog.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "GUI") == 0) {
        ccDynamicGUI.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "TextWindowGUI") == 0) {
        ccDynamicTextWindowGUI.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Object") == 0) {
        ccDynamicObject.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "String") == 0) {
        myScriptStringImpl.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "File") == 0) {
        // files cannot be restored properly -- so just recreate
        // the object; attempting any operations on it will fail
        sc_File *scf = new sc_File();
        ccRegisterUnserializedObject(index, scf, scf);
    }
    else if (strcmp(objectType, "Overlay") == 0) {
        ScriptOverlay *scf = new ScriptOverlay();
        scf->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "DateTime") == 0) {
        ScriptDateTime *scf = new ScriptDateTime();
        scf->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "ViewFrame") == 0) {
        ScriptViewFrame *scf = new ScriptViewFrame();
        scf->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "DynamicSprite") == 0) {
        ScriptDynamicSprite *scf = new ScriptDynamicSprite();
        scf->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "DrawingSurface") == 0) {
        ScriptDrawingSurface *sds = new ScriptDrawingSurface();
        sds->Unserialize(index, &mems, data_sz);

        if (sds->isLinkedBitmapOnly)
        {
            dialogOptionsRenderingSurface = sds;
        }
    }
    else if (strcmp(objectType, "DialogOptionsRendering") == 0 || // old historical name
             strcmp(objectType, "DialogOptionsRenderingInfo") == 0)
    {
        ccDialogOptionsRendering.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "StringDictionary") == 0 || // old historical name
             strcmp(objectType, "Dictionary") == 0)
    {
        Dict_Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "StringSet") == 0 || // old historical name
             strcmp(objectType, "Set") == 0)
    {
        Set_Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Viewport2") == 0 || // old name (used for wrong reasons)
             strcmp(objectType, "Viewport") == 0)
    {
        Viewport_Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Camera2") == 0 || // old name (used for wrong reasons)
             strcmp(objectType, "Camera") == 0)
    {
        Camera_Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "AudioChannel") == 0)
    {
        ccDynamicAudio.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "AudioClip") == 0)
    {
        ccDynamicAudioClip.Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "Joystick") == 0)
    {
        // joysticks cannot be restored properly, any operation will fail
        auto *scj = new ScriptJoystick();
        ccRegisterUnserializedObject(index, scj, scj);
    }
    else if (strcmp(objectType, "MotionPath") == 0)
    {
        ScriptMotionPath *sc_path = new ScriptMotionPath();
        sc_path->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "ShaderProgram") == 0)
    {
        ScriptShaderProgram *sc_shader = new ScriptShaderProgram();
        sc_shader->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "ShaderInstance") == 0)
    {
        ScriptShaderInstance *sc_shinst = new ScriptShaderInstance();
        sc_shinst->Unserialize(index, &mems, data_sz);
    }
    else if (strcmp(objectType, "AnimatedOverlay") == 0) {
        ScriptAnimatedOverlay *sc_aover = new ScriptAnimatedOverlay();
        sc_aover->Unserialize(index, &mems, data_sz);
    }
    else
    {
        // check if the type is read by a plugin
        for (const auto &pr : pluginReaders) {
            if (pr.Type == objectType) {
                pr.Reader->Unserialize(index, serializedData, dataSize);
                return;
            }
        }
        // FIXME: find a way to return error back to caller instead
        quitprintf("Unserialise: unknown object type: '%s'", objectType);
    }
}

int AGSDeSerializer::RegisterGUIControl(int index, int guinum, int objnum)
{
    GUIControl *obj = guis[guinum].GetControl(objnum);
    switch (guis[guinum].GetControlType(objnum))
    {
    case kGUIButton: return ccRegisterUnserializedPersistentObject(index, obj, &ccDynamicGUIButton);
    case kGUILabel: return ccRegisterUnserializedPersistentObject(index, obj, &ccDynamicGUILabel);
    case kGUIInvWindow: return ccRegisterUnserializedPersistentObject(index, obj, &ccDynamicGUIInvWindow);
    case kGUISlider: return ccRegisterUnserializedPersistentObject(index, obj, &ccDynamicGUISlider);
    case kGUITextBox: return ccRegisterUnserializedPersistentObject(index, obj, &ccDynamicGUITextBox);
    case kGUIListBox: return ccRegisterUnserializedPersistentObject(index, obj, &ccDynamicGUIListBox);
    default: assert(false); /* should not happen */ return 0;
    }
}

void AGSDeSerializer::UnserializeGUIControl(int index, AGS::Common::Stream *in, size_t data_sz)
{
    int guinum = in->ReadInt32();
    int objnum = in->ReadInt32();
    int handle = RegisterGUIControl(index, guinum, objnum);
    (StaticGUIControlsHandles[guinum])[objnum] = handle;
}
