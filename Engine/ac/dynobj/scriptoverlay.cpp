
#include "ac/dynobj/scriptoverlay.h"
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/overlay.h"
#include "ac/rundefines.h"
#include "ac/screenoverlay.h"

extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];

int ScriptOverlay::Dispose(const char *address, bool force) 
{
    // since the managed object is being deleted, remove the
    // reference so it doesn't try and dispose something else
    // with that handle later
    int overlayIndex = find_overlay_of_type(overlayId);
    if (overlayIndex >= 0)
    {
        screenover[overlayIndex].associatedOverlayHandle = 0;
    }

    // if this is being removed voluntarily (ie. pointer out of
    // scope) then remove the associateed overlay
    // Otherwise, it's a Restre Game or something so don't
    if ((!force) && (!isBackgroundSpeech) && (Overlay_GetValid(this)))
    {
        Remove();
    }

    delete this;
    return 1;
}

const char *ScriptOverlay::GetType() {
    return "Overlay";
}

int ScriptOverlay::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(overlayId);
    SerializeInt(borderWidth);
    SerializeInt(borderHeight);
    SerializeInt(isBackgroundSpeech);
    return EndSerialize();
}

void ScriptOverlay::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    overlayId = UnserializeInt();
    borderWidth = UnserializeInt();
    borderHeight = UnserializeInt();
    isBackgroundSpeech = UnserializeInt();
    ccRegisterUnserializedObject(index, this, this);
}

void ScriptOverlay::Remove() 
{
    int overlayIndex = find_overlay_of_type(overlayId);
    if (overlayIndex < 0)
    {
        quit("ScriptOverlay::Remove: overlay is not there!");
    }
    remove_screen_overlay_index(overlayIndex);
    overlayId = -1;
}


ScriptOverlay::ScriptOverlay() {
    overlayId = -1;
}
