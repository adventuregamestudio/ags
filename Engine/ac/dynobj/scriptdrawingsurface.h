#ifndef __AC_SCRIPTDRAWINGSURFACE_H
#define __AC_SCRIPTDRAWINGSURFACE_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptDrawingSurface : AGSCCDynamicObject {
    int roomBackgroundNumber;
    int dynamicSpriteNumber;
    int dynamicSurfaceNumber;
    bool isLinkedBitmapOnly;
    Common::IBitmap *linkedBitmapOnly;
    int currentColour;
    int currentColourScript;
    int highResCoordinates;
    int modified;
    int hasAlphaChannel;
    Common::IBitmap* abufBackup;

    virtual int Dispose(const char *address, bool force);
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
    Common::IBitmap* GetBitmapSurface();
    void StartDrawing();
    void MultiplyThickness(int *adjustValue);
    void UnMultiplyThickness(int *adjustValue);
    void MultiplyCoordinates(int *xcoord, int *ycoord);
    void FinishedDrawing();
    void FinishedDrawingReadOnly();

    ScriptDrawingSurface();
};

#endif // __AC_SCRIPTDRAWINGSURFACE_H