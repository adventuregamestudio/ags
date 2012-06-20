#ifndef __AC_SCRIPTDIALOGOPTIONSRENDERING_H
#define __AC_SCRIPTDIALOGOPTIONSRENDERING_H

#include "ac/dynobj/scriptdrawingsurface.h"

struct ScriptDialogOptionsRendering : AGSCCDynamicObject {
    int x, y, width, height;
    int parserTextboxX, parserTextboxY;
    int parserTextboxWidth;
    int dialogID;
    int activeOptionID;
    ScriptDrawingSurface *surfaceToRenderTo;
    bool surfaceAccessed;

    // return the type name of the object
    virtual const char *GetType();

    // serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    virtual int Serialize(const char *address, char *buffer, int bufsize);

    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    void Reset();

    ScriptDialogOptionsRendering();
};


#endif // __AC_SCRIPTDIALOGOPTIONSRENDERING_H