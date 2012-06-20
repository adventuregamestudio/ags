
#include "wgt2allg.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/ac_roomstruct.h"
#include "sprcache.h"
#include "acrun/ac_rundefines.h"
#include "ac/ac_common.h"
#include "acmain/ac_drawingsurface.h"
#include "acrun/ac_gamestate.h"
#include "ac/ac_gamesetupstruct.h"

extern roomstruct thisroom;
extern SpriteCache spriteset;
extern block dynamicallyCreatedSurfaces[MAX_DYNAMIC_SURFACES];
extern GameState play;
extern GameSetupStruct game;

BITMAP* ScriptDrawingSurface::GetBitmapSurface()
{
    if (roomBackgroundNumber >= 0)
        return thisroom.ebscene[roomBackgroundNumber];
    else if (dynamicSpriteNumber >= 0)
        return spriteset[dynamicSpriteNumber];
    else if (dynamicSurfaceNumber >= 0)
        return dynamicallyCreatedSurfaces[dynamicSurfaceNumber];
    else if (linkedBitmapOnly != NULL)
        return linkedBitmapOnly;
    else
        quit("!DrawingSurface: attempted to use surface after Release was called");

    return NULL;
}

void ScriptDrawingSurface::StartDrawing()
{
    abufBackup = abuf;
    abuf = this->GetBitmapSurface();
}

void ScriptDrawingSurface::FinishedDrawingReadOnly()
{
    abuf = abufBackup;
}

void ScriptDrawingSurface::FinishedDrawing()
{
    FinishedDrawingReadOnly();
    modified = 1;
}

int ScriptDrawingSurface::Dispose(const char *address, bool force) {

    // dispose the drawing surface
    DrawingSurface_Release(this);
    delete this;
    return 1;
}

const char *ScriptDrawingSurface::GetType() {
    return "DrawingSurface";
}

int ScriptDrawingSurface::Serialize(const char *address, char *buffer, int bufsize) {
    StartSerialize(buffer);
    SerializeInt(roomBackgroundNumber);
    SerializeInt(dynamicSpriteNumber);
    SerializeInt(dynamicSurfaceNumber);
    SerializeInt(currentColour);
    SerializeInt(currentColourScript);
    SerializeInt(highResCoordinates);
    SerializeInt(modified);
    SerializeInt(hasAlphaChannel);
    SerializeInt(isLinkedBitmapOnly ? 1 : 0);
    return EndSerialize();
}

void ScriptDrawingSurface::Unserialize(int index, const char *serializedData, int dataSize) {
    StartUnserialize(serializedData, dataSize);
    roomBackgroundNumber = UnserializeInt();
    dynamicSpriteNumber = UnserializeInt();
    dynamicSurfaceNumber = UnserializeInt();
    currentColour = UnserializeInt();
    currentColourScript = UnserializeInt();
    highResCoordinates = UnserializeInt();
    modified = UnserializeInt();
    hasAlphaChannel = UnserializeInt();
    isLinkedBitmapOnly = (UnserializeInt() != 0);
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDrawingSurface::ScriptDrawingSurface() 
{
    roomBackgroundNumber = -1;
    dynamicSpriteNumber = -1;
    dynamicSurfaceNumber = -1;
    isLinkedBitmapOnly = false;
    linkedBitmapOnly = NULL;
    currentColour = play.raw_color;
    currentColourScript = 0;
    modified = 0;
    hasAlphaChannel = 0;
    highResCoordinates = 0;

    if ((game.options[OPT_NATIVECOORDINATES] != 0) &&
        (game.default_resolution > 2))
    {
        highResCoordinates = 1;
    }
}
