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

ScriptDateTime* DateTime_Now_Core();
ScriptDateTime* DateTime_Now();
int DateTime_GetYear(ScriptDateTime *sdt);
int DateTime_GetMonth(ScriptDateTime *sdt);
int DateTime_GetDayOfMonth(ScriptDateTime *sdt);
int DateTime_GetHour(ScriptDateTime *sdt);
int DateTime_GetMinute(ScriptDateTime *sdt);
int DateTime_GetSecond(ScriptDateTime *sdt);
int DateTime_GetRawTime(ScriptDateTime *sdt);
int sc_GetTime(int whatti) ;
int GetRawTime ();

#endif // __AC_DATETIME_H