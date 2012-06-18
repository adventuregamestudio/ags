#ifndef __AC_SCRIPTDYNAMICSPRITE_H
#define __AC_SCRIPTDYNAMICSPRITE_H

#include "acrun/ac_ccdynamicobject.h"

struct ScriptDynamicSprite : AGSCCDynamicObject {
    int slot;

    virtual int Dispose(const char *address, bool force);
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    ScriptDynamicSprite(int slot);
    ScriptDynamicSprite();
};

#endif // __AC_SCRIPTDYNAMICSPRITE_H