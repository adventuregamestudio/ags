#ifndef __AC_DATETIME_H
#define __AC_DATETIME_H

#include "acrun/ac_ccdynamicobject.h"

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

#endif // __AC_DATETIME_H