#ifndef __AC_SCRIPTOVERLAY_H
#define __AC_SCRIPTOVERLAY_H

#include "acrun/ac_ccdynamicobject.h"

struct ScriptOverlay : AGSCCDynamicObject {
    int overlayId;
    int borderWidth;
    int borderHeight;
    int isBackgroundSpeech;

    virtual int Dispose(const char *address, bool force);
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
    void Remove();
    ScriptOverlay();
};

#endif // __AC_SCRIPTOVERLAY_H