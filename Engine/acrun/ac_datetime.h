#ifndef __AC_DATETIME_H
#define __AC_DATETIME_H

#include "ac/dynobj/scriptdatetime.h"

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