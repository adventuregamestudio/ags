
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTDATETIME_H
#define __AGS_EE_DYNOBJ__SCRIPTDATETIME_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptDateTime : AGSCCDynamicObject {
    int year, month, day;
    int hour, minute, second;
    int rawUnixTime;

    virtual int Dispose(const char *address, bool force);
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    ScriptDateTime();
};

#endif // __AGS_EE_DYNOBJ__SCRIPTDATETIME_H
